#ifndef __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H

#include "NoteStore.h"
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/UserWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/SavedSearch.h>
#include <oauth.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class FullSynchronizationManager: public QObject
{
    Q_OBJECT
public:
     explicit FullSynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                         QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                         QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult> pOAuthResult,
                                         QObject * parent = nullptr);

Q_SIGNALS:
    void failure(QString errorDescription);
    void finished();
    void rateLimitExceeded(qint32 secondsToWait);

public Q_SLOTS:
    void start(qint32 afterUsn = 0);

// private signals
Q_SIGNALS:
    void addUser(UserWrapper user, QUuid requestId = QUuid());
    void updateUser(UserWrapper user, QUuid requestId = QUuid());
    void findUser(UserWrapper user, QUuid requestId = QUuid());
    void deleteUser(UserWrapper user, QUuid requestId = QUuid());
    void expungeUser(UserWrapper user, QUuid requestId = QUuid());

    void addNotebook(Notebook notebook, QUuid requestId = QUuid());
    void updateNotebook(Notebook notebook, QUuid requestId = QUuid());
    void findNotebook(Notebook notebook, QUuid requestId = QUuid());
    void expungeNotebook(Notebook notebook, QUuid requestId = QUuid());

    void addNote(Note note, Notebook notebook, QUuid requestId = QUuid());
    void updateNote(Note note, Notebook notebook, QUuid requestId = QUuid());
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId = QUuid());
    void deleteNote(Note note, QUuid requestId = QUuid());
    void expungeNote(Note note, QUuid requestId = QUuid());

    void addTag(Tag tag, QUuid requestId = QUuid());
    void updateTag(Tag tag, QUuid requestId = QUuid());
    void findTag(Tag tag, QUuid requestId = QUuid());
    void deleteTag(Tag tag, QUuid requestId = QUuid());
    void expungeTag(Tag tag, QUuid requestId = QUuid());

    void addResource(ResourceWrapper resource, Note note, QUuid requestId = QUuid());
    void updateResource(ResourceWrapper resource, Note note, QUuid requestId = QUuid());
    void findResource(ResourceWrapper resource, bool withBinaryData, QUuid requestId = QUuid());
    void expungeResource(ResourceWrapper resource, QUuid requestId = QUuid());

    void addLinkedNotebook(LinkedNotebook notebook, QUuid requestId = QUuid());
    void updateLinkedNotebook(LinkedNotebook notebook, QUuid requestId = QUuid());
    void findLinkedNotebook(LinkedNotebook linkedNotebook, QUuid requestId = QUuid());
    void expungeLinkedNotebook(LinkedNotebook notebook, QUuid requestId = QUuid());

    void addSavedSearch(SavedSearch savedSearch, QUuid requestId = QUuid());
    void updateSavedSearch(SavedSearch savedSearch, QUuid requestId = QUuid());
    void findSavedSearch(SavedSearch savedSearch, QUuid requestId = QUuid());
    void expungeSavedSearch(SavedSearch savedSearch, QUuid requestId = QUuid());

    // signals to request dirty & not yet synchronized objects from local storage
    void requestLocalUnsynchronizedTags(LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset,
                                        LocalStorageManager::ListTagsOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QUuid requestId);
    void requestLocalUnsynchronizedSavedSearches(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QUuid requestId);
    void requestLocalUnsynchronizedNotebooks(LocalStorageManager::ListObjectsOptions flag,
                                             size_t limit, size_t offset,
                                             LocalStorageManager::ListNotebooksOrder::type order,
                                             LocalStorageManager::OrderDirection::type orderDirection,
                                             QUuid requestId);
    void requestLocalUnsynchronizedNotes(LocalStorageManager::ListObjectsOptions flag,
                                         size_t limit, size_t offset,
                                         LocalStorageManager::ListNotesOrder::type order,
                                         LocalStorageManager::OrderDirection::type orderDirection,
                                         QUuid requestId);

