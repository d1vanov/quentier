#ifndef __LIB_QUTE_NOTE__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H
#define __LIB_QUTE_NOTE__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H

#include "NoteStore.h"
#include <qute_note/local_storage/LocalStorageManager.h>
#include <qute_note/types/Tag.h>
#include <qute_note/types/SavedSearch.h>
#include <qute_note/types/Notebook.h>
#include <qute_note/types/Note.h>
#include <QObject>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class SendLocalChangesManager: public QObject
{
    Q_OBJECT
public:
    explicit SendLocalChangesManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                     QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                     QObject * parent = nullptr);

    bool active() const;

Q_SIGNALS:
    void failure(QString errorDescription);

    void finished(qint32 lastUpdateCount, QHash<QString,qint32> lastUpdateCountByLinkedNotebookGuid);

    void rateLimitExceeded(qint32 secondsToWait);
    void conflictDetected();
    void shouldRepeatIncrementalSync();

    void paused(bool pendingAuthenticaton);
    void stopped();

    void requestAuthenticationToken();
    void requestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString, QString> > linkedNotebookGuidsAndShareKeys);

    // progress information
    void receivedUserAccountDirtyObjects();
    void receivedAllDirtyObjects();

public Q_SLOTS:
    void start(qint32 updateCount, QHash<QString,qint32> updateCountByLinkedNotebookGuid);
    void stop();
    void pause();
    void resume();

    void onAuthenticationTokensForLinkedNotebooksReceived(QHash<QString,QString> authenticationTokensByLinkedNotebookGuid,
                                                          QHash<QString,qevercloud::Timestamp> authenticationTokenExpirationTimesByLinkedNotebookGuid);

// private signals:
Q_SIGNALS:
    // signals to request dirty & not yet synchronized objects from local storage
    void requestLocalUnsynchronizedTags(LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset,
                                        LocalStorageManager::ListTagsOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QString linkedNotebookGuid, QUuid requestId);
    void requestLocalUnsynchronizedSavedSearches(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QString linkedNotebookGuid, QUuid requestId);
    void requestLocalUnsynchronizedNotebooks(LocalStorageManager::ListObjectsOptions flag,
                                             size_t limit, size_t offset,
                                             LocalStorageManager::ListNotebooksOrder::type order,
                                             LocalStorageManager::OrderDirection::type orderDirection,
                                             QString linkedNotebookGuid, QUuid requestId);
    void requestLocalUnsynchronizedNotes(LocalStorageManager::ListObjectsOptions flag,
                                         size_t limit, size_t offset,
                                         LocalStorageManager::ListNotesOrder::type order,
                                         LocalStorageManager::OrderDirection::type orderDirection,
                                         QString linkedNotebookGuid, QUuid requestId);

    // signal to request the list of linked notebooks so that all linked notebook guids would be available
    void requestLinkedNotebooksList(LocalStorageManager::ListObjectsOptions flag,
                                    size_t limit, size_t offset,
                                    LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                    LocalStorageManager::OrderDirection::type orderDirection,
                                    QUuid requestId);

    void updateTag(Tag tag, QUuid requestId);
    void updateSavedSearch(SavedSearch savedSearch, QUuid requestId);
    void updateNotebook(Notebook notebook, QUuid requestId);
    void updateNote(Note note, QUuid requestId);

    void findNotebook(Notebook notebook, QUuid requestId);

private Q_SLOTS:
    void onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions flag,
                                  size_t limit, size_t offset,
                                  LocalStorageManager::ListTagsOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QList<Tag> tags, QString linkedNotebookGuid, QUuid requestId);
    void onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                               size_t limit, size_t offset,
                               LocalStorageManager::ListTagsOrder::type order,
                               LocalStorageManager::OrderDirection::type orderDirection,
                               QString errorDescription, QString linkedNotebookGuid, QUuid requestId);

    void onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                           size_t limit, size_t offset,
                                           LocalStorageManager::ListSavedSearchesOrder::type order,
                                           LocalStorageManager::OrderDirection::type orderDirection,
                                           QList<SavedSearch> savedSearches, QString linkedNotebookGuid, QUuid requestId);
    void onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset,
                                        LocalStorageManager::ListSavedSearchesOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QString linkedNotebookGuid, QString errorDescription, QUuid requestId);

    void onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                       size_t limit, size_t offset,
                                       LocalStorageManager::ListNotebooksOrder::type order,
                                       LocalStorageManager::OrderDirection::type orderDirection,
                                       QList<Notebook> notebooks, QString linkedNotebookGuid, QUuid requestId);
    void onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                    size_t limit, size_t offset,
                                    LocalStorageManager::ListNotebooksOrder::type order,
                                    LocalStorageManager::OrderDirection::type orderDirection,
                                    QString linkedNotebookGuid, QString errorDescription, QUuid requestId);

    void onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                   size_t limit, size_t offset,
                                   LocalStorageManager::ListNotesOrder::type order,
                                   LocalStorageManager::OrderDirection::type orderDirection,
                                   QList<Note> notes, QString linkedNotebookGuid, QUuid requestId);
    void onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions flag,
                                size_t limit, size_t offset,
                                LocalStorageManager::ListNotesOrder::type order,
                                LocalStorageManager::OrderDirection::type orderDirection,
                                QString linkedNotebookGuid, QString errorDescription, QUuid requestId);

    void onListLinkedNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset,
                                        LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QList<LinkedNotebook> linkedNotebooks, QUuid requestId);
    void onListLinkedNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                     size_t limit, size_t offset,
                                     LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                     LocalStorageManager::OrderDirection::type orderDirection,
                                     QString errorDescription, QUuid requestId);

    void onUpdateTagCompleted(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId);

    void onUpdateSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch savedSearch, QString errorDescription, QUuid requestId);

    void onUpdateNotebookCompleted(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);

    void onUpdateNoteCompleted(Note note, Notebook notebook, QUuid requestId);
    void onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId);

    void onFindNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);

