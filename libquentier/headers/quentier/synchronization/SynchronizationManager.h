/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_H
#define LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_H

#include <quentier/types/Account.h>
#include <quentier/utility/Linkage.h>
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QObject>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(SynchronizationManagerPrivate)

class QUENTIER_EXPORT SynchronizationManager: public QObject
{
    Q_OBJECT
public:
    SynchronizationManager(const QString & consumerKey, const QString & consumerSecret,
                           const QString & host, LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    virtual ~SynchronizationManager();

    bool active() const;
    bool paused() const;

public Q_SLOTS:
    void synchronize();
    void pause();
    void resume();
    void stop();
    void revokeAuthentication(const qevercloud::UserID userId);

Q_SIGNALS:
    void failed(QNLocalizedString errorDescription);
    void finished(Account account);

    void authenticationRevoked(bool success, QNLocalizedString errorDescription,
                               qevercloud::UserID userId);

    // state signals
    void remoteToLocalSyncPaused(bool pendingAuthenticaton);
    void remoteToLocalSyncStopped();

    void sendLocalChangesPaused(bool pendingAuthenticaton);
    void sendLocalChangesStopped();

    // other informative signals
    void willRepeatRemoteToLocalSyncAfterSendingChanges();
    void detectedConflictDuringLocalChangesSending();
    void rateLimitExceeded(qint32 secondsToWait);

    void remoteToLocalSyncDone();
    void progress(QNLocalizedString message, double workDonePercentage);

private:
    SynchronizationManager() Q_DECL_EQ_DELETE;
    Q_DISABLE_COPY(SynchronizationManager)

    SynchronizationManagerPrivate * d_ptr;
    Q_DECLARE_PRIVATE(SynchronizationManager)
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_H
