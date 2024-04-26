/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DateTime.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/System.h>

#include <QApplication>
#include <QDateTime>
#include <QMetaObject>
#include <QProgressDialog>
#include <QPushButton>
#include <QTimerEvent>

#include <algorithm>
#include <limits>

namespace quentier {

UpdateManager::UpdateManager(
    IIdleStateInfoProviderPtr idleStateInfoProvider, QObject * parent) :
    QObject{parent},
    m_idleStateInfoProvider{std::move(idleStateInfoProvider)}
{
    readPersistentSettings();

    if (m_updateCheckEnabled) {
        if (m_checkForUpdatesOnStartup) {
            QMetaObject::invokeMethod(
                this, &UpdateManager::checkForUpdatesImpl,
                Qt::QueuedConnection);
        }
        else {
            // pretend we've just checked for update to ensure the check
            // won't run immediately
            m_lastCheckForUpdatesTimestamp =
                QDateTime::currentMSecsSinceEpoch();

            setupNextCheckForUpdatesTimer();
        }
    }
}

UpdateManager::~UpdateManager() = default;

void UpdateManager::setEnabled(const bool enabled)
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::setEnabled: " << (enabled ? "true" : "false"));

    if (m_updateCheckEnabled == enabled) {
        QNDEBUG(
            "update::UpdateManager",
            "Already " << (enabled ? "enabled" : "disabled"));
        return;
    }

    m_updateCheckEnabled = enabled;

    if (m_updateCheckEnabled) {
        setupNextCheckForUpdatesTimer();
        return;
    }

    if (!clearCurrentUpdateProvider()) {
        QNINFO(
            "update::UpdateManager",
            "UpdateManager was disabled but update provider has already been "
            "started, won't cancel it");
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
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::setUseContinuousUpdateChannel: "
            << (continuous ? "true" : "false"));

    if (m_useContinuousUpdateChannel == continuous) {
        QNDEBUG(
            "update::UpdateManager",
            "Already using " << (continuous ? "continuous" : "non-continuous")
                             << " update channel");
        return;
    }

    m_useContinuousUpdateChannel = continuous;
    restartUpdateCheckerIfActive();
}

void UpdateManager::setCheckForUpdatesIntervalMsec(const qint64 interval)
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::setCheckForUpdatesIntervalMsec: " << interval);

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
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::setUpdateChannel: " << channel);

    if (m_updateChannel == channel) {
        QNDEBUG("update::UpdateManager", "Update channel didn't change");
        return;
    }

    m_updateChannel = std::move(channel);
    restartUpdateCheckerIfActive();
}

void UpdateManager::setUpdateProvider(const UpdateProvider provider)
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::setUpdateProvider: " << provider);

    if (m_updateProvider == provider) {
        QNDEBUG("update::UpdateManager", "Update provider didn't change");
        return;
    }

    m_updateProvider = provider;
    restartUpdateCheckerIfActive();
}

