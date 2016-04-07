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
        Notebook first;
        first.setName("First");
        first.setLocal(true);
        first.setDirty(true);

        Notebook second;
        second.setName("Second");
        second.setLocal(true);
        second.setDirty(false);
        second.setStack("Stack 1");

        Notebook third;
        third.setName("Third");
        third.setLocal(false);
        third.setGuid(UidGenerator::Generate());
        third.setDirty(true);
        third.setStack("Stack 1");

        Notebook fourth;
        fourth.setName("Fourth");
        fourth.setLocal(false);
        fourth.setGuid(UidGenerator::Generate());
        fourth.setDirty(false);
        fourth.setPublished(true);
        fourth.setStack("Stack 1");

        Notebook fifth;
        fifth.setName("Fifth");
        fifth.setLocal(false);
        fifth.setGuid(UidGenerator::Generate());
        fifth.setLinkedNotebookGuid(fifth.guid());
        fifth.setDirty(false);
        fifth.setStack("Stack 1");

        Notebook sixth;
        sixth.setName("Sixth");
        sixth.setLocal(false);
        sixth.setGuid(UidGenerator::Generate());
        sixth.setDirty(false);

        Notebook seventh;
        seventh.setName("Seventh");
        seventh.setLocal(false);
        seventh.setGuid(UidGenerator::Generate());
        seventh.setDirty(false);
        seventh.setPublished(true);

        Notebook eighth;
        eighth.setName("Eighth");
        eighth.setLocal(false);
        eighth.setGuid(UidGenerator::Generate());
        eighth.setLinkedNotebookGuid(eighth.guid());
        eighth.setDirty(false);

        Notebook nineth;
        nineth.setName("Nineth");
        nineth.setLocal(true);
        nineth.setDirty(false);
        nineth.setStack("Stack 2");

        Notebook tenth;
        tenth.setName("Tenth");
        tenth.setLocal(false);
        tenth.setGuid(UidGenerator::Generate());
        tenth.setDirty(false);
        tenth.setStack("Stack 2");

#define ADD_NOTEBOOK(notebook) \
        m_pLocalStorageManagerThreadWorker->onAddNotebookRequest(notebook, QUuid())

        ADD_NOTEBOOK(first);
        ADD_NOTEBOOK(second);
        ADD_NOTEBOOK(third);
        ADD_NOTEBOOK(fourth);
        ADD_NOTEBOOK(fifth);
        ADD_NOTEBOOK(sixth);
        ADD_NOTEBOOK(seventh);
        ADD_NOTEBOOK(eighth);
        ADD_NOTEBOOK(nineth);
        ADD_NOTEBOOK(tenth);

#undef ADD_NOTEBOOK

        NotebookModel * model = new NotebookModel(*m_pLocalStorageManagerThreadWorker, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid notebook model item index for local uid");
            emit failure();
            return;
        }

        QModelIndex secondParentIndex = model->parent(secondIndex);
        secondIndex = model->index(secondIndex.row(), NotebookModel::Columns::Dirty, secondParentIndex);
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid notebook model item index for dirty column");
            emit failure();
            return;
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            QNWARNING("Was able to change the dirty flag in the notebook model manually which is not intended");
            emit failure();
            return;
        }

        QVariant data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the notebook model while expected to get the state of dirty flag");
            emit failure();
            return;
        }

        if (data.toBool()) {
            QNWARNING("The dirty state appears to have changed after setData in notebook model even though the method returned false");
            emit failure();
            return;
        }

        // TODO: add manual tests here

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
