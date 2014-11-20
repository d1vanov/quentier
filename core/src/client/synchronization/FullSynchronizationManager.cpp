#include "FullSynchronizationManager.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <algorithm>

namespace qute_note {

FullSynchronizationManager::FullSynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                       QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                       QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult> pOAuthResult,
                                                       QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_pNoteStore(pNoteStore),
    m_pOAuthResult(pOAuthResult),
    m_maxSyncChunkEntries(50),
    m_syncChunks(),
    m_tags(),
    m_tagsToAddPerRenamingUpdateRequestId(),
    m_findTagRequestIds(),
    m_addTagRequestIds(),
    m_updateTagRequestIds(),
    m_savedSearches(),
    m_savedSearchesToAddPerRenamingUpdateRequestId(),
    m_findSavedSearchRequestIds(),
    m_addSavedSearchRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_linkedNotebooks(),
    m_findLinkedNotebookRequestIds(),
    m_addLinkedNotebookRequestIds(),
    m_updateLinkedNotebookRequestIds(),
    m_notebooks(),
    m_findNotebookRequestIds(),
    m_addNotebookRequestIds(),
    m_updateNotebookRequestIds()
{
    createConnections();
}

void FullSynchronizationManager::start()
{
    QNDEBUG("FullSynchronizationManager::start");

    QUTE_NOTE_CHECK_PTR(m_pNoteStore.data());
    QUTE_NOTE_CHECK_PTR(m_pOAuthResult.data());

    qint32 afterUsn = 0;
    m_syncChunks.clear();
    qevercloud::SyncChunk * pSyncChunk = nullptr;

    QNDEBUG("Downloading sync chunks:");

    while(!pSyncChunk || (pSyncChunk->chunkHighUSN < pSyncChunk->updateCount))
    {
        if (pSyncChunk) {
            afterUsn = pSyncChunk->chunkHighUSN;
        }

        m_syncChunks.push_back(qevercloud::SyncChunk());
        pSyncChunk = &(m_syncChunks.back());
        *pSyncChunk = m_pNoteStore->getSyncChunk(afterUsn, m_maxSyncChunkEntries, true,
                                                 m_pOAuthResult->authenticationToken);
        QNDEBUG("Received sync chunk: " << *pSyncChunk);
    }

    QNDEBUG("Done. Processing tags from buffered sync chunks");

    launchTagsSync();
    launchSavedSearchSync();
    launchLinkedNotebookSync();

    // TODO: continue from here
}

void FullSynchronizationManager::onFindUserCompleted(UserWrapper user, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindUserFailed(UserWrapper user, QString errorDescription, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onFindDataElementCompleted<NotebooksList, Notebook>(notebook, requestId, "Notebook",
                                                        m_notebooks, m_findNotebookRequestIds);
}

void FullSynchronizationManager::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    onFindDataElementFailed<NotebooksList, Notebook>(notebook, requestId, errorDescription,
                                                     "Notebook", m_notebooks, m_findNotebookRequestIds);
}

void FullSynchronizationManager::onFindNoteCompleted(Note note, bool withResourceBinaryData, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindTagCompleted(Tag tag, QUuid requestId)
{
    onFindDataElementCompleted<TagsList, Tag>(tag, requestId, "Tag",
                                              m_tags, m_findTagRequestIds);
}

void FullSynchronizationManager::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    onFindDataElementFailed<TagsList, Tag>(tag, requestId, errorDescription,
                                           "Tag", m_tags, m_findTagRequestIds);
}

void FullSynchronizationManager::onFindResourceCompleted(ResourceWrapper resource, bool withResourceBinaryData, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindResourceFailed(ResourceWrapper resource, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    onFindDataElementCompleted<LinkedNotebooksList, LinkedNotebook>(linkedNotebook, requestId, "LinkedNotebook",
                                                                    m_linkedNotebooks, m_findLinkedNotebookRequestIds);
}

void FullSynchronizationManager::onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook,
                                                            QString errorDescription, QUuid requestId)
{
    onFindDataElementFailed<LinkedNotebooksList, LinkedNotebook>(linkedNotebook, requestId, errorDescription,
                                                                 "LinkedNotebook", m_linkedNotebooks,
                                                                 m_findLinkedNotebookRequestIds);
}

void FullSynchronizationManager::onFindSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId)
{
    onFindDataElementCompleted<SavedSearchesList, SavedSearch>(savedSearch, requestId, "SavedSearch",
                                                               m_savedSearches, m_findSavedSearchRequestIds);
}

