#include "TagModelTestHelper.h"
#include "../../models/TagModel.h"
#include "modeltest.h"
#include <qute_note/utility/SysInfo.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

TagModelTestHelper::TagModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                       QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerThreadWorker(pLocalStorageManagerThreadWorker)
{
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModelTestHelper,onAddTagFailed,Tag,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModelTestHelper,onUpdateTagFailed,Tag,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModelTestHelper,onFindTagFailed,Tag,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listTagsFailed,
                                                                LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListTagsOrder::type,
                                                                LocalStorageManager::OrderDirection::type,
                                                                QString,QString,QUuid),
                     this, QNSLOT(TagModelTestHelper,onListTagsFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListTagsOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModelTestHelper,onExpungeTagFailed,Tag,QString,QUuid));
}

void TagModelTestHelper::test()
{
    // TODO: implement
}

void TagModelTestHelper::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onAddTagFailed: tag = " << tag << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void TagModelTestHelper::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onUpdateTagFailed: tag = " << tag << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void TagModelTestHelper::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onFindTagFailed: tag = " << tag << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

void TagModelTestHelper::onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                          size_t limit, size_t offset,
                                          LocalStorageManager::ListTagsOrder::type order,
                                          LocalStorageManager::OrderDirection::type orderDirection,
                                          QString linkedNotebookGuid,
                                          QString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onListTagsFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid: is null = " << (linkedNotebookGuid.isNull() ? "true" : "false")
            << ", is empty = " << (linkedNotebookGuid.isEmpty() ? "true" : "false") << ", value = "
            << linkedNotebookGuid << ", error description = " << errorDescription << ", request id = "
            << requestId);

    emit failure();
}

void TagModelTestHelper::onExpungeTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onExpungeTagFailed: tag = " << tag << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    emit failure();
}

} // namespace qute_note