void UpdateManager::checkForUpdatesImpl()
{
    QNDEBUG("update::UpdateManager", "UpdateManager::checkForUpdatesImpl");

    if (!clearCurrentUpdateProvider()) {
        QNDEBUG(
            "update::UpdateManager",
            "Update provider is already in progress, won't check for updates "
            "right now");
        return;
    }

    clearCurrentUpdateUrl();
    clearCurrentUpdateChecker();

    m_currentUpdateChecker = newUpdateChecker(m_updateProvider, this);
    m_currentUpdateChecker->setUpdateChannel(m_updateChannel);

    m_currentUpdateChecker->setUseContinuousUpdateChannel(
        m_useContinuousUpdateChannel);

    QObject::connect(
        m_currentUpdateChecker, &IUpdateChecker::failure, this,
        &UpdateManager::onCheckForUpdatesError,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        m_currentUpdateChecker, &IUpdateChecker::noUpdatesAvailable, this,
        &UpdateManager::onNoUpdatesAvailable,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        m_currentUpdateChecker,
        &IUpdateChecker::updatesFromUrlAvailable, this,
        &UpdateManager::onUpdatesAvailableAtUrl,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        m_currentUpdateChecker,
        &IUpdateChecker::updatesFromProviderAvailable,
        this, &UpdateManager::onUpdatesAvailable,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    m_lastCheckForUpdatesTimestamp = QDateTime::currentMSecsSinceEpoch();
    m_currentUpdateChecker->checkForUpdates();
}

void UpdateManager::readPersistentSettings()
{
    int checkForUpdatesOption = -1;

    readPersistentUpdateSettings(
        m_updateCheckEnabled, m_checkForUpdatesOnStartup,
        m_useContinuousUpdateChannel, checkForUpdatesOption, m_updateChannel,
        m_updateProvider);

    m_checkForUpdatesIntervalMsec = checkForUpdatesIntervalMsecFromOption(
        static_cast<CheckForUpdatesInterval>(checkForUpdatesOption));
}

void UpdateManager::setupNextCheckForUpdatesTimer()
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::setupNextCheckForUpdatesTimer");

    if (Q_UNLIKELY(m_checkForUpdatesIntervalMsec <= 0)) {
        QNWARNING(
            "update::UpdateManager",
            "Won't set up next check for updates timer: wrong interval = "
                << m_checkForUpdatesIntervalMsec);
        return;
    }

    if (Q_UNLIKELY(m_lastCheckForUpdatesTimestamp <= 0)) {
        QNINFO(
            "update::UpdateManager",
            "No last check for updates timestamp, checking for updates now");
        checkForUpdatesImpl();
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 msecPassed = now - m_lastCheckForUpdatesTimestamp;
    if (msecPassed >= m_checkForUpdatesIntervalMsec) {
        QNINFO(
            "update::UpdateManager",
            "Last check for updates was too long ago ("
                << printableDateTimeFromTimestamp(
                       m_lastCheckForUpdatesTimestamp)
                << ", now is " << printableDateTimeFromTimestamp(now)
                << "), checking for updates now");
        checkForUpdatesImpl();
        return;
    }

    const int msecLeft = static_cast<int>(std::min(
        m_checkForUpdatesIntervalMsec - msecPassed,
        qint64(std::numeric_limits<int>::max())));

    if (m_nextUpdateCheckTimerId >= 0) {
        QNDEBUG(
            "update::UpdateManager",
            "Next update check timer was active, killing it to relaunch");
        killTimer(m_nextUpdateCheckTimerId);
        m_nextUpdateCheckTimerId = -1;
    }

    m_nextUpdateCheckTimerId = startTimer(msecLeft);

    QNDEBUG(
        "update::UpdateManager",
        "Last check for updates was done at "
            << printableDateTimeFromTimestamp(m_lastCheckForUpdatesTimestamp)
            << ", now is " << printableDateTimeFromTimestamp(now)
            << ", will check for updates in " << msecLeft << " milliseconds at "
            << printableDateTimeFromTimestamp(now + msecLeft)
            << ", timer id = " << m_nextUpdateCheckTimerId);
}

void UpdateManager::recycleUpdateChecker()
{
    if (m_currentUpdateChecker) {
        m_currentUpdateChecker->disconnect(this);
        m_currentUpdateChecker->deleteLater();
        m_currentUpdateChecker = nullptr;
    }
}

bool UpdateManager::checkIdleState() const
{
    const qint64 idleTime = m_idleStateInfoProvider->idleTime();
    if (idleTime < 0) {
        return true;
    }

    // 5 minutes in msec
    constexpr int minIdleTimeForUpdateNotification = 180000;

    return idleTime >= minIdleTimeForUpdateNotification;
}

void UpdateManager::scheduleNextIdleStateCheckForUpdateNotification()
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::scheduleNextIdleStateCheckForUpdateNotification");

    if (m_nextIdleStatePollTimerId >= 0) {
        QNDEBUG(
            "update::UpdateManager",
            "Next idle state poll timer is already active");
        return;
    }

    // 15 minutes in msec
    constexpr int idleStatePollInterval = 900000;
    m_nextIdleStatePollTimerId = startTimer(idleStatePollInterval);
}