void FullSynchronizationManager::onFindSavedSearchFailed(SavedSearch savedSearch, QString errorDescription, QUuid requestId)
{
    onFindDataElementFailed<SavedSearchesList, SavedSearch>(savedSearch, requestId, errorDescription,
                                                            "SavedSearch", m_savedSearches, m_findSavedSearchRequestIds);
}

template <class ElementType>
void FullSynchronizationManager::onAddDataElementCompleted(const ElementType & element,
                                                           const QUuid & requestId,
                                                           const QString & typeName,
                                                           QSet<QUuid> & addElementRequestIds)
{
    QSet<QUuid>::iterator it = addElementRequestIds.find(requestId);
    if (it != addElementRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onAddDataElementCompleted<" << typeName
                << ">: " << typeName << " = " << element << ", requestId = " << requestId);

        Q_UNUSED(addElementRequestIds.erase(it));
    }
}

void FullSynchronizationManager::onAddTagCompleted(Tag tag, QUuid requestId)
{
    onAddDataElementCompleted<Tag>(tag, requestId, "Tag", m_addTagRequestIds);
}

void FullSynchronizationManager::onAddSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onAddDataElementCompleted<SavedSearch>(search, requestId, "SavedSearch", m_addSavedSearchRequestIds);
}

template <class ElementType>
void FullSynchronizationManager::onAddDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                        const QString & errorDescription, const QString & typeName,
                                                        QSet<QUuid> & addElementRequestIds)
{
    QSet<QUuid>::iterator it = addElementRequestIds.find(requestId);
    if (it != addElementRequestIds.end())
    {
        QNWARNING("FullSyncronizationManager::onAddDataElementFailed<" << typeName
                  << ">: " << typeName << " = " << element << ", error description = "
                  << errorDescription << ", requestId = " << requestId);

        Q_UNUSED(addElementRequestIds.erase(it));

        QString error = QT_TR_NOOP("Can't add remote " + typeName + " to local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void FullSynchronizationManager::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed<Tag>(tag, requestId, errorDescription, "Tag", m_addTagRequestIds);
}

void FullSynchronizationManager::onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed<SavedSearch>(search, requestId, errorDescription, "SavedSearch", m_addSavedSearchRequestIds);
}

template <class ElementType, class ElementsToAddByUuid>
void FullSynchronizationManager::onUpdateDataElementCompleted(const ElementType & element,
                                                              const QUuid & requestId,
                                                              const QString & typeName,
                                                              QSet<QUuid> & updateElementRequestIds,
                                                              ElementsToAddByUuid & elementsToAddByUuid)
{
    QSet<QUuid>::iterator it = updateElementRequestIds.find(requestId);
    if (it != updateElementRequestIds.end())
    {
        QNDEBUG("FullSynchronizartionManager::onUpdateDataElementCompleted<" << typeName
                << ">: " << typeName << " = " << element << ", requestId = " << requestId);

        Q_UNUSED(updateElementRequestIds.erase(it));

        typename ElementsToAddByUuid::iterator addIt = elementsToAddByUuid.find(requestId);
        if (addIt != elementsToAddByUuid.end())
        {
            ElementType elementToAdd = addIt.value();
            Q_UNUSED(elementsToAddByUuid.erase(addIt));

            QNDEBUG("Adding new " << typeName << " after renaming the dirty local duplicate: " << elementToAdd);
            emitAddRequest(elementToAdd);
        }
    }
}

void FullSynchronizationManager::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    onUpdateDataElementCompleted<Tag, QHash<QUuid, Tag> >(tag, requestId, "Tag", m_updateTagRequestIds,
                                                          m_tagsToAddPerRenamingUpdateRequestId);
}

