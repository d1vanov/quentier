#include "TagModelTestHelper.h"
#include "../../models/TagModel.h"
#include "modeltest.h"
#include "Macros.h"
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

        // NOTE: exploiting the direct connection used in the current test environment:
        // after the following lines the local storage would be filled with the test objects
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

        TagCache cache(20);

        TagModel * model = new TagModel(*m_pLocalStorageManagerThreadWorker, cache, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for local uid");
        }

        QModelIndex secondParentIndex = model->parent(secondIndex);
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Dirty, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for dirty column");
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the dirty flag in tag model manually which is not intended");
        }

        QVariant data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL("The dirty state appears to have changed after setData in tag model even though the method returned false");
        }

        // Should be able to make the non-synchronizable (local) item synchronizable (non-local)
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Synchronizable, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL("Can't change the synchronizable flag from false to true for tag model item");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have not changed after setData in tag model even though the method returned true");
        }

        // Verify the dirty flag has changed as a result of making the item synchronizable
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Dirty, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while expected to get the state of dirty flag");
        }

        if (!data.toBool()) {
            FAIL("The dirty state hasn't changed after making the tag model item synchronizable while it was expected to have changed");
        }

        // Should not be able to make the synchronizable (non-local) item non-synchronizable (local)
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Synchronizable, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the synchronizable flag in tag model from true to false which is not intended");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have changed after setData in tag model even though the method returned false");
        }

        // Should be able to change name
        secondIndex = model->index(secondIndex.row(), TagModel::Columns::Name, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for name column");
        }

        QString newName = "Second (name modified)";
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            FAIL("Can't change the name of the tag model item");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while expected to get the name of the tag item");
        }

        if (data.toString() != newName) {
            FAIL("The name of the tag item returned by the model does not match the name just set to this item: "
                 "received " << data.toString() << ", expected " << newName);
        }

        // Should not be able to remove the row with a synchronizable (non-local) tag
        res = model->removeRow(secondIndex.row(), secondParentIndex);
        if (res) {
            FAIL("Was able to remove the row with a synchronizable tag which is not intended");
        }

        QModelIndex secondIndexAfterFailedRemoval = model->indexForLocalUid(second.localUid());
        if (!secondIndexAfterFailedRemoval.isValid()) {
            FAIL("Can't get the valid tag item model index after the failed row removal attempt");
        }

        if (secondIndexAfterFailedRemoval.row() != secondIndex.row()) {
            FAIL("Tag model returned item index with a different row after the failed row removal attempt");
        }

        // Should be able to remove the row with a non-synchronizable (local) tag
        QModelIndex firstIndex = model->indexForLocalUid(first.localUid());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for local uid");
        }

        QModelIndex firstParentIndex = model->parent(firstIndex);
        res = model->removeRow(firstIndex.row(), firstParentIndex);
        if (!res) {
            FAIL("Can't remove the row with a non-synchronizable tag item from the model");
        }

        QModelIndex firstIndexAfterRemoval = model->indexForLocalUid(first.localUid());
        if (firstIndexAfterRemoval.isValid()) {
            FAIL("Was able to get the valid model index for the removed tag item by local uid which is not intended");
        }

        // Should be able to promote the items
        QModelIndex twelvethIndex = model->indexForLocalUid(twelveth.localUid());
        const TagModelItem * twelvethItem = model->itemForIndex(twelvethIndex);
        const TagModelItem * tenthItem = twelvethItem->parent();
        if (!tenthItem) {
            FAIL("Parent of one of tag model items is null");
        }

        twelvethIndex = model->promote(twelvethIndex);
        const TagModelItem * newTwelvethItem = model->itemForIndex(twelvethIndex);
        if (twelvethItem != newTwelvethItem) {
            FAIL("The tag model returns different pointers to items before and after the item promotion");
        }

        int rowInTenth = tenthItem->rowForChild(twelvethItem);
        if (rowInTenth >= 0) {
            FAIL("Tag model item can still be found within the original parent's children after the promotion");
        }

        QModelIndex eighthIndex = model->indexForLocalUid(eighth.localUid());
        const TagModelItem * eighthItem = model->itemForIndex(eighthIndex);
        if (!eighthItem) {
            FAIL("Can't get the tag model item pointer from the model index");
        }

        int rowInEighth = eighthItem->rowForChild(twelvethItem);
        if (rowInEighth < 0) {
            FAIL("Can't find tag model item within its original grand parent's children after the promotion");
        }

        // Should be able to demote the items
        int eighthNumChildren = eighthItem->numChildren();
        if (eighthNumChildren < 2) {
            FAIL("Expected for the eighth item to have at least two children at this moment of test");
        }

        const TagModelItem * firstEighthChild = eighthItem->childAtRow(0);
        const TagModelItem * secondEighthChild = eighthItem->childAtRow(1);

        QModelIndex secondEighthChildIndex = model->indexForItem(secondEighthChild);
        secondEighthChildIndex = model->demote(secondEighthChildIndex);

        int formerSecondEighthChildRowInEighth = eighthItem->rowForChild(secondEighthChild);
        if (formerSecondEighthChildRowInEighth >= 0) {
            FAIL("Tag model item can still be found within the original parent's children after the demotion");
        }

        int formerSecondEighthChildRowInNewParent = firstEighthChild->rowForChild(secondEighthChild);
        if (formerSecondEighthChildRowInNewParent < 0) {
            FAIL("Can't find tag model item within the children of its expected new parent after the demotion");
        }

        // Check the sorting for tag items: by default should sort by name in ascending order
        QModelIndex fifthIndex = model->indexForLocalUid(fifth.localUid());
        if (!fifthIndex.isValid()) {
            FAIL("Can't get the valid tah model item index for local uid");
        }

        const TagModelItem * fifthItem = model->itemForIndex(fifthIndex);
        if (!fifthItem) {
            FAIL("Can't get the tag model item pointer from the model index");
        }

        const TagModelItem * fakeRootItem = fifthItem->parent();
        if (!fakeRootItem) {
            FAIL("Can't get the fake root item in the tag model: getting null pointer instead");
        }

        res = checkSorting(*model, fakeRootItem);
        if (!res) {
            FAIL("Sorting check failed for the tag model for ascending order");
        }

        // Change the sort order and check the sorting again
        model->sort(TagModel::Columns::Name, Qt::DescendingOrder);

        res = checkSorting(*model, fakeRootItem);
        if (!res) {
            FAIL("Sorting check failed for the tag model for descending order");
        }

        emit success();
        return;
    }
    CATCH_EXCEPTION()

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

