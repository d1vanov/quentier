/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include "TestMacros.h"
#include "TestUtils.h"
#include "modeltest.h"

#include <lib/model/note/NoteCache.h>
#include <lib/model/notebook/NotebookCache.h>
#include <lib/model/tag/TagModel.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/UidGenerator.h>

#include <QEventLoop>

namespace quentier {

TagModelTestHelper::TagModelTestHelper(
    local_storage::ILocalStoragePtr localStorage, QObject * parent) :
    QObject{parent},
    m_localStorage{std::move(localStorage)}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "TagModelTestHelper ctor: local storage is null"}};
    }
}

TagModelTestHelper::~TagModelTestHelper() = default;

void TagModelTestHelper::test()
{
    QNDEBUG("tests:model_test:tag", "TagModelTestHelper::test");

    ErrorString errorDescription;

    try {
        qevercloud::Tag first;
        first.setName(QStringLiteral("First"));
        first.setLocalOnly(true);
        first.setLocallyModified(true);

        qevercloud::Tag second;
        second.setName(QStringLiteral("Second"));
        second.setLocalOnly(true);
        second.setLocallyModified(false);
        second.setGuid(UidGenerator::Generate());

        qevercloud::Tag third;
        third.setName(QStringLiteral("Third"));
        third.setLocalOnly(false);
        third.setLocallyModified(true);
        third.setGuid(UidGenerator::Generate());

        qevercloud::Tag fourth;
        fourth.setName(QStringLiteral("Fourth"));
        fourth.setLocalOnly(false);
        fourth.setLocallyModified(false);
        fourth.setGuid(UidGenerator::Generate());

        qevercloud::Tag fifth;
        fifth.setName(QStringLiteral("Fifth"));
        fifth.setLocalOnly(false);
        fifth.setLocallyModified(false);
        fifth.setGuid(UidGenerator::Generate());

        qevercloud::Tag sixth;
        sixth.setName(QStringLiteral("Sixth"));
        sixth.setLocalOnly(false);
        sixth.setLocallyModified(false);
        sixth.setGuid(UidGenerator::Generate());
        sixth.setParentTagLocalId(fifth.localId());
        sixth.setParentGuid(fifth.guid());

        qevercloud::Tag seventh;
        seventh.setName(QStringLiteral("Seventh"));
        seventh.setLocalOnly(false);
        seventh.setLocallyModified(false);
        seventh.setGuid(UidGenerator::Generate());
        seventh.setParentTagLocalId(fifth.localId());
        seventh.setParentGuid(fifth.guid());

        qevercloud::Tag eighth;
        eighth.setName(QStringLiteral("Eighth"));
        eighth.setLocalOnly(false);
        eighth.setLocallyModified(true);
        eighth.setGuid(UidGenerator::Generate());
        eighth.setParentTagLocalId(fifth.localId());
        eighth.setParentGuid(fifth.guid());

        qevercloud::Tag ninth;
        ninth.setName(QStringLiteral("Ninth"));
        ninth.setLocalOnly(false);
        ninth.setLocallyModified(false);
        ninth.setGuid(UidGenerator::Generate());
        ninth.setParentTagLocalId(sixth.localId());
        ninth.setParentGuid(sixth.guid());

        qevercloud::Tag tenth;
        tenth.setName(QStringLiteral("Tenth"));
        tenth.setLocalOnly(false);
        tenth.setLocallyModified(true);
        tenth.setParentTagLocalId(eighth.localId());
        tenth.setParentGuid(eighth.guid());

        qevercloud::Tag eleventh;
        eleventh.setName(QStringLiteral("Eleventh"));
        eleventh.setLocalOnly(false);
        eleventh.setLocallyModified(true);
        eleventh.setParentTagLocalId(tenth.localId());

        qevercloud::Tag twelveth;
        twelveth.setName(QStringLiteral("Twelveth"));
        twelveth.setLocalOnly(false);
        twelveth.setLocallyModified(true);
        twelveth.setParentTagLocalId(tenth.localId());

        const auto putTag = [this](const qevercloud::Tag & tag) {
            auto f = m_localStorage->putTag(tag);

            waitForFuture(f);

            try {
                f.waitForFinished();
            }
            catch (const IQuentierException & e) {
                Q_EMIT failure(e.errorMessage());
                return false;
            }
            catch (const QException & e) {
                Q_EMIT failure(ErrorString{e.what()});
                return false;
            }

            return true;
        };

        // Will add tags one by one to ensure that any parent tag would be
        // present in the local storage by the time its child tags are added
        for (const auto & tag: std::as_const(
                 QList<qevercloud::Tag>{}
                 << first << second << third << fourth << fifth << sixth
                 << seventh << eighth << ninth << tenth << eleventh
                 << twelveth))
        {
            if (!putTag(tag)) {
                return;
            }
        }

        TagCache cache{20};
        Account account{QStringLiteral("Default user"), Account::Type::Local};

        auto * model =
            new TagModel(account, m_localStorage, cache, this);

        if (!model->allTagsListed()) {
            QEventLoop loop;
            QObject::connect(
                model, &TagModel::notifyAllTagsListed, &loop,
                &QEventLoop::quit);
        }

        ModelTest t1{model};
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        auto secondIndex = model->indexForLocalId(second.localId());
        if (!secondIndex.isValid()) {
            FAIL("Can't get valid tag item model index for local id");
        }

        auto secondParentIndex = model->parent(secondIndex);

        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(TagModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid tag item model index for dirty column");
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change the dirty flag in tag model "
                << "manually which is not intended");
        }

        auto data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL(
                "The dirty state appears to have changed after setData in "
                << "tag model even though the method returned false");
        }

        // Should only be able to make the non-synchronizable (local) item
        // synchronizable (non-local) with non-local account
        // 1) Trying with local account
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Synchronizable),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL(
                "Can't get valid tag item model index for "
                << "synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change the synchronizable flag "
                << "from false to true for tag model item with local account");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the state of synchronizable flag");
        }

        if (data.toBool()) {
            FAIL(
                "Even though setData returned false on attempt "
                << "to make the tag item synchronizable with "
                << "the local account, the actual data within "
                << "the model appears to have changed");
        }

        // 2) Trying the non-local account
        account = Account(
            QStringLiteral("Evernote user"), Account::Type::Evernote,
            qevercloud::UserID(1));

        model->setAccount(account);

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL(
                "Wasn't able to change the synchronizable flag "
                << "from false to true for tag model item "
                << "even with the account of Evernote type");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The synchronizable state appears to have not "
                << "changed after setData in tag model "
                << "even though the method returned true");
        }

        // Verify the dirty flag has changed as a result of making the item
        // synchronizable
        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(TagModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid tag item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the state of dirty flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The dirty state hasn't changed after making "
                << "the tag model item synchronizable while it was "
                << "expected to have changed");
        }

        // Should not be able to make the synchronizable (non-local) item
        // non-synchronizable (local)
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(TagModel::Column::Synchronizable),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL(
                "Can't get valid tag item model index for "
                << "synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change the synchronizable flag in "
                << "tag model from true to false which is not intended");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The synchronizable state appears to have changed "
                << "after setData in tag model even though the method "
                << "returned false");
        }

        // Should be able to change name
        // But first clear the dirty flag from the tag to ensure it would be
        // automatically set when changing the name

        second.setLocalOnly(false);
        second.setLocallyModified(false);
        if (!putTag(second)) {
            return;
        }

        // Ensure the dirty flag was cleared
        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(TagModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid tag item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the dirty flag of model item");
        }

        if (data.toBool()) {
            FAIL(
                "The tag model item is still dirty even though "
                << "this flag for this item was updated in the local "
                << "storage to false");
        }

        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(TagModel::Column::Name),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid tag item model index for name column");
        }

        QString newName = QStringLiteral("Second (name modified)");
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            FAIL("Can't change the name of the tag model item");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the name of the tag item");
        }

        if (data.toString() != newName) {
            FAIL(
                "The name of the tag item returned by the model "
                << "does not match the name just set to this item: received "
                << data.toString() << ", expected " << newName);
        }

        // Ensure the dirty flag has changed to true
        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(TagModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid tag item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the dirty flag of model item");
        }

        if (!data.toBool()) {
            FAIL(
                "The dirty flag appears to not have changed as "
                << "a result of changing the name of the tag model item");
        }

        // Should not be able to remove the row with a synchronizable
        // (non-local) tag
        res = model->removeRow(secondIndex.row(), secondParentIndex);
        if (res) {
            FAIL(
                "Was able to remove the row with a synchronizable "
                << "tag which is not intended");
        }

        auto secondIndexAfterFailedRemoval =
            model->indexForLocalId(second.localId());

        if (!secondIndexAfterFailedRemoval.isValid()) {
            FAIL(
                "Can't get valid tag item model index after "
                << "the failed row removal attempt");
        }

        if (secondIndexAfterFailedRemoval.row() != secondIndex.row()) {
            FAIL(
                "Tag model returned item index with a different "
                << "row after the failed row removal attempt");
        }

        // Should be able to remove the row with a (local) tag having empty guid
        auto firstIndex = model->indexForLocalId(first.localId());
        if (!firstIndex.isValid()) {
            FAIL("Can't get valid tag item model index for local id");
        }

        auto firstParentIndex = model->parent(firstIndex);
        res = model->removeRow(firstIndex.row(), firstParentIndex);
        if (!res) {
            FAIL(
                "Can't remove the row with a tag item with empty "
                "guid from the model");
        }

        auto firstIndexAfterRemoval = model->indexForLocalId(first.localId());

        if (firstIndexAfterRemoval.isValid()) {
            FAIL(
                "Was able to get valid model index for "
                << "the removed tag item by local id which is not intended");
        }

        // Should be able to promote the items
        // But first clear the dirty flag from the tag to be promoted to ensure
        // it would be automatically set after the promotion

        twelveth.setLocallyModified(false);
        if (!putTag(twelveth)) {
            return;
        }

        auto twelvethIndex = model->indexForLocalId(twelveth.localId());

        if (!twelvethIndex.isValid()) {
            FAIL("Can't get valid index to tag model item by local id");
        }

        twelvethIndex = model->index(
            twelvethIndex.row(), static_cast<int>(TagModel::Column::Dirty),
            twelvethIndex.parent());

        if (!twelvethIndex.isValid()) {
            FAIL("Can't get valid tag item model index for dirty column");
        }

        data = model->data(twelvethIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the tag model while "
                << "expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL(
                "The tag model item is still dirty even though "
                << "this flag for this item was updated in the local "
                << "storage to false");
        }

        twelvethIndex = model->index(
            twelvethIndex.row(), static_cast<int>(TagModel::Column::Name),
            twelvethIndex.parent());

        const auto * twelvethItem = model->itemForIndex(twelvethIndex);
        const auto * tenthItem = twelvethItem->parent();
        if (!tenthItem) {
            FAIL("Parent of one of tag model items is null");
        }

        twelvethIndex = model->promote(twelvethIndex);
        const auto * newTwelvethItem = model->itemForIndex(twelvethIndex);
        if (twelvethItem != newTwelvethItem) {
            FAIL(
                "The tag model returns different pointers to "
                << "items before and after the item promotion");
        }

        const auto * newTwelvethTagItem = newTwelvethItem->cast<TagItem>();
        if (!newTwelvethTagItem) {
            FAIL("No tag item under the tag model item");
        }

        if (!newTwelvethTagItem->isDirty()) {
            FAIL(
                "The dirty flag hasn't been automatically set "
                << "to true after promoting the item");
        }

        int rowInTenth = tenthItem->rowForChild(twelvethItem);
        if (rowInTenth >= 0) {
            FAIL(
                "Tag model item can still be found within "
                << "the original parent's children after the promotion");
        }

        auto eighthIndex = model->indexForLocalId(eighth.localId());
        const auto * eighthItem = model->itemForIndex(eighthIndex);
        if (!eighthItem) {
            FAIL("Can't get the tag model item pointer from the model index");
        }

        int rowInEighth = eighthItem->rowForChild(twelvethItem);
        if (rowInEighth < 0) {
            FAIL(
                "Can't find tag model item within its original "
                << "grand parent's children after the promotion");
        }

        // We'll be updating the twelveth tag in local storage manually further,
        // so reflecting the change in the model in the local object as well
        twelveth.setParentGuid(newTwelvethTagItem->parentGuid());
        twelveth.setParentTagLocalId(newTwelvethTagItem->parentLocalId());

        // Should be able to demote the items
        int eighthChildCount = eighthItem->childrenCount();
        if (eighthChildCount < 2) {
            FAIL(
                "Expected for the eighth item to have at least "
                "two children at this moment of test");
        }

        const auto * firstEighthChild = eighthItem->childAtRow(0);
        const auto * secondEighthChild = eighthItem->childAtRow(1);

        // First make the second child of eighth tag non-dirty to ensure that
        // it would be marked as dirty after demoting
        if (!secondEighthChild) {
            FAIL("Unexpected null pointer to tag model item");
        }

        const auto * secondEighthChildTag =
            secondEighthChild->cast<TagItem>();

        if (!secondEighthChildTag) {
            FAIL("Unexpected null pointer to tag item");
        }

        if (secondEighthChildTag->localId() ==
            newTwelvethTagItem->localId()) {
            twelveth.setLocallyModified(false);
            if (!putTag(twelveth)) {
                return;
            }
        }
        else {
            tenth.setLocallyModified(false);
            if (!putTag(tenth)) {
                return;
            }
        }

        if (secondEighthChildTag->isDirty()) {
            FAIL(
                "The dirty flag should have been cleared from "
                << "tag model item but it hasn't been");
        }

        auto secondEighthChildIndex = model->indexForItem(secondEighthChild);

        secondEighthChildIndex = model->demote(secondEighthChildIndex);

        int formerSecondEighthChildRowInEighth =
            eighthItem->rowForChild(secondEighthChild);

        if (formerSecondEighthChildRowInEighth >= 0) {
            FAIL(
                "Tag model item can still be found within "
                << "the original parent's children after the demotion");
        }

        int formerSecondEighthChildRowInNewParent =
            firstEighthChild->rowForChild(secondEighthChild);

        if (formerSecondEighthChildRowInNewParent < 0) {
            FAIL(
                "Can't find tag model item within the children "
                << "of its expected new parent after the demotion");
        }

        if (!secondEighthChildTag->isDirty()) {
            FAIL(
                "The tag model item hasn't been automatically "
                << "marked as dirty after demoting it");
        }

        // We might update the just demoted tag in the local storage further,
        // so need to reflect the change done within the model in the local
        // object as well
        if (secondEighthChildTag->localId() == twelveth.localId()) {
            twelveth.setParentGuid(secondEighthChildTag->parentGuid());
            twelveth.setParentTagLocalId(secondEighthChildTag->parentLocalId());
        }
        else {
            tenth.setParentGuid(secondEighthChildTag->parentGuid());
            tenth.setParentTagLocalId(secondEighthChildTag->parentLocalId());
        }

        // Remove the only remaining child tag from the eighth tag item using
        // TagModel::removeFromParent method
        auto firstEighthChildIndex = model->indexForItem(firstEighthChild);

        if (!firstEighthChildIndex.isValid()) {
            FAIL("Can't get valid tag model item index for given tag item");
        }

        const auto * firstEighthChildTag = firstEighthChild->cast<TagItem>();
        if (!firstEighthChildTag) {
            FAIL("Unexpected null pointer to tag item");
        }

        // First make the tag non-dirty to ensure the dirty flag would be set
        // to true automatically after removing the tag from parent
        if (firstEighthChildTag->localId() == twelveth.localId()) {
            twelveth.setLocallyModified(false);
            if (!putTag(twelveth)) {
                return;
            }
        }
        else {
            tenth.setLocallyModified(false);
            if (!putTag(tenth)) {
                return;
            }
        }

        if (firstEighthChildTag->isDirty()) {
            FAIL(
                "The dirty flag should have been cleared from "
                << "the tag item but it hasn't been");
        }

        auto formerFirstEighthChildIndex =
            model->removeFromParent(firstEighthChildIndex);

        if (!formerFirstEighthChildIndex.isValid()) {
            FAIL("Failed to remove the tag item from parent");
        }

        // Verify the item has indeed been removed from the children of
        // the eighth tag
        int formerFirstEighthChildRowInEighth =
            eighthItem->rowForChild(firstEighthChild);

        if (formerFirstEighthChildRowInEighth >= 0) {
            FAIL(
                "Tag model item can still be found within the original "
                << "parent's children after its removal from there");
        }

        // Verity the dirty flag has been set automatically to the tag item
        // removed from its parent
        if (!firstEighthChildTag->isDirty()) {
            FAIL(
                "Tag model item which was removed from its "
                << "parent was not marked as the dirty one");
        }

        // Check the sorting for tag items: by default should sort by name in
        // ascending order
        auto fifthIndex = model->indexForLocalId(fifth.localId());
        if (!fifthIndex.isValid()) {
            FAIL("Can't get valid tag model item index for local id");
        }

        const auto * fifthItem = model->itemForIndex(fifthIndex);
        if (!fifthItem) {
            FAIL("Can't get the tag model item pointer from the model index");
        }

        const auto * allTagsRootItem = fifthItem->parent();
        if (!allTagsRootItem) {
            FAIL(
                "Can't get all tags root item in the tag model: "
                << "getting null pointer instead");
        }

        ErrorString errorDescription;
        res = checkSorting(*model, allTagsRootItem, errorDescription);
        if (!res) {
            FAIL(
                "Sorting check failed for the tag model for "
                << "ascending order: " << errorDescription);
        }

        // Set the account to local again to test accounting for tag name
        // reservation in create/remove/create cycles
        account = Account{QStringLiteral("Local user"), Account::Type::Local};
        model->setAccount(account);

        // Should not be able to create a tag with existing name
        errorDescription.clear();

        QModelIndex thirteenthTagIndex = model->createTag(
            third.name().value(), QString{}, QString{}, errorDescription);

        if (thirteenthTagIndex.isValid()) {
            FAIL(
                "Was able to create tag with the same name as "
                << "the already existing one");
        }

        // The error description should say something about the inability to
        // create the tag
        if (errorDescription.isEmpty()) {
            FAIL(
                "The error description about the inability to "
                << "create a tag due to the name collision is empty");
        }

        // Should be able to create the tag with a new name
        QString thirteenthTagName = QStringLiteral("Thirteenth");
        errorDescription.clear();

        thirteenthTagIndex = model->createTag(
            thirteenthTagName, third.name().value(), QString{},
            errorDescription);

        if (!thirteenthTagIndex.isValid()) {
            FAIL(
                "Wasn't able to create a tag with the name not "
                << "present within the tag model");
        }

        // Should no longer be able to create the tag with the same name as
        // the just added one
        QModelIndex fourteenthTagIndex = model->createTag(
            thirteenthTagName, fourth.name().value(), QString{},
            errorDescription);

        if (fourteenthTagIndex.isValid()) {
            FAIL(
                "Was able to create a tag with the same name "
                << "as the just created tag");
        }

        // The error description should say something about the inability
        // to create the tag
        if (errorDescription.isEmpty()) {
            FAIL(
                "The error description about the inability "
                << "to create a tag due to the name collision is empty");
        }

        // Should be able to remove the just added tag
        res = model->removeRow(
            thirteenthTagIndex.row(), thirteenthTagIndex.parent());

        if (!res) {
            FAIL(
                "Wasn't able to remove the tag just added "
                << "to the tag model");
        }

        // Should again be able to create the tag with the same name
        errorDescription.clear();

        thirteenthTagIndex = model->createTag(
            thirteenthTagName, QString(), QString(), errorDescription);

        if (!thirteenthTagIndex.isValid()) {
            FAIL(
                "Wasn't able to create a tag with the same "
                << "name as the just removed one");
        }

        // Change the sort order and check the sorting again
        model->sort(
            static_cast<int>(TagModel::Column::Name), Qt::DescendingOrder);

        res = checkSorting(*model, allTagsRootItem, errorDescription);
        if (!res) {
            FAIL(
                "Sorting check failed for the tag model for "
                << "descending order: " << errorDescription);
        }

        // After expunging the tag being the parent for other tags, the child
        // tags should not be present within the model as well as the parent one
        {
            auto f = m_localStorage->expungeTagByLocalId(tenth.localId());

            waitForFuture(f);

            try {
                f.waitForFinished();
            }
            catch (const IQuentierException & e) {
                Q_EMIT failure(e.errorMessage());
                return;
            }
            catch (const QException & e) {
                Q_EMIT failure(ErrorString{e.what()});
                return;
            }
        }

        auto tenthIndex = model->indexForLocalId(tenth.localId());
        if (tenthIndex.isValid()) {
            FAIL(
                "The tag model returns valid index for "
                << "the local id corresponding to the tag expunged "
                << "from the local storage");
        }

        auto eleventhIndex = model->indexForLocalId(eleventh.localId());
        if (eleventhIndex.isValid()) {
            FAIL(
                "The tag model returns valid index for the local "
                << "id corresponding to the tag being the child "
                << "of a tag being expunged");
        }

        // Should be able to change the parent of the tag externally and have
        // the model recognize it
        const auto * newThirteenthModelItem =
            model->itemForIndex(thirteenthTagIndex);

        if (!newThirteenthModelItem) {
            FAIL("Can't find the tag model item corresponding to index");
        }

        const auto * newThirteenthTagItem =
            newThirteenthModelItem->cast<TagItem>();

        if (!newThirteenthTagItem) {
            FAIL("Unexpected null pointer to tag item");
        }

        qevercloud::Tag thirteenth;
        thirteenth.setLocalId(newThirteenthTagItem->localId());
        thirteenth.setGuid(newThirteenthTagItem->guid());
        thirteenth.setLocalOnly(!newThirteenthTagItem->isSynchronizable());
        thirteenth.setLocallyModified(newThirteenthTagItem->isDirty());

        // Reparent the thirteenth tag to the second tag
        thirteenth.setParentGuid(second.guid());
        thirteenth.setParentTagLocalId(second.localId());

        if (!putTag(thirteenth)) {
            return;
        }

        if (newThirteenthTagItem->parentLocalId() != second.localId()) {
            FAIL(
                "The parent local id of the externally updated "
                << "tag was not picked up from the updated tag");
        }

        const auto * secondTagItem = model->itemForLocalId(second.localId());
        if (!secondTagItem) {
            FAIL("Can't fint tag model item for local id");
        }

        if (secondTagItem->rowForChild(newThirteenthModelItem) < 0) {
            FAIL(
                "The new parent item doesn't contain the child "
                << "item which was externally updated");
        }

        Q_EMIT success();
        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
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

    if (model.sortOrder() == Qt::AscendingOrder) {
        std::sort(sortedChildren.begin(), sortedChildren.end(), LessByName());
    }
    else {
        std::sort(
            sortedChildren.begin(), sortedChildren.end(), GreaterByName());
    }

    bool res = (children == sortedChildren);
    if (!res) {
        errorDescription.setBase(
            "The list of child tags is not equal to the list of sorted child "
            "tags");

        errorDescription.details() =
            QStringLiteral("Root item: ") + pRootItem->toString();

        errorDescription.details() += QStringLiteral("\nChild tags: ");

        for (const auto * tagModelItem: qAsConst(children)) {
            if (!tagModelItem) {
                errorDescription.details() += QStringLiteral("<null>; ");
                continue;
            }

            const auto * tagItem = tagModelItem->cast<TagItem>();
            if (tagItem) {
                errorDescription.details() += QStringLiteral("tag: ");
                errorDescription.details() += tagItem->name();
                errorDescription.details() += QStringLiteral("; ");
                continue;
            }

            const auto * linkedNotebookItem =
                tagModelItem->cast<TagLinkedNotebookRootItem>();

            if (linkedNotebookItem) {
                errorDescription.details() +=
                    QStringLiteral("linked notebook: ");

                errorDescription.details() += linkedNotebookItem->username();
                errorDescription.details() += QStringLiteral("; ");
                continue;
            }

            errorDescription.details() += QStringLiteral("unknown: ");
            errorDescription.details() += tagModelItem->toString();
        }

        errorDescription.details() += QStringLiteral("\nSorted child tags: ");

        for (const auto * tagModelItem: qAsConst(sortedChildren)) {
            if (!tagModelItem) {
                errorDescription.details() += QStringLiteral("<null>; ");
                continue;
            }

            const auto * tagItem = tagModelItem->cast<TagItem>();
            if (tagItem) {
                errorDescription.details() += QStringLiteral("tag: ");
                errorDescription.details() += tagItem->name();
                errorDescription.details() += QStringLiteral("; ");
                continue;
            }

            const auto * linkedNotebookItem =
                tagModelItem->cast<TagLinkedNotebookRootItem>();

            if (linkedNotebookItem) {
                errorDescription.details() +=
                    QStringLiteral("linked notebook: ");

                errorDescription.details() += linkedNotebookItem->username();
                errorDescription.details() += QStringLiteral("; ");
                continue;
            }

            errorDescription.details() += QStringLiteral("unknown: ");
            errorDescription.details() += tagModelItem->toString();
        }

        return false;
    }

    for (const auto * pChildItem: qAsConst(children)) {
        res = checkSorting(model, pChildItem, errorDescription);
        if (!res) {
            return false;
        }
    }

    return true;
}

