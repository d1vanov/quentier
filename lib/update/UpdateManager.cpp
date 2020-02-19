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
#include <QProgressDialog>
#include <QTimerEvent>

#include <algorithm>
#include <limits>

namespace quentier {

UpdateManager::UpdateManager(QObject * parent) :
    QObject(parent)
{
    readPersistentSettings();
}

UpdateManager::~UpdateManager() = default;

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

    m_nextUpdateCheckTimerId = startTimer(msecLeft);

    QNDEBUG("Last check for updates was done at "
        << printableDateTimeFromTimestamp(m_lastCheckForUpdatesTimestamp)
        << ", now is " << printableDateTimeFromTimestamp(now)
        << ", will check for updates in " << msecLeft << " milliseconds at "
        << printableDateTimeFromTimestamp(now + msecLeft)
        << ", timer id = " << m_nextUpdateCheckTimerId);
}

void UpdateManager::recycleUpdateChecker(QObject * sender)
{
    auto * pUpdateChecker = qobject_cast<IUpdateChecker*>(sender);
    if (pUpdateChecker) {
        pUpdateChecker->disconnect(this);
        pUpdateChecker->deleteLater();
    }
}

void UpdateManager::checkForUpdates()
{
    QNDEBUG("UpdateManager::checkForUpdates");

    auto * pUpdateChecker = newUpdateChecker(m_updateProvider, this);
    pUpdateChecker->setUpdateChannel(m_updateChannel);
    pUpdateChecker->setUseContinuousUpdateChannel(m_useContinuousUpdateChannel);

    QObject::connect(
        pUpdateChecker,
        &IUpdateChecker::failure,
        this,
        &UpdateManager::onCheckForUpdatesError);

    QObject::connect(
        pUpdateChecker,
        &IUpdateChecker::noUpdatesAvailable,
        this,
        &UpdateManager::onNoUpdatesAvailable);

    QObject::connect(
        pUpdateChecker,
        qOverload<QUrl>(&IUpdateChecker::updatesAvailable),
        this,
        &UpdateManager::onUpdatesAvailableAtUrl);

    QObject::connect(
        pUpdateChecker,
        qOverload<std::shared_ptr<IUpdateProvider>>(
            &IUpdateChecker::updatesAvailable),
        this,
        &UpdateManager::onUpdatesAvailable);

    pUpdateChecker->checkForUpdates();
}

void UpdateManager::onCheckForUpdatesError(ErrorString errorDescription)
{
    QNDEBUG("UpdateManager::onCheckForUpdatesError: " << errorDescription);

    Q_EMIT notifyError(errorDescription);
    recycleUpdateChecker(sender());
}

void UpdateManager::onNoUpdatesAvailable()
{
    QNDEBUG("UpdateManager::onNoUpdatesAvailable");
    recycleUpdateChecker(sender());
}

void UpdateManager::onUpdatesAvailableAtUrl(QUrl downloadUrl)
{
    QNDEBUG("UpdateManager::onUpdatesAvailableAtUrl: " << downloadUrl);

    recycleUpdateChecker(sender());

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
        return;
    }

    openUrl(downloadUrl);
}

void UpdateManager::onUpdatesAvailable(
    std::shared_ptr<IUpdateProvider> provider)
{
    QNDEBUG("UpdateManager::onUpdatesAvailable");

    recycleUpdateChecker(sender());

    Q_ASSERT(provider);

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
        return;
    }

    if (m_pCurrentUpdateProvider) {
        m_pCurrentUpdateProvider->disconnect(this);
        m_pCurrentUpdateProvider.reset();
    }

    m_pCurrentUpdateProvider = std::move(provider);

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

    if (timerId == m_nextUpdateCheckTimerId) {
        checkForUpdates();
        setupNextCheckForUpdatesTimer();
    }
}

} // namespace quentier