void FullSynchronizationManager::onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onUpdateDataElementCompleted<SavedSearch, QHash<QUuid, SavedSearch> >(search, requestId, "SavedSearch",
                                                                          m_updateSavedSearchRequestIds,
                                                                          m_savedSearchesToAddPerRenamingUpdateRequestId);
}

template <class ElementType, class ElementsToAddByUuid>
void FullSynchronizationManager::onUpdateDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                           const QString & errorDescription, const QString & typeName,
                                                           QSet<QUuid> & updateElementRequestIds,
                                                           ElementsToAddByUuid & elementsToAddByUuid)
{
    QSet<QUuid>::iterator it = updateElementRequestIds.find(requestId);
    if (it != updateElementRequestIds.end())
    {
        QNWARNING("FullSynchronizationManager::onUpdateDataElementFailed<" << typeName
                  << ">: " << typeName << " = " << element << ", errorDescription = "
                  << errorDescription << ", requestId = " << requestId);

        Q_UNUSED(updateElementRequestIds.erase(it));

        typename ElementsToAddByUuid::iterator addIt = elementsToAddByUuid.find(requestId);
        if (addIt != elementsToAddByUuid.end())
        {
            Q_UNUSED(elementsToAddByUuid.erase(addIt));

            QString error = QT_TR_NOOP("Can't rename local dirty duplicate " + typeName +
                                       " in local storage: ");
            error += errorDescription;
            emit failure(error);
        }
        else
        {
            QString error = QT_TR_NOOP("Can't update remote " + typeName + " in local storage: ");
            error += errorDescription;
            emit failure(error);
        }
    }
}

void FullSynchronizationManager::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    onUpdateDataElementFailed<Tag, QHash<QUuid, Tag> >(tag, requestId, errorDescription,
                                                       "Tag", m_updateTagRequestIds,
                                                       m_tagsToAddPerRenamingUpdateRequestId);
}

void FullSynchronizationManager::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    onUpdateDataElementFailed<SavedSearch, QHash<QUuid, SavedSearch> >(search, requestId, errorDescription,
                                                                       "SavedSearch", m_updateSavedSearchRequestIds,
                                                                       m_savedSearchesToAddPerRenamingUpdateRequestId);
}

void FullSynchronizationManager::onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    onAddDataElementCompleted<LinkedNotebook>(linkedNotebook, requestId, "LinkedNotebook", m_addLinkedNotebookRequestIds);
}

void FullSynchronizationManager::onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed<LinkedNotebook>(linkedNotebook, requestId, errorDescription,
                                           "LinkedNotebook", m_addLinkedNotebookRequestIds);
}

void FullSynchronizationManager::onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateLinkedNotebookRequestIds.find(requestId);
    if (it != m_updateLinkedNotebookRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateLinkedNotebookCompleted: linkedNotebook = "
                << linkedNotebook << ", requestId = " << requestId);

        Q_UNUSED(m_updateLinkedNotebookRequestIds.erase(it));
    }
}

void FullSynchronizationManager::onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateLinkedNotebookRequestIds.find(requestId);
    if (it != m_updateLinkedNotebookRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateLinkedNotebookFailed: linkedNotebook = "
                << linkedNotebook << ", errorDescription = " << errorDescription
                << ", requestId = " << requestId);

        QString error = QT_TR_NOOP("Can't update linked notebook in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void FullSynchronizationManager::onAddNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onAddDataElementCompleted<Notebook>(notebook, requestId, "Notebook", m_addNotebookRequestIds);
}

void FullSynchronizationManager::onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed<Notebook>(notebook, requestId, errorDescription, "Notebook",
                                     m_addNotebookRequestIds);
}

void FullSynchronizationManager::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateNotebookRequestIds.find(requestId);
    if (it != m_updateNotebookRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateNotebookCompleted: notebook = " << notebook
                << ", requestId = " << requestId);

        Q_UNUSED(m_updateNotebookRequestIds.erase(it));
    }
}

