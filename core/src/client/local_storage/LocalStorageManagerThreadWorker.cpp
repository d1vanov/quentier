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

void LocalStorageManagerThreadWorker::onGetUserCountRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetUserCount(errorDescription);
        if (count < 0) {
            emit getUserCountFailed(errorDescription, requestId);
        }
        else {
            emit getUserCountComplete(count, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onSwitchUserRequest(QString username, qint32 userId,
                                                          bool startFromScratch, QUuid requestId)
{
    try {
        m_pLocalStorageManager->SwitchUser(username, userId, startFromScratch);
    }
    catch(const std::exception & exception) {
        emit switchUserFailed(userId, QString(exception.what()), requestId);
        return;
    }

    emit switchUserComplete(userId, requestId);
}

void LocalStorageManagerThreadWorker::onAddUserRequest(UserWrapper user, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddUser(user, errorDescription);
        if (!res) {
            emit addUserFailed(user, errorDescription, requestId);
            return;
        }

        emit addUserComplete(user, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateUserRequest(UserWrapper user, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateUser(user, errorDescription);
        if (!res) {
            emit updateUserFailed(user, errorDescription, requestId);
            return;
        }

        emit updateUserComplete(user, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindUserRequest(UserWrapper user, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindUser(user, errorDescription);
        if (!res) {
            emit findUserFailed(user, errorDescription, requestId);
            return;
        }

        emit findUserComplete(user, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onDeleteUserRequest(UserWrapper user, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->DeleteUser(user, errorDescription);
        if (!res) {
            emit deleteUserFailed(user, errorDescription, requestId);
            return;
        }

        emit deleteUserComplete(user, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeUserRequest(UserWrapper user, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeUser(user, errorDescription);
        if (!res) {
            emit expungeUserFailed(user, errorDescription, requestId);
            return;
        }

        emit expungeUserComplete(user, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetNotebookCountRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetNotebookCount(errorDescription);
        if (count < 0) {
            emit getNotebookCountFailed(errorDescription, requestId);
        }
        else {
            emit getNotebookCountComplete(count, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddNotebookRequest(Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddNotebook(notebook, errorDescription);
        if (!res) {
            emit addNotebookFailed(notebook, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNotebook(notebook);
        }

        emit addNotebookComplete(notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateNotebookRequest(Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateNotebook(notebook, errorDescription);
        if (!res) {
            emit updateNotebookFailed(notebook, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNotebook(notebook);
        }

        emit updateNotebookComplete(notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindNotebookRequest(Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool foundNotebookInCache = false;
        if (m_useCache)
        {
            bool notebookHasGuid = notebook.hasGuid();
            if (notebookHasGuid || !notebook.localGuid().isEmpty())
            {
                const QString guid = (notebookHasGuid ? notebook.guid() : notebook.localGuid());
                LocalStorageCacheManager::WhichGuid wg = (notebookHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalGuid);

                const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(guid, wg);
                if (pNotebook) {
                    notebook = *pNotebook;
                    foundNotebookInCache = true;
                }
            }
            else if (notebook.hasName() && !notebook.name().isEmpty())
            {
                const QString notebookName = notebook.name();
                const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebookByName(notebookName);
                if (pNotebook) {
                    notebook = *pNotebook;
                    foundNotebookInCache = true;
                }
            }
        }

        if (!foundNotebookInCache)
        {
            bool res = m_pLocalStorageManager->FindNotebook(notebook, errorDescription);
            if (!res) {
                emit findNotebookFailed(notebook, errorDescription, requestId);
                return;
            }
        }

        emit findNotebookComplete(notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindDefaultNotebookRequest(Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindDefaultNotebook(notebook, errorDescription);
        if (!res) {
            emit findDefaultNotebookFailed(notebook, errorDescription, requestId);
            return;
        }

        emit findDefaultNotebookComplete(notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindLastUsedNotebookRequest(Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindLastUsedNotebook(notebook, errorDescription);
        if (!res) {
            emit findLastUsedNotebookFailed(notebook, errorDescription, requestId);
            return;
        }

        emit findLastUsedNotebookComplete(notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindDefaultOrLastUsedNotebookRequest(Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindDefaultOrLastUsedNotebook(notebook, errorDescription);
        if (!res) {
            emit findDefaultOrLastUsedNotebookFailed(notebook, errorDescription, requestId);
            return;
        }

        emit findDefaultOrLastUsedNotebookComplete(notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllNotebooksRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<Notebook> notebooks = m_pLocalStorageManager->ListAllNotebooks(errorDescription);
        if (notebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllNotebooksFailed(errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            foreach(const Notebook & notebook, notebooks) {
                m_pLocalStorageCacheManager->cacheNotebook(notebook);
            }
        }

        emit listAllNotebooksComplete(notebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllSharedNotebooksRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<SharedNotebookWrapper> sharedNotebooks = m_pLocalStorageManager->ListAllSharedNotebooks(errorDescription);
        if (sharedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllSharedNotebooksFailed(errorDescription, requestId);
            return;
        }

        emit listAllSharedNotebooksComplete(sharedNotebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid, QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<SharedNotebookWrapper> sharedNotebooks = m_pLocalStorageManager->ListSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
        if (sharedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listSharedNotebooksPerNotebookGuidFailed(notebookGuid, errorDescription, requestId);
            return;
        }

        emit listSharedNotebooksPerNotebookGuidComplete(notebookGuid, sharedNotebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeNotebookRequest(Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeNotebook(notebook, errorDescription);
        if (!res) {
            emit expungeNotebookFailed(notebook, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeNotebook(notebook);
        }

        emit expungeNotebookComplete(notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetLinkedNotebookCountRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetLinkedNotebookCount(errorDescription);
        if (count < 0) {
            emit getLinkedNotebookCountFailed(errorDescription, requestId);
        }
        else {
            emit getLinkedNotebookCountComplete(count, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddLinkedNotebook(linkedNotebook, errorDescription);
        if (!res) {
            emit addLinkedNotebookFailed(linkedNotebook, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheLinkedNotebook(linkedNotebook);
        }

        emit addLinkedNotebookComplete(linkedNotebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateLinkedNotebook(linkedNotebook, errorDescription);
        if (!res) {
            emit updateLinkedNotebookFailed(linkedNotebook, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheLinkedNotebook(linkedNotebook);
        }

        emit updateLinkedNotebookComplete(linkedNotebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId)
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
                emit findLinkedNotebookFailed(linkedNotebook, errorDescription, requestId);
                return;
            }
        }

        emit findLinkedNotebookComplete(linkedNotebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllLinkedNotebooksRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<LinkedNotebook> linkedNotebooks = m_pLocalStorageManager->ListAllLinkedNotebooks(errorDescription);
        if (linkedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllLinkedNotebooksFailed(errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            foreach(const LinkedNotebook & linkedNotebook, linkedNotebooks) {
                m_pLocalStorageCacheManager->cacheLinkedNotebook(linkedNotebook);
            }
        }

        emit listAllLinkedNotebooksComplete(linkedNotebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeLinkedNotebook(linkedNotebook, errorDescription);
        if (!res) {
            emit expungeLinkedNotebookFailed(linkedNotebook, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeLinkedNotebook(linkedNotebook);
        }

        emit expungeLinkedNotebookComplete(linkedNotebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetNoteCountRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetNoteCount(errorDescription);
        if (count < 0) {
            emit getNoteCountFailed(errorDescription, requestId);
        }
        else {
            emit getNoteCountComplete(count, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddNoteRequest(Note note, Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddNote(note, notebook, errorDescription);
        if (!res) {
            emit addNoteFailed(note, notebook, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNote(note);
        }

        emit addNoteComplete(note, notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateNoteRequest(Note note, Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateNote(note, notebook, errorDescription);
        if (!res) {
            emit updateNoteFailed(note, notebook, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNote(note);
        }

        emit updateNoteComplete(note, notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindNoteRequest(Note note, bool withResourceBinaryData, QUuid requestId)
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
                emit findNoteFailed(note, withResourceBinaryData, errorDescription, requestId);
                return;
            }
        }

        emit findNoteComplete(note, withResourceBinaryData, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllNotesPerNotebookRequest(Notebook notebook,
                                                                       bool withResourceBinaryData, QUuid requestId)
{
    try
    {
        QString errorDescription;

        QList<Note> notes = m_pLocalStorageManager->ListAllNotesPerNotebook(notebook, errorDescription,
                                                                          withResourceBinaryData);
        if (notes.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllNotesPerNotebookFailed(notebook, withResourceBinaryData, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            foreach(const Note & note, notes) {
                m_pLocalStorageCacheManager->cacheNote(note);
            }
        }

        emit listAllNotesPerNotebookComplete(notebook, withResourceBinaryData, notes, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onDeleteNoteRequest(Note note, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->DeleteNote(note, errorDescription);
        if (!res) {
            emit deleteNoteFailed(note, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNote(note);
        }

        emit deleteNoteComplete(note, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeNoteRequest(Note note, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeNote(note, errorDescription);
        if (!res) {
            emit expungeNoteFailed(note, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeNote(note);
        }

        emit expungeNoteComplete(note, requestId);
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

void LocalStorageManagerThreadWorker::onLinkTagWithNoteRequest(Tag tag, Note note, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->LinkTagWithNote(tag, note, errorDescription);
        if (!res) {
            emit linkTagWithNoteFailed(tag, note, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheTag(tag);
        }

        emit linkTagWithNoteComplete(tag, note, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindTagRequest(Tag tag, QUuid requestId)
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
            else if (tag.hasName() && !tag.name().isEmpty())
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
                emit findTagFailed(tag, errorDescription, requestId);
                return;
            }
        }

        emit findTagComplete(tag, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllTagsPerNoteRequest(Note note, QUuid requestId)
{
    try
    {
        QString errorDescription;

        QList<Tag> tags = m_pLocalStorageManager->ListAllTagsPerNote(note, errorDescription);
        if (tags.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllTagsPerNoteFailed(note, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            foreach(const Tag & tag, tags) {
                m_pLocalStorageCacheManager->cacheTag(tag);
            }
        }

        emit listAllTagsPerNoteComplete(tags, note, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllTagsRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;

        QList<Tag> tags = m_pLocalStorageManager->ListAllTags(errorDescription);
        if (tags.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllTagsFailed(errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            foreach(const Tag & tag, tags) {
                m_pLocalStorageCacheManager->cacheTag(tag);
            }
        }

        emit listAllTagsComplete(tags, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onDeleteTagRequest(Tag tag, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->DeleteTag(tag, errorDescription);
        if (!res) {
            emit deleteTagFailed(tag, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheTag(tag);
        }

        emit deleteTagComplete(tag, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeTagRequest(Tag tag, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeTag(tag, errorDescription);
        if (!res) {
            emit expungeTagFailed(tag, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeTag(tag);
        }

        emit expungeTagComplete(tag, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetResourceCountRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetEnResourceCount(errorDescription);
        if (count < 0) {
            emit getResourceCountFailed(errorDescription, requestId);
        }
        else {
            emit getResourceCountComplete(count, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddResourceRequest(ResourceWrapper resource, Note note, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddEnResource(resource, note, errorDescription);
        if (!res) {
            emit addResourceFailed(resource, note, errorDescription, requestId);
            return;
        }

        emit addResourceComplete(resource, note, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateResourceRequest(ResourceWrapper resource, Note note, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateEnResource(resource, note, errorDescription);
        if (!res) {
            emit updateResourceFailed(resource, note, errorDescription, requestId);
            return;
        }

        emit updateResourceComplete(resource, note, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindResourceRequest(ResourceWrapper resource, bool withBinaryData, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->FindEnResource(resource, errorDescription, withBinaryData);
        if (!res) {
            emit findResourceFailed(resource, withBinaryData, errorDescription, requestId);
            return;
        }

        emit findResourceComplete(resource, withBinaryData, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeResourceRequest(ResourceWrapper resource, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeEnResource(resource, errorDescription);
        if (!res) {
            emit expungeResourceFailed(resource, errorDescription, requestId);
            return;
        }

        emit expungeResourceComplete(resource, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetSavedSearchCountRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->GetSavedSearchCount(errorDescription);
        if (count < 0) {
            emit getSavedSearchCountFailed(errorDescription, requestId);
        }
        else {
            emit getSavedSearchCountComplete(count, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddSavedSearchRequest(SavedSearch search, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->AddSavedSearch(search, errorDescription);
        if (!res) {
            emit addSavedSearchFailed(search, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheSavedSearch(search);
        }

        emit addSavedSearchComplete(search, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateSavedSearchRequest(SavedSearch search, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->UpdateSavedSearch(search, errorDescription);
        if (!res) {
            emit updateSavedSearchFailed(search, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheSavedSearch(search);
        }

        emit updateSavedSearchComplete(search, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onFindSavedSearchRequest(SavedSearch search, QUuid requestId)
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
                emit findSavedSearchFailed(search, errorDescription, requestId);
                return;
            }
        }

        emit findSavedSearchComplete(search, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllSavedSearchesRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<SavedSearch> searches = m_pLocalStorageManager->ListAllSavedSearches(errorDescription);
        if (searches.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllSavedSearchesFailed(errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            foreach(const SavedSearch & search, searches) {
                m_pLocalStorageCacheManager->cacheSavedSearch(search);
            }
        }

        emit listAllSavedSearchesComplete(searches, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeSavedSearch(SavedSearch search, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->ExpungeSavedSearch(search, errorDescription);
        if (!res) {
            emit expungeSavedSearchFailed(search, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->expungeSavedSearch(search);
        }

        emit expungeSavedSearchComplete(search, requestId);
    }
    CATCH_EXCEPTION
}

} // namespace qute_note
