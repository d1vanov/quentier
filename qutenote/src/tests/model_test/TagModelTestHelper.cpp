#include "TagModelTestHelper.h"
#include "../../models/TagModel.h"
#include "modeltest.h"
#include <qute_note/utility/SysInfo.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/UidGenerator.h>

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
    QNDEBUG("TagModelTestHelper::test");

    try {
        Tag first;
        first.setName("First");
        first.setLocal(true);
        first.setDirty(true);
        first.setGuid(UidGenerator::Generate());

        Tag second;
        second.setName("Second");
        second.setLocal(true);
        second.setDirty(false);
        second.setGuid(UidGenerator::Generate());

        Tag third;
        third.setName("Third");
        third.setLocal(false);
        third.setDirty(true);
        third.setGuid(UidGenerator::Generate());

        Tag fourth;
        fourth.setName("Fourth");
        fourth.setLocal(false);
        fourth.setDirty(false);
        fourth.setGuid(UidGenerator::Generate());

        Tag fifth;
        fifth.setName("Fifth");
        fifth.setLocal(false);
        fifth.setDirty(false);
        fifth.setGuid(UidGenerator::Generate());

        Tag sixth;
        sixth.setName("Sixth");
        sixth.setLocal(false);
        sixth.setDirty(false);
        sixth.setGuid(UidGenerator::Generate());
        sixth.setParentLocalUid(fifth.localUid());
        sixth.setParentGuid(fifth.guid());

        Tag seventh;
        seventh.setName("Seventh");
        seventh.setLocal(false);
        seventh.setDirty(false);
        seventh.setGuid(UidGenerator::Generate());
        seventh.setParentLocalUid(fifth.localUid());
        seventh.setParentGuid(fifth.guid());

        Tag eighth;
        eighth.setName("Eighth");
        eighth.setLocal(false);
        eighth.setDirty(true);
        eighth.setGuid(UidGenerator::Generate());
        eighth.setParentLocalUid(fifth.localUid());
        eighth.setParentGuid(fifth.guid());

        Tag nineth;
        nineth.setName("Nineth");
        nineth.setLocal(false);
        nineth.setDirty(false);
        nineth.setGuid(UidGenerator::Generate());
        nineth.setParentLocalUid(sixth.localUid());
        nineth.setParentGuid(sixth.guid());

        Tag tenth;
        tenth.setName("Tenth");
        tenth.setLocal(false);
        tenth.setDirty(true);
        tenth.setParentLocalUid(eighth.localUid());
        tenth.setParentGuid(eighth.guid());

        Tag eleventh;
        eleventh.setName("Eleventh");
        eleventh.setLocal(false);
        eleventh.setDirty(true);
        eleventh.setParentLocalUid(tenth.localUid());

        Tag twelveth;
        twelveth.setName("Twelveth");
        twelveth.setLocal(false);
        twelveth.setDirty(true);
        twelveth.setParentLocalUid(twelveth.localUid());

#define ADD_TAG(tag) \
        m_pLocalStorageManagerThreadWorker->onAddTagRequest(tag, QUuid())

        ADD_TAG(first);
        ADD_TAG(second);
        ADD_TAG(third);
        ADD_TAG(fourth);
        ADD_TAG(fifth);
        ADD_TAG(sixth);
        ADD_TAG(seventh);
        ADD_TAG(eighth);
        ADD_TAG(nineth);
        ADD_TAG(tenth);
        ADD_TAG(eleventh);
        ADD_TAG(twelveth);

#undef ADD_TAG

        TagModel * model = new TagModel(*m_pLocalStorageManagerThreadWorker, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid tag item model index for local uid");
            emit failure();
            return;
        }

        QModelIndex secondParentIndex = model->parent(secondIndex);
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Dirty, secondParentIndex);
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid tag item model index for dirty column");
            emit failure();
            return;
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            QNWARNING("Was able to change the dirty flag in tag model manually which is not intended");
            emit failure();
            return;
        }

        QVariant data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the tag model while expected to get the state of dirty flag");
            emit failure();
            return;
        }

        if (data.toBool()) {
            QNWARNING("The dirty state appears to have changed after setData in tag model even though the method returned false");
            emit failure();
            return;
        }

        // TODO: continue from here

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
