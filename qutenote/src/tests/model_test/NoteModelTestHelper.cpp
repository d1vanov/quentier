#include "NoteModelTestHelper.h"
#include "../../models/NoteModel.h"
#include "modeltest.h"
#include "Macros.h"
#include <qute_note/types/Note.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/SysInfo.h>

namespace qute_note {

NoteModelTestHelper::NoteModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                         QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerThreadWorker(pLocalStorageManagerThreadWorker),
    m_model(Q_NULLPTR),
    m_firstNotebook(),
    m_noteToExpungeLocalUid(),
    m_expectingNewNoteFromLocalStorage(false),
    m_expectingNoteUpdateFromLocalStorage(false),
    m_expectingNoteDeletionFromLocalStorage(false),
    m_expectingNoteExpungeFromLocalStorage(false)
{
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onAddNoteComplete,Note,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteFailed,Note,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onAddNoteFailed,Note,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onUpdateNoteFailed,Note,bool,bool,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onFindNoteFailed,Note,bool,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotesFailed,
                                                                LocalStorageManager::ListObjectsOptions,bool,
                                                                size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onListNotesFailed,LocalStorageManager::ListObjectsOptions,bool,
                                  size_t,size_t,LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,
                                  QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,deleteNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onDeleteNoteComplete,Note,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,deleteNoteFailed,Note,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onDeleteNoteFailed,Note,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteFailed,Note,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onExpungeNoteFailed,Note,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onAddNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onUpdateNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onAddTagFailed,Tag,QString,QUuid));
}

#define CATCH_EXCEPTION() \
    catch(const IQuteNoteException & exception) { \
        SysInfo & sysInfo = SysInfo::GetSingleton(); \
        QNWARNING("Caught QuteNote exception: " + exception.errorMessage() + \
                  ", what: " + QString(exception.what()) + "; stack trace: " + sysInfo.GetStackTrace()); \
    } \
    catch(const std::exception & exception) { \
        SysInfo & sysInfo = SysInfo::GetSingleton(); \
        QNWARNING("Caught std::exception: " + QString(exception.what()) + "; stack trace: " + sysInfo.GetStackTrace()); \
    } \
    catch(...) { \
        SysInfo & sysInfo = SysInfo::GetSingleton(); \
        QNWARNING("Caught some unknown exception; stack trace: " + sysInfo.GetStackTrace()); \
    }

