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
        twelveth.setParentLocalUid(tenth.localUid());

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

        // Should be able to make the non-synchronizable (local) item synchronizable (non-local)
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Synchronizable, secondParentIndex);
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid tag item model index for synchronizable column");
            emit failure();
            return;
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            QNWARNING("Can't change the synchronizable flag from false to true for tag model");
            emit failure();
            return;
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the tag model while expected to get the state of synchronizable flag");
            emit failure();
            return;
        }

        if (!data.toBool()) {
            QNWARNING("The synchronizable state appears to have not changed after setData in tag model even though the method returned true");
            emit failure();
            return;
        }

        // Verify the dirty flag has changed as a result of makind the item synchronizable
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Dirty, secondParentIndex);
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid tag item model index for dirty column");
            emit failure();
            return;
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the tag model while expected to get the state of dirty flag");
            emit failure();
            return;
        }

        if (!data.toBool()) {
            QNWARNING("The dirty state hasn't changed after making the tag model item synchronizable while it was expected to have changed");
            emit failure();
            return;
        }

        // Should not be able to make the synchronizable (non-local) item non-synchronizable (local)
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Synchronizable, secondParentIndex);
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid tag item model index for synchronizable column");
            emit failure();
            return;
        }

        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            QNWARNING("Was able to change the synchronizable flag in tag model from true to false which is not intended");
            emit failure();
            return;
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the tag model while expected to get the state of synchronizable flag");
            emit failure();
            return;
        }

        if (!data.toBool()) {
            QNWARNING("The synchronizable state appears to have changed after setData in tag model even though the method returned false");
            emit failure();
            return;
        }

        // Should be able to change name
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Name, secondParentIndex);
        if (!secondIndex.isValid()) {
            QNWARNING("Can't get the valid tag item model index for name column");
            emit failure();
            return;
        }

        QString newName = "Second (name modified)";
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            QNWARNING("Can't change the name of the tag model item");
            emit failure();
            return;
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            QNWARNING("Null data was returned by the tag model while expected to get the name of the tag item");
            emit failure();
            return;
        }

        if (data.toString() != newName) {
            QNWARNING("The name of the tag item returned by the model does not match the name just set to this item: "
                      "received " << data.toString() << ", expected " << newName);
            emit failure();
            return;
        }

        // Should not be able to remove the row with a synchronizable (non-local) saved search
        res = model->removeRow(secondIndex.row(), secondParentIndex);
        if (res) {
            QNWARNING("Was able to remove the row with a synchronizable tag which is not intended");
            emit failure();
            return;
        }

        QModelIndex secondIndexAfterFailedRemoval = model->indexForLocalUid(second.localUid());
        if (!secondIndexAfterFailedRemoval.isValid()) {
            QNWARNING("Can't get the valid tag item model index after the failed row removal attempt");
            emit failure();
            return;
        }

        if (secondIndexAfterFailedRemoval.row() != secondIndex.row()) {
            QNWARNING("Tag model returned item index with a different row after the failed row removal attempt");
            emit failure();
            return;
        }

        // Should be able to remove the row with a non-synchronizable (local) saved search
        QModelIndex firstIndex = model->indexForLocalUid(first.localUid());
        if (!firstIndex.isValid()) {
            QNWARNING("Can't get the valid tag item model index for local uid");
            emit failure();
            return;
        }

        QModelIndex firstParentIndex = model->parent(firstIndex);
        res = model->removeRow(firstIndex.row(), firstParentIndex);
        if (!res) {
            QNWARNING("Can't remove the row with a non-synchronizable tag item from the model");
            emit failure();
            return;
        }

        QModelIndex firstIndexAfterRemoval = model->indexForLocalUid(first.localUid());
        if (firstIndexAfterRemoval.isValid()) {
            QNWARNING("Was able to get the valid model index for the removed tag item by local uid which is not intended");
            emit failure();
            return;
        }

        // Should be able to promote the items
        QModelIndex twelvethIndex = model->indexForLocalUid(twelveth.localUid());
        const TagModelItem * twelvethItem = model->itemForIndex(twelvethIndex);
        const TagModelItem * tenthItem = twelvethItem->parent();
        if (!tenthItem) {
            QNWARNING("Parent of one of tag model items is null");
            emit failure();
            return;
        }

        twelvethIndex = model->promote(twelvethIndex);
        const TagModelItem * newTwelvethItem = model->itemForIndex(twelvethIndex);
        if (twelvethItem != newTwelvethItem) {
            QNWARNING("The tag model returns different pointers to items before and after the item promotion");
            emit failure();
            return;
        }

        int rowInTenth = tenthItem->rowForChild(twelvethItem);
        if (rowInTenth >= 0) {
            QNWARNING("Tag model item can still be found within the original parent's children after the promotion");
            emit failure();
            return;
        }

        QModelIndex eighthIndex = model->indexForLocalUid(eighth.localUid());
        const TagModelItem * eighthItem = model->itemForIndex(eighthIndex);
        if (!eighthItem) {
            QNWARNING("Can't get the tag model item pointer from the model index");
            emit failure();
            return;
        }

        int rowInEighth = eighthItem->rowForChild(twelvethItem);
        if (rowInEighth < 0) {
            QNWARNING("Can't find tag model item within its original grand parent's children after the promotion");
            emit failure();
            return;
        }

        // Should be able to demote the items
        // TODO: take two items within the eighth item's children: first row and second row
        // TODO: fetch the parent of the item at the second row
        // TODO: do demote for the second row
        // TODO: verify that the item previously being at the second row is now a child of the item at the first row
        // TODO: and that it is not a child of the original parent

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
