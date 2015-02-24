#ifndef __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H

#include "NoteStore.h"
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/Tag.h>
#include <client/types/SavedSearch.h>
#include <client/types/Notebook.h>
#include <client/types/Note.h>
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

Q_SIGNALS:
    void failure(QString errorDescription);
    void finished();
    void rateLimitExceeded(qint32 secondsToWait);

public Q_SLOTS:
    void start();

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

private:
    SendLocalChangesManager() Q_DECL_DELETE;

private:
    void createConnections();
    void disconnectFromLocalStorage();

    void requestStuffFromLocalStorage(const QString & linkedNotebookGuid);

    void checkListLocalStorageObjectsCompletion();

    void sendLocalChanges();
    void sendTags();
    void sendSavedSearches();
    void sendNotebooks();
    void sendNotes();

private:
    LocalStorageManagerThreadWorker &   m_localStorageManagerThreadWorker;
    NoteStore                           m_noteStore;

    bool                                m_connectedToLocalStorage;

    QUuid                               m_listDirtyTagsRequestId;
    QUuid                               m_listDirtySavedSearchesRequestId;
    QUuid                               m_listDirtyNotebooksRequestId;
    QUuid                               m_listDirtyNotesRequestId;
    QUuid                               m_listLinkedNotebooksRequestId;

    QHash<QUuid,QString>                m_linkedNotebookGuidByListDirtyTagsRequestIds;
    QHash<QUuid,QString>                m_linkedNotebookGuidByListDirtyNotebooksRequestIds;
    QHash<QUuid,QString>                m_linkedNotebookGuidByListDirtyNotesRequestIds;

    QList<Tag>                          m_tags;
    QList<SavedSearch>                  m_savedSearches;
    QList<Notebook>                     m_notebooks;
    QList<Note>                         m_notes;

    QStringList                         m_linkedNotebookGuids;
    int                                 m_lastProcessedLinkedNotebookGuidIndex;

    QSet<QUuid>                         m_updateTagRequestIds;
    QSet<QUuid>                         m_updateSavedSearchRequestIds;
    QSet<QUuid>                         m_updateNotebookRequestIds;
    QSet<QUuid>                         m_updateNoteRequestIds;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H