void UpdateManager::askUserAndLaunchUpdate()
{
    QNDEBUG("update::UpdateManager", "UpdateManager::askUserAndLaunchUpdate");

    if (m_currentUpdateProvider) {
        if (m_updateProviderInProgress) {
            QNDEBUG(
                "update::UpdateManager",
                "Update provider has already been started");
            return;
        }

        QNDEBUG(
            "update::UpdateManager",
            "Update provider is ready, notifying user");

        auto * parentWidget = qobject_cast<QWidget *>(parent());

        const int res = informationMessageBox(
            parentWidget, tr("Updates available"),
            tr("A newer version of Quentier is available. Would you like to "
               "download and install it?"),
            {}, QMessageBox::Ok | QMessageBox::No);

        if (res != QMessageBox::Ok) {
            QNDEBUG("update::UpdateManager", "User refused to download and install updates");
            setupNextCheckForUpdatesTimer();
            return;
        }

        QObject::connect(
            m_currentUpdateProvider.get(), &IUpdateProvider::finished, this,
            &UpdateManager::onUpdateProviderFinished);

        QObject::connect(
            m_currentUpdateProvider.get(), &IUpdateProvider::progress, this,
            &UpdateManager::onUpdateProviderProgress);

        m_updateProgressDialog.reset(new QProgressDialog(
            tr("Updating, please wait..."), {}, 0, 100, parentWidget));

        m_updateProgressDialog->setWindowModality(Qt::WindowModal);

        m_updateProgressDialog->setMinimum(0);
        m_updateProgressDialog->setMaximum(100);
        m_updateProgressDialog->setMinimumDuration(0);

        // Show progress dialog right away - downloading can be quite slow
        // so immediate feedback is required
        m_updateProgressDialog->show();

        auto * cancelButton = new QPushButton;
        m_updateProgressDialog->setCancelButton(cancelButton);
        m_updateProgressDialog->setCancelButtonText(tr("Cancel"));

        QObject::connect(
            cancelButton, &QPushButton::clicked, this,
            &UpdateManager::onCancelUpdateProvider);

        m_currentUpdateProvider->run();
        m_updateProviderInProgress = true;
    }
    else if (!m_currentUpdateUrl.isEmpty()) {
        if (m_currentUpdateUrlOnceOpened) {
            QNDEBUG(
                "update::UpdateManager",
                "Update download URL has already been opened");
            return;
        }

        QNDEBUG(
            "update::UpdateManager",
            "Update download URL is ready, notifying user");

        auto * parentWidget = qobject_cast<QWidget *>(parent());

        const int res = informationMessageBox(
            parentWidget, tr("Updates available"),
            tr("A newer version of Quentier is available. Would you like to "
               "download it?"),
            {}, QMessageBox::Ok | QMessageBox::No);

        if (res != QMessageBox::Ok) {
            QNDEBUG("update::UpdateManager", "User refused to download updates");
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
    QNDEBUG(
        "update::UpdateManager", "UpdateManager::clearCurrentUpdateProvider");

    if (!m_currentUpdateProvider) {
        return true;
    }

    if (m_updateProviderInProgress) {
        return false;
    }

    m_currentUpdateProvider->disconnect(this);
    m_currentUpdateProvider.reset();
    return true;
}

void UpdateManager::clearCurrentUpdateUrl()
{
    QNDEBUG("update::UpdateManager", "UpdateManager::clearCurrentUpdateUrl");

    if (!m_currentUpdateUrl.isEmpty()) {
        m_currentUpdateUrl = QUrl{};
        m_currentUpdateUrlOnceOpened = false;
    }
}

void UpdateManager::clearCurrentUpdateChecker()
{
    QNDEBUG(
        "update::UpdateManager", "UpdateManager::clearCurrentUpdateChecker");

    if (m_currentUpdateChecker) {
        m_currentUpdateChecker->disconnect(this);
        m_currentUpdateChecker->deleteLater();
        m_currentUpdateChecker = nullptr;
    }
}

void UpdateManager::restartUpdateCheckerIfActive()
{
    QNDEBUG(
        "update::UpdateManager", "UpdateManager::restartUpdateCheckerIfActive");

    if (!m_currentUpdateChecker) {
        return;
    }

    clearCurrentUpdateChecker();
    checkForUpdatesImpl();
}

void UpdateManager::checkForUpdates()
{
    QNDEBUG("update::UpdateManager", "UpdateManager::checkForUpdates");

    if (m_updateInstalledPendingRestart) {
        offerUserToRestart();
        return;
    }

    m_currentUpdateCheckInvokedByUser = true;
    auto * parentWidget = qobject_cast<QWidget *>(parent());

    m_checkForUpdatesProgressDialog.reset(new QProgressDialog{
        tr("Checking for updates, please wait..."), {}, 0, 100, parentWidget});

    // Forbid cancelling the check for updates
    m_checkForUpdatesProgressDialog->setCancelButton(nullptr);
    m_checkForUpdatesProgressDialog->setWindowModality(Qt::WindowModal);

    checkForUpdatesImpl();
}

void UpdateManager::onCheckForUpdatesError(ErrorString errorDescription)
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::onCheckForUpdatesError: " << errorDescription);

    if (m_currentUpdateCheckInvokedByUser) {
        m_currentUpdateCheckInvokedByUser = false;
        closeCheckForUpdatesProgressDialog();

        auto * parentWidget = qobject_cast<QWidget *>(parent());

        warningMessageBox(
            parentWidget, tr("Failed to check for updates"),
            tr("Error occurred during the attempt to check for updates"),
            errorDescription.localizedString());
    }

    Q_EMIT notifyError(errorDescription);

    recycleUpdateChecker();
    setupNextCheckForUpdatesTimer();
}

void UpdateManager::onNoUpdatesAvailable()
{
    QNDEBUG("update::UpdateManager", "UpdateManager::onNoUpdatesAvailable");

    if (m_currentUpdateCheckInvokedByUser) {
        m_currentUpdateCheckInvokedByUser = false;
        closeCheckForUpdatesProgressDialog();

        auto * parentWidget = qobject_cast<QWidget *>(parent());

        informationMessageBox(
            parentWidget, tr("No updates"),
            tr("No updates are available at this time"), {});
    }

    recycleUpdateChecker();
    setupNextCheckForUpdatesTimer();
}

void UpdateManager::onUpdatesAvailableAtUrl(QUrl downloadUrl)
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::onUpdatesAvailableAtUrl: " << downloadUrl);

    if (m_currentUpdateCheckInvokedByUser) {
        m_currentUpdateCheckInvokedByUser = false;
        closeCheckForUpdatesProgressDialog();
        // Message box would be displayed below
    }

    recycleUpdateChecker();

    m_currentUpdateUrl = std::move(downloadUrl);
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
    QNDEBUG("update::UpdateManager", "UpdateManager::onUpdatesAvailable");

    if (m_currentUpdateCheckInvokedByUser) {
        m_currentUpdateCheckInvokedByUser = false;
        closeCheckForUpdatesProgressDialog();
        // Message box would be displayed below
    }

    recycleUpdateChecker();

    Q_ASSERT(provider);

    if (m_currentUpdateProvider) {
        m_currentUpdateProvider->disconnect(this);
        m_currentUpdateProvider.reset();
    }

    m_currentUpdateProvider = std::move(provider);
    m_updateProviderInProgress = false;

    if (!checkIdleState()) {
        scheduleNextIdleStateCheckForUpdateNotification();
        return;
    }

    askUserAndLaunchUpdate();
}

