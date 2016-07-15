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

#ifndef LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_PRIVATE_H
#define LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_PRIVATE_H

#include "RemoteToLocalSynchronizationManager.h"
#include "SendLocalChangesManager.h"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QEverCloud.h>
#include <oauth.h>
#include <keychain.h>
#include <QObject>

namespace quentier {

class SynchronizationManagerPrivate: public QObject
{
    Q_OBJECT
public:
    SynchronizationManagerPrivate(const QString & consumerKey, const QString & consumerSecret,
                                  LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    virtual ~SynchronizationManagerPrivate();

    bool active() const;
    bool paused() const;

Q_SIGNALS:
    void notifyError(QNLocalizedString errorDescription);
    void notifyRemoteToLocalSyncDone();
    void notifyFinish();

// state signals
    void remoteToLocalSyncPaused(bool pendingAuthenticaton);
    void remoteToLocalSyncStopped();

    void sendLocalChangesPaused(bool pendingAuthenticaton);
    void sendLocalChangesStopped();

// other informative signals
    void willRepeatRemoteToLocalSyncAfterSendingChanges();
    void detectedConflictDuringLocalChangesSending();
    void rateLimitExceeded(qint32 secondsToWait);

    void progress(QNLocalizedString message, double workDonePercentage);

public Q_SLOTS:
    void synchronize();
    void pause();
    void resume();
    void stop();

Q_SIGNALS:
// private signals
    void sendAuthenticationToken(QString authToken, qevercloud::Timestamp expirationTime);
    void sendAuthenticationTokensForLinkedNotebooks(QHash<QString,QString> authenticationTokensByLinkedNotebookGuids,
                                                    QHash<QString,qevercloud::Timestamp> authenticatonTokenExpirationTimesByLinkedNotebookGuids);
    void sendLastSyncParameters(qint32 lastUpdateCount, qevercloud::Timestamp lastSyncTime,
                                QHash<QString,qint32> lastUpdateCountByLinkedNotebookGuid,
                                QHash<QString,qevercloud::Timestamp> lastSyncTimeByLinkedNotebookGuid);

    void pauseRemoteToLocalSync();
    void resumeRemoteToLocalSync();
    void stopRemoteToLocalSync();

    void pauseSendingLocalChanges();
    void resumeSendingLocalChanges();
    void stopSendingLocalChanges();

private Q_SLOTS:
    void onOAuthResult(bool result);
    void onOAuthSuccess();
    void onOAuthFailure();

    void onKeychainJobFinished(QKeychain::Job * job);

    void onRequestAuthenticationToken();
    void onRequestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString,QString> > linkedNotebookGuidsAndShareKeys);

    void onRequestLastSyncParameters();

    void onRemoteToLocalSyncFinished(qint32 lastUpdateCount, qevercloud::Timestamp lastSyncTime,
                                     QHash<QString,qint32> lastUpdateCountByLinkedNotebookGuid,
                                     QHash<QString,qevercloud::Timestamp> lastSyncTimeByLinkedNotebookGuid);
    void onRemoteToLocalSyncPaused(bool pendingAuthenticaton);
    void onRemoteToLocalSyncStopped();

    // RemoteToLocalSyncManager's progress tracking slots
    void onRemoteToLocalSyncChunksDownloaded();
    void onRemoteToLocalSyncFullNotesContentDownloaded();
    void onRemoteToLocalSyncExpungedFromServerToClient();
    void onRemoteToLocalSyncLinkedNotebooksSyncChunksDownloaded();
    void onRemoteToLocalSyncLinkedNotebooksFullNotesContentDownloaded();

    void onShouldRepeatIncrementalSync();
    void onConflictDetectedDuringLocalChangesSending();

    void onLocalChangesSent(qint32 lastUpdateCount, QHash<QString,qint32> lastUpdateCountByLinkedNotebookGuid);
    void onSendLocalChangesPaused(bool pendingAuthenticaton);
    void onSendLocalChangesStopped();

    // SendLocalChangesManager's progress tracking slots
    void onSendingLocalChangesReceivedUsersDirtyObjects();
    void onSendingLocalChangesReceivedAllDirtyObjects();

    void onRateLimitExceeded(qint32 secondsToWait);

private:
    SynchronizationManagerPrivate() Q_DECL_DELETE;
    SynchronizationManagerPrivate(const SynchronizationManagerPrivate & other) Q_DECL_DELETE;
    SynchronizationManagerPrivate & operator=(const SynchronizationManagerPrivate & other) Q_DECL_DELETE;

    void createConnections();

    void readLastSyncParameters();

    struct AuthContext
    {
        enum type {
            Blank = 0,
            SyncLaunch,
            Request,
            AuthToLinkedNotebooks,
        };
    };

    void authenticate(const AuthContext::type authContext);
    void launchOAuth();
    void finalizeAuthentication();

    void launchStoreOAuthResult();
    void finalizeStoreOAuthResult();

    void launchSync();
    void launchFullSync();
    void launchIncrementalSync();
    void sendChanges();

    virtual void timerEvent(QTimerEvent * pTimerEvent);

    void clear();

    bool validAuthentication() const;
    bool checkIfTimestampIsAboutToExpireSoon(const qevercloud::Timestamp timestamp) const;
    void authenticateToLinkedNotebooks();

    void onReadAuthTokenFinished();
    void onWriteAuthTokenFinished();

    void updatePersistentSyncSettings();

private:
    QString                                 m_consumerKey;
    QString                                 m_consumerSecret;

    qint32                                  m_maxSyncChunkEntries;

    qint32                                  m_lastUpdateCount;
    qevercloud::Timestamp                   m_lastSyncTime;
    QHash<QString,qint32>                   m_cachedLinkedNotebookLastUpdateCountByGuid;
    QHash<QString,qevercloud::Timestamp>    m_cachedLinkedNotebookLastSyncTimeByGuid;
    bool                                    m_onceReadLastSyncParams;

    NoteStore                               m_noteStore;
    AuthContext::type                       m_authContext;

    int                                     m_launchSyncPostponeTimerId;

    QScopedPointer<qevercloud::EvernoteOAuthWebView>                m_pOAuthWebView;
    QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult>   m_pOAuthResult;

    RemoteToLocalSynchronizationManager     m_remoteToLocalSyncManager;
    SendLocalChangesManager                 m_sendLocalChangesManager;

    QList<QPair<QString,QString> >          m_linkedNotebookGuidsAndShareKeysWaitingForAuth;
    QHash<QString,QString>                  m_cachedLinkedNotebookAuthTokensByGuid;
    QHash<QString,qevercloud::Timestamp>    m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid;

    int                                     m_authenticateToLinkedNotebooksPostponeTimerId;
    bool                                    m_receivedRequestToAuthenticateToLinkedNotebooks;

    QKeychain::ReadPasswordJob              m_readAuthTokenJob;
    QKeychain::WritePasswordJob             m_writeAuthTokenJob;

    QHash<QString,QSharedPointer<QKeychain::ReadPasswordJob> >   m_readLinkedNotebookAuthTokenJobsByGuid;
    QHash<QString,QSharedPointer<QKeychain::WritePasswordJob> >  m_writeLinkedNotebookAuthTokenJobsByGuid;

    QSet<QString>                           m_linkedNotebookGuidsWithoutLocalAuthData;

    bool                                    m_shouldRepeatIncrementalSyncAfterSendingChanges;

    bool                                    m_paused;
    bool                                    m_remoteToLocalSyncWasActiveOnLastPause;
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_PRIVATE_H
