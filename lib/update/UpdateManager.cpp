/*
 * Copyright 2020 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "UpdateManager.h"

#include "IUpdateChecker.h"

#include <lib/utility/ExitCodes.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/Utility.h>

#include <QApplication>
#include <QDateTime>
#include <QMetaObject>
#include <QProgressDialog>
#include <QTimerEvent>

#include <algorithm>
#include <limits>

namespace quentier {

// 5 minutes in msec
#define MIN_IDLE_TIME_FOR_UPDATE_NOTIFICATION 180000

// 15 minutes in msec
#define IDLE_STATE_POLL_INTERVAL 900000

UpdateManager::UpdateManager(
        IIdleStateInfoProviderPtr idleStateInfoProvider, QObject * parent) :
    QObject(parent),
    m_pIdleStateInfoProvider(std::move(idleStateInfoProvider))
{
    readPersistentSettings();

    if (m_updateCheckEnabled)
    {
        if (m_checkForUpdatesOnStartup)
        {
            QMetaObject::invokeMethod(
                this,
                "checkForUpdates",
                Qt::QueuedConnection);
        }
        else
        {
            // pretend we've just checked for update to ensure the check
            // won't run immediately
            m_lastCheckForUpdatesTimestamp = QDateTime::currentMSecsSinceEpoch();

            setupNextCheckForUpdatesTimer();
        }
    }
}

UpdateManager::~UpdateManager() = default;

void UpdateManager::setEnabled(const bool enabled)
{
    QNDEBUG("UpdateManager::setEnabled: " << (enabled ? "true" : "false"));

    if (m_updateCheckEnabled == enabled) {
        QNDEBUG("Already " << (enabled ? "enabled" : "disabled"));
        return;
    }

    m_updateCheckEnabled = enabled;

    if (m_updateCheckEnabled) {
        setupNextCheckForUpdatesTimer();
        return;
    }

    if (!clearCurrentUpdateProvider()) {
        QNINFO("UpdateManager was disabled but update provider "
            << "has already been started, won't cancel it");
        return;
    }

    clearCurrentUpdateUrl();
    clearCurrentUpdateChecker();

    if (m_nextUpdateCheckTimerId >= 0) {
        killTimer(m_nextUpdateCheckTimerId);
        m_nextUpdateCheckTimerId = -1;
    }

    if (m_nextIdleStatePollTimerId >= 0) {
        killTimer(m_nextIdleStatePollTimerId);
        m_nextIdleStatePollTimerId = -1;
    }
}

void UpdateManager::setUseContinuousUpdateChannel(const bool continuous)
{
    QNDEBUG("UpdateManager::setUseContinuousUpdateChannel: "
        << (continuous ? "true" : "false"));

    if (m_useContinuousUpdateChannel == continuous) {
        QNDEBUG("Already using "
            << (continuous ? "continuous" : "non-continuous")
            << " update channel");
        return;
    }

    m_useContinuousUpdateChannel = continuous;
    restartUpdateCheckerIfActive();
}

void UpdateManager::setCheckForUpdatesIntervalMsec(const qint64 interval)
{
    QNDEBUG("UpdateManager::setCheckForUpdatesIntervalMsec: " << interval);

    if (m_checkForUpdatesIntervalMsec == interval) {
        return;
    }

    m_checkForUpdatesIntervalMsec = interval;

    if (m_nextUpdateCheckTimerId >= 0) {
        killTimer(m_nextUpdateCheckTimerId);
        m_nextUpdateCheckTimerId = -1;
        setupNextCheckForUpdatesTimer();
    }
}

void UpdateManager::setUpdateChannel(QString channel)
{
    QNDEBUG("UpdateManager::setUpdateChannel: " << channel);

    if (m_updateChannel == channel) {
        QNDEBUG("Update channel didn't change");
        return;
    }

    m_updateChannel = std::move(channel);
    restartUpdateCheckerIfActive();
}

void UpdateManager::setUpdateProvider(UpdateProvider provider)
{
    QNDEBUG("UpdateManager::setUpdateProvider: " << provider);

    if (m_updateProvider == provider) {
        QNDEBUG("Update provider didn't change");
        return;
    }

    m_updateProvider = provider;
    restartUpdateCheckerIfActive();
}

void UpdateManager::readPersistentSettings()
{
    int checkForUpdatesOption = -1;

    readPersistentUpdateSettings(
        m_updateCheckEnabled,
        m_checkForUpdatesOnStartup,
        m_useContinuousUpdateChannel,
        checkForUpdatesOption,
        m_updateChannel,
        m_updateProvider);

    m_checkForUpdatesIntervalMsec = checkForUpdatesIntervalMsecFromOption(
        static_cast<CheckForUpdatesInterval>(checkForUpdatesOption));
}

void UpdateManager::setupNextCheckForUpdatesTimer()
{
    QNDEBUG("UpdateManager::setupNextCheckForUpdatesTimer");

    if (Q_UNLIKELY(m_checkForUpdatesIntervalMsec <= 0)) {
        QNWARNING("Won't set up next check for updates timer: wrong interval = "
            << m_checkForUpdatesIntervalMsec);
        return;
    }

    if (Q_UNLIKELY(m_lastCheckForUpdatesTimestamp <= 0)) {
        QNINFO("No last check for updates timestamp, checking for updates now");
        checkForUpdates();
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 msecPassed = now - m_lastCheckForUpdatesTimestamp;
    if (msecPassed >= m_checkForUpdatesIntervalMsec)
    {
        QNINFO("Last check for updates was too long ago ("
            << printableDateTimeFromTimestamp(m_lastCheckForUpdatesTimestamp)
            << ", now is " << printableDateTimeFromTimestamp(now)
            << "), checking for updates now");
        checkForUpdates();
        return;
    }

    int msecLeft = static_cast<int>(
        std::min(
            m_checkForUpdatesIntervalMsec - msecPassed,
            qint64(std::numeric_limits<int>::max())));

    if (m_nextUpdateCheckTimerId >= 0) {
        QNDEBUG("Next update check timer was active, killing it to relaunch");
        killTimer(m_nextUpdateCheckTimerId);
        m_nextUpdateCheckTimerId = -1;
    }

    m_nextUpdateCheckTimerId = startTimer(msecLeft);

    QNDEBUG("Last check for updates was done at "
        << printableDateTimeFromTimestamp(m_lastCheckForUpdatesTimestamp)
        << ", now is " << printableDateTimeFromTimestamp(now)
        << ", will check for updates in " << msecLeft << " milliseconds at "
        << printableDateTimeFromTimestamp(now + msecLeft)
        << ", timer id = " << m_nextUpdateCheckTimerId);
}

void UpdateManager::recycleUpdateChecker()
{
    if (m_pCurrentUpdateChecker) {
        m_pCurrentUpdateChecker->disconnect(this);
        m_pCurrentUpdateChecker->deleteLater();
        m_pCurrentUpdateChecker = nullptr;
    }
}

bool UpdateManager::checkIdleState() const
{
    qint64 idleTime = m_pIdleStateInfoProvider->idleTime();
    if (idleTime < 0) {
        return true;
    }

    return idleTime >= MIN_IDLE_TIME_FOR_UPDATE_NOTIFICATION;
}

void UpdateManager::scheduleNextIdleStateCheckForUpdateNotification()
{
    QNDEBUG("UpdateManager::scheduleNextIdleStateCheckForUpdateNotification");

    if (m_nextIdleStatePollTimerId >= 0) {
        QNDEBUG("Next idle state poll timer is already active");
        return;
    }

    m_nextIdleStatePollTimerId = startTimer(IDLE_STATE_POLL_INTERVAL);
}

void UpdateManager::askUserAndLaunchUpdate()
{
    QNDEBUG("UpdateManager::askUserAndLaunchUpdate");

    if (m_pCurrentUpdateProvider)
    {
        if (m_updateProviderInProgress) {
            QNDEBUG("Update provider has already been started");
            return;
        }

        QNDEBUG("Update provider is ready, notifying user");

        QWidget * parentWidget = qobject_cast<QWidget*>(parent());
        int res = informationMessageBox(
            parentWidget,
            tr("Updates available"),
            tr("A newer version of Quentier is available. Would you like to "
            "download and install it?"),
            QString(),
            QMessageBox::Ok | QMessageBox::No);
        if (res != QMessageBox::Ok) {
            QNDEBUG("User refused to download and install updates");
            setupNextCheckForUpdatesTimer();
            return;
        }

        QObject::connect(
            m_pCurrentUpdateProvider.get(),
            &IUpdateProvider::finished,
            this,
            &UpdateManager::onUpdateProviderFinished);

        QObject::connect(
            m_pCurrentUpdateProvider.get(),
            &IUpdateProvider::progress,
            this,
            &UpdateManager::onUpdateProviderProgress);

        m_pUpdateProgressDialog.reset(new QProgressDialog(
            tr("Updating, please wait..."),
            {},
            0,
            100,
            parentWidget));

        // Forbid cancelling the update
        m_pUpdateProgressDialog->setCancelButton(nullptr);

        m_pUpdateProgressDialog->setWindowModality(Qt::WindowModal);

        m_pCurrentUpdateProvider->run();
        m_updateProviderInProgress = true;
    }
    else if (!m_currentUpdateUrl.isEmpty())
    {
        if (m_currentUpdateUrlOnceOpened) {
            QNDEBUG("Update download URL has already been opened");
            return;
        }

        QNDEBUG("Update download URL is ready, notifying user");

        QWidget * parentWidget = qobject_cast<QWidget*>(parent());
        int res = informationMessageBox(
            parentWidget,
            tr("Updates available"),
            tr("A newer version of Quentier is available. Would you like to "
            "download it?"),
            {},
            QMessageBox::Ok | QMessageBox::No);
        if (res != QMessageBox::Ok) {
            QNDEBUG("User refused to download updates");
            setupNextCheckForUpdatesTimer();
            return;
        }

        openUrl(m_currentUpdateUrl);
        m_currentUpdateUrlOnceOpened = true;
    }

    setupNextCheckForUpdatesTimer();
}

bool UpdateManager::clearCurrentUpdateProvider()
{
    QNDEBUG("UpdateManager::clearCurrentUpdateProvider");

    if (!m_pCurrentUpdateProvider) {
        return true;
    }

    if (m_updateProviderInProgress) {
        return false;
    }

    m_pCurrentUpdateProvider->disconnect(this);
    m_pCurrentUpdateProvider.reset();
    return true;
}

void UpdateManager::clearCurrentUpdateUrl()
{
    QNDEBUG("UpdateManager::clearCurrentUpdateUrl");

    if (!m_currentUpdateUrl.isEmpty()) {
        m_currentUpdateUrl = QUrl();
        m_currentUpdateUrlOnceOpened = false;
    }
}

void UpdateManager::clearCurrentUpdateChecker()
{
    QNDEBUG("UpdateManager::clearCurrentUpdateChecker");

    if (m_pCurrentUpdateChecker) {
        m_pCurrentUpdateChecker->disconnect(this);
        m_pCurrentUpdateChecker->deleteLater();
        m_pCurrentUpdateChecker = nullptr;
    }
}

void UpdateManager::restartUpdateCheckerIfActive()
{
    QNDEBUG("UpdateManager::restartUpdateCheckerIfActive");

    if (!m_pCurrentUpdateChecker) {
        return;
    }

    clearCurrentUpdateChecker();
    checkForUpdates();
}

void UpdateManager::checkForUpdates()
{
    QNDEBUG("UpdateManager::checkForUpdates");

    if (!clearCurrentUpdateProvider()) {
        QNDEBUG("Update provider is already in progress, won't "
            << "check for updates right now");
        return;
    }

    clearCurrentUpdateUrl();
    clearCurrentUpdateChecker();

    m_pCurrentUpdateChecker = newUpdateChecker(m_updateProvider, this);
    m_pCurrentUpdateChecker->setUpdateChannel(m_updateChannel);
    m_pCurrentUpdateChecker->setUseContinuousUpdateChannel(m_useContinuousUpdateChannel);

    QObject::connect(
        m_pCurrentUpdateChecker,
        &IUpdateChecker::failure,
        this,
        &UpdateManager::onCheckForUpdatesError,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        m_pCurrentUpdateChecker,
        &IUpdateChecker::noUpdatesAvailable,
        this,
        &UpdateManager::onNoUpdatesAvailable,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pCurrentUpdateChecker,
        qOverload<QUrl>(&IUpdateChecker::updatesAvailable),
        this,
        &UpdateManager::onUpdatesAvailableAtUrl,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        m_pCurrentUpdateChecker,
        qOverload<std::shared_ptr<IUpdateProvider>>(
            &IUpdateChecker::updatesAvailable),
        this,
        &UpdateManager::onUpdatesAvailable,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
#else
    QObject::connect(
        m_pCurrentUpdateChecker,
        SIGNAL(updatesAvailable(QUrl)),
        this,
        SLOT(onUpdatesAvailableAtUrl(QUrl)),
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        m_pCurrentUpdateChecker,
        SIGNAL(updatesAvailable(std::shared_ptr<IUpdateProvider>)),
        this,
        SLOT(onUpdatesAvailable(std::shared_ptr<IUpdateProvider>)),
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
#endif

    m_lastCheckForUpdatesTimestamp = QDateTime::currentMSecsSinceEpoch();
    m_pCurrentUpdateChecker->checkForUpdates();
}

void UpdateManager::onCheckForUpdatesError(ErrorString errorDescription)
{
    QNDEBUG("UpdateManager::onCheckForUpdatesError: " << errorDescription);

    Q_EMIT notifyError(errorDescription);

    recycleUpdateChecker();
    setupNextCheckForUpdatesTimer();
}

void UpdateManager::onNoUpdatesAvailable()
{
    QNDEBUG("UpdateManager::onNoUpdatesAvailable");

    recycleUpdateChecker();
    setupNextCheckForUpdatesTimer();
}

void UpdateManager::onUpdatesAvailableAtUrl(QUrl downloadUrl)
{
    QNDEBUG("UpdateManager::onUpdatesAvailableAtUrl: " << downloadUrl);

    recycleUpdateChecker();

    m_currentUpdateUrl = downloadUrl;
    m_currentUpdateUrlOnceOpened = false;

    if (!checkIdleState()) {
        scheduleNextIdleStateCheckForUpdateNotification();
        return;
    }

    askUserAndLaunchUpdate();
}

void UpdateManager::onUpdatesAvailable(
    std::shared_ptr<IUpdateProvider> provider)
{
    QNDEBUG("UpdateManager::onUpdatesAvailable");

    recycleUpdateChecker();

    Q_ASSERT(provider);

    if (m_pCurrentUpdateProvider) {
        m_pCurrentUpdateProvider->disconnect(this);
        m_pCurrentUpdateProvider.reset();
    }

    m_pCurrentUpdateProvider = std::move(provider);
    m_updateProviderInProgress = false;

    if (!checkIdleState()) {
        scheduleNextIdleStateCheckForUpdateNotification();
        return;
    }

    askUserAndLaunchUpdate();
}

void UpdateManager::onUpdateProviderFinished(
    bool status, ErrorString errorDescription, bool needsRestart)
{
    QNDEBUG("UpdateManager::onUpdateProviderFinished: status = "
        << (status ? "true" : "false") << ", error description = "
        << errorDescription << ", needs restart = "
        << (needsRestart ? "true" : "false"));

    Q_ASSERT(m_pUpdateProgressDialog);
    m_pUpdateProgressDialog->setValue(100);
    m_pUpdateProgressDialog->close();
    m_pUpdateProgressDialog.reset();

    m_updateProviderInProgress = false;

    if (!status) {
        Q_EMIT notifyError(errorDescription);
        return;
    }

    if (!needsRestart) {
        return;
    }

    QWidget * parentWidget = qobject_cast<QWidget*>(parent());
    int res = questionMessageBox(
        parentWidget,
        tr("Restart is required"),
        tr("Restart is required in order to complete the update. Would you "
           "like to restart now?"),
        {},
        QMessageBox::Ok | QMessageBox::No);
    if (res != QMessageBox::Ok) {
        QNDEBUG("User refused to restart after update");
        return;
    }

    qApp->exit(RESTART_EXIT_CODE);
}

void UpdateManager::onUpdateProviderProgress(double value, QString message)
{
    QNDEBUG("UpdateManager::onUpdateProviderProgress: value = "
        << value << ", message = " << message);

    Q_ASSERT(m_pUpdateProgressDialog);

    int percentage = static_cast<int>(value * 100.0);
    percentage = std::max(percentage, 100);

    m_pUpdateProgressDialog->setValue(percentage);
}

void UpdateManager::timerEvent(QTimerEvent * pTimerEvent)
{
    if (Q_UNLIKELY(!pTimerEvent))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent"));
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    int timerId = pTimerEvent->timerId();
    killTimer(timerId);

    QNDEBUG("Timer event for timer id " << timerId);

    if (timerId == m_nextUpdateCheckTimerId)
    {
        m_nextUpdateCheckTimerId = -1;

        if (m_updateProviderInProgress || m_currentUpdateUrlOnceOpened) {
            QNDEBUG("Will not check for updates on timer: update has already "
                << "been launched");
        }
        else {
            checkForUpdates();
        }
    }
    else if (timerId == m_nextIdleStatePollTimerId)
    {
        m_nextIdleStatePollTimerId = -1;
        askUserAndLaunchUpdate();
    }
}

} // namespace quentier