private Q_SLOTS:
    void onFindUserCompleted(UserWrapper user, QUuid requestId);
    void onFindUserFailed(UserWrapper user, QString errorDescription, QUuid requestId);
    void onFindNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindNoteCompleted(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId);
    void onFindTagCompleted(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onFindResourceCompleted(ResourceWrapper resource, bool withResourceBinaryData, QUuid requestId);
    void onFindResourceFailed(ResourceWrapper resource, bool withResourceBinaryData,
                              QString errorDescription, QUuid requestId);
    void onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId);
    void onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId);
    void onFindSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId);
    void onFindSavedSearchFailed(SavedSearch savedSearch, QString errorDescription, QUuid requestId);

    void onAddTagCompleted(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onUpdateTagCompleted(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId);

    void onAddSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);
    void onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);

    void onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId);
    void onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId);
    void onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId);
    void onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId);

    void onAddNotebookCompleted(Notebook notebook, QUuid requestId);
    void onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNotebookCompleted(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);

    void onAddNoteCompleted(Note note, Notebook notebook, QUuid requestId);
    void onAddNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNoteCompleted(Note note, Notebook notebook, QUuid requestId);
    void onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId);


    void onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions flag,
                                  size_t limit, size_t offset,
                                  LocalStorageManager::ListTagsOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QList<Tag> tags, QUuid requestId);
    void onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                               size_t limit, size_t offset,
                               LocalStorageManager::ListTagsOrder::type order,
                               LocalStorageManager::OrderDirection::type orderDirection,
                               QString errorDescription, QUuid requestId);

    void onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                           size_t limit, size_t offset,
                                           LocalStorageManager::ListSavedSearchesOrder::type order,
                                           LocalStorageManager::OrderDirection::type orderDirection,
                                           QList<SavedSearch> savedSearches, QUuid requestId);
    void onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset,
                                        LocalStorageManager::ListSavedSearchesOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QString errorDescription, QUuid requestId);

    void onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                       size_t limit, size_t offset,
                                       LocalStorageManager::ListNotebooksOrder::type order,
                                       LocalStorageManager::OrderDirection::type orderDirection,
                                       QList<Notebook> notebooks, QUuid requestId);
    void onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                    size_t limit, size_t offset,
                                    LocalStorageManager::ListNotebooksOrder::type order,
                                    LocalStorageManager::OrderDirection::type orderDirection,
                                    QString errorDescription, QUuid requestId);

    void onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                   size_t limit, size_t offset,
                                   LocalStorageManager::ListNotesOrder::type order,
                                   LocalStorageManager::OrderDirection::type orderDirection,
                                   QList<Note> notes, QUuid requestId);
    void onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions flag,
                                size_t limit, size_t offset,
                                LocalStorageManager::ListNotesOrder::type order,
                                LocalStorageManager::OrderDirection::type orderDirection,
                                QString errorDescription, QUuid requestId);