void UpdateManager::onUpdateProviderFinished(
    const bool status, ErrorString errorDescription, const bool needsRestart,
    const UpdateProvider updateProviderKind)
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::onUpdateProviderFinished: status = "
            << (status ? "true" : "false")
            << ", error description = " << errorDescription
            << ", needs restart = " << (needsRestart ? "true" : "false")
            << ", update provider kind = " << updateProviderKind);

    closeUpdateProgressDialog();
    m_updateProviderInProgress = false;
    m_lastUpdateProviderProgress = 0;

    if (!status) {
        Q_EMIT notifyError(std::move(errorDescription));
        return;
    }

    if (!needsRestart) {
        return;
    }

    m_updateInstalledPendingRestart = true;
    offerUserToRestart();
}

void UpdateManager::onUpdateProviderProgress(
    const double value, const QString & message)
{
    QNDEBUG(
        "update::UpdateManager",
        "UpdateManager::onUpdateProviderProgress: value = "
            << value << ", message = " << message);

    Q_ASSERT(m_updateProgressDialog);

    int percentage = static_cast<int>(value * 100.0);
    percentage = std::min(percentage, 100);

    if (percentage <= m_lastUpdateProviderProgress) {
        return;
    }

    m_lastUpdateProviderProgress = percentage;

    QNDEBUG(
        "update::UpdateManager",
        "Setting update progress percentage: " << percentage);

    m_updateProgressDialog->setValue(percentage);
}

