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

#include "NotebookModelTestHelper.h"

#include "TestMacros.h"
#include "TestUtils.h"
#include "modeltest.h"

#include <lib/model/notebook/NotebookModel.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/UidGenerator.h>

#include <qevercloud/types/LinkedNotebook.h>
#include <qevercloud/types/Notebook.h>

namespace quentier {

NotebookModelTestHelper::NotebookModelTestHelper(
    local_storage::ILocalStoragePtr localStorage, QObject * parent) :
    QObject{parent},
    m_localStorage{std::move(localStorage)}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{
            ErrorString{"NotebookModelTestHelper ctor: local storage is null"}};
    }
}

NotebookModelTestHelper::~NotebookModelTestHelper() = default;

void NotebookModelTestHelper::test()
{
    QNDEBUG("tests:model_test:notebook", "NotebookModelTestHelper::test");

    ErrorString errorDescription;

    try {
        qevercloud::LinkedNotebook firstLinkedNotebook;
        firstLinkedNotebook.setGuid(UidGenerator::Generate());
        firstLinkedNotebook.setUsername(QStringLiteral("userone"));

        qevercloud::LinkedNotebook secondLinkedNotebook;
        secondLinkedNotebook.setGuid(UidGenerator::Generate());
        secondLinkedNotebook.setUsername(QStringLiteral("usertwo"));

        qevercloud::Notebook first;
        first.setName(QStringLiteral("First"));
        first.setLocalOnly(true);
        first.setLocallyModified(true);

        qevercloud::Notebook second;
        second.setName(QStringLiteral("Second"));
        second.setLocalOnly(true);
        second.setLocallyModified(false);
        second.setStack(QStringLiteral("Stack 1"));

        qevercloud::Notebook third;
        third.setName(QStringLiteral("Third"));
        third.setLocalOnly(false);
        third.setGuid(UidGenerator::Generate());
        third.setLocallyModified(true);
        third.setStack(QStringLiteral("Stack 1"));

        qevercloud::Notebook fourth;
        fourth.setName(QStringLiteral("Fourth"));
        fourth.setLocalOnly(false);
        fourth.setGuid(UidGenerator::Generate());
        fourth.setLocallyModified(false);
        fourth.setPublished(true);
        fourth.setStack(QStringLiteral("Stack 1"));

        qevercloud::Notebook fifth;
        fifth.setName(QStringLiteral("Fifth"));
        fifth.setLocalOnly(false);
        fifth.setGuid(UidGenerator::Generate());
        fifth.setLinkedNotebookGuid(firstLinkedNotebook.guid());
        fifth.setLocallyModified(false);
        fifth.setStack(QStringLiteral("Stack 1"));

        qevercloud::Notebook sixth;
        sixth.setName(QStringLiteral("Sixth"));
        sixth.setLocalOnly(false);
        sixth.setGuid(UidGenerator::Generate());
        sixth.setLocallyModified(false);

        qevercloud::Notebook seventh;
        seventh.setName(QStringLiteral("Seventh"));
        seventh.setLocalOnly(false);
        seventh.setGuid(UidGenerator::Generate());
        seventh.setLocallyModified(false);
        seventh.setPublished(true);

        qevercloud::Notebook eighth;
        eighth.setName(QStringLiteral("Eighth"));
        eighth.setLocalOnly(false);
        eighth.setGuid(UidGenerator::Generate());
        eighth.setLinkedNotebookGuid(secondLinkedNotebook.guid());
        eighth.setLocallyModified(false);

        qevercloud::Notebook ninth;
        ninth.setName(QStringLiteral("Ninth"));
        ninth.setLocalOnly(true);
        ninth.setLocallyModified(false);
        ninth.setStack(QStringLiteral("Stack 2"));

        qevercloud::Notebook tenth;
        tenth.setName(QStringLiteral("Tenth"));
        tenth.setLocalOnly(false);
        tenth.setGuid(UidGenerator::Generate());
        tenth.setLocallyModified(false);
        tenth.setStack(QStringLiteral("Stack 2"));

        {
            auto future = threading::whenAll(
                QList<QFuture<void>>{}
                << m_localStorage->putLinkedNotebook(firstLinkedNotebook)
                << m_localStorage->putLinkedNotebook(secondLinkedNotebook));

            waitForFuture(future);

            try {
                future.waitForFinished();
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

        {
            auto future = threading::whenAll(
                QList<QFuture<void>>{} << m_localStorage->putNotebook(first)
                                       << m_localStorage->putNotebook(second)
                                       << m_localStorage->putNotebook(third)
                                       << m_localStorage->putNotebook(fourth)
                                       << m_localStorage->putNotebook(fifth)
                                       << m_localStorage->putNotebook(sixth)
                                       << m_localStorage->putNotebook(seventh)
                                       << m_localStorage->putNotebook(eighth)
                                       << m_localStorage->putNotebook(ninth)
                                       << m_localStorage->putNotebook(tenth));

            waitForFuture(future);

            try {
                future.waitForFinished();
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

        NotebookCache cache{5};
        Account account{QStringLiteral("Default user"), Account::Type::Local};

        auto * model = new NotebookModel(account, m_localStorage, cache, this);

        ModelTest t1{model};
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        auto secondIndex = model->indexForLocalId(second.localId());
        if (!secondIndex.isValid()) {
            FAIL(
                "Can't get valid notebook model item index for local id "
                << second.localId());
        }

        auto secondParentIndex = model->parent(secondIndex);

        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(NotebookModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL(
                "Can't get valid notebook model item index for dirty column, "
                << "local id = " << second.localId());
        }

        bool res = model->setData(secondIndex, QVariant{true}, Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change dirty flag in the notebook "
                << "model manually which is not intended");
        }

        auto data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the notebook model "
                << "while expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL(
                "The dirty state appears to have changed after setData in "
                << "notebook model even though the method returned false");
        }

        // Should only be able to make the non-synchronizable (local) item
        // synchronizable (non-local) with non-local account
        // 1) Trying with local account
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(NotebookModel::Column::Synchronizable),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL(
                "Can't get valid notebook model item index for synchronizable "
                << "column");
        }

        res = model->setData(secondIndex, QVariant{true}, Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change synchronizable flag from false to "
                << "true for notebook model item with local account");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the notebook model "
                << "while expected to get the state of synchronizable flag");
        }

        if (data.toBool()) {
            FAIL(
                "Even though setData returned false on attempt to make "
                << "the notebook item synchronizable with local account, "
                << "the actual data within the model appears to have changed");
        }

        // 2) Trying the non-local account
        account = Account(
            QStringLiteral("Evernote user"), Account::Type::Evernote,
            qevercloud::UserID(1));

        model->setAccount(account);

        res = model->setData(secondIndex, QVariant{true}, Qt::EditRole);
        if (!res) {
            FAIL(
                "Wasn't able to change synchronizable flag from false to true "
                << "for notebook model item even with the account of Evernote "
                << "type");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the notebook model "
                << "while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The synchronizable state appears to have not changed after "
                << "setData in notebook model even though the method returned "
                << "true");
        }

        // Verify the dirty flag has changed as a result of making the item
        // synchronizable
        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(NotebookModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid notebook model item index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the notebook model "
                << "while expected to get the state of dirty flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The dirty state hasn't changed after making the notebook "
                << "model item synchronizable while it was expected to have "
                << "changed");
        }

        // Should not be able to make the synchronizable (non-local) item
        // non-synchronizable (local)
        secondIndex = model->index(
            secondIndex.row(),
            static_cast<int>(NotebookModel::Column::Synchronizable),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL(
                "Can't get valid notebook item model index "
                << "for synchronizable column");
        }

        res = model->setData(secondIndex, QVariant{false}, Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change synchronizable flag in the notebook model "
                << "from true to false which is not intended");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the notebook model "
                << "while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The synchronizable state appears to have changed "
                << "after setData in notebook model even though "
                << "the method returned false");
        }

        // Should be able to change name
        // But first clear the dirty flag from the tag to ensure it would be
        // automatically set when changing the name
        second.setLocalOnly(false);
        second.setLocallyModified(false);

        {
            auto future = m_localStorage->putNotebook(second);
            waitForFuture(future);

            try {
                future.waitForFinished();
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

        // Ensure the dirty flag was cleared
        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(NotebookModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid notebook item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the notebook model "
                << "while expected to get the dirty flag of model item");
        }

        if (data.toBool()) {
            FAIL(
                "The notebook model item is still dirty even "
                << "though this flag for this item "
                << "was updated in the local storage to false");
        }

        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(NotebookModel::Column::Name),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid notebook model item index for name column");
        }

        QString newName = QStringLiteral("Second (name modified)");
        res = model->setData(secondIndex, QVariant{newName}, Qt::EditRole);
        if (!res) {
            FAIL("Can't change the name of the notebook model item");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the notebook model "
                << "while expected to get the name of the tag item");
        }

        if (data.toString() != newName) {
            FAIL(
                "The name of the notebook item returned by "
                << "the model does not match the name just set to "
                << "this item: received " << data.toString() << ", expected "
                << newName);
        }

        // Ensure the dirty flag has changed to true
        secondIndex = model->index(
            secondIndex.row(), static_cast<int>(NotebookModel::Column::Dirty),
            secondParentIndex);

        if (!secondIndex.isValid()) {
            FAIL("Can't get valid notebook item model index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the notebook model "
                << "while expected to get the dirty flag of model item");
        }

        if (!data.toBool()) {
            FAIL(
                "The dirty flag appears to have not changed as a result of "
                << "changing the name of the notebook model item");
        }

        // Should not be able to remove the row with a synchronizable
        // (non-local) notebook
        auto thirdIndex = model->indexForLocalId(third.localId());
        if (!thirdIndex.isValid()) {
            FAIL("Can't get valid notebook model item index for dirty column");
        }

        auto thirdParentIndex = model->parent(thirdIndex);

        res = model->removeRow(thirdIndex.row(), thirdParentIndex);
        if (res) {
            FAIL(
                "Was able to remove the row corresponding to the notebook "
                << "with non-empty guid which is not intended");
        }

        auto thirdIndexAfterFailedRemoval =
            model->indexForLocalId(third.localId());

        if (!thirdIndexAfterFailedRemoval.isValid()) {
            FAIL(
                "Can't get the valid notebook model item index "
                << "after the failed row removal attempt");
        }

        if (thirdIndexAfterFailedRemoval.row() != thirdIndex.row()) {
            FAIL(
                "Notebook model returned item index with "
                << "a different row after the failed row removal attempt");
        }

        // Should be able to remove row with a non-synchronizable (local)
        // notebook
        auto firstIndex = model->indexForLocalId(first.localId());
        if (!firstIndex.isValid()) {
            FAIL("Can't get valid notebook model item index for local id");
        }

        auto firstParentIndex = model->parent(firstIndex);
        res = model->removeRow(firstIndex.row(), firstParentIndex);
        if (!res) {
            FAIL(
                "Can't remove row with a non-synchronizable notebook item "
                << "from the model");
        }

        auto firstIndexAfterRemoval = model->indexForLocalId(first.localId());
        if (firstIndexAfterRemoval.isValid()) {
            FAIL(
                "Was able to get valid model index for the removed notebook "
                << "item by local id which is not intended");
        }

        // Should be able to move the non-stacked item to the existing stack
        auto sixthIndex = model->indexForLocalId(sixth.localId());
        if (!sixthIndex.isValid()) {
            FAIL("Can't get valid notebook model item index for local id");
        }

        auto sixthIndexMoved =
            model->moveToStack(sixthIndex, fifth.stack().value());
        if (!sixthIndexMoved.isValid()) {
            FAIL(
                "Can't get valid notebook model item index from the method "
                << "intended to move the item to the stack");
        }

        const auto * sixthItem = model->itemForIndex(sixthIndexMoved);
        if (!sixthItem) {
            FAIL(
                "Can't get notebook model item moved to the stack from its "
                << "model index");
        }

        if (sixthItem->type() != INotebookModelItem::Type::Notebook) {
            FAIL(
                "Notebook model item has wrong type after being "
                << "moved to the stack: " << *sixthItem);
        }

        const auto * sixthNotebookItem = sixthItem->cast<NotebookItem>();
        if (!sixthNotebookItem) {
            FAIL(
                "Failed to cast notebook model item to notebook item "
                << "even though it's of notebook type");
        }

        if (sixthNotebookItem->stack() != fifth.stack()) {
            FAIL(
                "Notebook item's stack is not equal to the one "
                << "expected as the notebook model item was moved "
                << "to this stack: " << fifth.stack().value()
                << "; notebook item: " << *sixthNotebookItem);
        }

        const auto * sixthParentItem = sixthItem->parent();
        if (!sixthParentItem) {
            FAIL(
                "The notebook model item has null parent after "
                << "it has been moved to the existing stack");
        }

        if (sixthParentItem->type() != INotebookModelItem::Type::Stack) {
            FAIL(
                "The notebook model item has parent of non-stack "
                << "type after it has been moved to the existing stack");
        }

        const auto * sixthParentStackItem = sixthParentItem->cast<StackItem>();

        if (!sixthParentStackItem) {
            FAIL("Can't cast notebook model item of stack type to stack item");
        }

        if (sixthParentStackItem->name() != fifth.stack()) {
            FAIL(
                "The notebook model item has stack parent which "
                << "name doesn't correspond to the expected one "
                << "after that item has been moved to the existing stack");
        }

        // Ensure the item moved to another stack was marked as dirty
        if (!sixthNotebookItem->isDirty()) {
            FAIL(
                "The dirty flag hasn't been automatically set "
                << "to true after moving the notebook to another stack");
        }

        // Should be able to move the non-stacked item to the new stack
        const QString newStack = QStringLiteral("My brand new stack");

        auto seventhIndex = model->indexForLocalId(seventh.localId());
        if (!seventhIndex.isValid()) {
            FAIL(
                "Can't get the valid notebook model item index "
                << "from the method intended to move the item to the stack");
        }

        auto seventhIndexMoved = model->moveToStack(seventhIndex, newStack);
        if (!seventhIndexMoved.isValid()) {
            FAIL("Can't get the valid notebook model item index for local id");
        }

        const auto * seventhItem = model->itemForIndex(seventhIndexMoved);
        if (!seventhItem) {
            FAIL(
                "Can't get notebook model item moved to the stack from its "
                << "model index");
        }

        if (seventhItem->type() != INotebookModelItem::Type::Notebook) {
            FAIL(
                "Notebook model item has wrong type after being "
                << "moved to the stack: " << *seventhItem);
        }

        const auto * seventhNotebookItem = seventhItem->cast<NotebookItem>();
        if (!seventhNotebookItem) {
            FAIL(
                "Can't cast notebook model item to notebook item "
                << "even though it's of notebook type");
        }

        if (seventhNotebookItem->stack() != newStack) {
            FAIL(
                "Notebook item's stack is not equal to the one "
                << "expected as the notebook model item was moved "
                << "to the new stack: " << newStack
                << "; notebook item: " << *seventhNotebookItem);
        }

        const auto * seventhParentItem = seventhItem->parent();
        if (!seventhParentItem) {
            FAIL(
                "The notebook model item has null parent after "
                << "it has been moved to the new stack");
        }

        if (seventhParentItem->type() != INotebookModelItem::Type::Stack) {
            FAIL(
                "The notebook model item has parent of non-stack "
                << "type after it has been moved to the existing stack");
        }

        const auto * seventhParentStackItem =
            seventhParentItem->cast<StackItem>();

        if (!seventhParentStackItem) {
            FAIL(
                "The notebook model item has parent of stack type "
                << "but can't cast the parent item to StackItem after "
                << "the model item has been moved to the existing stack");
        }

        if (seventhParentStackItem->name() != newStack) {
            FAIL(
                "The notebook model item has stack parent which "
                << "name doesn't correspond to the expected one "
                << "after that item has been moved to the existing stack");
        }

        if (!seventhNotebookItem->isDirty()) {
            FAIL(
                "The dirty flag hasn't been automatically set to "
                << "true after moving the non-stacked notebook to a stack");
        }

        // Should be able to move items from one stack to the other one
        auto fourthIndex = model->indexForLocalId(fourth.localId());
        if (!fourthIndex.isValid()) {
            FAIL("Can't get valid notebook model item index for local id");
        }

        const auto * fourthItem = model->itemForIndex(fourthIndex);
        if (!fourthItem) {
            FAIL(
                "Can't get notebook model item for its index "
                << "in turn retrieved from the local id");
        }

        const auto * fourthParentItem = fourthItem->parent();
        if (!fourthParentItem) {
            FAIL("Detected notebook model item having null parent");
        }

        if (fourthParentItem->type() != INotebookModelItem::Type::Stack) {
            FAIL(
                "Detected notebook model item which unexpectedly "
                << "doesn't have a parent of stack type");
        }

        const auto * fourthParentStackItem =
            fourthParentItem->cast<StackItem>();

        if (!fourthParentStackItem) {
            FAIL(
                "Can't cast notebook model item to stack item even though it "
                << "is of stack type");
        }

        auto newFourthItemIndex = model->moveToStack(fourthIndex, newStack);
        if (!newFourthItemIndex.isValid()) {
            FAIL(
                "Can't get valid notebook model item index after the attempt "
                << "to move the item to another stack");
        }

        fourth.setStack(newStack);

        const auto * fourthItemFromNewIndex =
            model->itemForIndex(newFourthItemIndex);

        if (!fourthItemFromNewIndex) {
            FAIL(
                "Can't get notebook model item after moving it to another "
                << "stack");
        }

        if (fourthItem != fourthItemFromNewIndex) {
            FAIL(
                "The memory address of the notebook model item "
                << "appears to have changed as a result of moving "
                << "the item into another stack");
        }

        const auto * fourthItemNewParent = fourthItemFromNewIndex->parent();
        if (!fourthItemNewParent) {
            FAIL(
                "Notebook model item's parent has become null as "
                << "a result of moving the item to another stack");
        }

        if (fourthItemNewParent->type() != INotebookModelItem::Type::Stack) {
            FAIL(
                "Notebook model item's parent has non-stack type "
                << "after moving the item to another stack");
        }

        const auto * fourthItemNewStackItem =
            fourthItemNewParent->cast<StackItem>();

        if (!fourthItemNewStackItem) {
            FAIL(
                "Notebook model item's parent is of stack type "
                << "but can't cast it to stack item after moving the original "
                << "item to another stack");
        }

        if (fourthItemNewStackItem->name() != newStack) {
            FAIL(
                "Notebook model item's parent stack item has "
                << "unexpected name after the attempt to move "
                << "the item to another stack");
        }

        int row = fourthParentItem->rowForChild(fourthItem);
        if (row >= 0) {
            FAIL(
                "Notebook model item has been moved to another "
                << "stack but it can still be found within the old "
                << "parent item's children");
        }

        row = fourthItemNewParent->rowForChild(fourthItemFromNewIndex);
        if (row < 0) {
            FAIL(
                "Notebook model item has been moved to another "
                << "stack but its row within its new parent cannot be found");
        }

        const auto * fourthNotebookItem = fourthItem->cast<NotebookItem>();
        if (!fourthNotebookItem) {
            FAIL(
                "Can't cast notebook model item to notebook item after "
                << "moving the item to another stack");
        }

        if (!fourthNotebookItem->isDirty()) {
            FAIL(
                "The notebook item wasn't automatically marked as "
                << "dirty after moving it to another stack");
        }

        // Should be able to remove the notebook item from the stack
        // But first reset the dirty state of the notebook to ensure it would
        // be set automatically afterwards

        fourth.setLocallyModified(false);

        {
            auto future = m_localStorage->putNotebook(fourth);
            waitForFuture(future);

            try {
                future.waitForFinished();
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

        if (fourthNotebookItem->isDirty()) {
            FAIL(
                "The notebook item is still marked as dirty after "
                << "the flag has been cleared externally");
        }

        auto fourthIndexRemovedFromStack =
            model->removeFromStack(newFourthItemIndex);

        if (!fourthIndexRemovedFromStack.isValid()) {
            FAIL(
                "Can't get valid notebook model item index after the attempt "
                << "to remove the item from the stack");
        }

        const auto * fourthItemRemovedFromStack =
            model->itemForIndex(fourthIndexRemovedFromStack);

        if (!fourthItemRemovedFromStack) {
            FAIL(
                "Can't get the notebook model item from the index after "
                << "the item's removal from the stack");
        }

        if (fourthItemRemovedFromStack != fourthItemFromNewIndex) {
            FAIL(
                "The memory address of the notebook model item appears to "
                << "have changed as a result of removing the item from "
                << "the stack");
        }

        const auto * fourthItemNoStackParent =
            fourthItemRemovedFromStack->parent();

        if (!fourthItemNoStackParent) {
            FAIL(
                "Notebook model item has null parent after being "
                << "removed from the stack");
        }

        if (fourthItemNoStackParent == fourthItemNewParent) {
            FAIL(
                "The notebook model item's parent hasn't changed "
                << "as a result of removing the item from the stack");
        }

        if (fourthItemRemovedFromStack->type() !=
            INotebookModelItem::Type::Notebook) {
            FAIL(
                "The notebook model item's type has unexpectedly "
                << "changed after the item's removal from the stack");
        }

        const auto * fourthItemRemovedFromStackNotebookItem =
            fourthItemRemovedFromStack->cast<NotebookItem>();

        if (!fourthItemRemovedFromStackNotebookItem) {
            FAIL(
                "Can't cast the notebook model item to notebook item "
                << "after the item's removal from the stack");
        }

        if (!fourthItemRemovedFromStackNotebookItem->stack().isEmpty()) {
            FAIL(
                "The notebook item's stack is still not empty "
                << "after the model item has been removed from the stack");
        }

        if (!fourthItemRemovedFromStackNotebookItem->isDirty()) {
            FAIL(
                "The notebook item was not automatically marked "
                << "as dirty after removing the item from its stack");
        }

        // Should be able to remove the last item from the stack and thus cause
        // the stack item to disappear
        seventhIndexMoved = model->indexForLocalId(seventh.localId());
        if (!seventhIndexMoved.isValid()) {
            FAIL(
                "The model index of the notebook item is unexpectedly "
                << "invalid");
        }

        seventhIndexMoved = model->removeFromStack(seventhIndexMoved);
        if (!seventhIndexMoved.isValid()) {
            FAIL(
                "The model returned invalid model index after "
                << "the attempt to remove the last item from the stack");
        }

        seventhItem = model->itemForIndex(seventhIndexMoved);
        if (!seventhItem) {
            FAIL(
                "Can't get notebook model item after its "
                << "removal from the stack from model index");
        }

        if (seventhItem->type() != INotebookModelItem::Type::Notebook) {
            FAIL(
                "Notebook model item has wrong type after being "
                << "removed from its stack: " << *seventhItem);
        }

        seventhNotebookItem = seventhItem->cast<NotebookItem>();
        if (!seventhNotebookItem) {
            FAIL(
                "Can't cast notebook model item to notebook item even though "
                << "it's of notebook type");
        }

        if (!seventhNotebookItem->stack().isEmpty()) {
            FAIL(
                "The notebook item has non-empty stack after "
                << "the item's removal from the stack");
        }

        // Ensure the stack item ceased to exist after the last notebook item
        // has been removed from it
        auto emptyStackItemIndex = model->indexForNotebookStack(newStack, {});
        if (emptyStackItemIndex.isValid()) {
            FAIL(
                "Notebook model returned valid model index for "
                << "stack which should have been removed from "
                << "the model after the removal of the last notebook "
                << "contained in this stack");
        }

        // Should be able to change the stack of the notebook externally and
        // have the model handle it
        fourth.setStack(newStack);

        {
            auto future = m_localStorage->putNotebook(fourth);
            waitForFuture(future);

            try {
                future.waitForFinished();
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

        auto restoredStackItemIndex =
            model->indexForNotebookStack(newStack, {});

        if (!restoredStackItemIndex.isValid()) {
            FAIL(
                "Notebook model returned invalid model index for "
                << "stack which should have been created after "
                << "the external update of a notebook with this stack value");
        }

        newFourthItemIndex = model->indexForLocalId(fourth.localId());
        if (!newFourthItemIndex.isValid()) {
            FAIL(
                "Can't get valid notebook model item index "
                << "for notebook local id");
        }

        fourthItem = model->itemForIndex(newFourthItemIndex);
        if (!fourthItem) {
            FAIL("Can't get notebook model item for notebook local id");
        }

        fourthItemNewParent = fourthItem->parent();
        if (!fourthItemNewParent) {
            FAIL(
                "Notebook model item has no parent after "
                << "the external update of a notebook with another stack");
        }

        if (fourthItemNewParent->type() != INotebookModelItem::Type::Stack) {
            FAIL(
                "Notebook model item has parent of non-stack type even though "
                << "it should have stack parent");
        }

        fourthItemNewStackItem = fourthItemNewParent->cast<StackItem>();
        if (!fourthItemNewStackItem) {
            FAIL(
                "Can't cast notebook model item has to stack item even "
                << "though it is of stack type");
        }

        if (fourthItemNewStackItem->name() != newStack) {
            FAIL("The notebook stack item has wrong stack name");
        }

        // Set the account to local again to test accounting for notebook name
        // reservation in create/remove/create cycles
        account = Account{QStringLiteral("Local user"), Account::Type::Local};
        model->setAccount(account);

        // Should not be able to create the notebook with existing name
        ErrorString errorDescription;

        auto eleventhNotebookIndex = model->createNotebook(
            tenth.name().value(), tenth.stack().value(), errorDescription);

        if (eleventhNotebookIndex.isValid()) {
            FAIL(
                "Was able to create a notebook with the same name "
                << "as the already existing one");
        }

        // The error description should say something about the inability to
        // create the notebook
        if (errorDescription.isEmpty()) {
            FAIL(
                "The error description about the inability to "
                << "create a notebook due to the name collision is empty");
        }

        // Should be able to create the notebook with a new name
        QString eleventhNotebookName = QStringLiteral("Eleventh");
        errorDescription.clear();

        eleventhNotebookIndex = model->createNotebook(
            eleventhNotebookName, tenth.stack().value(), errorDescription);

        if (!eleventhNotebookIndex.isValid()) {
            FAIL(
                "Wasn't able to create a notebook with the name "
                << "not present within the notebook model");
        }

        // Should no longer be able to create the notebook with the same name
        // as the just added one
        QModelIndex twelvethNotebookIndex = model->createNotebook(
            eleventhNotebookName, fifth.stack().value(), errorDescription);

        if (twelvethNotebookIndex.isValid()) {
            FAIL(
                "Was able to create a notebook with the same "
                << "name as just created one");
        }

        // The error description should say something about the inability
        // to create the notebook
        if (errorDescription.isEmpty()) {
            FAIL(
                "The error description about the inability to "
                << "create a notebook due to the name collision is empty");
        }

        // Should be able to remove the just added local (non-synchronizable)
        // notebook
        res = model->removeRow(
            eleventhNotebookIndex.row(), eleventhNotebookIndex.parent());

        if (!res) {
            FAIL(
                "Wasn't able to remove the non-synchronizable "
                << "notebook just added to the notebook model");
        }

        // Should again be able to create the notebook with the same name
        errorDescription.clear();

        eleventhNotebookIndex = model->createNotebook(
            eleventhNotebookName, QString(), errorDescription);

        if (!eleventhNotebookIndex.isValid()) {
            FAIL(
                "Wasn't able to create a notebook with "
                << "the same name as the just removed one");
        }

        // Check the sorting for notebook and stack items: by default should
        // sort by name in ascending order
        res = checkSorting(*model, fourthItemNoStackParent);
        if (!res) {
            FAIL(
                "Sorting check failed for the notebook model "
                << "for ascending order");
        }

        // Change the sort order and check the sorting again
        model->sort(
            static_cast<int>(NotebookModel::Column::Name), Qt::DescendingOrder);

        res = checkSorting(*model, fourthItemNoStackParent);
        if (!res) {
            FAIL(
                "Sorting check failed for the notebook model "
                << "for descending order");
        }

        Q_EMIT success();
        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

bool NotebookModelTestHelper::checkSorting(
    const NotebookModel & model, const INotebookModelItem * rootItem) const
{
    if (!rootItem) {
        QNWARNING(
            "tests:model_test:notebook",
            "Found null pointer to notebook "
                << "model item when checking the sorting");
        return false;
    }

    auto children = rootItem->children();
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
        return false;
    }

    for (auto it = children.begin(), end = children.end(); it != end; ++it) {
        const auto * pChild = *it;
        res = checkSorting(model, pChild);
        if (!res) {
            return false;
        }
    }

    return true;
}

QString itemName(const INotebookModelItem * item)
{
    const auto * pNotebookItem = item->cast<NotebookItem>();
    if (pNotebookItem) {
        return pNotebookItem->name();
    }

    const auto * stackItem = item->cast<StackItem>();
    if (stackItem) {
        return stackItem->name();
    }

    const auto * linkedNotebookItem = item->cast<LinkedNotebookRootItem>();
    if (linkedNotebookItem) {
        return linkedNotebookItem->username();
    }

    return {};
}

bool NotebookModelTestHelper::LessByName::operator()(
    const INotebookModelItem * lhs,
    const INotebookModelItem * rhs) const noexcept
{
    if ((lhs->type() == INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs->type() != INotebookModelItem::Type::AllNotebooksRoot))
    {
        return false;
    }
    else if (
        (lhs->type() != INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs->type() == INotebookModelItem::Type::AllNotebooksRoot))
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((lhs->type() == INotebookModelItem::Type::LinkedNotebook) &&
        (rhs->type() != INotebookModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if (
        (lhs->type() != INotebookModelItem::Type::LinkedNotebook) &&
        (rhs->type() == INotebookModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    auto leftName = itemName(lhs);
    auto rightName = itemName(rhs);
    return (leftName.localeAwareCompare(rightName) <= 0);
}

bool NotebookModelTestHelper::GreaterByName::operator()(
    const INotebookModelItem * lhs,
    const INotebookModelItem * rhs) const noexcept
{
    if ((lhs->type() == INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs->type() != INotebookModelItem::Type::AllNotebooksRoot))
    {
        return false;
    }
    else if (
        (lhs->type() != INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs->type() == INotebookModelItem::Type::AllNotebooksRoot))
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((lhs->type() == INotebookModelItem::Type::LinkedNotebook) &&
        (rhs->type() != INotebookModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if (
        (lhs->type() != INotebookModelItem::Type::LinkedNotebook) &&
        (rhs->type() == INotebookModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    auto leftName = itemName(lhs);
    auto rightName = itemName(rhs);
    return (leftName.localeAwareCompare(rightName) > 0);
}

} // namespace quentier
