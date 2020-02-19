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

#ifndef QUENTIER_UPDATE_UPDATE_MANAGER_H
#define QUENTIER_UPDATE_UPDATE_MANAGER_H

#include "IUpdateProvider.h"

#include <lib/preferences/UpdateSettings.h>

#include <quentier/types/ErrorString.h>

#include <QObject>
#include <QUrl>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QProgressDialog)

namespace quentier {

class UpdateManager: public QObject
{
    Q_OBJECT
public:
    explicit UpdateManager(QObject * parent = nullptr);

    virtual ~UpdateManager() override;

    bool isEnabled() const
    {
        return m_updateCheckEnabled;
    }

    bool shouldCheckForUpdatesOnStartup() const
    {
        return m_checkForUpdatesOnStartup;
    }

    bool useContinuousUpdateChannel() const
    {
        return m_useContinuousUpdateChannel;
    }

    qint64 checkForUpdatesIntervalMsec() const
    {
        return m_checkForUpdatesIntervalMsec;
    }

    const QString & updateChannel() const
    {
        return m_updateChannel;
    }

    UpdateProvider updateProvider() const
    {
        return m_updateProvider;
    }

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

public Q_SLOTS:
    void checkForUpdates();

private Q_SLOTS:
    void onCheckForUpdatesError(ErrorString errorDescription);
    void onNoUpdatesAvailable();
    void onUpdatesAvailableAtUrl(QUrl downloadUrl);
    void onUpdatesAvailable(std::shared_ptr<IUpdateProvider> provider);

    void onUpdateProviderFinished(
        bool status, ErrorString errorDescription, bool needsRestart);

    void onUpdateProviderProgress(double value, QString message);

private:
    void readPersistentSettings();
    void setupNextCheckForUpdatesTimer();
    void recycleUpdateChecker(QObject * sender);

private:
    virtual void timerEvent(QTimerEvent * pTimerEvent) override;

private:
    Q_DISABLE_COPY(UpdateManager);

private:
    bool    m_updateCheckEnabled = false;
    bool    m_checkForUpdatesOnStartup = false;
    bool    m_useContinuousUpdateChannel = false;

    qint64  m_checkForUpdatesIntervalMsec = 0;
    QString m_updateChannel;

    UpdateProvider  m_updateProvider;

    qint64  m_lastCheckForUpdatesTimestamp = 0;

    int     m_nextUpdateCheckTimerId = -1;

    std::shared_ptr<IUpdateProvider>    m_pCurrentUpdateProvider;

    std::unique_ptr<QProgressDialog>    m_pUpdateProgressDialog;
};

} // namespace quentier

#endif // QUENTIER_UPDATE_UPDATE_MANAGER_H
