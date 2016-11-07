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

/**
 * @brief The SynchronizationManager class encapsulates methods and signals & slots required to perform the full or partial
 * synchronization of data with remote Evernote servers. The class also deals with authentication with Evernote service through
 * OAuth.
 */
class QUENTIER_EXPORT SynchronizationManager: public QObject
{
    Q_OBJECT
public:
    /**
     * @param consumerKey - consumer key for your application obtained from the Evernote service
     * @param consumerSecret - consumer secret for your application obtained from the Evernote service
     * @param host - the host to use for the connection with the Evernote service - typically www.evernote.com
     *               but could be sandbox.evernote.com or some other one
     * @param localStorageManagerThreadWorker - local storage manager
     */
    SynchronizationManager(const QString & consumerKey, const QString & consumerSecret,
                           const QString & host, LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);

    virtual ~SynchronizationManager();

    /**
     * @return true if the synchronization is being performed at the moment, false otherwise
     */
    bool active() const;

    /**
     * @return true if the synchronization has been paused, false otherwise
     */
    bool paused() const;

public Q_SLOTS:

    /**
     * Use this slot to set the current account for the synchronization manager. If the slot is called during the synchronization running,
     * it would stop, any internal caches belonging to previously selected account (if any) would be purged (but persistent settings
     * like the authentication token saved in the system keychain would remain). Setting the current account won't automatically start
     * the synchronization for it, use @link synchronize @endling slot for this.
     *
     * The attempt to set the current account of "Local" type would just clean up the synchronization manager as if it was just created
     */
    void setAccount(Account account);

    /**
     * Use this slot to launch the synchronization of data
     */
    void synchronize();

    /**
     * Use this slot to pause the running synchronization; if no synchronization is running by the moment of this slot call,
     * it has no effect
     */
    void pause();

    /**
     * Use this slot to resume the previously paused synchronization; if no synchronization was paused before the call of this slot,
     * it has no effect
     */
    void resume();

    /**
     * Use this slot to stop the running synchronization; if no synchronization is running by the moment of this slot call,
     * it has no effect
     */
    void stop();

    /**
     * Use this slot to remove any previously cached authentication tokens (and shard ids) for a given user ID. After the call
     * of this method the next attempt to synchronize the data for this user ID would cause the launch of OAuth to get the new
     * authentication token
     */
    void revokeAuthentication(const qevercloud::UserID userId);

Q_SIGNALS:
    /**
     * This signal is emitted when the synchronization fails; at this moment there is no error code explaining the reason
     * of the failure programmatically so the only explanation available is the textual one for the end user
     */
    void failed(QNLocalizedString errorDescription);

    /**
     * This signal is emitted when the synchronization is finished
     * @param account - represents the latest version of @link Account @endlink structure filled during the synchronization procedure
     */
    void finished(Account account);

    /**
     * This signal is emitted in response to the attempt to revoke the authentication for a given user ID
     * @param success - true if the authentication was revoked successfully, false otherwise
     * @param errorDescription - the textual explanation of the failure to revoke the authentication
     * @param userId - the ID of the user for which the revoke of the authentication was requested
     */
    void authenticationRevoked(bool success, QNLocalizedString errorDescription,
                               qevercloud::UserID userId);

    /**
     * This signal is emitted when the "remote to local" synchronization step is paused
     * @param pendingAuthentication - true if the reason for pausing is the need
     * for new authentication token(s), false otherwise
     */
    void remoteToLocalSyncPaused(bool pendingAuthenticaton);

    /**
     * This signal is emitted when the "remote to local" synchronization step is stopped
     */
    void remoteToLocalSyncStopped();

    /**
     * This signal is emitted when the "send local changes" synchronization step is paused
     * @param pendingAuthentication - true if the reason for pausing is the need for new authentication token(s), false otherwise
     */
    void sendLocalChangesPaused(bool pendingAuthenticaton);

    /**
     * This signal is emitted when the "send local changes" synchronization step is stopped
     */
    void sendLocalChangesStopped();

    /**
     * This signal is emitted if during the "send local changes" synchronization step it was found out that new changes
     * from the Evernote service are available yet no conflict between remote and local changes was found yet.
     *
     * Such situation can rarely happen in case of changes introduced concurrently with the
     * running synchronization - perhaps via another client. The algorithm will handle it,
     * the signal is just for the sake of diagnostic
     */
    void willRepeatRemoteToLocalSyncAfterSendingChanges();

    /**
     * This signal is emitted if during the "send local changes" synchronization step it was found out that new changes
     * from the Evernote service are available AND some of them conflict with the local changes being sent.
     *
     * Such situation can rarely happen in case of changes introduced concurrently with the
     * running synchronization - perhaps via another client. The algorithm will handle it by repeating the "remote to local"
     * incremental synchronization step, the signal is just for the sake of diagnostic
     */
    void detectedConflictDuringLocalChangesSending();

    /**
     * This signal is emitted when the Evernote API rate limit is breached during the synchronization; the algorithm
     * will handle it by auto-pausing itself until the time necessary to wait passes and then automatically continuing
     * the synchronization.
     * @param secondsToWait - the amount of time (in seconds) necessary to wait before the synchronization will continue
     */
    void rateLimitExceeded(qint32 secondsToWait);

    /**
     * This signal is emitted when the "remote to local" synchronization step is finished; once that step is done,
     * the algorithn switches to sending the local changes back to the Evernote service
     */
    void remoteToLocalSyncDone();

    /**
     * This signal is emitted several times during the synchronization to notify the interested clients
     * of the synchronization procedure progress
     * @param message - textual description of what is currently being done during the synchronization
     * @param workDonePercentage - the approximate percentage of finished synchronization work
     */
    void progress(QNLocalizedString message, double workDonePercentage);

private:
    SynchronizationManager() Q_DECL_EQ_DELETE;
    Q_DISABLE_COPY(SynchronizationManager)

    SynchronizationManagerPrivate * d_ptr;
    Q_DECLARE_PRIVATE(SynchronizationManager)
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_H
