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
        m_firstNotebook.setName("First notebook");
        m_firstNotebook.setLocal(true);
        m_firstNotebook.setDirty(false);

        m_secondNotebook.setName("Second notebook");
        m_secondNotebook.setLocal(false);
        m_secondNotebook.setGuid(UidGenerator::Generate());
        m_secondNotebook.setDirty(false);
        m_secondNotebook.setShortcut(true);

        m_thirdNotebook.setName("Third notebook");
        m_thirdNotebook.setLocal(true);
        m_thirdNotebook.setDirty(true);
        m_thirdNotebook.setShortcut(true);

        m_pLocalStorageManagerThreadWorker->onAddNotebookRequest(m_firstNotebook, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNotebookRequest(m_secondNotebook, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNotebookRequest(m_thirdNotebook, QUuid());

        m_firstSavedSearch.setName("First saved search");
        m_firstSavedSearch.setLocal(false);
        m_firstSavedSearch.setGuid(UidGenerator::Generate());
        m_firstSavedSearch.setDirty(false);
        m_firstSavedSearch.setShortcut(true);

        m_secondSavedSearch.setName("Second saved search");
        m_secondSavedSearch.setLocal(true);
        m_secondSavedSearch.setDirty(true);

        m_thirdSavedSearch.setName("Third saved search");
        m_thirdSavedSearch.setLocal(true);
        m_thirdSavedSearch.setDirty(false);
        m_thirdSavedSearch.setShortcut(true);

        m_fourthSavedSearch.setName("Fourth saved search");
        m_fourthSavedSearch.setLocal(false);
        m_fourthSavedSearch.setGuid(UidGenerator::Generate());
        m_fourthSavedSearch.setDirty(true);
        m_fourthSavedSearch.setShortcut(true);

        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(m_firstSavedSearch, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(m_secondSavedSearch, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(m_thirdSavedSearch, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(m_fourthSavedSearch, QUuid());

        m_firstTag.setName("First tag");
        m_firstTag.setLocal(true);
        m_firstTag.setDirty(false);

        m_secondTag.setName("Second tag");
        m_secondTag.setLocal(false);
        m_secondTag.setGuid(UidGenerator::Generate());
        m_secondTag.setDirty(true);
        m_secondTag.setShortcut(true);

        m_thirdTag.setName("Third tag");
        m_thirdTag.setLocal(false);
        m_thirdTag.setGuid(UidGenerator::Generate());
        m_thirdTag.setDirty(false);
        m_thirdTag.setParentLocalUid(m_secondTag.localUid());
        m_thirdTag.setParentGuid(m_secondTag.guid());

        m_fourthTag.setName("Fourth tag");
        m_fourthTag.setLocal(true);
        m_fourthTag.setDirty(true);
        m_fourthTag.setShortcut(true);

        m_pLocalStorageManagerThreadWorker->onAddTagRequest(m_firstTag, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddTagRequest(m_secondTag, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddTagRequest(m_thirdTag, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddTagRequest(m_fourthTag, QUuid());

        m_firstNote.setTitle("First note");
        m_firstNote.setContent("<en-note><h1>First note</h1></en-note>");
        m_firstNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_firstNote.setModificationTimestamp(m_firstNote.creationTimestamp());
        m_firstNote.setNotebookLocalUid(m_firstNotebook.localUid());
        m_firstNote.setLocal(true);
        m_firstNote.setTagLocalUids(QStringList() << m_firstTag.localUid() << m_secondTag.localUid());
        m_firstNote.setDirty(false);
        m_firstNote.setShortcut(true);

        m_secondNote.setTitle("Second note");
        m_secondNote.setContent("<en-note><h1>Second note</h1></en-note>");
        m_secondNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_secondNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_secondNote.setNotebookLocalUid(m_firstNotebook.localUid());
        m_secondNote.setLocal(true);
        m_secondNote.setTagLocalUids(QStringList() << m_firstTag.localUid() << m_fourthTag.localUid());
        m_secondNote.setDirty(true);
        m_secondNote.setShortcut(true);

        m_thirdNote.setGuid(UidGenerator::Generate());
        m_thirdNote.setTitle("Third note");
        m_thirdNote.setContent("<en-note><h1>Third note</h1></en-note>");
        m_thirdNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_thirdNote.setModificationTimestamp(m_thirdNote.creationTimestamp());
        m_thirdNote.setNotebookLocalUid(m_secondNotebook.localUid());
        m_thirdNote.setNotebookGuid(m_secondNotebook.guid());
        m_thirdNote.setLocal(false);
        m_thirdNote.setTagLocalUids(QStringList() << m_thirdTag.localUid());
        m_thirdNote.setTagGuids(QStringList() << m_thirdTag.guid());
        m_thirdNote.setShortcut(true);

        m_fourthNote.setGuid(UidGenerator::Generate());
        m_fourthNote.setTitle("Fourth note");
        m_fourthNote.setContent("<en-note><h1>Fourth note</h1></en-note>");
        m_fourthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_fourthNote.setModificationTimestamp(m_fourthNote.creationTimestamp());
        m_fourthNote.setNotebookLocalUid(m_secondNotebook.localUid());
        m_fourthNote.setNotebookGuid(m_secondNotebook.guid());
        m_fourthNote.setLocal(false);
        m_fourthNote.setDirty(true);
        m_fourthNote.setTagLocalUids(QStringList() << m_secondTag.localUid() << m_thirdTag.localUid());
        m_fourthNote.setTagGuids(QStringList() << m_secondTag.guid() << m_thirdTag.guid());

        m_fifthNote.setGuid(UidGenerator::Generate());
        m_fifthNote.setTitle("Fifth note");
        m_fifthNote.setContent("<en-note><h1>Fifth note</h1></en-note>");
        m_fifthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_fifthNote.setModificationTimestamp(m_fifthNote.creationTimestamp());
        m_fifthNote.setNotebookLocalUid(m_secondNotebook.localUid());
        m_fifthNote.setNotebookGuid(m_secondNotebook.guid());
        m_fifthNote.setDeletionTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_fifthNote.setLocal(false);
        m_fifthNote.setDirty(true);

        m_sixthNote.setTitle("Sixth note");
        m_sixthNote.setContent("<en-note><h1>Sixth note</h1></en-note>");
        m_sixthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_sixthNote.setModificationTimestamp(m_sixthNote.creationTimestamp());
        m_sixthNote.setNotebookLocalUid(m_thirdNotebook.localUid());
        m_sixthNote.setLocal(true);
        m_sixthNote.setDirty(true);
        m_sixthNote.setTagLocalUids(QStringList() << m_fourthTag.localUid());
        m_sixthNote.setShortcut(true);

        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(m_firstNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(m_secondNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(m_thirdNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(m_fourthNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(m_fifthNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(m_sixthNote, QUuid());

        NoteCache noteCache(10);
        NotebookCache notebookCache(3);
        TagCache tagCache(5);
        SavedSearchCache savedSearchCache(5);

        FavoritesModel * model = new FavoritesModel(*m_pLocalStorageManagerThreadWorker, noteCache,
                                                    notebookCache, tagCache, savedSearchCache, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // TODO: implement model-specific tests here

        emit success();
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
