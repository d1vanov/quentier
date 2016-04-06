#include "NotebookModelTestHelper.h"
#include "../../models/NotebookModel.h"
#include "modeltest.h"
#include <qute_note/utility/SysInfo.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/UidGenerator.h>

namespace qute_note {

NotebookModelTestHelper::NotebookModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                                 QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerThreadWorker(pLocalStorageManagerThreadWorker)
{
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onAddNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onUpdateNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onFindNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksFailed,
                                                                LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,
                                                                QString,QString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onListNotebooksFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onExpungeNotebookFailed,Notebook,QString,QUuid));
}

void NotebookModelTestHelper::test()
{
    QNDEBUG("NotebookModelTestHelper::test");

    try {
        // TODO: implement
        emit success();
        return;
    }
    catch(const IQuteNoteException & exception) {
        SysInfo & sysInfo = SysInfo::GetSingleton();
        QNWARNING("Caught QuteNote exception: " + exception.errorMessage() +
                  ", what: " + QString(exception.what()) + "; stack trace: " + sysInfo.GetStackTrace());
    }
    catch(const std::exception & exception) {
        SysInfo & sysInfo = SysInfo::GetSingleton();
        QNWARNING("Caught std::exception: " + QString(exception.what()) + "; stack trace: " + sysInfo.GetStackTrace());
    }
    catch(...) {
        SysInfo & sysInfo = SysInfo::GetSingleton();
        QNWARNING("Caught some unknown exception; stack trace: " + sysInfo.GetStackTrace());
    }

    emit failure();
}

void NotebookModelTestHelper::onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onAddNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NotebookModelTestHelper::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onUpdateNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NotebookModelTestHelper::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onFindNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NotebookModelTestHelper::onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
                                                    LocalStorageManager::ListNotebooksOrder::type order,
                                                    LocalStorageManager::OrderDirection::type orderDirection,
                                                    QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onListNotebooksFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = " << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid)
            << ", error description = " << errorDescription << ", request id = " << requestId);

    emit failure();
}

void NotebookModelTestHelper::onExpungeNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onExpungeNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

} // namespace qute_note