#define MODEL_ITEM_NAME(item, itemName)                                        \
    if (item->type() == ITagModelItem::Type::Tag) {                            \
        const auto * tagItem = item->cast<TagItem>();                          \
        if (tagItem) {                                                         \
            itemName = tagItem->nameUpper();                                   \
        }                                                                      \
    }                                                                          \
    else if (item->type() == ITagModelItem::Type::LinkedNotebook) {            \
        const auto * linkedNotebookItem =                                      \
            item->cast<TagLinkedNotebookRootItem>();                           \
        if (linkedNotebookItem) {                                              \
            itemName = linkedNotebookItem->username().toUpper();               \
        }                                                                      \
    }                                                                          \
    // MODEL_ITEM_NAME

bool TagModelTestHelper::LessByName::operator()(
    const ITagModelItem * lhs, const ITagModelItem * rhs) const noexcept
{
    if (lhs->type() == ITagModelItem::Type::AllTagsRoot &&
        rhs->type() != ITagModelItem::Type::AllTagsRoot)
    {
        return false;
    }

    if (lhs->type() != ITagModelItem::Type::AllTagsRoot &&
        rhs->type() == ITagModelItem::Type::AllTagsRoot)
    {
        return true;
    }

    if (lhs->type() == ITagModelItem::Type::LinkedNotebook &&
        rhs->type() != ITagModelItem::Type::LinkedNotebook)
    {
        return false;
    }

    if (lhs->type() != ITagModelItem::Type::LinkedNotebook &&
        rhs->type() == ITagModelItem::Type::LinkedNotebook)
    {
        return true;
    }

    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) <= 0);
}

bool TagModelTestHelper::GreaterByName::operator()(
    const ITagModelItem * lhs, const ITagModelItem * rhs) const noexcept
{
    if (lhs->type() == ITagModelItem::Type::AllTagsRoot &&
        rhs->type() != ITagModelItem::Type::AllTagsRoot)
    {
        return false;
    }

    if (lhs->type() != ITagModelItem::Type::AllTagsRoot &&
        rhs->type() == ITagModelItem::Type::AllTagsRoot)
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if (lhs->type() == ITagModelItem::Type::LinkedNotebook &&
        rhs->type() != ITagModelItem::Type::LinkedNotebook)
    {
        return false;
    }

    if (lhs->type() != ITagModelItem::Type::LinkedNotebook &&
        rhs->type() == ITagModelItem::Type::LinkedNotebook)
    {
        return true;
    }

    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) > 0);
}

} // namespace quentier
