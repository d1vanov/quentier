/*
 * Copyright 2016-2019 Dmitry Ivanov
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
#include "modeltest.h"
#include "TestMacros.h"

#include <lib/model/NotebookModel.h>

#include <quentier/utility/SysInfo.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/UidGenerator.h>
#include <quentier/exception/IQuentierException.h>

namespace quentier {

NotebookModelTestHelper::NotebookModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerAsync(pLocalStorageManagerAsync)
{
    QObject::connect(pLocalStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(NotebookModelTestHelper,onAddNotebookFailed,
                            Notebook,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(NotebookModelTestHelper,onUpdateNotebookFailed,
                            Notebook,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(NotebookModelTestHelper,onFindNotebookFailed,
                            Notebook,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listNotebooksFailed,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListNotebooksOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,ErrorString,QUuid),
                     this,
                     QNSLOT(NotebookModelTestHelper,onListNotebooksFailed,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListNotebooksOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(NotebookModelTestHelper,onExpungeNotebookFailed,
                            Notebook,ErrorString,QUuid));
}

void NotebookModelTestHelper::test()
{
    QNDEBUG("NotebookModelTestHelper::test");

    ErrorString errorDescription;

    try {
        LinkedNotebook firstLinkedNotebook;
        firstLinkedNotebook.setGuid(UidGenerator::Generate());
        firstLinkedNotebook.setUsername(QStringLiteral("userone"));

        LinkedNotebook secondLinkedNotebook;
        secondLinkedNotebook.setGuid(UidGenerator::Generate());
        secondLinkedNotebook.setUsername(QStringLiteral("usertwo"));

        Notebook first;
        first.setName(QStringLiteral("First"));
        first.setLocal(true);
        first.setDirty(true);

        Notebook second;
        second.setName(QStringLiteral("Second"));
        second.setLocal(true);
        second.setDirty(false);
        second.setStack(QStringLiteral("Stack 1"));

        Notebook third;
        third.setName(QStringLiteral("Third"));
        third.setLocal(false);
        third.setGuid(UidGenerator::Generate());
        third.setDirty(true);
        third.setStack(QStringLiteral("Stack 1"));

        Notebook fourth;
        fourth.setName(QStringLiteral("Fourth"));
        fourth.setLocal(false);
        fourth.setGuid(UidGenerator::Generate());
        fourth.setDirty(false);
        fourth.setPublished(true);
        fourth.setStack(QStringLiteral("Stack 1"));

        Notebook fifth;
        fifth.setName(QStringLiteral("Fifth"));
        fifth.setLocal(false);
        fifth.setGuid(UidGenerator::Generate());
        fifth.setLinkedNotebookGuid(firstLinkedNotebook.guid());
        fifth.setDirty(false);
        fifth.setStack(QStringLiteral("Stack 1"));

        Notebook sixth;
        sixth.setName(QStringLiteral("Sixth"));
        sixth.setLocal(false);
        sixth.setGuid(UidGenerator::Generate());
        sixth.setDirty(false);

        Notebook seventh;
        seventh.setName(QStringLiteral("Seventh"));
        seventh.setLocal(false);
        seventh.setGuid(UidGenerator::Generate());
        seventh.setDirty(false);
        seventh.setPublished(true);

        Notebook eighth;
        eighth.setName(QStringLiteral("Eighth"));
        eighth.setLocal(false);
        eighth.setGuid(UidGenerator::Generate());
        eighth.setLinkedNotebookGuid(secondLinkedNotebook.guid());
        eighth.setDirty(false);

        Notebook ninth;
        ninth.setName(QStringLiteral("Ninth"));
        ninth.setLocal(true);
        ninth.setDirty(false);
        ninth.setStack(QStringLiteral("Stack 2"));

        Notebook tenth;
        tenth.setName(QStringLiteral("Tenth"));
        tenth.setLocal(false);
        tenth.setGuid(UidGenerator::Generate());
        tenth.setDirty(false);
        tenth.setStack(QStringLiteral("Stack 2"));

        m_pLocalStorageManagerAsync->onAddLinkedNotebookRequest(
            firstLinkedNotebook,
            QUuid());
        m_pLocalStorageManagerAsync->onAddLinkedNotebookRequest(
            secondLinkedNotebook,
            QUuid());

#define ADD_NOTEBOOK(notebook)                                                 \
        m_pLocalStorageManagerAsync->onAddNotebookRequest(notebook, QUuid())   \
// ADD_NOTEBOOK

        // NOTE: exploiting the direct connection used in current test
        // environment: after the following lines the local storage would be
        // filled with test objects
        ADD_NOTEBOOK(first);
        ADD_NOTEBOOK(second);
        ADD_NOTEBOOK(third);
        ADD_NOTEBOOK(fourth);
        ADD_NOTEBOOK(fifth);
        ADD_NOTEBOOK(sixth);
        ADD_NOTEBOOK(seventh);
        ADD_NOTEBOOK(eighth);
        ADD_NOTEBOOK(ninth);
        ADD_NOTEBOOK(tenth);

#undef ADD_NOTEBOOK

        NotebookCache cache(5);
        Account account(QStringLiteral("Default user"), Account::Type::Local);

        NotebookModel * model =
            new NotebookModel(account, *m_pLocalStorageManagerAsync, cache, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "for local uid");
        }

        QModelIndex secondParentIndex = model->parent(secondIndex);
        secondIndex = model->index(secondIndex.row(),
                                   NotebookModel::Columns::Dirty,
                                   secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "for dirty column");
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the dirty flag in the notebook "
                 "model manually which is not intended");
        }

        QVariant data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the notebook model "
                 "while expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL("The dirty state appears to have changed after setData in "
                 "notebook model even though the method returned false");
        }

        // Should only be able to make the non-synchronizable (local) item
        // synchronizable (non-local) with non-local account
        // 1) Trying with local account
        secondIndex = model->index(secondIndex.row(),
                                   NotebookModel::Columns::Synchronizable,
                                   secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "for synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the synchronizable flag from false to "
                 "true for notebook model item with local account");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the notebook model "
                 "while expected to get the state of synchronizable flag");
        }

        if (data.toBool()) {
            FAIL("Even though setData returned false on attempt "
                 "to make the notebook item synchronizable "
                 "with the local account, the actual data within "
                 "the model appears to have changed");
        }

        // 2) Trying the non-local account
        account = Account(QStringLiteral("Evernote user"),
                          Account::Type::Evernote,
                          qevercloud::UserID(1));
        model->updateAccount(account);

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL("Wasn't able to change the synchronizable flag "
                 "from false to true for notebook model item "
                 "even with the account of Evernote type");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the notebook model "
                 "while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have not "
                 "changed after setData in notebook model "
                 "even though the method returned true");
        }

        // Verify the dirty flag has changed as a result of making the item
        // synchronizable
        secondIndex = model->index(secondIndex.row(),
                                   NotebookModel::Columns::Dirty,
                                   secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the notebook model "
                 "while expected to get the state of dirty flag");
        }

        if (!data.toBool()) {
            FAIL("The dirty state hasn't changed after making "
                 "the notebook model item synchronizable while "
                 "it was expected to have changed");
        }

        // Should not be able to make the synchronizable (non-local) item
        // non-synchronizable (local)
        secondIndex = model->index(secondIndex.row(),
                                   NotebookModel::Columns::Synchronizable,
                                   secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid notebook item model index "
                 "for synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the synchronizable flag in "
                 "notebook model from true to false which is not "
                 "intended");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the notebook model "
                 "while expected to get the state of "
                 "synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have changed "
                 "after setData in notebook model even though "
                 "the method returned false");
        }

        // Should be able to change name
        // But first clear the dirty flag from the tag to ensure it would be
        // automatically set when changing the name

        second.setLocal(false);
        second.setDirty(false);
        m_pLocalStorageManagerAsync->onUpdateNotebookRequest(second, QUuid());

        // Ensure the dirty flag was cleared
        secondIndex = model->index(secondIndex.row(),
                                   NotebookModel::Columns::Dirty,
                                   secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid notebook item model index "
                 "for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the notebook model "
                 "while expected to get the dirty flag of model item");
        }

        if (data.toBool()) {
            FAIL("The notebook model item is still dirty even "
                 "though this flag for this item "
                 "was updated in the local storage to false");
        }

        secondIndex = model->index(secondIndex.row(),
                                   NotebookModel::Columns::Name,
                                   secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "for name column");
        }

        QString newName = QStringLiteral("Second (name modified)");
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            FAIL("Can't change the name of the notebook model item");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the notebook model "
                 "while expected to get the name of the tag item");
        }

        if (data.toString() != newName) {
            FAIL("The name of the notebook item returned by "
                 << "the model does not match the name just set to "
                 << "this item: received " << data.toString() << ", expected "
                 << newName);
        }

        // Ensure the dirty flag has changed to true
        secondIndex = model->index(secondIndex.row(),
                                   NotebookModel::Columns::Dirty,
                                   secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid notebook item model index "
                 "for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the notebook model "
                 "while expected to get the dirty flag of model "
                 "item");
        }

        if (!data.toBool()) {
            FAIL("The dirty flag appears to not have changed as "
                 "a result of changing the name of the notebook "
                 "model item");
        }

        // Should not be able to remove the row with a synchronizable (non-local)
        // notebook
        QModelIndex thirdIndex = model->indexForLocalUid(third.localUid());
        if (!thirdIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index for dirty column");
        }

        QModelIndex thirdParentIndex = model->parent(thirdIndex);

        res = model->removeRow(thirdIndex.row(), thirdParentIndex);
        if (res) {
            FAIL("Was able to remove the row corresponding to "
                 "the notebook with non-empty guid which is not "
                 "intended");
        }

        QModelIndex thirdIndexAfterFailedRemoval =
            model->indexForLocalUid(third.localUid());
        if (!thirdIndexAfterFailedRemoval.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "after the failed row removal attempt");
        }

        if (thirdIndexAfterFailedRemoval.row() != thirdIndex.row()) {
            FAIL("Notebook model returned item index with "
                 "a different row after the failed row removal attempt");
        }

        // Should be able to remove row with a non-synchronizable (local) notebook
        QModelIndex firstIndex = model->indexForLocalUid(first.localUid());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "for local uid");
        }

        QModelIndex firstParentIndex = model->parent(firstIndex);
        res = model->removeRow(firstIndex.row(), firstParentIndex);
        if (!res) {
            FAIL("Can't remove the row with a non-synchronizable "
                 "notebook item from the model");
        }

        QModelIndex firstIndexAfterRemoval = model->indexForLocalUid(first.localUid());
        if (firstIndexAfterRemoval.isValid()) {
            FAIL("Was able to get the valid model index for "
                 "the removed notebook item by local uid which is not intended");
        }

        // Should be able to move the non-stacked item to the existing stack
        QModelIndex sixthIndex = model->indexForLocalUid(sixth.localUid());
        if (!sixthIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "for local uid");
        }

        QModelIndex sixthIndexMoved = model->moveToStack(sixthIndex, fifth.stack());
        if (!sixthIndexMoved.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "from the method intended to move the item to the stack");
        }

        const NotebookModelItem * sixthItem = model->itemForIndex(sixthIndexMoved);
        if (!sixthItem) {
            FAIL("Can't get the notebook model item moved to "
                 "the stack from its model index");
        }

        if (sixthItem->type() != NotebookModelItem::Type::Notebook) {
            FAIL("Notebook model item has wrong type after being "
                 << "moved to the stack: " << *sixthItem);
        }

        const NotebookItem * sixthNotebookItem = sixthItem->notebookItem();
        if (!sixthNotebookItem) {
            FAIL("Notebook model item has null pointer to "
                 "the notebook item even though it's of notebook type");
        }

        if (sixthNotebookItem->stack() != fifth.stack()) {
            FAIL("Notebook item's stack is not equal to the one "
                 << "expected as the notebook model item was moved "
                 << "to this stack: " << fifth.stack() << "; notebook item: "
                 << *sixthNotebookItem);
        }

        const NotebookModelItem * sixthParentItem = sixthItem->parent();
        if (!sixthParentItem) {
            FAIL("The notebook model item has null parent after "
                 "it has been moved to the existing stack");
        }

        if (sixthParentItem->type() != NotebookModelItem::Type::Stack) {
            FAIL("The notebook model item has parent of non-stack "
                 "type after it has been moved to the existing stack");
        }

        const NotebookStackItem * sixthParentStackItem =
            sixthParentItem->notebookStackItem();
        if (!sixthParentStackItem) {
            FAIL("The notebook model item has parent of stack type "
                 "but with null pointer to the stack item after "
                 "the model item has been moved to the existing stack");
        }

        if (sixthParentStackItem->name() != fifth.stack()) {
            FAIL("The notebook model item has stack parent which "
                 "name doesn't correspond to the expected one "
                 "after that item has been moved to the existing stack");
        }

        // Ensure the item moved to another stack was marked as dirty
        if (!sixthNotebookItem->isDirty()) {
            FAIL("The dirty flag hasn't been automatically set "
                 "to true after moving the notebook to another stack");
        }

        // Should be able to move the non-stacked item to the new stack
        const QString newStack = QStringLiteral("My brand new stack");

        QModelIndex seventhIndex = model->indexForLocalUid(seventh.localUid());
        if (!seventhIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "from the method intended to move the item to the stack");
        }

        QModelIndex seventhIndexMoved = model->moveToStack(seventhIndex, newStack);
        if (!seventhIndexMoved.isValid()) {
            FAIL("Can't get the valid notebook model item index for local uid");
        }

        const NotebookModelItem * seventhItem = model->itemForIndex(seventhIndexMoved);
        if (!seventhItem) {
            FAIL("Can't get the notebook model item moved to "
                 "the stack from its model index");
        }

        if (seventhItem->type() != NotebookModelItem::Type::Notebook) {
            FAIL("Notebook model item has wrong type after being "
                 << "moved to the stack: " << *seventhItem);
        }

        const NotebookItem * seventhNotebookItem = seventhItem->notebookItem();
        if (!seventhNotebookItem) {
            FAIL("Notebook model item has null pointer to "
                 "the notebook item even though it's of notebook type");
        }

        if (seventhNotebookItem->stack() != newStack) {
            FAIL("Notebook item's stack is not equal to the one "
                 << "expected as the notebook model item was moved "
                 << "to the new stack: " << newStack << "; notebook item: "
                 << *seventhNotebookItem);
        }

        const NotebookModelItem * seventhParentItem = seventhItem->parent();
        if (!seventhParentItem) {
            FAIL("The notebook model item has null pointer after "
                 "it has been moved to the new stack");
        }

        if (seventhParentItem->type() != NotebookModelItem::Type::Stack) {
            FAIL("The notebook model item has parent of non-stack "
                 "type after it has been moved to the existing stack");
        }

        const NotebookStackItem * seventhParentStackItem =
            seventhParentItem->notebookStackItem();
        if (!seventhParentStackItem) {
            FAIL("The notebook model item has parent of stack type "
                 "but with null pointer to the stack item after "
                 "the model item has been moved to the existing stack");
        }

        if (seventhParentStackItem->name() != newStack) {
            FAIL("The notebook model item has stack parent which "
                 "name doesn't correspond to the expected one "
                 "after that item has been moved to the existing stack");
        }

        if (!seventhNotebookItem->isDirty()) {
            FAIL("The dirty flag hasn't been automatically set to "
                 "true after moving the non-stacked notebook to a stack");
        }

        // Should be able to move items from one stack to the other one
        QModelIndex fourthIndex = model->indexForLocalUid(fourth.localUid());
        if (!fourthIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index for local uid");
        }

        const NotebookModelItem * fourthItem = model->itemForIndex(fourthIndex);
        if (!fourthItem) {
            FAIL("Can't get the notebook model item for its index "
                 "in turn retrieved from the local uid");
        }

        const NotebookModelItem * fourthParentItem = fourthItem->parent();
        if (!fourthParentItem) {
            FAIL("Detected notebook model item having null parent");
        }

        if (fourthParentItem->type() != NotebookModelItem::Type::Stack) {
            FAIL("Detected notebook model item which unexpectedly "
                 "doesn't have a parent of stack type");
        }

        const NotebookStackItem * fourthParentStackItem =
            fourthParentItem->notebookStackItem();
        if (!fourthParentStackItem) {
            FAIL("Detected notebook model item which parent of "
                 "stack type has null pointer to the stack item");
        }

        QModelIndex newFourthItemIndex = model->moveToStack(fourthIndex, newStack);
        if (!newFourthItemIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "after the attempt to move the item to another stack");
        }

        fourth.setStack(newStack);

        const NotebookModelItem * fourthItemFromNewIndex =
            model->itemForIndex(newFourthItemIndex);
        if (!fourthItemFromNewIndex) {
            FAIL("Can't get the notebook model item after moving "
                 "it to another stack");
        }

        if (fourthItem != fourthItemFromNewIndex) {
            FAIL("The memory address of the notebook model item "
                 "appears to have changes as a result of moving "
                 "the item into another stack");
        }

        const NotebookModelItem * fourthItemNewParent =
            fourthItemFromNewIndex->parent();
        if (!fourthItemNewParent) {
            FAIL("Notebook model item's parent has become null as "
                 "a result of moving the item to another stack");
        }

        if (fourthItemNewParent->type() != NotebookModelItem::Type::Stack) {
            FAIL("Notebook model item's parent has non-stack type "
                 "after moving the item to another stack");
        }

        const NotebookStackItem * fourthItemNewStackItem =
            fourthItemNewParent->notebookStackItem();
        if (!fourthItemNewStackItem) {
            FAIL("Notebook model item's parent is of stack type "
                 "but has null pointer to the stack item after "
                 "moving the original item to another stack");
        }

        if (fourthItemNewStackItem->name() != newStack) {
            FAIL("Notebook model item's parent stack item has "
                 "unexpected name after the attempt to move "
                 "the item to another stack");
        }

        int row = fourthParentItem->rowForChild(fourthItem);
        if (row >= 0) {
            FAIL("Notebook model item has been moved to another "
                 "stack but it can still be found within the old "
                 "parent item's children");
        }

        row = fourthItemNewParent->rowForChild(fourthItemFromNewIndex);
        if (row < 0) {
            FAIL("Notebook model item has been moved to another "
                 "stack but its row within its new parent cannot be found");
        }

        if (!fourthItem->notebookItem()) {
            FAIL("The notebook model item doesn't have the notebook "
                 "item after moving to another stack");
        }

        if (!fourthItem->notebookItem()->isDirty()) {
            FAIL("The notebook item wasn't automatically marked as "
                 "dirty after moving it to another stack");
        }

        // Should be able to remove the notebook item from the stack
        // But first reset the dirty state of the notebook to ensure it would
        // be set automatically afterwards

        fourth.setDirty(false);
        m_pLocalStorageManagerAsync->onUpdateNotebookRequest(fourth, QUuid());

        if (fourthItem->notebookItem()->isDirty()) {
            FAIL("The notebook item is still marked as dirty after "
                 "the flag has been cleared externally");
        }

        QModelIndex fourthIndexRemovedFromStack =
            model->removeFromStack(newFourthItemIndex);
        if (!fourthIndexRemovedFromStack.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "after the attempt to remove the item from the stack");
        }

        const NotebookModelItem * fourthItemRemovedFromStack =
            model->itemForIndex(fourthIndexRemovedFromStack);
        if (!fourthItemRemovedFromStack) {
            FAIL("Can't get the pointer to the notebook model item "
                 "from the index after the item's removal from the stack");
        }

        if (fourthItemRemovedFromStack != fourthItemFromNewIndex) {
            FAIL("The memory address of the notebook model item "
                 "appears to have changed as a result of removing "
                 "the item from the stack");
        }

        const NotebookModelItem * fourthItemNoStackParent =
            fourthItemRemovedFromStack->parent();
        if (!fourthItemNoStackParent) {
            FAIL("Notebook model item has null parent after being "
                 "removed from the stack");
        }

        if (fourthItemNoStackParent == fourthItemNewParent) {
            FAIL("The notebook model item's parent hasn't changed "
                 "as a result of removing the item from the stack");
        }

        if (fourthItemRemovedFromStack->type() != NotebookModelItem::Type::Notebook) {
            FAIL("The notebook model item's type has unexpectedly "
                 "changed after the item's removal from the stack");
        }

        const NotebookItem * fourthItemRemovedFromStackNotebookItem =
            fourthItemRemovedFromStack->notebookItem();
        if (!fourthItemRemovedFromStackNotebookItem) {
            FAIL("The notebook model item's notebook item is "
                 "unexpectedly null after the item's removal "
                 "from the stack");
        }

        if (!fourthItemRemovedFromStackNotebookItem->stack().isEmpty()) {
            FAIL("The notebook item's stack is still not empty "
                 "after the model item has been removed from the stack");
        }

        if (!fourthItemRemovedFromStackNotebookItem->isDirty()) {
            FAIL("The notebook item was not automatically marked "
                 "as dirty after removing the item from its stack");
        }

        // Should be able to remove the last item from the stack and thus cause
        // the stack item to disappear
        seventhIndexMoved = model->indexForLocalUid(seventh.localUid());
        if (!seventhIndexMoved.isValid()) {
            FAIL("The model index of the notebook item is "
                 "unexpectedly invalid");
        }

        seventhIndexMoved = model->removeFromStack(seventhIndexMoved);
        if (!seventhIndexMoved.isValid()) {
            FAIL("The model returned invalid model index after "
                 "the attempt to remove the last item from the stack");
        }

        seventhItem = model->itemForIndex(seventhIndexMoved);
        if (!seventhItem) {
            FAIL("Can't get the notebook model item after its "
                 "removal from the stack from model index");
        }

        if (seventhItem->type() != NotebookModelItem::Type::Notebook) {
            FAIL("Notebook model item has wrong type after being "
                 << "removed from its stack: " << *seventhItem);
        }

        seventhNotebookItem = seventhItem->notebookItem();
        if (!seventhNotebookItem) {
            FAIL("Notebook model item has null pointer to "
                 "the notebook item even though it's of notebook type");
        }

        if (!seventhNotebookItem->stack().isEmpty()) {
            FAIL("Notebook item has non-empty stack after "
                 "the item's removal from the stack");
        }

        // Ensure the stack item ceased to exist after the last notebook item
        // has been removed from it
        QModelIndex emptyStackItemIndex =
            model->indexForNotebookStack(newStack, QString());
        if (emptyStackItemIndex.isValid()) {
            FAIL("Notebook model returned valid model index for "
                 "stack which should have been removed from "
                 "the model after the removal of the last notebook "
                 "contained in this stack");
        }

        // Should be able to change the stack of the notebook externally and have
        // the model handle it
        fourth.setStack(newStack);
        m_pLocalStorageManagerAsync->onUpdateNotebookRequest(fourth, QUuid());

        QModelIndex restoredStackItemIndex =
            model->indexForNotebookStack(newStack, QString());
        if (!restoredStackItemIndex.isValid()) {
            FAIL("Notebook model returned invalid model index for "
                 "stack which should have been created after "
                 "the external update of notebook with this stack value");
        }

        newFourthItemIndex = model->indexForLocalUid(fourth.localUid());
        if (!newFourthItemIndex.isValid()) {
            FAIL("Can't get the valid notebook model item index "
                 "for notebook local uid");
        }

        fourthItem = model->itemForIndex(newFourthItemIndex);
        if (!fourthItem) {
            FAIL("Can't get the notebook model item for notebook local uid");
        }

        fourthItemNewParent = fourthItem->parent();
        if (!fourthItemNewParent) {
            FAIL("Notebook model item has no parent after "
                 "the external update of notebook with another stack");
        }

        if (!fourthItemNewParent->notebookStackItem()) {
            FAIL("The notebook model item has no stack item even "
                 "though it should have one");
        }

        if (fourthItemNewParent->notebookStackItem()->name() != newStack) {
            FAIL("The notebook stack item has wrong stack name");
        }

        // Set the account to local again to test accounting for notebook name
        // reservation in create/remove/create cycles
        account = Account(QStringLiteral("Local user"), Account::Type::Local);
        model->updateAccount(account);

        // Should not be able to create the notebook with existing name
        ErrorString errorDescription;
        QModelIndex eleventhNotebookIndex = model->createNotebook(tenth.name(),
                                                                  tenth.stack(),
                                                                  errorDescription);
        if (eleventhNotebookIndex.isValid()) {
            FAIL("Was able to create notebook with the same name "
                 "as the already existing one");
        }

        // The error description should say something about the inability to
        // create the notebook
        if (errorDescription.isEmpty()) {
            FAIL("The error description about the inability to "
                 "create the notebook due to name collision is empty");
        }

        // Should be able to create the notebook with a new name
        QString eleventhNotebookName = QStringLiteral("Eleventh");
        errorDescription.clear();
        eleventhNotebookIndex = model->createNotebook(eleventhNotebookName,
                                                      tenth.stack(),
                                                      errorDescription);
        if (!eleventhNotebookIndex.isValid()) {
            FAIL("Wasn't able to create a notebook with the name "
                 "not present within the notebook model");
        }

        // Should no longer be able to create the notebook with the same name
        // as the just added one
        QModelIndex twelvethNotebookIndex =
            model->createNotebook(eleventhNotebookName, fifth.stack(),
                                  errorDescription);
        if (twelvethNotebookIndex.isValid()) {
            FAIL("Was able to create a notebook with the same "
                 "name as just created one");
        }

        // The error description should say something about the inability
        // to create the notebook
        if (errorDescription.isEmpty()) {
            FAIL("The error description about the inability to "
                 "create the notebook due to name collision is empty");
        }

        // Should be able to remove the just added local (non-synchronizable) notebook
        res = model->removeRow(eleventhNotebookIndex.row(),
                               eleventhNotebookIndex.parent());
        if (!res) {
            FAIL("Wasn't able to remove the non-synchronizable "
                 "notebook just added to the notebook model");
        }

        // Should again be able to create the notebook with the same name
        errorDescription.clear();
        eleventhNotebookIndex = model->createNotebook(eleventhNotebookName,
                                                      QString(),
                                                      errorDescription);
        if (!eleventhNotebookIndex.isValid()) {
            FAIL("Wasn't able to create the notebook with "
                 "the same name as the just removed one");
        }

        // Check the sorting for notebook and stack items: by default should
        // sort by name in ascending order
        res = checkSorting(*model, fourthItemNoStackParent);
        if (!res) {
            FAIL("Sorting check failed for the notebook model "
                 "for ascending order");
        }

        // Change the sort order and check the sorting again
        model->sort(NotebookModel::Columns::Name, Qt::DescendingOrder);

        res = checkSorting(*model, fourthItemNoStackParent);
        if (!res) {
            FAIL("Sorting check failed for the notebook model "
                 "for descending order");
        }

        Q_EMIT success();
        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void NotebookModelTestHelper::onAddNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onAddNotebookFailed: "
            << "notebook = " << notebook
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NotebookModelTestHelper::onUpdateNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onUpdateNotebookFailed: "
            << "notebook = " << notebook
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NotebookModelTestHelper::onFindNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onFindNotebookFailed: "
            << "notebook = " << notebook
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NotebookModelTestHelper::onListNotebooksFailed(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onListNotebooksFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = " << (linkedNotebookGuid.isNull()
                ? QStringLiteral("<null>")
                : linkedNotebookGuid)
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NotebookModelTestHelper::onExpungeNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG("NotebookModelTestHelper::onExpungeNotebookFailed: "
            << "notebook = " << notebook
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

bool NotebookModelTestHelper::checkSorting(
    const NotebookModel & model, const NotebookModelItem * rootItem) const
{
    if (!rootItem) {
        QNWARNING("Found null pointer to notebook model item "
                  "when checking the sorting");
        return false;
    }

    QList<const NotebookModelItem*> children = rootItem->children();
    if (children.isEmpty()) {
        return true;
    }

    QList<const NotebookModelItem*> sortedChildren = children;

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
        const NotebookModelItem * child = *it;
        res = checkSorting(model, child);
        if (!res) {
            return false;
        }
    }

    return true;
}

void NotebookModelTestHelper::notifyFailureWithStackTrace(
    ErrorString errorDescription)
{
    SysInfo sysInfo;
    errorDescription.details() += QStringLiteral("\nStack trace: ") +
                                  sysInfo.stackTrace();
    Q_EMIT failure(errorDescription);
}

bool NotebookModelTestHelper::LessByName::operator()(
    const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((lhs->type() == NotebookModelItem::Type::LinkedNotebook) &&
        (rhs->type() != NotebookModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if ((lhs->type() != NotebookModelItem::Type::LinkedNotebook) &&
             (rhs->type() == NotebookModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    const QString & leftName = (lhs->notebookStackItem()
                                ? lhs->notebookStackItem()->name()
                                : (lhs->notebookItem()
                                   ? lhs->notebookItem()->name()
                                   : lhs->notebookLinkedNotebookItem()->username()));
    const QString & rightName = (rhs->notebookStackItem()
                                 ? rhs->notebookStackItem()->name()
                                 : (rhs->notebookItem()
                                    ? rhs->notebookItem()->name()
                                    : rhs->notebookLinkedNotebookItem()->username()));

    return (leftName.localeAwareCompare(rightName) <= 0);
}

bool NotebookModelTestHelper::GreaterByName::operator()(
    const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((lhs->type() == NotebookModelItem::Type::LinkedNotebook) &&
        (rhs->type() != NotebookModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if ((lhs->type() != NotebookModelItem::Type::LinkedNotebook) &&
             (rhs->type() == NotebookModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    const QString & leftName = (lhs->notebookStackItem()
                                ? lhs->notebookStackItem()->name()
                                : (lhs->notebookItem()
                                   ? lhs->notebookItem()->name()
                                   : lhs->notebookLinkedNotebookItem()->username()));
    const QString & rightName = (rhs->notebookStackItem()
                                 ? rhs->notebookStackItem()->name()
                                 : (rhs->notebookItem()
                                    ? rhs->notebookItem()->name()
                                    : rhs->notebookLinkedNotebookItem()->username()));

    return (leftName.localeAwareCompare(rightName) > 0);
}

} // namespace quentier
