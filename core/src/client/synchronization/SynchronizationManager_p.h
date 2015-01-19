#ifndef __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SYNCHRONIZATION_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SYNCHRONIZATION_MANAGER_PRIVATE_H

#include "RemoteToLocalSynchronizationManager.h"
#include <tools/qt4helper.h>
#include <QEverCloud.h>
#include <oauth.h>
#include <keychain.h>
#include <QObject>

namespace qute_note {

class SynchronizationManagerPrivate: public QObject
{
    Q_OBJECT
public:
    SynchronizationManagerPrivate(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    virtual ~SynchronizationManagerPrivate();

    void synchronize();

Q_SIGNALS:
    void notifyError(QString errorDescription);

// private signals
    void sendAuthenticationTokensForLinkedNotebooks(QHash<QString,QString> authenticationTokensByLinkedNotebookGuids);

private Q_SLOTS:
    void onOAuthResult(bool result);
    void onOAuthSuccess();
    void onOAuthFailure();

    void onKeychainJobFinished(QKeychain::Job * job);

    void onRequestAuthenticationToken();
    void onRequestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString,QString> > linkedNotebookGuidsAndShareKeys);
    void onRemoteToLocalSyncFinished(qint32 lastUpdateCount, qint32 lastSyncTime);

private:
    SynchronizationManagerPrivate() Q_DECL_DELETE;
    SynchronizationManagerPrivate(const SynchronizationManagerPrivate & other) Q_DECL_DELETE;
    SynchronizationManagerPrivate & operator=(const SynchronizationManagerPrivate & other) Q_DECL_DELETE;

    void createConnections();

    struct AuthContext
    {
        enum type {
            Blank = 0,
            SyncLaunch,
            AuthToLinkedNotebooks
        };
    };

    void authenticate(const AuthContext::type authContext);
    void launchOAuth();
    void finalizeAuthentication();

    void launchStoreOAuthResult();
    void finalizeStoreOAuthResult();

    bool tryToGetSyncState(qevercloud::SyncState & syncState);

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

private:
    qint32      m_maxSyncChunkEntries;
    qint32      m_lastUpdateCount;
    qint32      m_lastSyncTime;

    NoteStore               m_noteStore;
    AuthContext::type       m_authContext;

    int         m_launchSyncPostponeTimerId;

    QScopedPointer<qevercloud::EvernoteOAuthWebView>                m_pOAuthWebView;
    QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult>   m_pOAuthResult;

    RemoteToLocalSynchronizationManager     m_remoteToLocalSyncManager;
    QList<QPair<QString,QString> >          m_linkedNotebookGuidsAndShareKeysWaitingForAuth;
    QHash<QString,QString>                  m_cachedLinkedNotebookAuthTokensByGuid;
    QHash<QString,qevercloud::Timestamp>    m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid;

    int             m_authenticateToLinkedNotebooksPostponeTimerId;
    bool            m_receivedRequestToAuthenticateToLinkedNotebooks;

    QKeychain::ReadPasswordJob  m_readAuthTokenJob;
    QKeychain::WritePasswordJob m_writeAuthTokenJob;

    QHash<QString,QSharedPointer<QKeychain::ReadPasswordJob> >   m_readLinkedNotebookAuthTokenJobsByGuid;
    QHash<QString,QSharedPointer<QKeychain::WritePasswordJob> >  m_writeLinkedNotebookAuthTokenJobsByGuid;

    QSet<QString>   m_linkedNotebookGuidsWithoutLocalAuthData;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SYNCHRONIZATION_MANAGER_PRIVATE_H
