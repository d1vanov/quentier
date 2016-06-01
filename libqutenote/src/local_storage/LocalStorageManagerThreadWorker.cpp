#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/SysInfo.h>

namespace qute_note {

LocalStorageManagerThreadWorker::LocalStorageManagerThreadWorker(const QString & username, const qint32 userId,
                                                                 const bool startFromScratch, const bool overrideLock,
                                                                 QObject * parent) :
    QObject(parent),
    m_username(username),
    m_userId(userId),
    m_startFromScratch(startFromScratch),
    m_overrideLock(overrideLock),
    m_pLocalStorageManager(Q_NULLPTR),
    m_useCache(true),
    m_pLocalStorageCacheManager(Q_NULLPTR)
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
        return Q_NULLPTR;
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

    m_pLocalStorageManager = new LocalStorageManager(m_username, m_userId, m_startFromScratch, m_overrideLock);

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
        int count = m_pLocalStorageManager->userCount(errorDescription);
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
        m_pLocalStorageManager->switchUser(username, userId, startFromScratch);
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

        bool res = m_pLocalStorageManager->addUser(user, errorDescription);
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

        bool res = m_pLocalStorageManager->updateUser(user, errorDescription);
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

        bool res = m_pLocalStorageManager->findUser(user, errorDescription);
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

        bool res = m_pLocalStorageManager->deleteUser(user, errorDescription);
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

        bool res = m_pLocalStorageManager->expungeUser(user, errorDescription);
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
        int count = m_pLocalStorageManager->notebookCount(errorDescription);
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

        bool res = m_pLocalStorageManager->addNotebook(notebook, errorDescription);
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