void UpdateManager::onCancelUpdateProvider()
{
    QNDEBUG("update::UpdateManager", "UpdateManager::onCancelUpdateProvider");

    Q_ASSERT(m_currentUpdateProvider);

    QObject::connect(
        m_currentUpdateProvider.get(), &IUpdateProvider::cancelled, this,
        [this] {
            QNDEBUG(
                "update::UpdateManager", "Successfully cancelled the update");

            closeUpdateProgressDialog();
            m_updateProviderInProgress = false;
            m_lastUpdateProviderProgress = 0;
        });

    QObject::connect(
        m_currentUpdateProvider.get(), &IUpdateProvider::cannotCancelUpdate,
        this, [this] {
            ErrorString error(QT_TR_NOOP("Failed to cancel the update"));
            QNWARNING("update::UpdateManager", error);
            notifyError(error);
        });

    m_currentUpdateProvider->cancel();
}

void UpdateManager::closeUpdateProgressDialog()
{
    Q_ASSERT(m_updateProgressDialog);
    m_updateProgressDialog->setValue(100);
    m_updateProgressDialog->close();
    m_updateProgressDialog.reset();
}

void UpdateManager::closeCheckForUpdatesProgressDialog()
{
    Q_ASSERT(m_checkForUpdatesProgressDialog);
    m_checkForUpdatesProgressDialog->setValue(100);
    m_checkForUpdatesProgressDialog->close();
    m_checkForUpdatesProgressDialog.reset();
}

void UpdateManager::timerEvent(QTimerEvent * pTimerEvent)
{
    if (Q_UNLIKELY(!pTimerEvent)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent")};
        QNWARNING("update::UpdateManager", errorDescription);
        Q_EMIT notifyError(std::move(errorDescription));
        return;
    }

    const int timerId = pTimerEvent->timerId();
    killTimer(timerId);

    QNDEBUG("update::UpdateManager", "Timer event for timer id " << timerId);

    if (timerId == m_nextUpdateCheckTimerId) {
        m_nextUpdateCheckTimerId = -1;

        if (m_updateProviderInProgress || m_currentUpdateUrlOnceOpened) {
            QNDEBUG(
                "update::UpdateManager",
                "Will not check for updates on timer: update has already been "
                "launched");
        }
        else if (m_updateInstalledPendingRestart) {
            QNDEBUG(
                "update::UpdateManager",
                "Update is already installed, pending app restart");
        }
        else {
            checkForUpdatesImpl();
        }
    }
    else if (timerId == m_nextIdleStatePollTimerId) {
        m_nextIdleStatePollTimerId = -1;
        askUserAndLaunchUpdate();
    }
}

void UpdateManager::offerUserToRestart()
{
    QNDEBUG("update::UpdateManager", "UpdateManager::offerUserToRestart");

    auto * parentWidget = qobject_cast<QWidget *>(parent());

    const int res = questionMessageBox(
        parentWidget, tr("Restart is required"),
        tr("Restart is required in order to complete the update. Would you "
           "like to restart now?"),
        {}, QMessageBox::Ok | QMessageBox::No);

    if (res != QMessageBox::Ok) {
        QNDEBUG(
            "update::UpdateManager", "User refused to restart after update");
        return;
    }

    Q_EMIT restartAfterUpdateRequested();
}

} // namespace quentier
