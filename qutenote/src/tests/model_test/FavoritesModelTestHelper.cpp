#include "FavoritesModelTestHelper.h"
#include "../../models/FavoritesModel.h"
#include "modeltest.h"
#include "Macros.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/SysInfo.h>

namespace qute_note {

FavoritesModelTestHelper::FavoritesModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                                   QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerThreadWorker(pLocalStorageManagerThreadWorker),
    m_model(Q_NULLPTR),
    m_firstNotebook(),
    m_secondNotebook(),
    m_thirdNotebook(),
    m_firstNote(),
    m_secondNote(),
    m_thirdNote(),
    m_fourthNote(),
    m_fifthNote(),
    m_sixthNote(),
    m_seventhNote(),
    m_eighthNote(),
    m_ninethNote(),
    m_firstTag(),
    m_secondTag(),
    m_thirdTag(),
    m_fourthTag(),
    m_firstSavedSearch(),
    m_secondSavedSearch(),
    m_thirdSavedSearch(),
    m_fourthSavedSearch()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onUpdateNoteFailed,Note,bool,bool,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onFindNoteFailed,Note,bool,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotesFailed,
                                                                  LocalStorageManager::ListObjectsOptions,bool,
                                                                  size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                                                                  LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onListNotesFailed,LocalStorageManager::ListObjectsOptions,bool,
                                  size_t,size_t,LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,
                                  QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onUpdateNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onFindNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksFailed,
                                                                  LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                  LocalStorageManager::ListNotebooksOrder::type,
                                                                  LocalStorageManager::OrderDirection::type,
                                                                  QString,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onListNotebooksFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onUpdateTagFailed,Tag,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onFindTagFailed,Tag,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listTagsFailed,
                                                                  LocalStorageManager::ListObjectsOptions,
                                                                  size_t,size_t,LocalStorageManager::ListTagsOrder::type,
                                                                  LocalStorageManager::OrderDirection::type,
                                                                  QString,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onListTagsFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListTagsOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onUpdateSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onUpdateSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onFindSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,
                                                                  LocalStorageManager::ListObjectsOptions,
                                                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                  LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(FavoritesModelTestHelper,onListSavedSearchesFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                                  QString,QUuid));
}

void FavoritesModelTestHelper::launchTest()
{
    QNDEBUG("FavoritesModelTestHelper::launchTest");

    try
    {
        // TODO: implement
        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void FavoritesModelTestHelper::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onUpdateNoteComplete: note = " << note
            << ", update resources = " << (updateResources ? "true" : "false")
            << ", update tags = " << (updateTags ? "true" : "false")
            << ", request id = " << requestId);

    // TODO: implement
}

void FavoritesModelTestHelper::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                                  QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onUpdateNoteFailed: note = " << note << "\nUpdate resources = "
            << (updateResources ? "true" : "false") << ", update tags = " << (updateTags ? "true" : "false")
            << ", error description: " << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onFindNoteFailed: note = " << note << "\nWith resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                                 size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onListNotesFailed: flag = " << flag << ", with resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onUpdateNotebookComplete: notebook = " << notebook
            << ", request id = " << requestId);

    // TODO: implement
}

void FavoritesModelTestHelper::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onUpdateNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onFindNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
                                                     LocalStorageManager::ListNotebooksOrder::type order,
                                                     LocalStorageManager::OrderDirection::type orderDirection,
                                                     QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onListNotebooksFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = " << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid)
            << ", error description = " << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onUpdateTagComplete: tag = " << tag << ", request id = " << requestId);

    // TODO: implement
}

void FavoritesModelTestHelper::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onUpdateTagFailed: tag = " << tag << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onFindTagFailed: tag = " << tag << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                                size_t limit, size_t offset,
                                                LocalStorageManager::ListTagsOrder::type order,
                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                QString linkedNotebookGuid,
                                                QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onListTagsFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid: is null = " << (linkedNotebookGuid.isNull() ? "true" : "false")
            << ", is empty = " << (linkedNotebookGuid.isEmpty() ? "true" : "false") << ", value = "
            << linkedNotebookGuid << ", error description = " << errorDescription << ", request id = "
            << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onUpdateSavedSearchComplete: search = " << search
            << ", request id = " << requestId);

    // TODO: implement
}

void FavoritesModelTestHelper::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onUpdateSavedSearchFailed: search = " << search << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onFindSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onFindSavedSearchFailed: search = " << search << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                         size_t limit, size_t offset,
                                                         LocalStorageManager::ListSavedSearchesOrder::type order,
                                                         LocalStorageManager::OrderDirection::type orderDirection,
                                                         QString errorDescription, QUuid requestId)
{
    QNDEBUG("FavoritesModelTestHelper::onListSavedSearchesFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", error description = " << errorDescription << ", request id = " << requestId);

    emit failure();
}

void FavoritesModelTestHelper::checkSorting(const FavoritesModel & model)
{
    QNDEBUG("FavoritesModelTestHelper::checkSorting");

    // TODO: implement
    Q_UNUSED(model)
}

} // namespace qute_note
