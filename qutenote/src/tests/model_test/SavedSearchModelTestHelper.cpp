#include "SavedSearchModelTestHelper.h"
#include "../../models/SavedSearchModel.h"
#include "modeltest.h"
#include <qute_note/utility/SysInfo.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

SavedSearchModelTestHelper::SavedSearchModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                                       QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerThreadWorker(pLocalStorageManagerThreadWorker)
{
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onAddSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onUpdateSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onFindSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,
                                                                LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onListSavedSearchesFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                                  QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onExpungeSavedSearchFailed,SavedSearch,QString,QUuid));
}

void SavedSearchModelTestHelper::test()
{
    QNDEBUG("SavedSearchModelTestHelper::test");

    try {
        SavedSearch first;
        first.setName("First search");
        first.setQuery("First search query");
        first.setLocal(true);
        first.setDirty(true);

        SavedSearch second;
        second.setName("Second search");
        second.setQuery("Second search query");
        second.setLocal(true);
        second.setDirty(false);

        SavedSearch third;
        third.setName("Third search");
        third.setQuery("Third search query");
        third.setLocal(false);
        third.setDirty(true);

        SavedSearch fourth;
        fourth.setName("Fourth search");
        fourth.setQuery("Fourth search query");
        fourth.setLocal(false);
        fourth.setDirty(false);

        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(first, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(second, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(third, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(fourth, QUuid());

        SavedSearchModel * model = new SavedSearchModel(*m_pLocalStorageManagerThreadWorker, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid saved search item model index for local uid");
            emit failure();
            return;
        }

        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Dirty, QModelIndex());
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid saved search item model index for dirty column");
            emit failure();
            return;
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            QNWARNING("Was able to change the dirty flag in saved search model manually which is not intended");
            emit failure();
            return;
        }

        QVariant data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the saved search model while expected to get the state of dirty flag");
            emit failure();
            return;
        }

        if (data.toBool()) {
            QNWARNING("The dirty state appears to have changed after setData in saved search model even though the method returned false");
            emit failure();
            return;
        }

        // Should be able to make the non-synchronizable (local) item synchronizable (non-local)
        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Synchronizable, QModelIndex());
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid saved search item model index for synchronizable column");
            emit failure();
            return;
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            QNWARNING("Can't change the synchronizable flag from false to true for saved search model");
            emit failure();
            return;
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the saved search model while expected to get the state of synchronizable flag");
            emit failure();
            return;
        }

        if (!data.toBool()) {
            QNWARNING("The synchronizable state appears to have not changed after setData in saved search model even though the method returned true");
            emit failure();
            return;
        }

        // Should not be able to make the synchronizable (non-local) item non-synchronizable (local)
        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            QNWARNING("Was able to change the synchronizable flag in saved search model from true to false which is not intended");
            emit failure();
            return;
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the saved search model while expected to get the state of synchronizable flag");
            emit failure();
            return;
        }

        if (!data.toBool()) {
            QNWARNING("The synchronizable state appears to have changed after setData in saved search model even though the method returned false");
            emit failure();
            return;
        }

        // Should be able to change name and query columns
        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Name);
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid saved search item model index for name column");
            emit failure();
            return;
        }

        QString newName = "second (name modified)";
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            QNWARNING("Can't change the name of the saved search model item");
            emit failure();
            return;
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the saved search model while expected to get the name of the saved search item");
            emit failure();
            return;
        }

        if (data.toString() != newName) {
            QNWARNING("The name of the saved search item returned by the model does not match the name just set to this item: "
                      "received " << data.toString() << ", expected " << newName);
            emit failure();
            return;
        }

        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Query, QModelIndex());
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid saved search item model index for query column");
            emit failure();
            return;
        }

        QString newQuery = "second query (modified)";
        res = model->setData(secondIndex, QVariant(newQuery), Qt::EditRole);
        if (!res) {
            QNWARNING("Can't change the query of the saved search model item");
            emit failure();
            return;
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the saved search model while expected to get the query of the saved search item");
            emit failure();
            return;
        }

        if (data.toString() != newQuery) {
            QNWARNING("The query of the saved search item returned by the model does not match the query just set to this item: "
                      "received " << data.toString() << ", expected " << newQuery);
            emit failure();
            return;
        }

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

void SavedSearchModelTestHelper::onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNDEBUG("SavedSearchModelTestHelper::onAddSavedSearchFailed: search = " << search << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void SavedSearchModelTestHelper::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNDEBUG("SavedSearchModelTestHelper::onUpdateSavedSearchFailed: search = " << search << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void SavedSearchModelTestHelper::onFindSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNDEBUG("SavedSearchModelTestHelper::onFindSavedSearchFailed: search = " << search << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void SavedSearchModelTestHelper::onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                           size_t limit, size_t offset,
                                                           LocalStorageManager::ListSavedSearchesOrder::type order,
                                                           LocalStorageManager::OrderDirection::type orderDirection,
                                                           QString errorDescription, QUuid requestId)
{
    QNDEBUG("SavedSearchModelTestHelper::onListSavedSearchesFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", error description = " << errorDescription << ", request id = " << requestId);

    emit failure();
}

void SavedSearchModelTestHelper::onExpungeSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNDEBUG("SavedSearchModelTestHelper::onExpungeSavedSearchFailed: search = " << search << ", error description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

} // namespace qute_note