private:
    virtual void timerEvent(QTimerEvent * pEvent);

private:
    SendLocalChangesManager() Q_DECL_DELETE;

private:
    void createConnections();
    void disconnectFromLocalStorage();

    void requestStuffFromLocalStorage(const QString & linkedNotebookGuid = QString());

    void checkListLocalStorageObjectsCompletion();

    void sendLocalChanges();
    void sendTags();
    void sendSavedSearches();
    void sendNotebooks();

    void checkAndSendNotes();
    void sendNotes();

    void findNotebooksForNotes();

    bool rateLimitIsActive() const;

    bool hasPendingRequests() const;
    void finalize();
    void clear();
    void killAllTimers();

    bool checkAndRequestAuthenticationTokensForLinkedNotebooks();

    void handleAuthExpiration();

private:
    class CompareGuidAndShareKeyByGuid
    {
    public:
        CompareGuidAndShareKeyByGuid(const QString & guid) : m_guid(guid) {}

        bool operator()(const QPair<QString, QString> & pair) const
        {
            return pair.first == m_guid;
        }

    private:
        QString m_guid;
    };

private:
    LocalStorageManagerThreadWorker     &   m_localStorageManagerThreadWorker;
    NoteStore                               m_noteStore;
    qint32                                  m_lastUpdateCount;
    QHash<QString,qint32>                   m_lastUpdateCountByLinkedNotebookGuid;

    bool                                    m_shouldRepeatIncrementalSync;

    bool                                    m_active;
    bool                                    m_paused;
    bool                                    m_requestedToStop;

    bool                                    m_connectedToLocalStorage;
    bool                                    m_receivedDirtyLocalStorageObjectsFromUsersAccount;
    bool                                    m_receivedAllDirtyLocalStorageObjects;

    QUuid                                   m_listDirtyTagsRequestId;
    QUuid                                   m_listDirtySavedSearchesRequestId;
    QUuid                                   m_listDirtyNotebooksRequestId;
    QUuid                                   m_listDirtyNotesRequestId;
    QUuid                                   m_listLinkedNotebooksRequestId;

    QSet<QUuid>                             m_listDirtyTagsFromLinkedNotebooksRequestIds;
    QSet<QUuid>                             m_listDirtyNotebooksFromLinkedNotebooksRequestIds;
    QSet<QUuid>                             m_listDirtyNotesFromLinkedNotebooksRequestIds;

    QList<Tag>                              m_tags;
    QList<SavedSearch>                      m_savedSearches;
    QList<Notebook>                         m_notebooks;
    QList<Note>                             m_notes;

    QList<QPair<QString, QString> >         m_linkedNotebookGuidsAndShareKeys;

    int                                     m_lastProcessedLinkedNotebookGuidIndex;
    QHash<QString, QString>                 m_authenticationTokensByLinkedNotebookGuid;
    QHash<QString,qevercloud::Timestamp>    m_authenticationTokenExpirationTimesByLinkedNotebookGuid;
    bool                                    m_pendingAuthenticationTokensForLinkedNotebooks;

    QSet<QUuid>                             m_updateTagRequestIds;
    QSet<QUuid>                             m_updateSavedSearchRequestIds;
    QSet<QUuid>                             m_updateNotebookRequestIds;
    QSet<QUuid>                             m_updateNoteRequestIds;

    QSet<QUuid>                             m_findNotebookRequestIds;
    QHash<QString, Notebook>                m_notebooksByGuidsCache;

    int                                     m_sendTagsPostponeTimerId;
    int                                     m_sendSavedSearchesPostponeTimerId;
    int                                     m_sendNotebooksPostponeTimerId;
    int                                     m_sendNotesPostponeTimerId;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H
