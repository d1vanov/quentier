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
    m_findSavedSearchRequestIds()
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
    QSet<QUuid>::iterator rit = m_findTagRequestIds.find(requestId);
    if (rit == m_findTagRequestIds.end()) {
        return;
    }

    QNDEBUG("FullSynchronizationManager::onFindTagCompleted: tag = " << tag
            << ", requestId = " << requestId);

    // Attempt to find this tag by name within the list of tags waiting for processing;
    // first simply try the front tag from the list to avoid the costly lookup
    if (!tag.hasName()) {
        QString errorDescription = QT_TR_NOOP("Found tag with empty name in local storage");
        QNWARNING(errorDescription << ": tag = " << tag);
        emit failure(errorDescription);
        return;
    }

    TagsList::iterator it = findTagInList(tag.name());
    if (it == m_tags.end()) {
        QString errorDescription = QT_TR_NOOP("Can't find tag by name within the list of remote tags waiting for processing");
        QNWARNING(errorDescription << ": tag = " << tag);
        emit failure(errorDescription);
        return;
    }

    // The tag exists both in the client and in the server
    const qevercloud::Tag & remoteTag = *it;
    if (!remoteTag.updateSequenceNum.isSet()) {
        QString errorDescription = QT_TR_NOOP("Found tag from sync chunk without the update sequence number");
        QNWARNING(errorDescription << ": " << remoteTag);
        emit failure(errorDescription);
    }

    if (!tag.hasUpdateSequenceNumber() || (remoteTag.updateSequenceNum.ref() > tag.updateSequenceNumber()))
    {
        if (!tag.isDirty())
        {
            // Remote tag is more recent, need to update the tag existing in local storage
            Tag updatedTag(remoteTag);
            updatedTag.unsetLocalGuid();
            QUuid updateTagRequestId = QUuid::createUuid();
            Q_UNUSED(m_updateTagRequestIds.insert(updateTagRequestId));
            emit updateTag(updatedTag, updateTagRequestId);
        }
        else
        {
            // Remote tag is more recent but the local one has been modified;
            // Evernote's synchronization protocol description suggests trying
            // to do field-by-field merge but it's overcomplicated and error-prone;
            // it's much easier to rename the existing local tag to make it clear
            // it has a conflict with its remote counterpart and mark dirty so that
            // it would be sent to the server along with other local changes
            QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

            tag.setGuid("");
            tag.setName(QObject::tr("Conflicted tag ") + tag.name() + "(" + currentDateTime + ")");
            tag.setLocal(true);

            QUuid updateTagRequestId = QUuid::createUuid();
            Tag newTag(remoteTag);
            m_tagsToAddPerRenamingUpdateRequestId[updateTagRequestId] = newTag;
            Q_UNUSED(m_updateTagRequestIds.insert(updateTagRequestId));

            emit updateTag(tag, updateTagRequestId);
        }
    }

    Q_UNUSED(m_tags.erase(it));
}

void FullSynchronizationManager::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator rit = m_findTagRequestIds.find(requestId);
    if (rit == m_findTagRequestIds.end()) {
        return;
    }

    QNDEBUG("FullSynchronizationManager::onFindTagFailed: tag = " << tag
            << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    // Attempt to find this tag by name within the list of tags waiting for processing;
    // first simply try the front tag from the list to avoid the costly lookup
    if (!tag.hasName()) {
        QString errorDescription = QT_TR_NOOP("Found tag with empty name in local storage");
        QNWARNING(errorDescription << ": tag = " << tag);
        emit failure(errorDescription);
        return;
    }

    TagsList::iterator it = findTagInList(tag.name());
    if (it == m_tags.end()) {
        QString errorDescription = QT_TR_NOOP("Can't find tag by name within the list of remote tags waiting for processing");
        QNWARNING(errorDescription << ": tag = " << tag);
        emit failure(errorDescription);
        return;
    }

    // Ok, this tag wasn't found in the local storage, need to add it there
    // also removing the tag from the list of tags waiting for processing

    Tag newTag(*it);
    QUuid addTagRequestId = QUuid::createUuid();
    Q_UNUSED(m_addTagRequestIds.insert(addTagRequestId));
    emit addTag(newTag, addTagRequestId);

    Q_UNUSED(m_tags.erase(it));
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
    // TODO: implement
}

void FullSynchronizationManager::onFindSavedSearchFailed(SavedSearch savedSearch, QString errorDescription, QUuid requestId)
{
    // TODO: implement
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

    m_tags.clear();
    int numSyncChunks = m_syncChunks.size();
    for(int i = 0; i < numSyncChunks; ++i)
    {
        const qevercloud::SyncChunk & syncChunk = m_syncChunks[i];

        if (syncChunk.tags.isSet()) {
            m_tags.append(syncChunk.tags.ref());
        }
    }

    if (m_tags.empty()) {
        return;
    }

    int numTags = m_tags.size();
    for(int i = 0; i < numTags; ++i)
    {
        Tag tagToFind;
        tagToFind.unsetLocalGuid();

        const qevercloud::Tag & remoteTag = m_tags[i];
        if (remoteTag.name.isSet() && !remoteTag.name->isEmpty()) {
            tagToFind.setName(remoteTag.name.ref());
        }
        else if (remoteTag.guid.isSet() && !remoteTag.guid->isEmpty()) {
            tagToFind.setGuid(remoteTag.guid.ref());
        }
        else {
            QString errorDescription = QT_TR_NOOP("Can't synchronize remote tag: no guid and no name");
            QNWARNING(errorDescription << ": " << remoteTag);
            emit failure(errorDescription);
            return;
        }

        QUuid findTagRequestId = QUuid::createUuid();
        Q_UNUSED(m_findTagRequestIds.insert(findTagRequestId));
        emit findTag(tagToFind, findTagRequestId);
    }
}

void FullSynchronizationManager::launchSavedSearchSync()
{
    QNDEBUG("FullSynchronizationManager::launchSavedSearchSync");

    m_savedSearches.clear();
    int numSyncChunks = m_syncChunks.size();
    for(int i = 0; i < numSyncChunks; ++i)
    {
        const qevercloud::SyncChunk & syncChunk = m_syncChunks[i];

        if (syncChunk.searches.isSet()) {
            m_savedSearches.append(syncChunk.searches.ref());
        }
    }

    if (m_savedSearches.empty()) {
        return;
    }

    int numSavedSearches = m_savedSearches.size();
    for(int i = 0; i < numSavedSearches; ++i)
    {
        SavedSearch searchToFind;
        searchToFind.unsetLocalGuid();

        const qevercloud::SavedSearch & remoteSearch = m_savedSearches[i];
        if (remoteSearch.name.isSet() && !remoteSearch.name->isEmpty()) {
            searchToFind.setName(remoteSearch.name.ref());
        }
        else if (remoteSearch.guid.isSet() && !remoteSearch.guid->isEmpty()) {
            searchToFind.setGuid(remoteSearch.guid.ref());
        }
        else {
            QString errorDescription = QT_TR_NOOP("Can't synchronize remote saved search: no guid and no name");
            QNWARNING(errorDescription << ": " << remoteSearch);
            emit failure(errorDescription);
            return;
        }

        QUuid findSavedSearchRequestId = QUuid::createUuid();
        Q_UNUSED(m_findSavedSearchRequestIds.insert(findSavedSearchRequestId));
        emit findSavedSearch(searchToFind, findSavedSearchRequestId);
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

} // namespace qute_note
