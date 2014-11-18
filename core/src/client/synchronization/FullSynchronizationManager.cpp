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
    m_updateSavedSearchRequestIds()
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
    // TODO: implement
}

void FullSynchronizationManager::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
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
    // TODO: implement
}

void FullSynchronizationManager::onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
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

void FullSynchronizationManager::onAddTagCompleted(Tag tag, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_addTagRequestIds.find(requestId);
    if (it != m_addTagRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onAddTagCompleted: tag = " << tag
                << ", requestId = " << requestId);
        Q_UNUSED(m_addTagRequestIds.erase(it));
    }
}

void FullSynchronizationManager::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_addTagRequestIds.find(requestId);
    if (it != m_addTagRequestIds.end())
    {
        QNWARNING("FullSynchronizationManager::onAddTagFailed: tag = " << tag
                  << ", error description = " << errorDescription << ", requestId = " << requestId);

        Q_UNUSED(m_addTagRequestIds.erase(it));

        QString error = QT_TR_NOOP("Can't add remote tag to local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void FullSynchronizationManager::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateTagRequestIds.find(requestId);
    if (it != m_updateTagRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateTagCompleted: tag = " << tag
                << ", requestId = " << requestId);

        Q_UNUSED(m_updateTagRequestIds.erase(it));

        QHash<QUuid,Tag>::iterator addIt = m_tagsToAddPerRenamingUpdateRequestId.find(requestId);
        if (addIt != m_tagsToAddPerRenamingUpdateRequestId.end())
        {
            Tag tagToAdd = addIt.value();
            Q_UNUSED(m_tagsToAddPerRenamingUpdateRequestId.erase(addIt));

            QNDEBUG("Adding new tag after renaming the dirty local duplicate: " << tagToAdd);
            QUuid addTagRequestId = QUuid::createUuid();
            Q_UNUSED(m_addTagRequestIds.insert(addTagRequestId));
            emit addTag(tagToAdd, addTagRequestId);
        }
    }
}

void FullSynchronizationManager::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateTagRequestIds.find(requestId);
    if (it != m_updateTagRequestIds.end())
    {
        QNWARNING("FullSynchronizationManager::onUpdateTagFailed: tag = " << tag << ", errorDescription = "
                  << errorDescription << ", requestId = " << requestId);

        Q_UNUSED(m_updateTagRequestIds.erase(it));

        QHash<QUuid,Tag>::iterator addIt = m_tagsToAddPerRenamingUpdateRequestId.find(requestId);
        if (addIt != m_tagsToAddPerRenamingUpdateRequestId.end())
        {
            Q_UNUSED(m_tagsToAddPerRenamingUpdateRequestId.erase(addIt));

            QString error = QT_TR_NOOP("Can't rename local dirty duplicate tag in local storage: ");
            error += errorDescription;
            emit failure(error);
        }
        else
        {
            QString error = QT_TR_NOOP("Can't update remote tag in local storage: ");
            error += errorDescription;
            emit failure(error);
        }
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

FullSynchronizationManager::TagsList::iterator FullSynchronizationManager::findTagInList(const QString & name)
{
    return findItemByName<TagsList, CompareItemByName<qevercloud::Tag> >(m_tags, name);
}

FullSynchronizationManager::SavedSearchesList::iterator FullSynchronizationManager::findSavedSearchInList(const QString & name)
{
    return findItemByName<SavedSearchesList, CompareItemByName<qevercloud::SavedSearch> >(m_savedSearches, name);
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

template <class T>
bool FullSynchronizationManager::CompareItemByName<T>::operator()(const T & item) const
{
    if (item.name.isSet()) {
        return (m_name == item.name.ref());
    }
    else {
        return false;
    }
}

template <class ContainerType, class LocalType>
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
        LocalType elementToFind;
        elementToFind.unsetLocalGuid();

        const typename ContainerType::value_type & remoteElement = container[i];
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
            return;
        }

        emitFindRequest<LocalType>(elementToFind);
    }
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

    // Attempt to find this data element by name within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasName()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " with empty name in local storage");
        QNWARNING(errorDescription << ": " << typeName << " = " << element);
        emit failure(errorDescription);
        return;
    }

    typename ContainerType::iterator it = findItemByName<ContainerType, CompareItemByName<typename ContainerType::value_type> >(container, element.name());
    if (it == container.end()) {
        QString errorDescription = QT_TR_NOOP("Can't find " + typeName + " by name within the list "
                                              "of remote elements waiting for processing");
        QNWARNING(errorDescription << ": " + typeName + " = " << element);
        emit failure(errorDescription);
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
            QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

            element.setGuid("");
            element.setName(QObject::tr("Conflicted ") + typeName + element.name() + "(" + currentDateTime + ")");
            element.setDirty(true);

            ElementType newElement(remoteElement);
            emitUpdateRequest(element, &newElement);
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

    // Attempt to find this data element within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasName()) {
        QString error = QT_TR_NOOP("Found " + typeName + " with empty name in local storage");
        QNWARNING(error << ": " << typeName << " = " << element);
        emit failure(error);
        return;
    }

    typename ContainerType::iterator it = findItemByName<ContainerType, CompareItemByName<typename ContainerType::value_type> >(container, element.name());
    if (it == container.end()) {
        QString error = QT_TR_NOOP("Can't find " + typeName + " by name within the list "
                                   "of remote elements waiting for processing");
        QNWARNING(error << ": " << typeName << " = " << element);
        emit failure(error);
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

} // namespace qute_note