private:
    void createConnections();

    void launchTagsSync();
    void launchSavedSearchSync();
    void launchLinkedNotebookSync();
    void launchNotebookSync();

    template <class ContainerType, class LocalType>
    void launchDataElementSync(const QString & typeName, ContainerType & container);

    template <class ElementType>
    void setConflicted(const QString & typeName, ElementType & element);

    template <class ElementType>
    void processConflictedElement(const ElementType & remoteElement,
                                  const QString & typeName, ElementType & element);

    template <class ElementType>
    void checkUpdateSequenceNumbersAndProcessConflictedElements(const ElementType & remoteElement,
                                                                const QString & typeName,
                                                                ElementType & localElement);

    template <class ContainerType>
    void appendDataElementsFromSyncChunkToContainer(const qevercloud::SyncChunk & syncChunk,
                                                    ContainerType & container);

    // ========= Find by guid helpers ===========

    template <class ElementType>
    void emitFindByGuidRequest(const QString & guid);

    template <class ElementType, class ContainerType>
    bool onFoundDuplicateByGuid(ElementType element, const QUuid & requestId,
                                const QString & typeName, ContainerType & container,
                                QSet<QUuid> & findByGuidRequestIds);

    template <class ContainerType, class ElementType>
    bool onNoDuplicateByGuid(ElementType element, const QUuid & requestId,
                             const QString & errorDescription,
                             const QString & typeName, ContainerType & container,
                             QSet<QUuid> & findElementRequestIds);

    // ========= Find by name helpers ===========

    template <class ElementType>
    void emitFindByNameRequest(const ElementType & elementToFind);

    template <class ContainerType, class ElementType>
    bool onFoundDuplicateByName(ElementType element, const QUuid & requestId,
                                const QString & typeName, ContainerType & container,
                                QSet<QUuid> & findElementRequestIds);

    template <class ContainerType, class ElementType>
    bool onNoDuplicateByName(ElementType element, const QUuid & requestId,
                             const QString & errorDescription,
                             const QString & typeName, ContainerType & container,
                             QSet<QUuid> & findElementRequestIds);

    // ========= Add helpers ============

    template <class ElementType>
    void emitAddRequest(const ElementType & elementToAdd);

    template <class ElementType>
    void onAddDataElementCompleted(const ElementType & element, const QUuid & requestId,
                                   const QString & typeName, QSet<QUuid> & addElementRequestIds);

    template <class ElementType>
    void onAddDataElementFailed(const ElementType & element, const QUuid & requestId,
                                const QString & errorDescription, const QString & typeName,
                                QSet<QUuid> & addElementRequestIds);

    // ========== Update helpers ===========

    template <class ElementType>
    void emitUpdateRequest(const ElementType & elementToUpdate,
                           const ElementType * elementToAddLater = nullptr);

    template <class ElementType, class ElementsToAddByUuid>
    void onUpdateDataElementCompleted(const ElementType & element, const QUuid & requestId,
                                      const QString & typeName, QSet<QUuid> & updateElementRequestIds,
                                      ElementsToAddByUuid & elementsToAddByRenameRequestId);

    template <class ElementType, class ElementsToAddByUuid>
    void onUpdateDataElementFailed(const ElementType & element, const QUuid & requestId,
                                   const QString & errorDescription, const QString & typeName,
                                   QSet<QUuid> & updateElementRequestIds,
                                   ElementsToAddByUuid & elementsToAddByRenameRequestId);

    template<class ElementType>
    void performPostAddOrUpdateChecks();

    template <class ElementType>
    void unsetLocalGuid(ElementType & element);

    // ========= Find in blocks from sync chunk helpers ===========

    template <class ContainerType, class ElementType>
    typename ContainerType::iterator findItemByName(ContainerType & container,
                                                    const ElementType & element,
                                                    const QString & typeName);

    template <class ContainerType, class ElementType>
    typename ContainerType::iterator findItemByGuid(ContainerType & container,
                                                    const ElementType & element,
                                                    const QString & typeName);

    // ========= Helpers launching the sync of dependent data elements ==========
    void checkNotebooksAndTagsSyncAndLaunchNotesSync();
    void launchNotesSync();

    // Helpers launching the sync of content from someone else's shared notebooks, to be used
    // when LinkedNotebook representing pointers to content from someone else's account are in sync
    void checkLinkedNotebooksSyncAndLaunchLinkedNotebookContentSync();
    void checkLinkedNotebooksNotebooksAndTagsSyncAndLaynchLinkedNotebookNotesSync();

    void launchLinkedNotebookTagsSync();
    void launchLinkedNotebookNotebooksSync();
    void launchLinkedNotebookNotesSync();

    void checkServerDataMergeCompletion();
    void requestLocalUnsynchronizedData();

    void clear();

    void timerEvent(QTimerEvent * pEvent);

    qint32 tryToGetFullNoteData(Note & note, QString & errorDescription);

    const Notebook * getNotebookPerNote(const Note & note) const;