void FullSynchronizationManager::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateNotebookRequestIds.find(requestId);
    if (it != m_updateNotebookRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateNotebookFailed: notebook = " << notebook
                << ", requestId = " << requestId);

        QString error = QT_TR_NOOP("Can't update notebook in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void FullSynchronizationManager::createConnections()
{
    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this, SIGNAL(addUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddUserRequest(UserWrapper,QUuid)));
    QObject::connect(this, SIGNAL(updateUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateUserRequest(UserWrapper,QUuid)));
    QObject::connect(this, SIGNAL(findUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindUserRequest(UserWrapper,QUuid)));
    QObject::connect(this, SIGNAL(deleteUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteUserRequest(UserWrapper,QUuid)));
    QObject::connect(this, SIGNAL(expungeUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeUserRequest(UserWrapper,QUuid)));

    QObject::connect(this, SIGNAL(addNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(updateNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(findNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(expungeNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNotebookRequest(Notebook,QUuid)));

    QObject::connect(this, SIGNAL(addNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,Notebook,QUuid)));
    QObject::connect(this, SIGNAL(updateNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook,QUuid)));
    QObject::connect(this, SIGNAL(findNote(Note,bool,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNoteRequest(Note,bool,QUuid)));
    QObject::connect(this, SIGNAL(deleteNote(Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteNoteRequest(Note,QUuid)));
    QObject::connect(this, SIGNAL(expungeNote(Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNoteRequest(Note,QUuid)));

    QObject::connect(this, SIGNAL(addTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(updateTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(findTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(deleteTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(expungeTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeTagRequest(Tag,QUuid)));

    QObject::connect(this, SIGNAL(addResource(ResourceWrapper,Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::connect(this, SIGNAL(updateResource(ResourceWrapper,Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::connect(this, SIGNAL(findResource(ResourceWrapper,bool,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool,QUuid)));
    QObject::connect(this, SIGNAL(expungeResource(ResourceWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeResourceRequest(ResourceWrapper,QUuid)));

    QObject::connect(this, SIGNAL(addLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(updateLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(findLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(expungeLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeLinkedNotebookRequest(LinkedNotebook,QUuid)));

    QObject::connect(this, SIGNAL(addSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(updateSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(findSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(expungeSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeSavedSearch(SavedSearch,QUuid)));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findUserComplete(UserWrapper,QUuid)), this, SLOT(onFindUserCompleted(UserWrapper,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findUserFailed(UserWrapper,QString,QUuid)), this, SLOT(onFindUserFailed(UserWrapper,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook,QUuid)), this, SLOT(onFindNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onFindNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteComplete(Note,bool,QUuid)), this, SLOT(onFindNoteCompleted(Note,bool,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteFailed(Note,bool,QString,QUuid)), this, SLOT(onFindNoteFailed(Note,bool,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findTagComplete(Tag,QUuid)), this, SLOT(onFindTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findTagFailed(Tag,QString,QUuid)), this, SLOT(onFindTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool,QUuid)), this, SLOT(onFindResourceCompleted(ResourceWrapper,bool,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString,QUuid)), this, SLOT(onFindResourceFailed(ResourceWrapper,bool,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onFindLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onFindLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onFindSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onFindSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addTagComplete(Tag,QUuid)), this, SLOT(onAddTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addTagFailed(Tag,QString,QUuid)), this, SLOT(onAddTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag,QUuid)), this, SLOT(onUpdateTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString,QUuid)), this, SLOT(onUpdateTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onAddSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onAddSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onUpdateSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onAddLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onAddLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onUpdateLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onUpdateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNotebookComplete(Notebook,QUuid)), this, SLOT(onAddNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onAddNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookComplete(Notebook,QUuid)), this, SLOT(onUpdateNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onUpdateNotebookFailed(Notebook,QString,QUuid)));
}

void FullSynchronizationManager::launchTagsSync()
{
    QNDEBUG("FullSynchronizationManager::launchTagsSync");
    launchDataElementSync<TagsList, Tag>("Tag", m_tags);
}

void FullSynchronizationManager::launchSavedSearchSync()
{
    QNDEBUG("FullSynchronizationManager::launchSavedSearchSync");
    launchDataElementSync<SavedSearchesList, SavedSearch>("Saved search", m_savedSearches);
}

void FullSynchronizationManager::launchLinkedNotebookSync()
{
    QNDEBUG("FullSynchronizationManager::launchLinkedNotebookSync");
    launchDataElementSync<LinkedNotebooksList, LinkedNotebook>("Linked notebook", m_linkedNotebooks);
}

void FullSynchronizationManager::launchNotebookSync()
{
    QNDEBUG("FullSynchronizationManager::launchNotebookSync");
    launchDataElementSync<NotebooksList, Notebook>("Notebook", m_notebooks);
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::TagsList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                  FullSynchronizationManager::TagsList & container)
{
    if (syncChunk.tags.isSet()) {
        container.append(syncChunk.tags.ref());
    }
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::SavedSearchesList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                           FullSynchronizationManager::SavedSearchesList & container)
{
    if (syncChunk.searches.isSet()) {
        container.append(syncChunk.searches.ref());
    }
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::LinkedNotebooksList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                             FullSynchronizationManager::LinkedNotebooksList & container)
{
    if (syncChunk.linkedNotebooks.isSet()) {
        container.append(syncChunk.linkedNotebooks.ref());
    }
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::NotebooksList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                       FullSynchronizationManager::NotebooksList & container)
{
    if (syncChunk.notebooks.isSet()) {
        container.append(syncChunk.notebooks.ref());
    }
}

template <class ContainerType, class Predicate>
typename ContainerType::iterator FullSynchronizationManager::findItemByName(ContainerType & container,
                                                                            const QString & name)
{
    if (container.empty()) {
        return container.end();
    }

    // Try the front element first, in most cases it should be it
    const auto & frontItem = container.front();
    typename ContainerType::iterator it = container.begin();

    if (!frontItem.name.isSet() || (frontItem.name.ref() != name)) {
        it = std::find_if(container.begin(), container.end(), Predicate(name));
    }

    return it;
}

template <class ContainerType, class ElementType>
typename ContainerType::iterator FullSynchronizationManager::findItem(ContainerType & container,
                                                                      const ElementType &element,
                                                                      const QString & typeName)
{
    QNDEBUG("FullSynchronizationManager::findItem<" << typeName << ">");

    // Attempt to find this data element by name within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasName()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " with empty name in local storage");
        QNWARNING(errorDescription << ": " << typeName << " = " << element);
        emit failure(errorDescription);
        return container.end();
    }

    typename ContainerType::iterator it = findItemByName<ContainerType, CompareItemByName<typename ContainerType::value_type> >(container, element.name());
    if (it == container.end()) {
        QString errorDescription = QT_TR_NOOP("Can't find " + typeName + " by name within the list "
                                              "of remote elements waiting for processing");
        QNWARNING(errorDescription << ": " + typeName + " = " << element);
        emit failure(errorDescription);
        return container.end();
    }

    return it;
}

template <>
FullSynchronizationManager::LinkedNotebooksList::iterator FullSynchronizationManager::findItem(LinkedNotebooksList & linkedNotebooks,
                                                                                               const LinkedNotebook & linkedNotebook,
                                                                                               const QString & )
{
    QNDEBUG("FullSynchronizationManager::findItem<LinkedNotebook>, full template specialization");

    if (!linkedNotebook.hasGuid()) {
        QString errorDescription = QT_TR_NOOP("Found linked notebook with empty guid in local storage");
        QNWARNING(errorDescription << ": linked notebook = " << linkedNotebook);
        emit failure(errorDescription);
        return linkedNotebooks.end();
    }

    LinkedNotebooksList::iterator it = std::find_if(linkedNotebooks.begin(),
                                                    linkedNotebooks.end(),
                                                    CompareItemByGuid<qevercloud::LinkedNotebook>(linkedNotebook.guid()));
    if (it == linkedNotebooks.end()) {
        QString errorDescription = QT_TR_NOOP("Can't find linked notebook by guid within the list "
                                              "of remote linked notebooks waiting for processing");
        QNWARNING(errorDescription << ": linked notebook = " << linkedNotebook);
        emit failure(errorDescription);
        return linkedNotebooks.end();
    }

    return it;
}

template <class T>
bool FullSynchronizationManager::CompareItemByName<T>::operator()(const T & item) const
{
    if (item.name.isSet()) {
        return (m_name.toUpper() == item.name.ref().toUpper());
    }
    else {
        return false;
    }
}

template <class ElementType, class RemoteElementType>
bool FullSynchronizationManager::setupElementToFind(const RemoteElementType & remoteElement,
                                                    const QString & typeName, ElementType & elementToFind)
{
    if (remoteElement.name.isSet() && !remoteElement.name->isEmpty()) {
        elementToFind.setName(remoteElement.name.ref());
    }
    else if (remoteElement.guid.isSet() && !remoteElement.guid->isEmpty()) {
        elementToFind.setGuid(remoteElement.guid.ref());
    }
    else {
        QString errorDescription = QT_TR_NOOP("Can't synchronize remote " + typeName +
                                              ": no guid and no name");
        QNWARNING(errorDescription << ": " << remoteElement);
        emit failure(errorDescription);
        return false;
    }

    return true;
}

template <>
bool FullSynchronizationManager::setupElementToFind(const qevercloud::LinkedNotebook & remoteElement,
                                                    const QString &, LinkedNotebook & elementToFind)
{
    if (remoteElement.guid.isSet() && !remoteElement.guid->isEmpty()) {
        elementToFind.setGuid(remoteElement.guid.ref());
    }
    else {
        QString errorDescription = QT_TR_NOOP("Can't synchronize remote LinkedNotebook: no guid");
        QNWARNING(errorDescription << ": " << remoteElement);
        emit failure(errorDescription);
        return false;
    }

    return true;
}

template <class ContainerType, class ElementType>
void FullSynchronizationManager::launchDataElementSync(const QString & typeName,
                                                       ContainerType & container)
{
    container.clear();
    int numSyncChunks = m_syncChunks.size();
    for(int i = 0; i < numSyncChunks; ++i) {
        const qevercloud::SyncChunk & syncChunk = m_syncChunks[i];
        appendDataElementsFromSyncChunkToContainer<ContainerType>(syncChunk, container);
    }

    if (container.empty()) {
        return;
    }

    int numElements = container.size();
    for(int i = 0; i < numElements; ++i)
    {
        ElementType elementToFind;
        elementToFind.unsetLocalGuid();

        const typename ContainerType::value_type & remoteElement = container[i];
        if (!setupElementToFind(remoteElement, typeName, elementToFind)) {
            return;
        }

        emitFindRequest(elementToFind);
    }
}

template <class ElementType>
void setConflictedBase(const QString & typeName, ElementType & element)
{
    QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

    element.setGuid("");
    element.setName(QObject::tr("Conflicted ") + typeName + element.name() + "(" + currentDateTime + ")");
    element.setDirty(true);
}

template <class ElementType>
void FullSynchronizationManager::setConflicted(const QString & typeName, ElementType & element)
{
    setConflictedBase(typeName, element);
}

template <>
void FullSynchronizationManager::setConflicted<Tag>(const QString & typeName, Tag & tag)
{
    setConflictedBase(typeName, tag);
    tag.setLocal(true);
}

template <>
void FullSynchronizationManager::emitFindRequest<Tag>(const Tag & tag)
{
    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findTagRequestIds.insert(findElementRequestId));
    emit findTag(tag, findElementRequestId);
}

template <>
void FullSynchronizationManager::emitFindRequest<SavedSearch>(const SavedSearch & search)
{
    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchRequestIds.insert(findElementRequestId));
    emit findSavedSearch(search, findElementRequestId);
}

template <>
void FullSynchronizationManager::emitFindRequest<LinkedNotebook>(const LinkedNotebook & linkedNotebook)
{
    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findLinkedNotebookRequestIds.insert(findElementRequestId));
    emit findLinkedNotebook(linkedNotebook, findElementRequestId);
}

template <>
void FullSynchronizationManager::emitFindRequest<Notebook>(const Notebook & notebook)
{
    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookRequestIds.insert(findElementRequestId));
    emit findNotebook(notebook, findElementRequestId);
}

template <class ContainerType, class ElementType>
void FullSynchronizationManager::onFindDataElementCompleted(ElementType element,
                                                            const QUuid & requestId,
                                                            const QString & typeName,
                                                            ContainerType & container,
                                                            QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return;
    }

    QNDEBUG("FullSynchronizationManager::onFindDataElementCompleted<" << typeName << ">: "
            << typeName << " = " << element << ", requestId  = " << requestId);

    typename ContainerType::iterator it = findItem(container, element, typeName);
    if (it == container.end()) {
        return;
    }

    // The element exists both in the client and in the server
    const typename ContainerType::value_type & remoteElement = *it;
    if (!remoteElement.updateSequenceNum.isSet()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " from sync chunk without the update sequence number");
        QNWARNING(errorDescription << ": " << remoteElement);
        emit failure(errorDescription);
    }

    if (!element.hasUpdateSequenceNumber() || (remoteElement.updateSequenceNum.ref() > element.updateSequenceNumber()))
    {
        if (!element.isDirty())
        {
            // Remote element is more recent, need to update the element existing in local storage
            ElementType updatedElement(remoteElement);
            updatedElement.unsetLocalGuid();
            emitUpdateRequest(updatedElement);
        }
        else
        {
            // Remote element is more recent but the local one has been modified;
            // Evernote's synchronization protocol description suggests trying
            // to do field-by-field merge but it's overcomplicated and error-prone;
            // it's much easier to rename the existing local element to make it clear
            // it has a conflict with its remote counterpart and mark it dirty so that
            // it would be sent to the server along with other local changes
            processConflictedElement(remoteElement, typeName, element);
        }
    }

    Q_UNUSED(container.erase(it));
    Q_UNUSED(findElementRequestIds.erase(rit));
}

template <class ContainerType, class ElementType>
void FullSynchronizationManager::onFindDataElementFailed(ElementType element, const QUuid & requestId,
                                                         const QString & errorDescription,
                                                         const QString & typeName, ContainerType & container,
                                                         QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return;
    }

    QNDEBUG("FullSynchronizationManager::onFindDataElementFailed<" << typeName << ">: "
            << typeName << " = " << element << ", errorDescription = " << errorDescription
            << ", requestId = " << requestId);

    typename ContainerType::iterator it = findItem(container, element, typeName);
    if (it == container.end()) {
        return;
    }

    // Ok, this element wasn't found in the local storage, need to add it there
    // also removing the element from the list of ones waiting for processing
    ElementType newElement(*it);
    emitAddRequest(newElement);

    Q_UNUSED(container.erase(it));
    Q_UNUSED(findElementRequestIds.erase(rit));
}

template <>
void FullSynchronizationManager::emitAddRequest<Tag>(const Tag & tag)
{
    QUuid addTagRequestId = QUuid::createUuid();
    Q_UNUSED(m_addTagRequestIds.insert(addTagRequestId));
    emit addTag(tag, addTagRequestId);
}

template <>
void FullSynchronizationManager::emitAddRequest<SavedSearch>(const SavedSearch & search)
{
    QUuid addSavedSearchRequestId = QUuid::createUuid();
    Q_UNUSED(m_addSavedSearchRequestIds.insert(addSavedSearchRequestId));
    emit addSavedSearch(search, addSavedSearchRequestId);
}

template <>
void FullSynchronizationManager::emitAddRequest<LinkedNotebook>(const LinkedNotebook & linkedNotebook)
{
    QUuid addLinkedNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_addLinkedNotebookRequestIds.insert(addLinkedNotebookRequestId));
    emit addLinkedNotebook(linkedNotebook, addLinkedNotebookRequestId);
}

template <>
void FullSynchronizationManager::emitAddRequest<Notebook>(const Notebook & notebook)
{
    QUuid addNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_addNotebookRequestIds.insert(addNotebookRequestId));
    emit addNotebook(notebook, addNotebookRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<Tag>(const Tag & tag,
                                                        const Tag * tagToAddLater)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<Tag>: tag = " << tag
            << ", tagToAddLater = " << (tagToAddLater ? tagToAddLater->ToQString() : "<null>"));

    QUuid updateTagRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(updateTagRequestId));

    if (tagToAddLater) {
        m_tagsToAddPerRenamingUpdateRequestId[updateTagRequestId] = *tagToAddLater;
    }

    emit updateTag(tag, updateTagRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<SavedSearch>(const SavedSearch & search,
                                                                const SavedSearch * searchToAddLater)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<SavedSearch>: search = " << search
            << ", searchToAddLater = " << (searchToAddLater ? searchToAddLater->ToQString() : "<null>"));

    QUuid updateSavedSearchRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(updateSavedSearchRequestId));

    if (searchToAddLater) {
        m_savedSearchesToAddPerRenamingUpdateRequestId[updateSavedSearchRequestId] = *searchToAddLater;
    }

    emit updateSavedSearch(search, updateSavedSearchRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<LinkedNotebook>(const LinkedNotebook & linkedNotebook,
                                                                   const LinkedNotebook *)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<LinkedNotebook>: linked notebook = "
            << linkedNotebook);

    QUuid updateLinkedNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateLinkedNotebookRequestIds.insert(updateLinkedNotebookRequestId));
    emit updateLinkedNotebook(linkedNotebook, updateLinkedNotebookRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<Notebook>(const Notebook & notebook,
                                                             const Notebook *)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<Notebook>: notebook = " << notebook);

    QUuid updateNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(updateNotebookRequestId));
    emit updateNotebook(notebook, updateNotebookRequestId);
}

template <class T>
bool FullSynchronizationManager::CompareItemByGuid<T>::operator()(const T & item) const
{
    if (item.guid.isSet()) {
        return (m_guid == item.guid.ref());
    }
    else {
        return false;
    }
}

template <class ElementType, class RemoteElementType>
void FullSynchronizationManager::processConflictedElement(const RemoteElementType & remoteElement,
                                                          const QString & typeName, ElementType & element)
{
    setConflicted(typeName, element);

    ElementType elementToAdd(remoteElement);
    emitUpdateRequest(element, &elementToAdd);
}

template <>
void FullSynchronizationManager::processConflictedElement(const qevercloud::LinkedNotebook & remoteLinkedNotebook,
                                                          const QString &, LinkedNotebook & linkedNotebook)
{
    // Linked notebook itself is simply a pointer to another user's account;
    // The data it points to would have a separate synchronization procedure
    // including the synchronization for Notebook, Notes and Tags from another user's account
    // The linked notebook as a pointer to all this data would simply be overridden
    // by the server's version of it
    QString localGuid = linkedNotebook.localGuid();
    linkedNotebook = LinkedNotebook(remoteLinkedNotebook);
    linkedNotebook.setLocalGuid(localGuid);

    QUuid updateRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateLinkedNotebookRequestIds.insert(updateRequestId));
    emit updateLinkedNotebook(linkedNotebook, updateRequestId);
}

template <>
void FullSynchronizationManager::processConflictedElement(const qevercloud::Notebook & remoteNotebook,
                                                          const QString &, Notebook & notebook)
{
    // It is too costly to process the conflicting notebook the default way of creating
    // a local conflicting one because Notebook is a "parent" for all notes it contains;
    // notes are "parents" to resources, can be linked with tags etc. Therefore, it is better
    // to attempt to "merge" the remote changes into the conflicting notebook.
    // The simplest form of merge would be "discard local changes and replace them
    // with their remote counterparts". After all, it is the notebook, it doesn't
    // make much sense to change its settings locally and then attempting to synchronize
    QString localGuid = notebook.localGuid();
    notebook = Notebook(remoteNotebook);
    notebook.setLocalGuid(localGuid);

    QUuid updateRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(updateRequestId));
    emit updateNotebook(notebook, updateRequestId);
}

} // namespace qute_note