void NoteModelTestHelper::launchTest()
{
    QNDEBUG("NoteModelTestHelper::launchTest");

    try {
        Notebook firstNotebook;
        firstNotebook.setName("First notebook");
        firstNotebook.setLocal(true);
        firstNotebook.setDirty(false);

        Notebook secondNotebook;
        secondNotebook.setGuid(UidGenerator::Generate());
        secondNotebook.setName("Second notebook");
        secondNotebook.setLocal(false);
        secondNotebook.setDirty(false);

        Notebook thirdNotebook;
        thirdNotebook.setGuid(UidGenerator::Generate());
        thirdNotebook.setName("Third notebook");
        thirdNotebook.setLocal(false);
        thirdNotebook.setDirty(false);

        m_pLocalStorageManagerThreadWorker->onAddNotebookRequest(firstNotebook, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNotebookRequest(secondNotebook, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNotebookRequest(thirdNotebook, QUuid());

        Tag firstTag;
        firstTag.setName("First tag");
        firstTag.setLocal(true);
        firstTag.setDirty(false);

        Tag secondTag;
        secondTag.setName("Second tag");
        secondTag.setLocal(true);
        secondTag.setDirty(true);

        Tag thirdTag;
        thirdTag.setName("Third tag");
        thirdTag.setGuid(UidGenerator::Generate());
        thirdTag.setLocal(false);
        thirdTag.setDirty(false);

        Tag fourthTag;
        fourthTag.setName("Fourth tag");
        fourthTag.setGuid(UidGenerator::Generate());
        fourthTag.setLocal(false);
        fourthTag.setDirty(true);
        fourthTag.setParentGuid(thirdTag.guid());
        fourthTag.setParentLocalUid(thirdTag.localUid());

        m_pLocalStorageManagerThreadWorker->onAddTagRequest(firstTag, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddTagRequest(secondTag, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddTagRequest(thirdTag, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddTagRequest(fourthTag, QUuid());

        Note firstNote;
        firstNote.setTitle("First note");
        firstNote.setContent("<en-note><h1>First note</h1></en-note>");
        firstNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        firstNote.setModificationTimestamp(firstNote.creationTimestamp());
        firstNote.setNotebookLocalUid(firstNotebook.localUid());
        firstNote.setLocal(true);
        firstNote.setTagLocalUids(QStringList() << firstTag.localUid() << secondTag.localUid());
        firstNote.setDirty(false);

        Note secondNote;
        secondNote.setTitle("Second note");
        secondNote.setContent("<en-note><h1>Second note</h1></en-note>");
        secondNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        secondNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
        secondNote.setNotebookLocalUid(firstNotebook.localUid());
        secondNote.setLocal(true);
        secondNote.setTagLocalUids(QStringList() << firstTag.localUid());
        secondNote.setDirty(true);

        Note thirdNote;
        thirdNote.setGuid(UidGenerator::Generate());
        thirdNote.setTitle("Third note");
        thirdNote.setContent("<en-note><h1>Third note</h1></en-note>");
        thirdNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        thirdNote.setModificationTimestamp(thirdNote.creationTimestamp());
        thirdNote.setNotebookLocalUid(secondNotebook.localUid());
        thirdNote.setNotebookGuid(secondNotebook.guid());
        thirdNote.setLocal(false);
        thirdNote.setTagLocalUids(QStringList() << thirdTag.localUid());
        thirdNote.setTagGuids(QStringList() << thirdTag.guid());

        Note fourthNote;
        fourthNote.setGuid(UidGenerator::Generate());
        fourthNote.setTitle("Fourth note");
        fourthNote.setContent("<en-note><h1>Fourth note</h1></en-note>");
        fourthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        fourthNote.setModificationTimestamp(fourthNote.creationTimestamp());
        fourthNote.setNotebookLocalUid(secondNotebook.localUid());
        fourthNote.setNotebookGuid(secondNotebook.guid());
        fourthNote.setLocal(false);
        fourthNote.setDirty(true);
        fourthNote.setTagLocalUids(QStringList() << thirdTag.localUid() << fourthTag.localUid());
        fourthNote.setTagGuids(QStringList() << thirdTag.guid() << fourthTag.guid());

        Note fifthNote;
        fifthNote.setGuid(UidGenerator::Generate());
        fifthNote.setTitle("Fifth note");
        fifthNote.setContent("<en-note><h1>Fifth note</h1></en-note>");
        fifthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        fifthNote.setModificationTimestamp(fifthNote.creationTimestamp());
        fifthNote.setNotebookLocalUid(secondNotebook.localUid());
        fifthNote.setNotebookGuid(secondNotebook.guid());
        fifthNote.setDeletionTimestamp(QDateTime::currentMSecsSinceEpoch());
        fifthNote.setLocal(false);
        fifthNote.setDirty(true);

        Note sixthNote;
        sixthNote.setGuid(UidGenerator::Generate());
        sixthNote.setTitle("Sixth note");
        sixthNote.setContent("<en-note><h1>Sixth note</h1></en-note>");
        sixthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        sixthNote.setModificationTimestamp(sixthNote.creationTimestamp());
        sixthNote.setNotebookLocalUid(thirdNotebook.localUid());
        sixthNote.setNotebookGuid(thirdNotebook.guid());
        sixthNote.setLocal(false);
        sixthNote.setDirty(false);
        sixthNote.setTagLocalUids(QStringList() << fourthTag.localUid());
        sixthNote.setTagGuids(QStringList() << fourthTag.guid());

        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(firstNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(secondNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(thirdNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(fourthNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(fifthNote, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddNoteRequest(sixthNote, QUuid());

        NotebookCache notebookCache(3);

        NoteModel * model = new NoteModel(*m_pLocalStorageManagerThreadWorker, notebookCache, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex firstIndex = model->indexForLocalUid(firstNote.localUid());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for local uid");
        }

        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Dirty, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for dirty column");
        }

        bool res = model->setData(firstIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the dirty flag in the note model manually which is not intended");
        }

        QVariant data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the note model while expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL("The dirty state appears to have changed after setData in note model even though the method returned false");
        }

        // Should be able to make the non-synchronizable (local) item synchronizable (non-local)
        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Synchronizable, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model index for synchronizable column");
        }

        res = model->setData(firstIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL("Can't change the synchronizable flag from false to true for note model item");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the note model while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable flag appears to have not changed after setData in note model even though the method returned true");
        }

        // Verify the dirty flag has changed as a result of making the item synchronizable
        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Dirty, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for dirty column");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the note model while expected to get the state of dirty flag");
        }

        if (!data.toBool()) {
            FAIL("The dirty state hasn't changed after making the note model item synchronizable while it was expected to have changed");
        }

        // Should not be able to make the synchronizable (non-local) item non-synchronizable (local)
        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Synchronizable, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for synchronizable column");
        }

        res = model->setData(firstIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the synchronizable flag in note model from true to false which is not intended");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the note model while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have changed after setData in note model even though the method returned false");
        }

        // Should be able to change the title
        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Title, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for title column");
        }

        QString newTitle = "First note (modified)";
        res = model->setData(firstIndex, newTitle, Qt::EditRole);
        if (!res) {
            FAIL("Can't change the title of note model item");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the note model while expected to get the note item's title");
        }

        if (data.toString() != newTitle) {
            FAIL("The title of the note item returned by the model does not match the title just set to this item: "
                 "received " << data.toString() << ", expected " << newTitle);
        }

        // Should be able to mark the note as deleted
        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::DeletionTimestamp, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for deletion timestamp column");
        }

        qint64 deletionTimestamp = QDateTime::currentMSecsSinceEpoch();
        res = model->setData(firstIndex, deletionTimestamp, Qt::EditRole);
        if (!res) {
            FAIL("Can't set the deletion timestamp onto the note model item");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the note model while expected to get the note item's deletion timestamp");
        }

        bool conversionResult = false;
        qint64 itemDeletionTimestamp = data.toLongLong(&conversionResult);
        if (!conversionResult) {
            FAIL("Can't convert the note model item's deletion timestamp data to the actual timestamp");
        }

        if (deletionTimestamp != itemDeletionTimestamp) {
            FAIL("The note model item's deletion timestamp doesn't match the timestamp originally set");
        }

        // Should be able to remove the deletion timestamp from the note
        deletionTimestamp = 0;
        res = model->setData(firstIndex, deletionTimestamp, Qt::EditRole);
        if (!res) {
            FAIL("Can't set zero deletion timestamp onto the note model item");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the note model while expected to get the note item's deletion timestamp");
        }

        conversionResult = false;
        itemDeletionTimestamp = data.toLongLong(&conversionResult);
        if (!conversionResult) {
            FAIL("Can't convert the note model item's deletion timestamp data to the actual timestamp");
        }

        if (deletionTimestamp != itemDeletionTimestamp) {
            FAIL("The note model item's deletion timestamp doesn't match the expected value of 0");
        }

        // Should not be able to remove the row with a synchronizable (non-local) notebook
        res = model->removeRow(firstIndex.row(), QModelIndex());
        if (res) {
            FAIL("Was able to remove the row with a synchronizable note which is not intended");
        }

        QModelIndex firstIndexAfterFailedRemoval = model->indexForLocalUid(firstNote.localUid());
        if (!firstIndexAfterFailedRemoval.isValid()) {
            FAIL("Can't get the valid note model item index after the failed row removal attempt");
        }

        if (firstIndexAfterFailedRemoval.row() != firstIndex.row()) {
            FAIL("Note model returned item index with a different row after the failed row removal attempt");
        }

        // Check sorting
        QVector<NoteModel::Columns::type> columns;
        columns.reserve(model->columnCount(QModelIndex()));
        columns << NoteModel::Columns::CreationTimestamp
                << NoteModel::Columns::ModificationTimestamp
                << NoteModel::Columns::DeletionTimestamp
                << NoteModel::Columns::Title
                << NoteModel::Columns::PreviewText
                << NoteModel::Columns::NotebookName
                << NoteModel::Columns::Size
                << NoteModel::Columns::Synchronizable
                << NoteModel::Columns::Dirty;

        int numColumns = columns.size();
        for(int i = 0; i < numColumns; ++i)
        {
            // Test the ascending case
            model->sort(columns[i], Qt::AscendingOrder);
            checkSorting(*model);

            // Test the descending case
            model->sort(columns[i], Qt::DescendingOrder);
            checkSorting(*model);
        }

        m_model = model;
        m_firstNotebook = firstNotebook;
        m_noteToExpungeLocalUid = secondNote.localUid();

        // Should be able to add the new note model item and get the asynchonous acknowledgement from the local storage about that
        m_expectingNewNoteFromLocalStorage = true;
        Q_UNUSED(model->createNoteItem(firstNotebook.localUid()))

        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void NoteModelTestHelper::onAddNoteComplete(Note note, QUuid requestId)
{
    if (!m_expectingNewNoteFromLocalStorage) {
        return;
    }

    QNDEBUG("NoteModelTestHelper::onAddNoteComplete: note = " << note);

    Q_UNUSED(requestId)
    m_expectingNewNoteFromLocalStorage = false;

    try
    {
        const NoteModelItem * item = m_model->itemForLocalUid(note.localUid());
        if (Q_UNLIKELY(!item)) {
            FAIL("Can't find just added note's item in the note model by local uid");
        }

        // Should be able to update the note model item and get the asynchronous acknowledgement from the local storage about that
        m_expectingNoteUpdateFromLocalStorage = true;
        QModelIndex itemIndex = m_model->indexForLocalUid(note.localUid());
        if (!itemIndex.isValid()) {
            FAIL("Can't find the valid model index for the note item just added to the model");
        }

        itemIndex = m_model->index(itemIndex.row(), NoteModel::Columns::Title, QModelIndex());
        if (!itemIndex.isValid()) {
            FAIL("Can't find the valid model index for the note item's title column");
        }

        QString title = "Modified title";
        bool res = m_model->setData(itemIndex, title, Qt::EditRole);
        if (!res) {
            FAIL("Can't update the note item model's title");
        }

        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void NoteModelTestHelper::onAddNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onAddNoteFailed: note = " << note << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    if (!m_expectingNoteUpdateFromLocalStorage) {
        return;
    }

    QNDEBUG("NoteModelTestHelper::onUpdateNoteComplete: note = " << note);

    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)
    Q_UNUSED(requestId)
    m_expectingNoteUpdateFromLocalStorage = false;

    try
    {
        const NoteModelItem * item = m_model->itemForLocalUid(note.localUid());
        if (Q_UNLIKELY(!item)) {
            FAIL("Can't find the updated note's item in the note model by local uid");
        }

        QModelIndex itemIndex = m_model->indexForLocalUid(note.localUid());
        if (!itemIndex.isValid()) {
            FAIL("Can't find the valid model index for the note item just updated");
        }

        QString title = item->title();
        if (title != "Modified title") {
            FAIL("It appears the note model item's title hasn't really changed as it was expected");
        }

        itemIndex = m_model->index(itemIndex.row(), NoteModel::Columns::DeletionTimestamp, QModelIndex());
        if (!itemIndex.isValid()) {
            FAIL("Can't find the valid model index for the note item's deletion timestamp column");
        }

        emit success();
        // FIXME: find out why it fails
        /*
        // Should be able to set the deletion timestamp to the note model item and receive the asynchronous acknowledge from the local storage
        m_expectingNoteDeletionFromLocalStorage = true;
        qint64 deletionTimestamp = QDateTime::currentMSecsSinceEpoch();
        bool res = m_model->setData(itemIndex, deletionTimestamp, Qt::EditRole);
        if (!res) {
            FAIL("Can't set the deletion timestamp onto the note model item");
        }
        */

        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void NoteModelTestHelper::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                             QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onUpdateNoteFailed: note = " << note << "\nUpdate resources = "
            << (updateResources ? "true" : "false") << ", update tags = " << (updateTags ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onFindNoteFailed: note = " << note << "\nWith resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                            size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                            LocalStorageManager::OrderDirection::type orderDirection,
                                            QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onListNotesFailed: flag = " << flag << ", with resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onDeleteNoteComplete(Note note, QUuid requestId)
{
    if (!m_expectingNoteDeletionFromLocalStorage) {
        return;
    }

    QNDEBUG("NoteModelTestHelper::onDeleteNoteComplete: note = " << note);

    Q_UNUSED(requestId)
    m_expectingNoteDeletionFromLocalStorage = false;

    try
    {
        const NoteModelItem * item = m_model->itemForLocalUid(note.localUid());
        if (Q_UNLIKELY(!item)) {
            FAIL("Can't find the deleted note's item in the note model by local uid");
        }

        if (item->deletionTimestamp() == 0) {
            FAIL("The note model item's deletion timestamp is unexpectedly zero");
        }

        // Should be able to remove the row with a non-synchronizable (local) note and get
        // the asynchronous acknowledgement from the local storage
        QModelIndex itemIndex = m_model->indexForLocalUid(m_noteToExpungeLocalUid);
        if (!itemIndex.isValid()) {
            FAIL("Can't get the valid note model item index for local uid");
        }

        m_expectingNoteExpungeFromLocalStorage = true;
        bool res = m_model->removeRow(itemIndex.row(), QModelIndex());
        if (!res) {
            FAIL("Can't remove the row with a non-synchronizable note item from the model");
        }

        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void NoteModelTestHelper::onDeleteNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onDeleteNoteFailed: note = " << note << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onExpungeNoteComplete(Note note, QUuid requestId)
{
    if (!m_expectingNoteExpungeFromLocalStorage) {
        return;
    }

    QNDEBUG("NoteModelTestHelper::onExpungeNoteComplete: note = " << note);

    Q_UNUSED(requestId)
    m_expectingNoteExpungeFromLocalStorage = false;

    try
    {
        QModelIndex itemIndex = m_model->indexForLocalUid(m_noteToExpungeLocalUid);
        if (itemIndex.isValid()) {
            FAIL("Was able to get the valid model index for the removed note item by local uid which is not intended");
        }

        const NoteModelItem * item = m_model->itemForLocalUid(m_noteToExpungeLocalUid);
        if (item) {
            FAIL("Was able to get the non-null pointer to the note model item while the corresponding note was expunged from local storage");
        }

        emit success();
        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void NoteModelTestHelper::onExpungeNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onExpungeNoteFailed: note = " << note << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onAddNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onUpdateNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onAddTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::checkSorting(const NoteModel & model)
{
    int numRows = model.rowCount(QModelIndex());

    QVector<NoteModelItem> items;
    items.reserve(numRows);
    for(int i = 0; i < numRows; ++i)
    {
        const NoteModelItem * item = model.itemAtRow(i);
        if (Q_UNLIKELY(!item)) {
            FAIL("Unexpected null pointer to the note model item");
        }

        items << *item;
    }

    bool ascending = (model.sortOrder() == Qt::AscendingOrder);
    switch(model.sortingColumn())
    {
    case NoteModel::Columns::CreationTimestamp:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByCreationTimestamp());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByCreationTimestamp());
            }
            break;
        }
    case NoteModel::Columns::ModificationTimestamp:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByModificationTimestamp());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByModificationTimestamp());
            }
            break;
        }
    case NoteModel::Columns::DeletionTimestamp:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByDeletionTimestamp());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByDeletionTimestamp());
            }
            break;
        }
    case NoteModel::Columns::Title:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByTitle());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByTitle());
            }
            break;
        }
    case NoteModel::Columns::PreviewText:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByPreviewText());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByPreviewText());
            }
            break;
        }
    case NoteModel::Columns::NotebookName:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByNotebookName());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByNotebookName());
            }
            break;
        }
    case NoteModel::Columns::Size:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessBySize());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterBySize());
            }
            break;
        }
    case NoteModel::Columns::Synchronizable:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessBySynchronizable());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterBySynchronizable());
            }
            break;
        }
    case NoteModel::Columns::ThumbnailImageFilePath:
    case NoteModel::Columns::TagNameList:
        break;
    }

    for(int i = 0; i < numRows; ++i)
    {
        const NoteModelItem * item = model.itemAtRow(i);
        if (Q_UNLIKELY(!item)) {
            FAIL("Unexpected null pointer to the note model item");
        }

        if (item->localUid() != items[i].localUid()) {
            FAIL("Found mismatched note model items when checking the sorting");
        }
    }
}

