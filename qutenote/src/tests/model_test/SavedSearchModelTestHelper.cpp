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

        SavedSearch second;
        second.setName("Second search");
        second.setQuery("Second search query");

        SavedSearch third;
        third.setName("Third search");
        third.setQuery("Third search query");

        SavedSearch fourth;
        fourth.setName("Fourth search");
        fourth.setQuery("Fourth search query");

        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(first, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(second, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(third, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(fourth, QUuid());

        SavedSearchModel * model = new SavedSearchModel(*m_pLocalStorageManagerThreadWorker, this);
        ModelTest t1(model);
        Q_UNUSED(t1)
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
