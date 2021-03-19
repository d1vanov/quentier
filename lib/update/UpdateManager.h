/*
 * Copyright 2020-2021 Dmitry Ivanov
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

#ifndef QUENTIER_UPDATE_UPDATE_MANAGER_H
#define QUENTIER_UPDATE_UPDATE_MANAGER_H

#include "IUpdateProvider.h"

#include <lib/preferences/UpdateSettings.h>

#include <quentier/types/ErrorString.h>

#include <QJsonObject>
#include <QObject>
#include <QUrl>

#include <memory>

class QProgressDialog;

namespace quentier {

class IUpdateChecker;

class UpdateManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief The IIdleStateInfoProvider interface tells UpdateManager how much
     * time the app has been idle i.e. for how much time the user hasn't touched
     * the app.
     *
     * UpdateManager uses this information to make a decision whether to show
     * the update notification right now or to wait until the app is idle for
     * a sufficient time so the notification won't get in the way of the user.
     */
    class IIdleStateInfoProvider
    {
    public:
        virtual ~IIdleStateInfoProvider() = default;

        [[nodiscard]] virtual qint64 idleTime() const noexcept = 0;
    };

    using IIdleStateInfoProviderPtr = std::shared_ptr<IIdleStateInfoProvider>;

public:
    explicit UpdateManager(
        IIdleStateInfoProviderPtr idleStateInfoProvider,
        QObject * parent = nullptr);

    ~UpdateManager() override;

    [[nodiscard]] bool isEnabled() const noexcept
    {
        return m_updateCheckEnabled;
    }

    void setEnabled(bool enabled);

    [[nodiscard]] bool shouldCheckForUpdatesOnStartup() const noexcept
    {
        return m_checkForUpdatesOnStartup;
    }

    void setShouldCheckForUpdatesOnStartup(const bool check)
    {
        m_checkForUpdatesOnStartup = check;
    }

    [[nodiscard]] bool useContinuousUpdateChannel() const noexcept
    {
        return m_useContinuousUpdateChannel;
    }

    void setUseContinuousUpdateChannel(bool continuous);

    [[nodiscard]] qint64 checkForUpdatesIntervalMsec() const noexcept
    {
        return m_checkForUpdatesIntervalMsec;
    }

    void setCheckForUpdatesIntervalMsec(qint64 interval);

    [[nodiscard]] const QString & updateChannel() const noexcept
    {
        return m_updateChannel;
    }

    void setUpdateChannel(QString channel);

    [[nodiscard]] UpdateProvider updateProvider() const noexcept
    {
        return m_updateProvider;
    }

    void setUpdateProvider(UpdateProvider provider);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);
    void restartAfterUpdateRequested();

public Q_SLOTS:
    /**
     * Public slot for update checks invoked by user explicitly
     */
    void checkForUpdates();

private Q_SLOTS:
    void onCheckForUpdatesError(ErrorString errorDescription);
    void onNoUpdatesAvailable();
    void onUpdatesAvailableAtUrl(QUrl downloadUrl);
    void onUpdatesAvailable(std::shared_ptr<IUpdateProvider> provider);

    void onUpdateProviderFinished(
        bool status, ErrorString errorDescription, bool needsRestart,
        UpdateProvider updateProviderKind);

    void onUpdateProviderProgress(double value, QString message);
    void onCancelUpdateProvider();

    void checkForUpdatesImpl();

private:
    void readPersistentSettings();
    void setupNextCheckForUpdatesTimer();
    void recycleUpdateChecker();

    [[nodiscard]] bool checkIdleState() const;
    void scheduleNextIdleStateCheckForUpdateNotification();

    void askUserAndLaunchUpdate();

    /**
     * @return      False if update provider is in progress and thus was not
     *              cleared, true otherwise
     */
    [[nodiscard]] bool clearCurrentUpdateProvider();

    void clearCurrentUpdateUrl();
    void clearCurrentUpdateChecker();

    void restartUpdateCheckerIfActive();

    void closeUpdateProgressDialog();
    void closeCheckForUpdatesProgressDialog();

private:
    void timerEvent(QTimerEvent * pTimerEvent) override;

    void offerUserToRestart();

private:
    Q_DISABLE_COPY(UpdateManager)

private:
    IIdleStateInfoProviderPtr m_pIdleStateInfoProvider;

    bool m_updateCheckEnabled = false;
    bool m_checkForUpdatesOnStartup = false;
    bool m_useContinuousUpdateChannel = false;

    qint64 m_checkForUpdatesIntervalMsec = 0;
    QString m_updateChannel;

    UpdateProvider m_updateProvider;

    qint64 m_lastCheckForUpdatesTimestamp = 0;

    int m_nextUpdateCheckTimerId = -1;

    int m_nextIdleStatePollTimerId = -1;

    QUrl m_currentUpdateUrl;
    bool m_currentUpdateUrlOnceOpened = false;

    std::shared_ptr<IUpdateProvider> m_pCurrentUpdateProvider;
    bool m_updateProviderInProgress = false;
    int m_lastUpdateProviderProgress = 0;

    bool m_updateInstalledPendingRestart = false;

    IUpdateChecker * m_pCurrentUpdateChecker = nullptr;
    bool m_currentUpdateCheckInvokedByUser = false;

    std::unique_ptr<QProgressDialog> m_pCheckForUpdatesProgressDialog;
    std::unique_ptr<QProgressDialog> m_pUpdateProgressDialog;
};

} // namespace quentier

#endif // QUENTIER_UPDATE_UPDATE_MANAGER_H