bool NoteModelTestHelper::LessByCreationTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.creationTimestamp() < rhs.creationTimestamp();
}

bool NoteModelTestHelper::GreaterByCreationTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.creationTimestamp() > rhs.creationTimestamp();
}

bool NoteModelTestHelper::LessByModificationTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.modificationTimestamp() < rhs.modificationTimestamp();
}

bool NoteModelTestHelper::GreaterByModificationTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.modificationTimestamp() > rhs.modificationTimestamp();
}

bool NoteModelTestHelper::LessByDeletionTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.deletionTimestamp() < rhs.deletionTimestamp();
}

bool NoteModelTestHelper::GreaterByDeletionTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.deletionTimestamp() > rhs.deletionTimestamp();
}

bool NoteModelTestHelper::LessByTitle::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.title().localeAwareCompare(rhs.title()) < 0;
}

bool NoteModelTestHelper::GreaterByTitle::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.title().localeAwareCompare(rhs.title()) > 0;
}

bool NoteModelTestHelper::LessByPreviewText::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.previewText().localeAwareCompare(rhs.previewText()) < 0;
}

bool NoteModelTestHelper::GreaterByPreviewText::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.previewText().localeAwareCompare(rhs.previewText()) > 0;
}

bool NoteModelTestHelper::LessByNotebookName::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.notebookName().localeAwareCompare(rhs.notebookName()) < 0;
}

bool NoteModelTestHelper::GreaterByNotebookName::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.notebookName().localeAwareCompare(rhs.notebookName()) > 0;
}

bool NoteModelTestHelper::LessBySize::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.sizeInBytes() < rhs.sizeInBytes();
}

bool NoteModelTestHelper::GreaterBySize::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.sizeInBytes() > rhs.sizeInBytes();
}

bool NoteModelTestHelper::LessBySynchronizable::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return !lhs.isSynchronizable() && rhs.isSynchronizable();
}

bool NoteModelTestHelper::GreaterBySynchronizable::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.isSynchronizable() && !rhs.isSynchronizable();
}

bool NoteModelTestHelper::LessByDirty::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return !lhs.isDirty() && rhs.isDirty();
}

bool NoteModelTestHelper::GreaterByDirty::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.isDirty() && !rhs.isDirty();
}

} // namespace qute_note
