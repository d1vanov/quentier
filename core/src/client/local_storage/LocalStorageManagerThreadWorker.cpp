#include "LocalStorageManagerThreadWorker.h"
#include <logging/QuteNoteLogger.h>
#include <tools/SysInfo.h>

namespace qute_note {

LocalStorageManagerThreadWorker::LocalStorageManagerThreadWorker(const QString & username,
                                                                 const qint32 userId,
                                                                 const bool startFromScratch, QObject * parent) :
    QObject(parent),
    m_username(username),
    m_userId(userId),
    m_startFromScratch(startFromScratch),
    m_pLocalStorageManager(nullptr),
    m_useCache(true),
    m_pLocalStorageCacheManager(nullptr)
{}

LocalStorageManagerThreadWorker::~LocalStorageManagerThreadWorker()
{
    if (m_pLocalStorageManager) {
        delete m_pLocalStorageManager;
    }

    if (m_pLocalStorageCacheManager) {
        delete m_pLocalStorageCacheManager;
    }
}

void LocalStorageManagerThreadWorker::setUseCache(const bool useCache)
{
    if (m_useCache) {
        // Cache is being disabled - no point to store things in it anymore, it would get rotten pretty quick
        m_pLocalStorageCacheManager->clear();
    }

    m_useCache = useCache;
}

const LocalStorageCacheManager * LocalStorageManagerThreadWorker::localStorageCacheManager() const
{
    if (!m_useCache) {
        return nullptr;
    }
    else {
        return m_pLocalStorageCacheManager;
    }
}

void LocalStorageManagerThreadWorker::init()
{
    if (m_pLocalStorageManager) {
        delete m_pLocalStorageManager;
    }

    m_pLocalStorageManager = new LocalStorageManager(m_username, m_userId, m_startFromScratch);

    if (m_pLocalStorageCacheManager) {
        delete m_pLocalStorageCacheManager;
    }

    m_pLocalStorageCacheManager = new LocalStorageCacheManager();

    emit initialized();
}

#define CATCH_EXCEPTION \
    catch(const std::exception & e) { \
        QString error = "Caught exception: " + QString(e.what()) + \
                        QString(", backtrace: ") + \
                        SysInfo::GetSingleton().GetStackTrace(); \
        QNCRITICAL(error); \
        emit failure(error); \
    }

void LocalStorageManagerThreadWorker::onGetUserCountRequest()
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetUserCount(errorDescription);
        if (count < 0) {
            emit getUserCountFailed(errorDescription);
        }
        else {
            emit getUserCountComplete(count);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onSwitchUserRequest(QString username, qint32 userId,
                                                          bool startFromScratch)
{
    try {
        m_pLocalStorageManager->SwitchUser(username, userId, startFromScratch);
    }
    catch(const std::exception & exception) {
        emit switchUserFailed(userId, QString(exception.what()));
        return;
    }

    emit switchUserComplete(userId);
}

void LocalStorageManagerThreadWorker::onAddUserRequest(UserWrapper user)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddUser(user, errorDescription);
        if (!res) {
            emit addUserFailed(user, errorDescription);
            return;
        }

        emit addUserComplete(user);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateUserRequest(UserWrapper user)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateUser(user, errorDescription);
        if (!res) {
            emit updateUserFailed(user, errorDescription);
            return;
        }

        emit updateUserComplete(user);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindUserRequest(UserWrapper user)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindUser(user, errorDescription);
        if (!res) {
            emit findUserFailed(user, errorDescription);
            return;
        }

        emit findUserComplete(user);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onDeleteUserRequest(UserWrapper user)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->DeleteUser(user, errorDescription);
        if (!res) {
            emit deleteUserFailed(user, errorDescription);
            return;
        }

        emit deleteUserComplete(user);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeUserRequest(UserWrapper user)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeUser(user, errorDescription);
        if (!res) {
            emit expungeUserFailed(user, errorDescription);
            return;
        }

        emit expungeUserComplete(user);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetNotebookCountRequest()
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetNotebookCount(errorDescription);
        if (count < 0) {
            emit getNotebookCountFailed(errorDescription);
        }
        else {
            emit getNotebookCountComplete(count);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddNotebookRequest(Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddNotebook(notebook, errorDescription);
        if (!res) {
            emit addNotebookFailed(notebook, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNotebook(notebook);
        }

        emit addNotebookComplete(notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateNotebookRequest(Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateNotebook(notebook, errorDescription);
        if (!res) {
            emit updateNotebookFailed(notebook, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNotebook(notebook);
        }

        emit updateNotebookComplete(notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindNotebookRequest(Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool foundNotebookInCache = false;
        if (m_useCache)
        {
            bool notebookHasGuid = notebook.hasGuid();
            const QString guid = (notebookHasGuid ? notebook.guid() : notebook.localGuid());
            LocalStorageCacheManager::WhichGuid wg = (notebookHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalGuid);

            const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(guid, wg);
            if (pNotebook) {
                notebook = *pNotebook;
                foundNotebookInCache = true;
            }
        }

        if (!foundNotebookInCache)
        {
            bool res = m_pLocalStorageManager->FindNotebook(notebook, errorDescription);
            if (!res) {
                emit findNotebookFailed(notebook, errorDescription);
                return;
            }
        }

        emit findNotebookComplete(notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindDefaultNotebookRequest(Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindDefaultNotebook(notebook, errorDescription);
        if (!res) {
            emit findDefaultNotebookFailed(notebook, errorDescription);
            return;
        }

        emit findDefaultNotebookComplete(notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindLastUsedNotebookRequest(Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindLastUsedNotebook(notebook, errorDescription);
        if (!res) {
            emit findLastUsedNotebookFailed(notebook, errorDescription);
            return;
        }

        emit findLastUsedNotebookComplete(notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindDefaultOrLastUsedNotebookRequest(Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindDefaultOrLastUsedNotebook(notebook, errorDescription);
        if (!res) {
            emit findDefaultOrLastUsedNotebookFailed(notebook, errorDescription);
            return;
        }

        emit findDefaultOrLastUsedNotebookComplete(notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllNotebooksRequest()
{
    try
    {
        QString errorDescription;
        QList<Notebook> notebooks = m_pLocalStorageManager->ListAllNotebooks(errorDescription);
        if (notebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllNotebooksFailed(errorDescription);
            return;
        }

        if (m_useCache)
        {
            foreach(const Notebook & notebook, notebooks) {
                m_pLocalStorageCacheManager->cacheNotebook(notebook);
            }
        }

        emit listAllNotebooksComplete(notebooks);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllSharedNotebooksRequest()
{
    try
    {
        QString errorDescription;
        QList<SharedNotebookWrapper> sharedNotebooks = m_pLocalStorageManager->ListAllSharedNotebooks(errorDescription);
        if (sharedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllSharedNotebooksFailed(errorDescription);
            return;
        }

        emit listAllSharedNotebooksComplete(sharedNotebooks);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid)
{
    try
    {
        QString errorDescription;
        QList<SharedNotebookWrapper> sharedNotebooks = m_pLocalStorageManager->ListSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
        if (sharedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listSharedNotebooksPerNotebookGuidFailed(notebookGuid, errorDescription);
            return;
        }

        emit listSharedNotebooksPerNotebookGuidComplete(notebookGuid, sharedNotebooks);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeNotebookRequest(Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeNotebook(notebook, errorDescription);
        if (!res) {
            emit expungeNotebookFailed(notebook, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeNotebook(notebook);
        }

        emit expungeNotebookComplete(notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetLinkedNotebookCountRequest()
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetLinkedNotebookCount(errorDescription);
        if (count < 0) {
            emit getLinkedNotebookCountFailed(errorDescription);
        }
        else {
            emit getLinkedNotebookCountComplete(count);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddLinkedNotebook(linkedNotebook, errorDescription);
        if (!res) {
            emit addLinkedNotebookFailed(linkedNotebook, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheLinkedNotebook(linkedNotebook);
        }

        emit addLinkedNotebookComplete(linkedNotebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateLinkedNotebook(linkedNotebook, errorDescription);
        if (!res) {
            emit updateLinkedNotebookFailed(linkedNotebook, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheLinkedNotebook(linkedNotebook);
        }

        emit updateLinkedNotebookComplete(linkedNotebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    try
    {
        QString errorDescription;

        bool foundLinkedNotebookInCache = false;
        if (m_useCache && linkedNotebook.hasGuid())
        {
            const QString guid = linkedNotebook.guid();
            const LinkedNotebook * pLinkedNotebook = m_pLocalStorageCacheManager->findLinkedNotebook(guid);
            if (pLinkedNotebook) {
                linkedNotebook = *pLinkedNotebook;
                foundLinkedNotebookInCache = true;
            }
        }

        if (!foundLinkedNotebookInCache)
        {
            bool res = m_pLocalStorageManager->FindLinkedNotebook(linkedNotebook, errorDescription);
            if (!res) {
                emit findLinkedNotebookFailed(linkedNotebook, errorDescription);
                return;
            }
        }

        emit findLinkedNotebookComplete(linkedNotebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllLinkedNotebooksRequest()
{
    try
    {
        QString errorDescription;
        QList<LinkedNotebook> linkedNotebooks = m_pLocalStorageManager->ListAllLinkedNotebooks(errorDescription);
        if (linkedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllLinkedNotebooksFailed(errorDescription);
            return;
        }

        if (m_useCache)
        {
            foreach(const LinkedNotebook & linkedNotebook, linkedNotebooks) {
                m_pLocalStorageCacheManager->cacheLinkedNotebook(linkedNotebook);
            }
        }

        emit listAllLinkedNotebooksComplete(linkedNotebooks);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeLinkedNotebook(linkedNotebook, errorDescription);
        if (!res) {
            emit expungeLinkedNotebookFailed(linkedNotebook, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeLinkedNotebook(linkedNotebook);
        }

        emit expungeLinkedNotebookComplete(linkedNotebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetNoteCountRequest()
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetNoteCount(errorDescription);
        if (count < 0) {
            emit getNoteCountFailed(errorDescription);
        }
        else {
            emit getNoteCountComplete(count);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddNoteRequest(Note note, Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddNote(note, notebook, errorDescription);
        if (!res) {
            emit addNoteFailed(note, notebook, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNote(note);
        }

        emit addNoteComplete(note, notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateNoteRequest(Note note, Notebook notebook)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateNote(note, notebook, errorDescription);
        if (!res) {
            emit updateNoteFailed(note, notebook, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNote(note);
        }

        emit updateNoteComplete(note, notebook);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindNoteRequest(Note note, bool withResourceBinaryData)
{
    try
    {
        QString errorDescription;

        bool foundNoteInCache = false;
        if (m_useCache)
        {
            bool noteHasGuid = note.hasGuid();
            const QString guid = (noteHasGuid ? note.guid() : note.localGuid());
            LocalStorageCacheManager::WhichGuid wg = (noteHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalGuid);

            const Note * pNote = m_pLocalStorageCacheManager->findNote(guid, wg);
            if (pNote) {
                note = *pNote;
                foundNoteInCache = true;
            }
        }

        if (!foundNoteInCache)
        {
            bool res = m_pLocalStorageManager->FindNote(note, errorDescription, withResourceBinaryData);
            if (!res) {
                emit findNoteFailed(note, withResourceBinaryData, errorDescription);
                return;
            }
        }

        emit findNoteComplete(note, withResourceBinaryData);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllNotesPerNotebookRequest(Notebook notebook,
                                                                       bool withResourceBinaryData)
{
    try
    {
        QString errorDescription;

        QList<Note> notes = m_pLocalStorageManager->ListAllNotesPerNotebook(notebook, errorDescription,
                                                                          withResourceBinaryData);
        if (notes.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllNotesPerNotebookFailed(notebook, withResourceBinaryData, errorDescription);
            return;
        }

        if (m_useCache)
        {
            foreach(const Note & note, notes) {
                m_pLocalStorageCacheManager->cacheNote(note);
            }
        }

        emit listAllNotesPerNotebookComplete(notebook, withResourceBinaryData, notes);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onDeleteNoteRequest(Note note)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->DeleteNote(note, errorDescription);
        if (!res) {
            emit deleteNoteFailed(note, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNote(note);
        }

        emit deleteNoteComplete(note);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeNoteRequest(Note note)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeNote(note, errorDescription);
        if (!res) {
            emit expungeNoteFailed(note, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeNote(note);
        }

        emit expungeNoteComplete(note);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetTagCountRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetTagCount(errorDescription);
        if (count < 0) {
            emit getTagCountFailed(errorDescription, requestId);
        }
        else {
            emit getTagCountComplete(count, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddTagRequest(Tag tag, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddTag(tag, errorDescription);
        if (!res) {
            emit addTagFailed(tag, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheTag(tag);
        }

        emit addTagComplete(tag, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateTagRequest(Tag tag, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateTag(tag, errorDescription);
        if (!res) {
            emit updateTagFailed(tag, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheTag(tag);
        }

        emit updateTagComplete(tag, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onLinkTagWithNoteRequest(Tag tag, Note note)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->LinkTagWithNote(tag, note, errorDescription);
        if (!res) {
            emit linkTagWithNoteFailed(tag, note, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheTag(tag);
        }

        emit linkTagWithNoteComplete(tag, note);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindTagRequest(Tag tag)
{
    try
    {
        QString errorDescription;

        bool foundTagInCache = false;
        if (m_useCache)
        {
            bool tagHasGuid = tag.hasGuid();
            if (tagHasGuid || !tag.localGuid().isEmpty())
            {
                const QString guid = (tagHasGuid ? tag.guid() : tag.localGuid());
                LocalStorageCacheManager::WhichGuid wg = (tagHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalGuid);

                const Tag * pTag = m_pLocalStorageCacheManager->findTag(guid, wg);
                if (pTag) {
                    tag = *pTag;
                    foundTagInCache = true;
                }
            }
            else if (!tag.name().isEmpty())
            {
                const QString tagName = tag.name();
                const Tag * pTag = m_pLocalStorageCacheManager->findTagByName(tagName);
                if (pTag) {
                    tag = *pTag;
                    foundTagInCache = true;
                }
            }
        }

        if (!foundTagInCache)
        {
            bool res = m_pLocalStorageManager->FindTag(tag, errorDescription);
            if (!res) {
                emit findTagFailed(tag, errorDescription);
                return;
            }
        }

        emit findTagComplete(tag);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllTagsPerNoteRequest(Note note)
{
    try
    {
        QString errorDescription;

        QList<Tag> tags = m_pLocalStorageManager->ListAllTagsPerNote(note, errorDescription);
        if (tags.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllTagsPerNoteFailed(note, errorDescription);
            return;
        }

        if (m_useCache)
        {
            foreach(const Tag & tag, tags) {
                m_pLocalStorageCacheManager->cacheTag(tag);
            }
        }

        emit listAllTagsPerNoteComplete(tags, note);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllTagsRequest()
{
    try
    {
        QString errorDescription;

        QList<Tag> tags = m_pLocalStorageManager->ListAllTags(errorDescription);
        if (tags.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllTagsFailed(errorDescription);
            return;
        }

        if (m_useCache)
        {
            foreach(const Tag & tag, tags) {
                m_pLocalStorageCacheManager->cacheTag(tag);
            }
        }

        emit listAllTagsComplete(tags);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onDeleteTagRequest(Tag tag)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->DeleteTag(tag, errorDescription);
        if (!res) {
            emit deleteTagFailed(tag, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheTag(tag);
        }

        emit deleteTagComplete(tag);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeTagRequest(Tag tag)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeTag(tag, errorDescription);
        if (!res) {
            emit expungeTagFailed(tag, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeTag(tag);
        }

        emit expungeTagComplete(tag);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetResourceCountRequest()
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetEnResourceCount(errorDescription);
        if (count < 0) {
            emit getResourceCountFailed(errorDescription);
        }
        else {
            emit getResourceCountComplete(count);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddResourceRequest(ResourceWrapper resource, Note note)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddEnResource(resource, note, errorDescription);
        if (!res) {
            emit addResourceFailed(resource, note, errorDescription);
            return;
        }

        emit addResourceComplete(resource, note);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateResourceRequest(ResourceWrapper resource, Note note)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateEnResource(resource, note, errorDescription);
        if (!res) {
            emit updateResourceFailed(resource, note, errorDescription);
            return;
        }

        emit updateResourceComplete(resource, note);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindResourceRequest(ResourceWrapper resource, bool withBinaryData)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindEnResource(resource, errorDescription, withBinaryData);
        if (!res) {
            emit findResourceFailed(resource, withBinaryData, errorDescription);
            return;
        }

        emit findResourceComplete(resource, withBinaryData);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeResourceRequest(ResourceWrapper resource)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeEnResource(resource, errorDescription);
        if (!res) {
            emit expungeResourceFailed(resource, errorDescription);
            return;
        }

        emit expungeResourceComplete(resource);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetSavedSearchCountRequest()
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetSavedSearchCount(errorDescription);
        if (count < 0) {
            emit getSavedSearchCountFailed(errorDescription);
        }
        else {
            emit getSavedSearchCountComplete(count);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddSavedSearchRequest(SavedSearch search)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddSavedSearch(search, errorDescription);
        if (!res) {
            emit addSavedSearchFailed(search, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheSavedSearch(search);
        }

        emit addSavedSearchComplete(search);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateSavedSearchRequest(SavedSearch search)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateSavedSearch(search, errorDescription);
        if (!res) {
            emit updateSavedSearchFailed(search, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheSavedSearch(search);
        }

        emit updateSavedSearchComplete(search);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindSavedSearchRequest(SavedSearch search)
{
    try
    {
        QString errorDescription;

        bool foundCachedSavedSearch = false;
        if (m_useCache)
        {
            bool searchHasGuid = search.hasGuid();
            const QString guid = (searchHasGuid ? search.guid() : search.localGuid());
            const LocalStorageCacheManager::WhichGuid wg = (searchHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalGuid);

            const SavedSearch * pSearch = m_pLocalStorageCacheManager->findSavedSearch(guid, wg);
            if (pSearch) {
                search = *pSearch;
                foundCachedSavedSearch = true;
            }
        }

        if (!foundCachedSavedSearch)
        {
            bool res = m_pLocalStorageManager->FindSavedSearch(search, errorDescription);
            if (!res) {
                emit findSavedSearchFailed(search, errorDescription);
                return;
            }
        }

        emit findSavedSearchComplete(search);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllSavedSearchesRequest()
{
    try
    {
        QString errorDescription;
        QList<SavedSearch> searches = m_pLocalStorageManager->ListAllSavedSearches(errorDescription);
        if (searches.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllSavedSearchesFailed(errorDescription);
            return;
        }

        if (m_useCache)
        {
            foreach(const SavedSearch & search, searches) {
                m_pLocalStorageCacheManager->cacheSavedSearch(search);
            }
        }

        emit listAllSavedSearchesComplete(searches);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeSavedSearch(SavedSearch search)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeSavedSearch(search, errorDescription);
        if (!res) {
            emit expungeSavedSearchFailed(search, errorDescription);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeSavedSearch(search);
        }

        emit expungeSavedSearchComplete(search);
    }
    CATCH_EXCEPTION
}

} // namespace qute_note
