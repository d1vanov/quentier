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
    m_pLocalStorageManagerThreadWorker(pLocalStorageManagerThreadWorker)
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
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onUpdateNotebookComplete,Notebook,QUuid));
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

        // Should be able to remove the row with a non-synchronizable (local) note
        QModelIndex secondIndex = model->indexForLocalUid(secondNote.localUid());
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid note model item index for local uid");
        }

        res = model->removeRow(secondIndex.row(), QModelIndex());
        if (!res) {
            FAIL("Can't remove the row with a non-synchronizable note item from the model");
        }

        QModelIndex secondIndexAfterRemoval = model->indexForLocalUid(secondNote.localUid());
        if (secondIndexAfterRemoval.isValid()) {
            FAIL("Was able to get the valid model index for the removed note item by local uid which is not intended");
        }

        emit success();
        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void NoteModelTestHelper::onAddNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void NoteModelTestHelper::onAddNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onAddNoteFailed: note = " << note << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)
    Q_UNUSED(requestId)
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
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void NoteModelTestHelper::onDeleteNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NoteModelTestHelper::onDeleteNoteFailed: note = " << note << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NoteModelTestHelper::onExpungeNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
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

void NoteModelTestHelper::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
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

void NoteModelTestHelper::testAfterNewNoteAddition()
{
    QNDEBUG("NoteModelTestHelper::testAfterNewNoteAddition");

    // TODO: implement
}

void NoteModelTestHelper::testAfterNoteUpdate()
{
    QNDEBUG("NoteModelTestHelper::testAfterNoteUpdate");

    // TODO: imlement
}

} // namespace qute_note