private:
    FullSynchronizationManager() Q_DECL_DELETE;

private:
    template <class T>
    class CompareItemByName
    {
    public:
        CompareItemByName(const QString & name) : m_name(name) {}
        bool operator()(const T & item) const;

    private:
        const QString m_name;
    };

    template <class T>
    class CompareItemByGuid
    {
    public:
        CompareItemByGuid(const QString & guid) : m_guid(guid) {}
        bool operator()(const T & item) const;

    private:
        const QString m_guid;
    };

    typedef QList<qevercloud::Tag> TagsList;
    typedef QList<qevercloud::SavedSearch> SavedSearchesList;
    typedef QList<qevercloud::LinkedNotebook> LinkedNotebooksList;
    typedef QList<qevercloud::Notebook> NotebooksList;
    typedef QList<qevercloud::Note> NotesList;

private:
    LocalStorageManagerThreadWorker &                               m_localStorageManagerThreadWorker;
    NoteStore                                                       m_noteStore;
    QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult>   m_pOAuthResult;
    qint32                                                          m_maxSyncChunkEntries;

    QVector<qevercloud::SyncChunk>          m_syncChunks;

    TagsList                                m_tags;
    QHash<QUuid,Tag>                        m_tagsToAddPerRequestId;
    QSet<QUuid>                             m_findTagByNameRequestIds;
    QSet<QUuid>                             m_findTagByGuidRequestIds;
    QSet<QUuid>                             m_addTagRequestIds;
    QSet<QUuid>                             m_updateTagRequestIds;

    SavedSearchesList                       m_savedSearches;
    QHash<QUuid,SavedSearch>                m_savedSearchesToAddPerRequestId;
    QSet<QUuid>                             m_findSavedSearchByNameRequestIds;
    QSet<QUuid>                             m_findSavedSearchByGuidRequestIds;
    QSet<QUuid>                             m_addSavedSearchRequestIds;
    QSet<QUuid>                             m_updateSavedSearchRequestIds;

    LinkedNotebooksList                     m_linkedNotebooks;
    QSet<QUuid>                             m_findLinkedNotebookRequestIds;
    QSet<QUuid>                             m_addLinkedNotebookRequestIds;
    QSet<QUuid>                             m_updateLinkedNotebookRequestIds;

    NotebooksList                           m_notebooks;
    QHash<QUuid,Notebook>                   m_notebooksToAddPerRequestId;
    QSet<QUuid>                             m_findNotebookByNameRequestIds;
    QSet<QUuid>                             m_findNotebookByGuidRequestIds;
    QSet<QUuid>                             m_addNotebookRequestIds;
    QSet<QUuid>                             m_updateNotebookRequestIds;

    NotesList                               m_notes;
    QSet<QUuid>                             m_findNoteByGuidRequestIds;
    QSet<QUuid>                             m_addNoteRequestIds;
    QSet<QUuid>                             m_updateNoteRequestIds;

    typedef QHash<QUuid,QPair<Note,QUuid> > NoteDataPerFindNotebookRequestId;
    NoteDataPerFindNotebookRequestId        m_notesWithFindRequestIdsPerFindNotebookRequestId;

    QHash<QPair<QString,QString>,Notebook>  m_notebooksPerNoteGuids;

    QSet<QString>                           m_localGuidsOfElementsAlreadyAttemptedToFindByName;

    QHash<int,Note>                         m_notesToAddPerAPICallPostponeTimerId;
    QHash<int,Note>                         m_notesToUpdatePerAPICallPostponeTimerId;
    QHash<int,qint32>                       m_afterUsnForSyncChunkPerAPICallPostponeTimerId;

    QUuid                                   m_listDirtyTagsRequestId;
    QUuid                                   m_listDirtySavedSearchesRequestId;
    QUuid                                   m_listDirtyNotebooksRequestId;
    QUuid                                   m_listDirtyNotesRequestId;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H