bool TagModelTestHelper::checkSorting(const TagModel & model, const TagModelItem * rootItem) const
{
    if (!rootItem) {
        QNWARNING("Found null pointer to tag model item when checking the sorting");
        return false;
    }

    QList<const TagModelItem*> children = rootItem->children();
    if (children.isEmpty()) {
        return true;
    }

    QList<const TagModelItem*> sortedChildren = children;

    if (model.sortOrder() == Qt::AscendingOrder) {
        std::sort(sortedChildren.begin(), sortedChildren.end(), LessByName());
    }
    else {
        std::sort(sortedChildren.begin(), sortedChildren.end(), GreaterByName());
    }

    bool res = (children == sortedChildren);
    if (!res) {
        return false;
    }

    for(auto it = children.begin(), end = children.end(); it != end; ++it)
    {
        const TagModelItem * child = *it;
        res = checkSorting(model, child);
        if (!res) {
            return false;
        }
    }

    return true;
}

bool TagModelTestHelper::LessByName::operator()(const TagModelItem * lhs, const TagModelItem * rhs) const
{
    return (lhs->name().localeAwareCompare(rhs->name()) <= 0);
}

bool TagModelTestHelper::GreaterByName::operator()(const TagModelItem * lhs, const TagModelItem * rhs) const
{
    return (lhs->name().localeAwareCompare(rhs->name()) > 0);
}

} // namespace qute_note
