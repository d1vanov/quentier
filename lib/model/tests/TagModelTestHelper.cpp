/*
 * Copyright 2016-2020 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TagModelTestHelper.h"

#include "modeltest.h"
#include "TestMacros.h"

#include <lib/model/NotebookCache.h>
#include <lib/model/NoteCache.h>
#include <lib/model/tag/TagModel.h>

#include <quentier/exception/IQuentierException.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/UidGenerator.h>

namespace quentier {

TagModelTestHelper::TagModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerAsync(pLocalStorageManagerAsync)
{
    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::addTagFailed,
        this,
        &TagModelTestHelper::onAddTagFailed);

    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateTagFailed,
        this,
        &TagModelTestHelper::onUpdateTagFailed);

    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::findTagFailed,
        this,
        &TagModelTestHelper::onFindTagFailed);

    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::listTagsFailed,
        this,
        &TagModelTestHelper::onListTagsFailed);

    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::expungeTagFailed,
        this,
        &TagModelTestHelper::onExpungeTagFailed);
}

void TagModelTestHelper::test()
{
    QNDEBUG("TagModelTestHelper::test");

    ErrorString errorDescription;

    try {
        Tag first;
        first.setName(QStringLiteral("First"));
        first.setLocal(true);
        first.setDirty(true);

        Tag second;
        second.setName(QStringLiteral("Second"));
        second.setLocal(true);
        second.setDirty(false);
        second.setGuid(UidGenerator::Generate());

        Tag third;
        third.setName(QStringLiteral("Third"));
        third.setLocal(false);
        third.setDirty(true);
        third.setGuid(UidGenerator::Generate());

        Tag fourth;
        fourth.setName(QStringLiteral("Fourth"));
        fourth.setLocal(false);
        fourth.setDirty(false);
        fourth.setGuid(UidGenerator::Generate());

        Tag fifth;
        fifth.setName(QStringLiteral("Fifth"));
        fifth.setLocal(false);
        fifth.setDirty(false);
        fifth.setGuid(UidGenerator::Generate());

        Tag sixth;
        sixth.setName(QStringLiteral("Sixth"));
        sixth.setLocal(false);
        sixth.setDirty(false);
        sixth.setGuid(UidGenerator::Generate());
        sixth.setParentLocalUid(fifth.localUid());
        sixth.setParentGuid(fifth.guid());

        Tag seventh;
        seventh.setName(QStringLiteral("Seventh"));
        seventh.setLocal(false);
        seventh.setDirty(false);
        seventh.setGuid(UidGenerator::Generate());
        seventh.setParentLocalUid(fifth.localUid());
        seventh.setParentGuid(fifth.guid());

        Tag eighth;
        eighth.setName(QStringLiteral("Eighth"));
        eighth.setLocal(false);
        eighth.setDirty(true);
        eighth.setGuid(UidGenerator::Generate());
        eighth.setParentLocalUid(fifth.localUid());
        eighth.setParentGuid(fifth.guid());

        Tag ninth;
        ninth.setName(QStringLiteral("Ninth"));
        ninth.setLocal(false);
        ninth.setDirty(false);
        ninth.setGuid(UidGenerator::Generate());
        ninth.setParentLocalUid(sixth.localUid());
        ninth.setParentGuid(sixth.guid());

        Tag tenth;
        tenth.setName(QStringLiteral("Tenth"));
        tenth.setLocal(false);
        tenth.setDirty(true);
        tenth.setParentLocalUid(eighth.localUid());
        tenth.setParentGuid(eighth.guid());

        Tag eleventh;
        eleventh.setName(QStringLiteral("Eleventh"));
        eleventh.setLocal(false);
        eleventh.setDirty(true);
        eleventh.setParentLocalUid(tenth.localUid());

        Tag twelveth;
        twelveth.setName(QStringLiteral("Twelveth"));
        twelveth.setLocal(false);
        twelveth.setDirty(true);
        twelveth.setParentLocalUid(tenth.localUid());

#define ADD_TAG(tag)                                                           \
    m_pLocalStorageManagerAsync->onAddTagRequest(tag, QUuid())                 \
// ADD_TAG

        // NOTE: exploiting the direct connection used in the current test
        // environment: after the following lines the local storage would be
        // filled with the test objects
        ADD_TAG(first);
        ADD_TAG(second);
        ADD_TAG(third);
        ADD_TAG(fourth);
        ADD_TAG(fifth);
        ADD_TAG(sixth);
        ADD_TAG(seventh);
        ADD_TAG(eighth);
        ADD_TAG(ninth);
        ADD_TAG(tenth);
        ADD_TAG(eleventh);
        ADD_TAG(twelveth);

#undef ADD_TAG

        TagCache cache(20);
        Account account(QStringLiteral("Default user"), Account::Type::Local);

        TagModel * model = new TagModel(account, *m_pLocalStorageManagerAsync,
                                        cache, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for local uid");
        }

        QModelIndex secondParentIndex = model->parent(secondIndex);

        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for dirty column");
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the dirty flag in tag model "
                 "manually which is not intended");
        }

        QVariant data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL("The dirty state appears to have changed after setData in "
                 "tag model even though the method returned false");
        }

        // Should only be able to make the non-synchronizable (local) item
        // synchronizable (non-local) with non-local account
        // 1) Trying with local account
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Synchronizable),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for "
                 "synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the synchronizable flag "
                 "from false to true for tag model item with local account");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the state of synchronizable flag");
        }

        if (data.toBool()) {
            FAIL("Even though setData returned false on attempt "
                 "to make the tag item synchronizable with "
                 "the local account, the actual data within "
                 "the model appears to have changed");
        }

        // 2) Trying the non-local account
        account = Account(
            QStringLiteral("Evernote user"), Account::Type::Evernote,
            qevercloud::UserID(1));

        model->setAccount(account);

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL("Wasn't able to change the synchronizable flag "
                 "from false to true for tag model item "
                 "even with the account of Evernote type");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have not "
                 "changed after setData in tag model "
                 "even though the method returned true");
        }

        // Verify the dirty flag has changed as a result of making the item
        // synchronizable
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the state of dirty flag");
        }

        if (!data.toBool()) {
            FAIL("The dirty state hasn't changed after making "
                 "the tag model item synchronizable while it was "
                 "expected to have changed");
        }

        // Should not be able to make the synchronizable (non-local) item
        // non-synchronizable (local)
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Synchronizable),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for "
                 "synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the synchronizable flag in "
                 "tag model from true to false which is not intended");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have changed "
                 "after setData in tag model even though the method "
                 "returned false");
        }

        // Should be able to change name
        // But first clear the dirty flag from the tag to ensure it would be
        // automatically set when changing the name

        second.setLocal(false);
        second.setDirty(false);
        m_pLocalStorageManagerAsync->onUpdateTagRequest(second, QUuid());

        // Ensure the dirty flag was cleared
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the dirty flag of model item");
        }

        if (data.toBool()) {
            FAIL("The tag model item is still dirty even though "
                 "this flag for this item was updated in the local "
                 "storage to false");
        }

        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Name),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for name column");
        }

        QString newName = QStringLiteral("Second (name modified)");
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            FAIL("Can't change the name of the tag model item");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the name of the tag item");
        }

        if (data.toString() != newName) {
            FAIL("The name of the tag item returned by the model "
                 << "does not match the name just set to this item: received "
                 << data.toString() << ", expected " << newName);
        }

        // Ensure the dirty flag has changed to true
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the dirty flag of model item");
        }

        if (!data.toBool()) {
            FAIL("The dirty flag appears to not have changed as "
                 "a result of changing the name of the tag model item");
        }

        // Should not be able to remove the row with a synchronizable (non-local) tag
        res = model->removeRow(secondIndex.row(), secondParentIndex);
        if (res) {
            FAIL("Was able to remove the row with a synchronizable "
                 "tag which is not intended");
        }

        QModelIndex secondIndexAfterFailedRemoval =
            model->indexForLocalUid(second.localUid());
        if (!secondIndexAfterFailedRemoval.isValid()) {
            FAIL("Can't get the valid tag item model index after "
                 "the failed row removal attempt");
        }

        if (secondIndexAfterFailedRemoval.row() != secondIndex.row()) {
            FAIL("Tag model returned item index with a different "
                 "row after the failed row removal attempt");
        }

        // Should be able to remove the row with a (local) tag having empty guid
        QModelIndex firstIndex = model->indexForLocalUid(first.localUid());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for local uid");
        }

        QModelIndex firstParentIndex = model->parent(firstIndex);
        res = model->removeRow(firstIndex.row(), firstParentIndex);
        if (!res) {
            FAIL("Can't remove the row with a tag item with empty "
                 "guid from the model");
        }

        QModelIndex firstIndexAfterRemoval = model->indexForLocalUid(
            first.localUid());

        if (firstIndexAfterRemoval.isValid()) {
            FAIL("Was able to get the valid model index for "
                 "the removed tag item by local uid which is not intended");
        }

        // Should be able to promote the items
        // But first clear the dirty flag from the tag to be promoted to ensure
        // it would be automatically set after the promotion

        twelveth.setDirty(false);
        m_pLocalStorageManagerAsync->onUpdateTagRequest(twelveth, QUuid());

        QModelIndex twelvethIndex = model->indexForLocalUid(
            twelveth.localUid());

        if (!twelvethIndex.isValid()) {
            FAIL("Can't get the valid index to tag model item by local uid");
        }

        twelvethIndex = model->index(
            twelvethIndex.row(),
            static_cast<int>(TagModel::Column::Dirty),
            twelvethIndex.parent());

        if (!twelvethIndex.isValid()) {
            FAIL("Can't get the valid tag item model index for "
                 "dirty column");
        }

        data = model->data(twelvethIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the tag model while "
                 "expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL("The tag model item is still dirty even though "
                 "this flag for this item was updated in the local "
                 "storage to false");
        }

        twelvethIndex = model->index(
            twelvethIndex.row(),
            static_cast<int>(TagModel::Column::Name),
            twelvethIndex.parent());

        const auto * pTwelvethItem = model->itemForIndex(twelvethIndex);
        const auto * pTenthItem = pTwelvethItem->parent();
        if (!pTenthItem) {
            FAIL("Parent of one of tag model items is null");
        }

        twelvethIndex = model->promote(twelvethIndex);
        const auto * pNewTwelvethItem = model->itemForIndex(twelvethIndex);
        if (pTwelvethItem != pNewTwelvethItem) {
            FAIL("The tag model returns different pointers to "
                 "items before and after the item promotion");
        }

        const auto * pNewTwelvethTagItem = pNewTwelvethItem->cast<TagItem>();
        if (!pNewTwelvethTagItem) {
            FAIL("No tag item under the tag model item");
        }

        if (!pNewTwelvethTagItem->isDirty()) {
            FAIL("The dirty flag hasn't been automatically set "
                 "to true after promoting the item");
        }

        int rowInTenth = pTenthItem->rowForChild(pTwelvethItem);
        if (rowInTenth >= 0) {
            FAIL("Tag model item can still be found within "
                 "the original parent's children after the promotion");
        }

        QModelIndex eighthIndex = model->indexForLocalUid(eighth.localUid());
        const auto * pEighthItem = model->itemForIndex(eighthIndex);
        if (!pEighthItem) {
            FAIL("Can't get the tag model item pointer from the model index");
        }

        int rowInEighth = pEighthItem->rowForChild(pTwelvethItem);
        if (rowInEighth < 0) {
            FAIL("Can't find tag model item within its original "
                 "grand parent's children after the promotion");
        }

        // We'll be updating the twelveth tag in local storage manually further,
        // so reflecting the change in the model in the local object as well
        twelveth.setParentGuid(pNewTwelvethTagItem->parentGuid());
        twelveth.setParentLocalUid(pNewTwelvethTagItem->parentLocalUid());

        // Should be able to demote the items
        int eighthChildCount = pEighthItem->childrenCount();
        if (eighthChildCount < 2) {
            FAIL("Expected for the eighth item to have at least "
                 "two children at this moment of test");
        }

        const auto * pFirstEighthChild = pEighthItem->childAtRow(0);
        const auto * pSecondEighthChild = pEighthItem->childAtRow(1);

        // First make the second child of eighth tag non-dirty to ensure that
        // it would be marked as dirty after demoting
        if (!pSecondEighthChild) {
            FAIL("Unexpected null pointer to tag model item");
        }

        const auto * pSecondEighthChildTag =
            pSecondEighthChild->cast<TagItem>();

        if (!pSecondEighthChildTag) {
            FAIL("Unexpected null pointer to tag item");
        }

        if (pSecondEighthChildTag->localUid() ==
            pNewTwelvethTagItem->localUid())
        {
            twelveth.setDirty(false);
            m_pLocalStorageManagerAsync->onUpdateTagRequest(twelveth, QUuid());
        }
        else
        {
            tenth.setDirty(false);
            m_pLocalStorageManagerAsync->onUpdateTagRequest(tenth, QUuid());
        }

        if (pSecondEighthChildTag->isDirty()) {
            FAIL("The dirty flag should have been cleared from "
                 "tag model item but it hasn't been");
        }

        QModelIndex secondEighthChildIndex = model->indexForItem(
            pSecondEighthChild);

        secondEighthChildIndex = model->demote(secondEighthChildIndex);

        int formerSecondEighthChildRowInEighth = pEighthItem->rowForChild(
            pSecondEighthChild);

        if (formerSecondEighthChildRowInEighth >= 0) {
            FAIL("Tag model item can still be found within "
                 "the original parent's children after the demotion");
        }

        int formerSecondEighthChildRowInNewParent =
            pFirstEighthChild->rowForChild(pSecondEighthChild);

        if (formerSecondEighthChildRowInNewParent < 0) {
            FAIL("Can't find tag model item within the children "
                 "of its expected new parent after the demotion");
        }

        if (!pSecondEighthChildTag->isDirty()) {
            FAIL("The tag model item hasn't been automatically "
                 "marked as dirty after demoting it");
        }

        // We might update the just demoted tag in the local storage further,
        // so need to reflect the change done within the model in the local
        // object as well
        if (pSecondEighthChildTag->localUid() == twelveth.localUid()) {
            twelveth.setParentGuid(pSecondEighthChildTag->parentGuid());
            twelveth.setParentLocalUid(pSecondEighthChildTag->parentLocalUid());
        }
        else {
            tenth.setParentGuid(pSecondEighthChildTag->parentGuid());
            tenth.setParentLocalUid(pSecondEighthChildTag->parentLocalUid());
        }

        // Remove the only remaining child tag from the eighth tag item using
        // TagModel::removeFromParent method
        QModelIndex firstEighthChildIndex = model->indexForItem(
            pFirstEighthChild);

        if (!firstEighthChildIndex.isValid()) {
            FAIL("Can't get the valid tag model item index for given tag item");
        }

        const auto * pFirstEighthChildTag = pFirstEighthChild->cast<TagItem>();
        if (!pFirstEighthChildTag) {
            FAIL("Unexpected null pointer to tag item");
        }

        // First make the tag non-dirty to ensure the dirty flag would be set
        // to true automatically after removing the tag from parent
        if (pFirstEighthChildTag->localUid() == twelveth.localUid()) {
            twelveth.setDirty(false);
            m_pLocalStorageManagerAsync->onUpdateTagRequest(twelveth, QUuid());
        }
        else {
            tenth.setDirty(false);
            m_pLocalStorageManagerAsync->onUpdateTagRequest(tenth, QUuid());
        }

        if (pFirstEighthChildTag->isDirty()) {
            FAIL("The dirty flag should have been cleared from "
                 "the tag item but it hasn't been");
        }

        QModelIndex formerFirstEighthChildIndex =
            model->removeFromParent(firstEighthChildIndex);
        if (!formerFirstEighthChildIndex.isValid()) {
            FAIL("Failed to remove the tag item from parent");
        }

        // Verify the item has indeed been removed from the children of
        // the eighth tag
        int formerFirstEighthChildRowInEighth = pEighthItem->rowForChild(
            pFirstEighthChild);

        if (formerFirstEighthChildRowInEighth >= 0) {
            FAIL("Tag model item can still be found within "
                 "the original parent's children after its "
                 "removal from there");
        }

        // Verity the dirty flag has been set automatically to the tag item
        // removed from its parent
        if (!pFirstEighthChildTag->isDirty()) {
            FAIL("Tag model item which was removed from its "
                 "parent was not marked as the dirty one");
        }

        // Check the sorting for tag items: by default should sort by name in
        // ascending order
        QModelIndex fifthIndex = model->indexForLocalUid(fifth.localUid());
        if (!fifthIndex.isValid()) {
            FAIL("Can't get the valid tag model item index for local uid");
        }

        const auto * pFifthItem = model->itemForIndex(fifthIndex);
        if (!pFifthItem) {
            FAIL("Can't get the tag model item pointer from "
                 "the model index");
        }

        const auto * pAllTagsRootItem = pFifthItem->parent();
        if (!pAllTagsRootItem) {
            FAIL("Can't get all tags root item in the tag model: "
                 "getting null pointer instead");
        }

        ErrorString errorDescription;
        res = checkSorting(*model, pAllTagsRootItem, errorDescription);
        if (!res) {
            FAIL("Sorting check failed for the tag model for "
                 << "ascending order: " << errorDescription);
        }

        // Set the account to local again to test accounting for tag name
        // reservation in create/remove/create cycles
        account = Account(QStringLiteral("Local user"), Account::Type::Local);
        model->setAccount(account);

        // Should not be able to create a tag with existing name
        errorDescription.clear();

        QModelIndex thirteenthTagIndex = model->createTag(
            third.name(),
            QString(),
            QString(),
            errorDescription);

        if (thirteenthTagIndex.isValid()) {
            FAIL("Was able to create tag with the same name as "
                 "the already existing one");
        }

        // The error description should say something about the inability to
        // create the tag
        if (errorDescription.isEmpty()) {
            FAIL("The error description about the inability to "
                 "create a tag due to the name collision is empty");
        }

        // Should be able to create the tag with a new name
        QString thirteenthTagName = QStringLiteral("Thirteenth");
        errorDescription.clear();

        thirteenthTagIndex = model->createTag(
            thirteenthTagName,
            third.name(),
            QString(),
            errorDescription);

        if (!thirteenthTagIndex.isValid()) {
            FAIL("Wasn't able to create a tag with the name not "
                 "present within the tag model");
        }

        // Should no longer be able to create the tag with the same name as
        // the just added one
        QModelIndex fourteenthTagIndex = model->createTag(
            thirteenthTagName,
            fourth.name(),
            QString(),
            errorDescription);

        if (fourteenthTagIndex.isValid()) {
            FAIL("Was able to create a tag with the same name "
                 "as the just created tag");
        }

        // The error description should say something about the inability
        // to create the tag
        if (errorDescription.isEmpty()) {
            FAIL("The error description about the inability "
                 "to create a tag due to the name collision is empty");
        }

        // Should be able to remove the just added tag
        res = model->removeRow(
            thirteenthTagIndex.row(),
            thirteenthTagIndex.parent());

        if (!res) {
            FAIL("Wasn't able to remove the tag just added "
                 "to the tag model");
        }

        // Should again be able to create the tag with the same name
        errorDescription.clear();

        thirteenthTagIndex = model->createTag(
            thirteenthTagName,
            QString(),
            QString(),
            errorDescription);

        if (!thirteenthTagIndex.isValid()) {
            FAIL("Wasn't able to create a tag with the same "
                 "name as the just removed one");
        }

        // Change the sort order and check the sorting again
        model->sort(
            static_cast<int>(TagModel::Column::Name),
            Qt::DescendingOrder);

        res = checkSorting(*model, pAllTagsRootItem, errorDescription);
        if (!res) {
            FAIL("Sorting check failed for the tag model for "
                << "descending order: " << errorDescription);
        }

        // After expunging the tag being the parent for other tags, the child
        // tags should not be present within the model as well as the parent one
        m_pLocalStorageManagerAsync->onExpungeTagRequest(tenth, QUuid());

        QModelIndex tenthIndex = model->indexForLocalUid(tenth.localUid());
        if (tenthIndex.isValid()) {
            FAIL("The tag model returns valid index for "
                 "the local uid corresponding to the tag expunged "
                 "from the local storage");
        }

        QModelIndex eleventhIndex = model->indexForLocalUid(eleventh.localUid());
        if (eleventhIndex.isValid()) {
            FAIL("The tag model returns valid index for the local "
                 "uid corresponding to the tag being the child "
                 "of a tag being expunged");
        }

        // Should be able to change the parent of the tag externally and have
        // the model recognize it
        const auto * pNewThirteenthModelItem = model->itemForIndex(
            thirteenthTagIndex);

        if (!pNewThirteenthModelItem) {
            FAIL("Can't find the tag model item corresponding to index");
        }

        const auto * pNewThirteenthTagItem =
            pNewThirteenthModelItem->cast<TagItem>();

        if (!pNewThirteenthTagItem) {
            FAIL("Unexpected null pointer to tag item");
        }

        Tag thirteenth;
        thirteenth.setLocalUid(pNewThirteenthTagItem->localUid());
        thirteenth.setGuid(pNewThirteenthTagItem->guid());
        thirteenth.setLocal(!pNewThirteenthTagItem->isSynchronizable());
        thirteenth.setDirty(pNewThirteenthTagItem->isDirty());

        // Reparent the thirteenth tag to the second tag
        thirteenth.setParentGuid(second.guid());
        thirteenth.setParentLocalUid(second.localUid());

        m_pLocalStorageManagerAsync->onUpdateTagRequest(thirteenth, QUuid());

        if (pNewThirteenthTagItem->parentLocalUid() != second.localUid()) {
            FAIL("The parent local uid of the externally updated "
                 "tag was not picked up from the updated tag");
        }

        const auto * pSecondTagItem = model->itemForLocalUid(second.localUid());
        if (!pSecondTagItem) {
            FAIL("Can't fint tag model item for local uid");
        }

        if (pSecondTagItem->rowForChild(pNewThirteenthModelItem) < 0) {
            FAIL("The new parent item doesn't contain the child "
                 "item which was externally updated");
        }

        Q_EMIT success();
        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void TagModelTestHelper::onAddTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onAddTagFailed: tag = " << tag
        << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void TagModelTestHelper::onUpdateTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onUpdateTagFailed: tag = "
        << tag << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void TagModelTestHelper::onFindTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onFindTagFailed: tag = " << tag
        << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void TagModelTestHelper::onListTagsFailed(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListTagsOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid,
    ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onListTagsFailed: flag = "
        << flag << ", limit = " << limit << ", offset = " << offset
        << ", order = " << order << ", direction = " << orderDirection
        << ", linked notebook guid: is null = "
        << (linkedNotebookGuid.isNull()
            ? "true"
            : "false")
        << ", is empty = "
        << (linkedNotebookGuid.isEmpty()
            ? "true"
            : "false")
        << ", value = " << linkedNotebookGuid
        << ", error description = " << errorDescription
        << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void TagModelTestHelper::onExpungeTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("TagModelTestHelper::onExpungeTagFailed: tag = "
        << tag << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

bool TagModelTestHelper::checkSorting(
    const TagModel & model, const ITagModelItem * pRootItem,
    ErrorString & errorDescription) const
{
    if (!pRootItem) {
        errorDescription.setBase(
            "Found null pointer to tag model item when checking the sorting");
        return false;
    }

    auto children = pRootItem->children();
    if (children.isEmpty()) {
        return true;
    }

    auto sortedChildren = children;

    if (model.sortOrder() == Qt::AscendingOrder)
    {
        std::sort(
            sortedChildren.begin(),
            sortedChildren.end(),
            LessByName());
    }
    else
    {
        std::sort(
            sortedChildren.begin(),
            sortedChildren.end(),
            GreaterByName());
    }

    bool res = (children == sortedChildren);
    if (!res)
    {
        errorDescription.setBase(
            "The list of child tags is not equal to the list of sorted child "
            "tags");

        errorDescription.details() =
            QStringLiteral("Root item: ") + pRootItem->toString();

        errorDescription.details() += QStringLiteral("\nChild tags: ");

        for(const auto * pTagModelItem: qAsConst(children))
        {
            if (!pTagModelItem) {
                errorDescription.details() += QStringLiteral("<null>; ");
                continue;
            }

            const auto * pTagItem = pTagModelItem->cast<TagItem>();
            if (!pTagItem) {
                errorDescription.details() += QStringLiteral("tag: ");
                errorDescription.details() += pTagItem->name();
                errorDescription.details() += QStringLiteral("; ");
                continue;
            }

            const auto * pLinkedNotebookItem =
                pTagModelItem->cast<TagLinkedNotebookRootItem>();

            if (pLinkedNotebookItem)
            {
                errorDescription.details() +=
                    QStringLiteral("linked notebook: ");

                errorDescription.details() += pLinkedNotebookItem->username();
                errorDescription.details() += QStringLiteral("; ");
                continue;
            }

            errorDescription.details() += QStringLiteral("<unknown>; ");
        }

        errorDescription.details() += QStringLiteral("\nSorted child tags: ");

        for(const auto * pTagModelItem: qAsConst(sortedChildren))
        {
            if (!pTagModelItem) {
                errorDescription.details() += QStringLiteral("<null>; ");
                continue;
            }

            const auto * pTagItem = pTagModelItem->cast<TagItem>();
            if (pTagItem) {
                errorDescription.details() += QStringLiteral("tag: ");
                errorDescription.details() += pTagItem->name();
                errorDescription.details() += QStringLiteral("; ");
                continue;
            }

            const auto * pLinkedNotebookItem =
                pTagModelItem->cast<TagLinkedNotebookRootItem>();

            if (pLinkedNotebookItem)
            {
                errorDescription.details() +=
                    QStringLiteral("linked notebook: ");

                errorDescription.details() += pLinkedNotebookItem->username();
                errorDescription.details() += QStringLiteral("; ");
                continue;
            }

            errorDescription.details() += QStringLiteral("<unknown>; ");
        }

        return false;
    }

    for(const auto * pChildItem: qAsConst(children))
    {
        res = checkSorting(model, pChildItem, errorDescription);
        if (!res) {
            return false;
        }
    }

    return true;
}

void TagModelTestHelper::notifyFailureWithStackTrace(
    ErrorString errorDescription)
{
    SysInfo sysInfo;

    errorDescription.details() +=
        QStringLiteral("\nStack trace: ") + sysInfo.stackTrace();

    Q_EMIT failure(errorDescription);
}

#define MODEL_ITEM_NAME(item, itemName)                                        \
    if (item->type() == ITagModelItem::Type::Tag)                              \
    {                                                                          \
        const auto * pTagItem = item->cast<TagItem>();                         \
        if (pTagItem) {                                                        \
            itemName = pTagItem->nameUpper();                                  \
        }                                                                      \
    }                                                                          \
    else if (item->type() == ITagModelItem::Type::LinkedNotebook)              \
    {                                                                          \
        const auto * pLinkedNotebookItem =                                     \
            item->cast<TagLinkedNotebookRootItem>();                           \
        if (pLinkedNotebookItem) {                                             \
            itemName = pLinkedNotebookItem->username().toUpper();              \
        }                                                                      \
    }                                                                          \
// MODEL_ITEM_NAME

bool TagModelTestHelper::LessByName::operator()(
    const ITagModelItem * pLhs, const ITagModelItem * pRhs) const
{
    if ((pLhs->type() == ITagModelItem::Type::AllTagsRoot) &&
        (pRhs->type() != ITagModelItem::Type::AllTagsRoot))
    {
        return false;
    }
    else if ((pLhs->type() != ITagModelItem::Type::AllTagsRoot) &&
             (pRhs->type() == ITagModelItem::Type::AllTagsRoot))
    {
        return true;
    }

    if ((pLhs->type() == ITagModelItem::Type::LinkedNotebook) &&
        (pRhs->type() != ITagModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if ((pLhs->type() != ITagModelItem::Type::LinkedNotebook) &&
             (pRhs->type() == ITagModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName;
    MODEL_ITEM_NAME(pLhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(pRhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) <= 0);
}

bool TagModelTestHelper::GreaterByName::operator()(
    const ITagModelItem * pLhs, const ITagModelItem * pRhs) const
{
    if ((pLhs->type() == ITagModelItem::Type::AllTagsRoot) &&
        (pRhs->type() != ITagModelItem::Type::AllTagsRoot))
    {
        return false;
    }
    else if ((pLhs->type() != ITagModelItem::Type::AllTagsRoot) &&
             (pRhs->type() == ITagModelItem::Type::AllTagsRoot))
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((pLhs->type() == ITagModelItem::Type::LinkedNotebook) &&
        (pRhs->type() != ITagModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if ((pLhs->type() != ITagModelItem::Type::LinkedNotebook) &&
             (pRhs->type() == ITagModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName;
    MODEL_ITEM_NAME(pLhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(pRhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) > 0);
}

} // namespace quentier
