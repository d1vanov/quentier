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

#include <quentier/types/ErrorString.h>

#include <QObject>

namespace quentier {

class UpdateManager: public QObject
{
    Q_OBJECT
public:
    explicit UpdateManager(QObject * parent = nullptr);

private:
    void readPersistentSettings();
    void setupNextCheckForUpdatesTimer();

    void checkForUpdates();

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

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
    QString m_updateProviderName;

    qint64  m_lastCheckForUpdatesTimestamp = 0;

    int     m_nextUpdateCheckTimerId = -1;
};

} // namespace quentier

#endif // QUENTIER_UPDATE_UPDATE_MANAGER_H
