#ifndef __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H

#include <client/types/UserWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/SavedSearch.h>
#include <QEverCloud.h>
#include <oauth.h>
#include <QSharedPointer>

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

public Q_SLOTS:
    void start();

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

private:
    void createConnections();

    void launchTagsSync();
    void launchSavedSearchSync();
    void launchLinkedNotebookSync();
    void launchNotebookSync();

    template <class ElementType, class RemoteElementType>
    bool setupElementToFind(const RemoteElementType & remoteElement,
                            const QString & typeName, ElementType & element);

    template <class ContainerType, class LocalType>
    void launchDataElementSync(const QString & typeName, ContainerType & container);

    template <class ElementType>
    void setConflicted(const QString & typeName, ElementType & element);

    template <class ElementType, class RemoteElementType>
    void processConflictedElement(const RemoteElementType & remoteElement,
                                  const QString & typeName, ElementType & element);

    template <class ContainerType>
    void appendDataElementsFromSyncChunkToContainer(const qevercloud::SyncChunk & syncChunk,
                                                    ContainerType & container);

    template <class ElementType>
    void emitFindRequest(const ElementType & elementToFind);

    template <class ContainerType, class ElementType>
    void onFindDataElementCompleted(ElementType element, const QUuid & requestId,
                                    const QString & typeName, ContainerType & container,
                                    QSet<QUuid> & findElementRequestIds);

    template <class ContainerType, class ElementType>
    void onFindDataElementFailed(ElementType element, const QUuid & requestId,
                                 const QString & errorDescription,
                                 const QString & typeName, ContainerType & container,
                                 QSet<QUuid> & findElementRequestIds);

    template <class ElementType>
    void emitAddRequest(const ElementType & elementToAdd);

    template <class ElementType>
    void onAddDataElementCompleted(const ElementType & element, const QUuid & requestId,
                                   const QString & typeName, QSet<QUuid> & addElementRequestIds);

    template <class ElementType>
    void onAddDataElementFailed(const ElementType & element, const QUuid & requestId,
                                const QString & errorDescription, const QString & typeName,
                                QSet<QUuid> & addElementRequestIds);

    template <class ElementType>
    void emitUpdateRequest(const ElementType & elementToUpdate,
                           const ElementType * elementToAddLater = nullptr);

    template <class ElementType, class ElementsToAddByUuid>
    void onUpdateDataElementCompleted(const ElementType & element, const QUuid & requestId,
                                      const QString & typeName, QSet<QUuid> & updateElementRequestIds,
                                      ElementsToAddByUuid & elementsToAddByUuid);

    template <class ElementType, class ElementsToAddByUuid>
    void onUpdateDataElementFailed(const ElementType & element, const QUuid & requestId,
                                   const QString & errorDescription, const QString & typeName,
                                   QSet<QUuid> & updateElementRequestIds,
                                   ElementsToAddByUuid & elementsToAddByUuid);

    template <class ContainerType, class Predicate>
    typename ContainerType::iterator findItemByName(ContainerType & container,
                                                    const QString & name);

    template <class ContainerType, class ElementType>
    typename ContainerType::iterator findItem(ContainerType & container,
                                              const ElementType & element,
                                              const QString & typeName);

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

private:
    LocalStorageManagerThreadWorker &                               m_localStorageManagerThreadWorker;
    QSharedPointer<qevercloud::NoteStore>                           m_pNoteStore;
    QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult>   m_pOAuthResult;
    qint32                                                          m_maxSyncChunkEntries;

    QVector<qevercloud::SyncChunk>  m_syncChunks;

    TagsList                    m_tags;
    QHash<QUuid,Tag>            m_tagsToAddPerRenamingUpdateRequestId;
    QSet<QUuid>                 m_findTagRequestIds;
    QSet<QUuid>                 m_addTagRequestIds;
    QSet<QUuid>                 m_updateTagRequestIds;

    SavedSearchesList           m_savedSearches;
    QHash<QUuid,SavedSearch>    m_savedSearchesToAddPerRenamingUpdateRequestId;
    QSet<QUuid>                 m_findSavedSearchRequestIds;
    QSet<QUuid>                 m_addSavedSearchRequestIds;
    QSet<QUuid>                 m_updateSavedSearchRequestIds;

    LinkedNotebooksList         m_linkedNotebooks;
    QSet<QUuid>                 m_findLinkedNotebookRequestIds;
    QSet<QUuid>                 m_addLinkedNotebookRequestIds;
    QSet<QUuid>                 m_updateLinkedNotebookRequestIds;

    NotebooksList               m_notebooks;
    QSet<QUuid>                 m_findNotebookRequestIds;
    QSet<QUuid>                 m_addNotebookRequestIds;
    QSet<QUuid>                 m_updateNotebookRequestIds;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H