        bool res = m_pLocalStorageManager->updateNotebook(notebook, errorDescription);
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
            if (notebookHasGuid || !notebook.localUid().isEmpty())
            {
                const QString uid = (notebookHasGuid ? notebook.guid() : notebook.localUid());
                LocalStorageCacheManager::WhichUid wg = (notebookHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalUid);

                const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(uid, wg);
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
            bool res = m_pLocalStorageManager->findNotebook(notebook, errorDescription);
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

        bool res = m_pLocalStorageManager->findDefaultNotebook(notebook, errorDescription);
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

        bool res = m_pLocalStorageManager->findLastUsedNotebook(notebook, errorDescription);
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

        bool res = m_pLocalStorageManager->findDefaultOrLastUsedNotebook(notebook, errorDescription);
        if (!res) {
            emit findDefaultOrLastUsedNotebookFailed(notebook, errorDescription, requestId);
            return;
        }

        emit findDefaultOrLastUsedNotebookComplete(notebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllNotebooksRequest(size_t limit, size_t offset,
                                                                LocalStorageManager::ListNotebooksOrder::type order,
                                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                                QString linkedNotebookGuid, QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<Notebook> notebooks = m_pLocalStorageManager->listAllNotebooks(errorDescription, limit, offset, order,
                                                                             orderDirection, linkedNotebookGuid);
        if (notebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllNotebooksFailed(limit, offset, order, orderDirection, linkedNotebookGuid,
                                        errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numNotebooks = notebooks.size();
            for(int i = 0; i < numNotebooks; ++i) {
                const Notebook & notebook = notebooks[i];
                m_pLocalStorageCacheManager->cacheNotebook(notebook);
            }
        }

        emit listAllNotebooksComplete(limit, offset, order, orderDirection, linkedNotebookGuid, notebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllSharedNotebooksRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<SharedNotebookWrapper> sharedNotebooks = m_pLocalStorageManager->listAllSharedNotebooks(errorDescription);
        if (sharedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllSharedNotebooksFailed(errorDescription, requestId);
            return;
        }

        emit listAllSharedNotebooksComplete(sharedNotebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListNotebooksRequest(LocalStorageManager::ListObjectsOptions flag,
                                                             size_t limit, size_t offset,
                                                             LocalStorageManager::ListNotebooksOrder::type order,
                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                             QString linkedNotebookGuid, QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<Notebook> notebooks = m_pLocalStorageManager->listNotebooks(flag, errorDescription, limit,
                                                                          offset, order, orderDirection,
                                                                          linkedNotebookGuid);
        if (notebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listNotebooksFailed(flag, limit, offset, order, orderDirection, linkedNotebookGuid, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numNotebooks = notebooks.size();
            for(int i = 0; i < numNotebooks; ++i) {
                const Notebook & notebook = notebooks[i];
                m_pLocalStorageCacheManager->cacheNotebook(notebook);
            }
        }

        emit listNotebooksComplete(flag, limit, offset, order, orderDirection, linkedNotebookGuid, notebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid, QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<SharedNotebookWrapper> sharedNotebooks = m_pLocalStorageManager->listSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
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

        bool res = m_pLocalStorageManager->expungeNotebook(notebook, errorDescription);
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
        int count = m_pLocalStorageManager->linkedNotebookCount(errorDescription);
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

        bool res = m_pLocalStorageManager->addLinkedNotebook(linkedNotebook, errorDescription);
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

        bool res = m_pLocalStorageManager->updateLinkedNotebook(linkedNotebook, errorDescription);
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
            bool res = m_pLocalStorageManager->findLinkedNotebook(linkedNotebook, errorDescription);
            if (!res) {
                emit findLinkedNotebookFailed(linkedNotebook, errorDescription, requestId);
                return;
            }
        }

        emit findLinkedNotebookComplete(linkedNotebook, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllLinkedNotebooksRequest(size_t limit, size_t offset,
                                                                      LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                                      LocalStorageManager::OrderDirection::type orderDirection,
                                                                      QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<LinkedNotebook> linkedNotebooks = m_pLocalStorageManager->listAllLinkedNotebooks(errorDescription, limit,
                                                                                               offset, order, orderDirection);
        if (linkedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllLinkedNotebooksFailed(limit, offset, order, orderDirection, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numLinkedNotebooks = linkedNotebooks.size();
            for(int i = 0; i < numLinkedNotebooks; ++i) {
                const LinkedNotebook & linkedNotebook = linkedNotebooks[i];
                m_pLocalStorageCacheManager->cacheLinkedNotebook(linkedNotebook);
            }
        }

        emit listAllLinkedNotebooksComplete(limit, offset, order, orderDirection, linkedNotebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListLinkedNotebooksRequest(LocalStorageManager::ListObjectsOptions flag,
                                                                   size_t limit, size_t offset,
                                                                   LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                                   LocalStorageManager::OrderDirection::type orderDirection,
                                                                   QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<LinkedNotebook> linkedNotebooks = m_pLocalStorageManager->listLinkedNotebooks(flag, errorDescription, limit,
                                                                                            offset, order, orderDirection);
        if (linkedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
            emit listLinkedNotebooksFailed(flag, limit, offset, order, orderDirection, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numLinkedNotebooks = linkedNotebooks.size();
            for(int i = 0; i < numLinkedNotebooks; ++i) {
                const LinkedNotebook & linkedNotebook = linkedNotebooks[i];
                m_pLocalStorageCacheManager->cacheLinkedNotebook(linkedNotebook);
            }
        }

        emit listLinkedNotebooksComplete(flag, limit, offset, order, orderDirection, linkedNotebooks, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->expungeLinkedNotebook(linkedNotebook, errorDescription);
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
        int count = m_pLocalStorageManager->noteCount(errorDescription);
        if (count < 0) {
            emit getNoteCountFailed(errorDescription, requestId);
        }
        else {
            emit getNoteCountComplete(count, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetNoteCountPerNotebookRequest(Notebook notebook, QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->noteCountPerNotebook(notebook, errorDescription);
        if (count < 0) {
            emit getNoteCountPerNotebookFailed(errorDescription, notebook, requestId);
        }
        else {
            emit getNoteCountPerNotebookComplete(count, notebook, requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onAddNoteRequest(Note note, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->addNote(note, errorDescription);
        if (!res) {
            emit addNoteFailed(note, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNote(note);
        }

        emit addNoteComplete(note, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onUpdateNoteRequest(Note note, bool updateResources,
                                                          bool updateTags, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->updateNote(note, updateResources, updateTags, errorDescription);
        if (!res) {
            emit updateNoteFailed(note, updateResources, updateTags, errorDescription, requestId);
            return;
        }

        if (m_useCache) {
            m_pLocalStorageCacheManager->cacheNote(note);
        }

        emit updateNoteComplete(note, updateResources, updateTags, requestId);
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
            const QString uid = (noteHasGuid ? note.guid() : note.localUid());
            LocalStorageCacheManager::WhichUid wu = (noteHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalUid);

            const Note * pNote = m_pLocalStorageCacheManager->findNote(uid, wu);
            if (pNote) {
                note = *pNote;
                foundNoteInCache = true;
            }
        }

        if (!foundNoteInCache)
        {
            bool res = m_pLocalStorageManager->findNote(note, errorDescription, withResourceBinaryData);
            if (!res) {
                emit findNoteFailed(note, withResourceBinaryData, errorDescription, requestId);
                return;
            }
        }

        emit findNoteComplete(note, withResourceBinaryData, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllNotesPerNotebookRequest(Notebook notebook, bool withResourceBinaryData,
                                                                       LocalStorageManager::ListObjectsOptions flag,
                                                                       size_t limit, size_t offset,
                                                                       LocalStorageManager::ListNotesOrder::type order,
                                                                       LocalStorageManager::OrderDirection::type orderDirection,
                                                                       QUuid requestId)
{
    try
    {
        QString errorDescription;

        QList<Note> notes = m_pLocalStorageManager->listAllNotesPerNotebook(notebook, errorDescription,
                                                                            withResourceBinaryData, flag,
                                                                            limit, offset, order, orderDirection);
        if (notes.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllNotesPerNotebookFailed(notebook, withResourceBinaryData, flag, limit, offset,
                                               order, orderDirection, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numNotes = notes.size();
            for(int i = 0; i < numNotes; ++i) {
                const Note & note = notes[i];
                m_pLocalStorageCacheManager->cacheNote(note);
            }
        }

        emit listAllNotesPerNotebookComplete(notebook, withResourceBinaryData, flag, limit,
                                             offset, order, orderDirection, notes, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListNotesPerTagRequest(Tag tag, bool withResourceBinaryData,
                                                               LocalStorageManager::ListObjectsOptions flag,
                                                               size_t limit, size_t offset,
                                                               LocalStorageManager::ListNotesOrder::type order,
                                                               LocalStorageManager::OrderDirection::type orderDirection,
                                                               QUuid requestId)
{
    try
    {
        QString errorDescription;

        QList<Note> notes = m_pLocalStorageManager->listNotesPerTag(tag, errorDescription,
                                                                    withResourceBinaryData, flag,
                                                                    limit, offset, order, orderDirection);
        if (notes.isEmpty() && !errorDescription.isEmpty()) {
            emit listNotesPerTagFailed(tag, withResourceBinaryData, flag, limit, offset,
                                       order, orderDirection, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numNotes = notes.size();
            for(int i = 0; i < numNotes; ++i) {
                const Note & note = notes[i];
                m_pLocalStorageCacheManager->cacheNote(note);
            }
        }

        emit listNotesPerTagComplete(tag, withResourceBinaryData, flag, limit,
                                     offset, order, orderDirection, notes, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListNotesRequest(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData, size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order, LocalStorageManager::OrderDirection::type orderDirection, QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<Note> notes = m_pLocalStorageManager->listNotes(flag, errorDescription, withResourceBinaryData,
                                                              limit, offset, order, orderDirection);
        if (notes.isEmpty() && !errorDescription.isEmpty()) {
            emit listNotesFailed(flag, withResourceBinaryData, limit, offset, order,
                                 orderDirection, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numNotes = notes.size();
            for(int i = 0; i < numNotes; ++i) {
                const Note & note = notes[i];
                m_pLocalStorageCacheManager->cacheNote(note);
            }
        }

        emit listNotesComplete(flag, withResourceBinaryData, limit, offset, order, orderDirection, notes, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeNoteRequest(Note note, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->expungeNote(note, errorDescription);
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
        int count = m_pLocalStorageManager->tagCount(errorDescription);
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

        bool res = m_pLocalStorageManager->addTag(tag, errorDescription);
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

        bool res = m_pLocalStorageManager->updateTag(tag, errorDescription);
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

        bool res = m_pLocalStorageManager->linkTagWithNote(tag, note, errorDescription);
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
            if (tagHasGuid || !tag.localUid().isEmpty())
            {
                const QString uid = (tagHasGuid ? tag.guid() : tag.localUid());
                LocalStorageCacheManager::WhichUid wg = (tagHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalUid);

                const Tag * pTag = m_pLocalStorageCacheManager->findTag(uid, wg);
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
            bool res = m_pLocalStorageManager->findTag(tag, errorDescription);
            if (!res) {
                emit findTagFailed(tag, errorDescription, requestId);
                return;
            }
        }

        emit findTagComplete(tag, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllTagsPerNoteRequest(Note note, LocalStorageManager::ListObjectsOptions flag,
                                                                  size_t limit, size_t offset,
                                                                  LocalStorageManager::ListTagsOrder::type order,
                                                                  LocalStorageManager::OrderDirection::type orderDirection,
                                                                  QUuid requestId)
{
    try
    {
        QString errorDescription;

        QList<Tag> tags = m_pLocalStorageManager->listAllTagsPerNote(note, errorDescription, flag, limit,
                                                                     offset, order, orderDirection);
        if (tags.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllTagsPerNoteFailed(note, flag, limit, offset, order,
                                          orderDirection, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            foreach(const Tag & tag, tags) {
                m_pLocalStorageCacheManager->cacheTag(tag);
            }
        }

        emit listAllTagsPerNoteComplete(tags, note, flag, limit, offset, order, orderDirection, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllTagsRequest(size_t limit, size_t offset,
                                                           LocalStorageManager::ListTagsOrder::type order,
                                                           LocalStorageManager::OrderDirection::type orderDirection,
                                                           QString linkedNotebookGuid, QUuid requestId)
{
    try
    {
        QString errorDescription;

        QList<Tag> tags = m_pLocalStorageManager->listAllTags(errorDescription, limit, offset,
                                                              order, orderDirection, linkedNotebookGuid);
        if (tags.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllTagsFailed(limit, offset, order, orderDirection, linkedNotebookGuid, errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numTags = tags.size();
            for(int i = 0; i < numTags; ++i) {
                const Tag & tag = tags[i];
                m_pLocalStorageCacheManager->cacheTag(tag);
            }
        }

        emit listAllTagsComplete(limit, offset, order, orderDirection, linkedNotebookGuid, tags, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListTagsRequest(LocalStorageManager::ListObjectsOptions flag,
                                                        size_t limit, size_t offset,
                                                        LocalStorageManager::ListTagsOrder::type order,
                                                        LocalStorageManager::OrderDirection::type orderDirection,
                                                        QString linkedNotebookGuid, QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<Tag> tags = m_pLocalStorageManager->listTags(flag, errorDescription, limit, offset, order,
                                                           orderDirection, linkedNotebookGuid);
        if (tags.isEmpty() && !errorDescription.isEmpty()) {
            emit listTagsFailed(flag, limit, offset, order, orderDirection, linkedNotebookGuid, errorDescription, requestId);
        }

        if (m_useCache)
        {
            const int numTags = tags.size();
            for(int i = 0; i < numTags; ++i) {
                const Tag & tag = tags[i];
                m_pLocalStorageCacheManager->cacheTag(tag);
            }
        }

        emit listTagsComplete(flag, limit, offset, order, orderDirection, linkedNotebookGuid, tags, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeTagRequest(Tag tag, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->expungeTag(tag, errorDescription);
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

void LocalStorageManagerThreadWorker::onExpungeNotelessTagsFromLinkedNotebooksRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        bool res = m_pLocalStorageManager->expungeNotelessTagsFromLinkedNotebooks(errorDescription);
        if (!res) {
            emit expungeNotelessTagsFromLinkedNotebooksFailed(errorDescription, requestId);
        }
        else {
            emit expungeNotelessTagsFromLinkedNotebooksComplete(requestId);
        }
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onGetResourceCountRequest(QUuid requestId)
{
    try
    {
        QString errorDescription;
        int count = m_pLocalStorageManager->enResourceCount(errorDescription);
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

        bool res = m_pLocalStorageManager->addEnResource(resource, note, errorDescription);
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

        bool res = m_pLocalStorageManager->updateEnResource(resource, note, errorDescription);
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

        bool res = m_pLocalStorageManager->findEnResource(resource, errorDescription, withBinaryData);
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

        bool res = m_pLocalStorageManager->expungeEnResource(resource, errorDescription);
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
        int count = m_pLocalStorageManager->savedSearchCount(errorDescription);
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

        bool res = m_pLocalStorageManager->addSavedSearch(search, errorDescription);
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

        bool res = m_pLocalStorageManager->updateSavedSearch(search, errorDescription);
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
            if (searchHasGuid || !search.localUid().isEmpty())
            {
                const QString uid = (searchHasGuid ? search.guid() : search.localUid());
                const LocalStorageCacheManager::WhichUid wg = (searchHasGuid ? LocalStorageCacheManager::Guid : LocalStorageCacheManager::LocalUid);

                const SavedSearch * pSearch = m_pLocalStorageCacheManager->findSavedSearch(uid, wg);
                if (pSearch) {
                    search = *pSearch;
                    foundCachedSavedSearch = true;
                }
            }
            else if (search.hasName() && !search.name().isEmpty())
            {
                const QString searchName = search.name();
                const SavedSearch * pSearch = m_pLocalStorageCacheManager->findSavedSearchByName(searchName);
                if (pSearch) {
                    search = *pSearch;
                    foundCachedSavedSearch = true;
                }
            }
        }

        if (!foundCachedSavedSearch)
        {
            bool res = m_pLocalStorageManager->findSavedSearch(search, errorDescription);
            if (!res) {
                emit findSavedSearchFailed(search, errorDescription, requestId);
                return;
            }
        }

        emit findSavedSearchComplete(search, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListAllSavedSearchesRequest(size_t limit, size_t offset,
                                                                    LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                    LocalStorageManager::OrderDirection::type orderDirection,
                                                                    QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<SavedSearch> savedSearches = m_pLocalStorageManager->listAllSavedSearches(errorDescription, limit, offset,
                                                                                   order, orderDirection);
        if (savedSearches.isEmpty() && !errorDescription.isEmpty()) {
            emit listAllSavedSearchesFailed(limit, offset, order, orderDirection,
                                            errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numSavedSearches = savedSearches.size();
            for(int i = 0; i < numSavedSearches; ++i) {
                const SavedSearch & search = savedSearches[i];
                m_pLocalStorageCacheManager->cacheSavedSearch(search);
            }
        }

        emit listAllSavedSearchesComplete(limit, offset, order, orderDirection, savedSearches, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions flag,
                                                                 size_t limit, size_t offset,
                                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                                 QUuid requestId)
{
    try
    {
        QString errorDescription;
        QList<SavedSearch> savedSearches = m_pLocalStorageManager->listSavedSearches(flag, errorDescription, limit,
                                                                                     offset, order, orderDirection);
        if (savedSearches.isEmpty() && !errorDescription.isEmpty()) {
            emit listSavedSearchesFailed(flag, limit, offset, order, orderDirection,
                                         errorDescription, requestId);
            return;
        }

        if (m_useCache)
        {
            const int numSavedSearches = savedSearches.size();
            for(int i = 0; i < numSavedSearches; ++i) {
                const SavedSearch & search = savedSearches[i];
                m_pLocalStorageCacheManager->cacheSavedSearch(search);
            }
        }

        emit listSavedSearchesComplete(flag, limit, offset, order, orderDirection, savedSearches, requestId);
    }
    CATCH_EXCEPTION
}

void LocalStorageManagerThreadWorker::onExpungeSavedSearch(SavedSearch search, QUuid requestId)
{
    try
    {
        QString errorDescription;

        bool res = m_pLocalStorageManager->expungeSavedSearch(search, errorDescription);
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
