#include "NoteModelTestHelper.h"
#include <qute_note/types/Note.h>
#include <qute_note/logging/QuteNoteLogger.h>

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
}

void NoteModelTestHelper::launchTest()
{
    // TODO: implement
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
