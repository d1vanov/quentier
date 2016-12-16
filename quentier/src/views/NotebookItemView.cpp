/*
 * Copyright 2016 Dmitry Ivanov
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

#include "NotebookItemView.h"
#include "AccountToKey.h"
#include "../models/NotebookModel.h"
#include "../dialogs/AddOrEditNotebookDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScopedPointer>

#ifndef QT_MOC_RUN
#include <boost/scope_exit.hpp>
#endif

#define LAST_SELECTED_NOTEBOOK_KEY QStringLiteral("_LastSelectedNotebookLocalUid")
#define LAST_EXPANDED_STACK_ITEMS_KEY QStringLiteral("_LastExpandedStackItems")

#define REPORT_ERROR(error) \
    { \
        QNLocalizedString errorDescription = QNLocalizedString(error, this); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

NotebookItemView::NotebookItemView(QWidget * parent) :
    ItemView(parent),
    m_pNotebookItemContextMenu(Q_NULLPTR),
    m_pNotebookStackItemContextMenu(Q_NULLPTR),
    m_restoringNotebookStackItemsState(false)
{
    QObject::connect(this, QNSIGNAL(NotebookItemView,expanded,const QModelIndex&),
                     this, QNSLOT(NotebookItemView,onNotebookStackItemCollapsedOrExpanded,const QModelIndex&));
    QObject::connect(this, QNSIGNAL(NotebookItemView,collapsed,const QModelIndex&),
                     this, QNSLOT(NotebookItemView,onNotebookStackItemCollapsedOrExpanded,const QModelIndex&));
}

void NotebookItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("NotebookItemView::setModel"));

    NotebookModel * pPreviousModel = qobject_cast<NotebookModel*>(model());
    if (pPreviousModel) {
        QObject::disconnect(pPreviousModel, QNSIGNAL(NotebookModel,notifyError,QNLocalizedString),
                            this, QNSIGNAL(NotebookItemView,notifyError,QNLocalizedString));
        QObject::disconnect(pPreviousModel, QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                            this, QNSLOT(NotebookItemView,onAllNotebooksListed));
        QObject::disconnect(pPreviousModel, QNSIGNAL(NotebookModel,notifyNotebookStackRenamed,const QString&,const QString&),
                            this, QNSLOT(NotebookItemView,onNotebookStackRenamed,const QString&,const QString&));
    }

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(pModel);
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model has been set to the notebook view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pNotebookModel, QNSIGNAL(NotebookModel,notifyError,QNLocalizedString),
                     this, QNSIGNAL(NotebookItemView,notifyError,QNLocalizedString));
    QObject::connect(pNotebookModel, QNSIGNAL(NotebookModel,notifyNotebookStackRenamed,const QString&,const QString&),
                     this, QNSLOT(NotebookItemView,onNotebookStackRenamed,const QString&,const QString&));

    ItemView::setModel(pModel);

    if (pNotebookModel->allNotebooksListed()) {
        QNDEBUG(QStringLiteral("All notebooks are already listed within "
                               "the model, need to select the appropriate one"));
        restoreNotebookStackItemsState(*pNotebookModel);
        restoreLastSavedSelectionOrAutoSelectNotebook(*pNotebookModel);
        return;
    }

    QObject::connect(pNotebookModel, QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                     this, QNSLOT(NotebookItemView,onAllNotebooksListed));
}

QModelIndex NotebookItemView::currentlySelectedItemIndex() const
{
    QNDEBUG(QStringLiteral("NotebookItemView::currentlySelectedItemIndex"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return QModelIndex();
    }

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG(QStringLiteral("No selection model in the view"));
        return QModelIndex();
    }

    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The selection contains no model indexes"));
        return QModelIndex();
    }

    return singleRow(indexes, *pNotebookModel, NotebookModel::Columns::Name);
}

void NotebookItemView::deleteSelectedItem()
{
    QNDEBUG(QStringLiteral("NotebookItemView::deleteSelectedItem"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(QStringLiteral("No notebooks are selected, nothing to deete"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current notebook"),
                                       tr("No notebook is selected currently"),
                                       tr("Please select the notebook you want to delete")))
        return;
    }

    QModelIndex index = singleRow(indexes, *pNotebookModel, NotebookModel::Columns::Name);
    if (!index.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one notebook within the selection"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current notebook"),
                                       tr("More than one notebook is currently selected"),
                                       tr("Please select only the notebook you want to delete")))
        return;
    }

    deleteItem(index, *pNotebookModel);
}

void NotebookItemView::onAllNotebooksListed()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onAllNotebooksListed"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        REPORT_ERROR("Can't cast the model set to the notebook item view "
                     "to the notebook model")
        return;
    }

    QObject::disconnect(pNotebookModel, QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                        this, QNSLOT(NotebookItemView,onAllNotebooksListed));

    restoreNotebookStackItemsState(*pNotebookModel);
    restoreLastSavedSelectionOrAutoSelectNotebook(*pNotebookModel);
}

void NotebookItemView::onCreateNewNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onCreateNewNotebookAction"));
    emit newNotebookCreationRequested();
}

void NotebookItemView::onRenameNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onRenameNotebookAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't rename notebook, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't rename notebook, "
                     "can't get notebook's local uid from QAction")
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't rename notebook: the model returned invalid "
                     "index for the notebook's local uid");
        return;
    }

    edit(itemIndex);
}

void NotebookItemView::onDeleteNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onDeleteNotebookAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't delete notebook, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't delete notebook, "
                     "can't get notebook's local uid from QAction")
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't delete notebook: the model "
                     "returned invalid index for the notebook's local uid")
        return;
    }

    deleteItem(itemIndex, *pNotebookModel);
}

void NotebookItemView::onSetNotebookDefaultAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onSetNotebookDefaultAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't set notebook as default, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't delete notebook, "
                     "can't get notebook's local uid from QAction")
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't delete notebook, the model "
                     "returned invalid index for the notebook's local uid")
        return;
    }

    itemIndex = pNotebookModel->index(itemIndex.row(), NotebookModel::Columns::Default,
                                      itemIndex.parent());
    bool res = pNotebookModel->setData(itemIndex, true);
    if (res) {
        QNDEBUG(QStringLiteral("Successfully set the notebook as default"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to set the notebook as default; "
                                              "Check the status bar for message from the notebook model "
                                              "explaining why the action was not successful")));
}

void NotebookItemView::onShowNotebookInfoAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onShowNotebookInfoAction"));
    emit notebookInfoRequested();
}

void NotebookItemView::onEditNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onEditNotebookAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't edit notebook, "
                     "can't cast the slot invoker to QAction");
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't edit notebook, "
                     "can't get notebook's local uid from QAction")
        return;
    }

    QScopedPointer<AddOrEditNotebookDialog> pEditNotebookDialog(new AddOrEditNotebookDialog(pNotebookModel, this, itemLocalUid));
    pEditNotebookDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pEditNotebookDialog->exec())
}

void NotebookItemView::onMoveNotebookToStackAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onMoveNotebookToStackAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't move notebook to stack, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    QStringList itemLocalUidAndStack = pAction->data().toStringList();
    if (itemLocalUidAndStack.size() != 2) {
        REPORT_ERROR("Internal error: can't move notebook to stack, "
                     "can't retrieve the notebook local uid and target stack "
                     "from QAction data")
        return;
    }

    const QString & localUid = itemLocalUidAndStack.at(0);

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(localUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't move notebook to stack, "
                     "can't get valid model index for notebook's local uid")
        return;
    }

    const QString & targetStack = itemLocalUidAndStack.at(1);
    QModelIndex index = pNotebookModel->moveToStack(itemIndex, targetStack);
    if (!index.isValid()) {
        REPORT_ERROR("Can't move notebook to stack")
        return;
    }

    QNDEBUG(QStringLiteral("Successfully moved the notebook item to the target stack"));
}

void NotebookItemView::onRemoveNotebookFromStackAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onRemoveNotebookFromStackAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't remove the notebook from stack, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't remove the notebook from stack, "
                     "can't get notebook's local uid from QAction")
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't remove the notebook from stack, the model "
                     "returned invalid index for the notebook's local uid")
        return;
    }

    QModelIndex index = pNotebookModel->removeFromStack(itemIndex);
    if (!index.isValid()) {
        REPORT_ERROR("Can't remove the notebook from stack")
        return;
    }

    QNDEBUG(QStringLiteral("Successfully removed the notebook item from its stack"));
}

void NotebookItemView::onRenameNotebookStackAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onRenameNotebookStackAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't rename the notebook stack, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    QString notebookStack = pAction->data().toString();
    if (Q_UNLIKELY(notebookStack.isEmpty())) {
        REPORT_ERROR("Internal error: can't rename the notebook stack, "
                     "can't get notebook stack from QAction")
        return;
    }

    QModelIndex notebookStackItemIndex = pNotebookModel->indexForNotebookStack(notebookStack);
    if (!notebookStackItemIndex.isValid()) {
        REPORT_ERROR("Internal error: can't rename the notebook stack, "
                     "the model returned invalid index for the notebook stack")
        return;
    }

    saveNotebookStackItemsState();

    edit(notebookStackItemIndex);
}

void NotebookItemView::onRemoveNotebooksFromStackAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onRemoveNotebooksFromStackAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't remove all notebooks from stack, "
                     "can't cast the slot invoker to QAction");
        return;
    }

    QString notebookStack = pAction->data().toString();
    if (Q_UNLIKELY(notebookStack.isEmpty())) {
        REPORT_ERROR("Internal error: can't remove all notebooks from stack, "
                     "can't get notebook stack from QAction")
        return;
    }

    while(true)
    {
        QModelIndex notebookStackItemIndex = pNotebookModel->indexForNotebookStack(notebookStack);
        if (!notebookStackItemIndex.isValid()) {
            QNDEBUG(QStringLiteral("Notebook stack item index is invalid, breaking the loop"));
            break;
        }

        QModelIndex childIndex = notebookStackItemIndex.child(0, NotebookModel::Columns::Name);
        if (!childIndex.isValid()) {
            QNDEBUG(QStringLiteral("Detected invalid child item index for the notebook stack item, breaking the loop"));
            break;
        }

        Q_UNUSED(pNotebookModel->removeFromStack(childIndex))
    }

    QNDEBUG(QStringLiteral("Successfully removed notebook items from the stack"));
}

void NotebookItemView::onNotebookStackItemCollapsedOrExpanded(const QModelIndex & index)
{
    QNTRACE(QStringLiteral("NotebookItemView::onNotebookStackItemCollapsedOrExpanded: index: valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row() << QStringLiteral(", column = ") << index.column()
            << QStringLiteral(", parent: valid = ") << (index.parent().isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.parent().row() << QStringLiteral(", column = ") << index.parent().column());

    if (m_restoringNotebookStackItemsState) {
        QNDEBUG(QStringLiteral("Ignoring the event as the stack items are being restored currently"));
        return;
    }

    saveNotebookStackItemsState();
}

void NotebookItemView::onNotebookStackRenamed(const QString & previousStackName,
                                              const QString & newStackName)
{
    QNDEBUG(QStringLiteral("NotebookItemView::onNotebookStackRenamed: previous stack name = ")
            << previousStackName << QStringLiteral(", new stack name = ") << newStackName);

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNWARNING(QStringLiteral("Received notebook stack item rename event but can't cast the model to the notebook one"));
        return;
    }

    QString accountKey = accountToKey(pNotebookModel->account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(pNotebookModel->account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/NotebookItemView"));
    QStringList expandedStacks = appSettings.value(LAST_EXPANDED_STACK_ITEMS_KEY).toStringList();
    appSettings.endGroup();

    int previousStackIndex = expandedStacks.indexOf(previousStackName);
    if (previousStackIndex < 0) {
        QNDEBUG(QStringLiteral("The renamed stack item hasn't been expanded"));
    }
    else {
        expandedStacks.replace(previousStackIndex, newStackName);
    }

    setStacksExpanded(expandedStacks, *pNotebookModel);

    QModelIndex newStackItemIndex = pNotebookModel->indexForNotebookStack(newStackName);
    if (Q_UNLIKELY(!newStackItemIndex.isValid())) {
        QNWARNING(QStringLiteral("Can't select the just renamed notebook stack: notebook model returned "
                                 "invalid index for the new notebook stack name"));
        autoSelectNotebook(*pNotebookModel);
        return;
    }

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNWARNING(QStringLiteral("Can't select the just renamed notebook stack: no selection model in the view"));
        return;
    }

    QModelIndex left = pNotebookModel->index(newStackItemIndex.row(), NotebookModel::Columns::Name,
                                             newStackItemIndex.parent());
    QModelIndex right = pNotebookModel->index(newStackItemIndex.row(), NotebookModel::Columns::NumNotesPerNotebook,
                                              newStackItemIndex.parent());
    QItemSelection selection(left, right);
    pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);

    setFocus();
}

void NotebookItemView::selectionChanged(const QItemSelection & selected,
                                        const QItemSelection & deselected)
{
    QNDEBUG(QStringLiteral("NotebookItemView::selectionChanged"));

    BOOST_SCOPE_EXIT(this_, &selected, &deselected) {
        this_->ItemView::selectionChanged(selected, deselected);
    } BOOST_SCOPE_EXIT_END

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QModelIndexList selectedIndexes = selected.indexes();

    if (selectedIndexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The new selection is empty"));
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    QModelIndex sourceIndex = singleRow(selectedIndexes, *pNotebookModel, NotebookModel::Columns::Name);
    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        return;
    }

    const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR("Internal error: can't find the notebook model item corresponging "
                     "to the selected index");
        return;
    }

    QNTRACE(QStringLiteral("Currently selected notebook item: ") << *pModelItem);

    if (pModelItem->type() != NotebookModelItem::Type::Notebook) {
        QNDEBUG(QStringLiteral("Non-notebook item is selected"));
        return;
    }

    const NotebookItem * pNotebookItem = pModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem)) {
        REPORT_ERROR("Internal error: selected notebook model item seems to point "
                     "to notebook (not to stack) but returns null pointer to "
                     "the notebook item");
        return;
    }

    QString accountKey = accountToKey(pNotebookModel->account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(pNotebookModel->account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/NotebookItemView"));
    appSettings.setValue(LAST_SELECTED_NOTEBOOK_KEY, pNotebookItem->localUid());
    appSettings.endGroup();

    QNDEBUG(QStringLiteral("Persisted the currently selected notebook local uid: ")
            << pNotebookItem->localUid());
}

void NotebookItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("NotebookItemView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: notebook item view received context menu event with null pointer to "
                                 "the context menu event"));
        return;
    }

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used, not doing anything"));
        return;
    }

    QModelIndex clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR("Can't show the context menu for the notebook model item: "
                     "no item corresponding to the clicked item's index")
        return;
    }

    if (pModelItem->type() == NotebookModelItem::Type::Notebook)
    {
        const NotebookItem * pNotebookItem = pModelItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem)) {
            REPORT_ERROR("Can't show the context menu for the notebook item: "
                         "the model item reported that it points to the notebook item "
                         "but the pointer to the notebook item is null")
            return;
        }

        showNotebookItemContextMenu(*pNotebookItem, pEvent->globalPos(), *pNotebookModel);
    }
    else if (pModelItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * pNotebookStackItem = pModelItem->notebookStackItem();
        if (Q_UNLIKELY(!pNotebookStackItem)) {
            REPORT_ERROR("Can't show the context menu for the notebook stack item: "
                         "the model item reported that it points to the notebook stack item "
                         "but the pointer to the notebook stack item is null")
            return;
        }

        showNotebookStackItemContextMenu(*pNotebookStackItem, pEvent->globalPos(), *pNotebookModel);
    }
}

void NotebookItemView::deleteItem(const QModelIndex & itemIndex,
                                  NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::deleteItem"));

    const NotebookModelItem * pModelItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR("Internal error: can't find the notebook model item meant to be deleted");
        return;
    }

    if (pModelItem->type() == NotebookModelItem::Type::Notebook)
    {
        const NotebookItem * pNotebookItem = pModelItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem)) {
            REPORT_ERROR("Internal error: can't delete notebook: "
                         "the model item reported that it points to the notebook item "
                         "but the pointer to the notebook item is null")
            return;
        }

        int confirm = warningMessageBox(this, tr("Confirm the notebook deletion"),
                                        tr("Are you sure you want to delete the notebook?"),
                                        tr("Note that this action is not reversible and the removal "
                                           "of the notebook would also mean the removal of all the notes "
                                           "stored inside it!"), QMessageBox::Ok | QMessageBox::No);
        if (confirm != QMessageBox::Ok) {
            QNDEBUG(QStringLiteral("Notebook removal was not confirmed"));
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG(QStringLiteral("Successfully removed notebook"));
            autoSelectNotebook(model);
            return;
        }

        Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to delete the notebook; "
                                                  "Check the status bar for message from the notebook model "
                                                  "explaining why the notebook could not be removed")));
    }
    else if (pModelItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * pNotebookStackItem = pModelItem->notebookStackItem();
        if (Q_UNLIKELY(!pNotebookStackItem)) {
            REPORT_ERROR("Internal error: can't delete notebook stack: "
                         "the model item reported that it points to the notebook stack "
                         "but the pointer to the notebook stack item is null")
            return;
        }

        int confirm = warningMessageBox(this, tr("Confirm the notebook stack deletion"),
                                        tr("Are you sure you want to delete the whole stack of notebooks?"),
                                        tr("Note that this action is not reversible and the removal "
                                           "of the whole stack of notebooks would also mean "
                                           "the removal of all the notes stored inside the notebooks!"),
                                        QMessageBox::Ok | QMessageBox::No);
        if (confirm != QMessageBox::Ok) {
            QNDEBUG(QStringLiteral("Notebook stack removal was not confirmed"));
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG(QStringLiteral("Successfully removed notebook stack"));
            autoSelectNotebook(model);
            return;
        }

        Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to delete the notebook stack; "
                                                  "Check the status bar for message from the notebook model "
                                                  "explaining why the notebook stack could not be removed")));
    }
    else
    {
        REPORT_ERROR("Internal error: can't delete notebook(s): found notebook model item "
                     "of unknown type");
        QNWARNING(*pModelItem);
    }
}

void NotebookItemView::showNotebookItemContextMenu(const NotebookItem & item,
                                                   const QPoint & point,
                                                   NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::showNotebookItemContextMenu: (")
            << point.x() << QStringLiteral(", ") << point.y()
            << QStringLiteral("), item: ") << item);

    delete m_pNotebookItemContextMenu;
    m_pNotebookItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(NotebookItemView,slot)); \
        menu->addAction(pAction); \
    }

    ADD_CONTEXT_MENU_ACTION(tr("Create new notebook") + QStringLiteral("..."),
                            m_pNotebookItemContextMenu, onCreateNewNotebookAction,
                            item.localUid(), true);

    m_pNotebookItemContextMenu->addSeparator();

    bool canRename = (item.isUpdatable() && item.nameIsUpdatable());
    ADD_CONTEXT_MENU_ACTION(tr("Rename"), m_pNotebookItemContextMenu,
                            onRenameNotebookAction, item.localUid(), canRename);

    if (model.account().type() == Account::Type::Local) {
        ADD_CONTEXT_MENU_ACTION(tr("Delete"), m_pNotebookItemContextMenu,
                                onDeleteNotebookAction, item.localUid(), true);
    }

    ADD_CONTEXT_MENU_ACTION(tr("Edit") + QStringLiteral("..."),
                            m_pNotebookItemContextMenu,onEditNotebookAction,
                            item.localUid(), canRename);

    const QString & stack = item.stack();
    QStringList stacks = model.stacks();
    if (!stack.isEmpty()) {
        Q_UNUSED(stacks.removeAll(stack))
    }

    if (!stacks.isEmpty() || !stack.isEmpty()) {
        m_pNotebookItemContextMenu->addSeparator();
    }

    if (!stacks.isEmpty())
    {
        QMenu * pTargetStackSubMenu = m_pNotebookItemContextMenu->addMenu(tr("Move to stack"));
        for(auto it = stacks.constBegin(), end = stacks.constEnd(); it != end; ++it)
        {
            QStringList dataPair;
            dataPair.reserve(2);
            dataPair << item.localUid();
            dataPair << *it;
            ADD_CONTEXT_MENU_ACTION(*it, pTargetStackSubMenu, onMoveNotebookToStackAction,
                                    dataPair, item.isUpdatable());
        }
    }

    if (!stack.isEmpty())
    {
        ADD_CONTEXT_MENU_ACTION(tr("Remove from stack"), m_pNotebookItemContextMenu,
                                onRemoveNotebookFromStackAction, item.localUid(),
                                item.isUpdatable());
    }

    m_pNotebookItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Set default"), m_pNotebookItemContextMenu,
                            onSetNotebookDefaultAction, item.localUid(),
                            item.isUpdatable());

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pNotebookItemContextMenu,
                            onShowNotebookInfoAction, item.localUid(), true);

    m_pNotebookItemContextMenu->show();
    m_pNotebookItemContextMenu->exec(point);
}

void NotebookItemView::showNotebookStackItemContextMenu(const NotebookStackItem & item,
                                                        const QPoint & point,
                                                        NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::showNotebookStackItemContextMenu: (")
            << point.x() << QStringLiteral(", ") << point.y()
            << QStringLiteral(", item: ") << item);

    delete m_pNotebookStackItemContextMenu;
    m_pNotebookStackItemContextMenu = new QMenu(this);

    ADD_CONTEXT_MENU_ACTION(tr("Create new notebook") + QStringLiteral("..."),
                            m_pNotebookStackItemContextMenu, onCreateNewNotebookAction,
                            item.name(), true);

    m_pNotebookStackItemContextMenu->addSeparator();

    bool allChildrenUpdatable = false;

    QModelIndex stackItemIndex = model.indexForNotebookStack(item.name());
    const NotebookModelItem * pModelItem = model.itemForIndex(stackItemIndex);
    if (Q_LIKELY(pModelItem))
    {
        allChildrenUpdatable = true;

        QList<const NotebookModelItem*> children = pModelItem->children();
        for(auto it = children.constBegin(), end = children.constEnd(); it != end; ++it)
        {
            const NotebookModelItem * pChildItem = *it;
            if (Q_UNLIKELY(!pChildItem)) {
                QNWARNING(QStringLiteral("Found null child at stack item: ") << *pModelItem);
                continue;
            }

            if (Q_UNLIKELY(pChildItem->type() != NotebookModelItem::Type::Notebook)) {
                QNWARNING(QStringLiteral("Found child of non-notebook type at stack item: ") << *pModelItem
                          << QStringLiteral("\nChild item: ") << *pChildItem);
                continue;
            }

            const NotebookItem * pNotebookItem = pChildItem->notebookItem();
            if (Q_UNLIKELY(!pNotebookItem)) {
                QNWARNING(QStringLiteral("Found child of notebook type but with null notebook item at stack item: ")
                          << *pModelItem << QStringLiteral("\nChild item: ") << *pChildItem);
                continue;
            }

            if (!pNotebookItem->isUpdatable()) {
                allChildrenUpdatable = false;
                break;
            }
        }
    }

    ADD_CONTEXT_MENU_ACTION(tr("Rename"), m_pNotebookStackItemContextMenu,
                            onRenameNotebookStackAction, item.name(), allChildrenUpdatable);

    ADD_CONTEXT_MENU_ACTION(tr("Remove stack"), m_pNotebookStackItemContextMenu,
                            onRemoveNotebooksFromStackAction, item.name(), allChildrenUpdatable);

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pNotebookStackItemContextMenu,
                            onShowNotebookInfoAction, item.name(), true);

    m_pNotebookStackItemContextMenu->show();
    m_pNotebookStackItemContextMenu->exec(point);
}

void NotebookItemView::saveNotebookStackItemsState()
{
    QNDEBUG(QStringLiteral("NotebookItemView::saveNotebookStackItemsState"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QString accountKey = accountToKey(pNotebookModel->account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(pNotebookModel->account());
        return;
    }

    QStringList result;

    QModelIndexList indexes = pNotebookModel->persistentIndexes();
    for(auto it = indexes.constBegin(), end = indexes.constEnd(); it != end; ++it)
    {
        const QModelIndex & index = *it;

        if (!isExpanded(index)) {
            continue;
        }

        QString stackItemName = index.data(Qt::DisplayRole).toString();
        if (Q_UNLIKELY(stackItemName.isEmpty())) {
            continue;
        }

        result << stackItemName;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/NotebookItemView"));
    appSettings.setValue(LAST_EXPANDED_STACK_ITEMS_KEY, result);
    appSettings.endGroup();
}

void NotebookItemView::restoreNotebookStackItemsState(const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::restoreNotebookStackItemsState"));

    QString accountKey = accountToKey(model.account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(model.account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/NotebookItemView"));
    QStringList expandedStacks = appSettings.value(LAST_EXPANDED_STACK_ITEMS_KEY).toStringList();
    appSettings.endGroup();

    m_restoringNotebookStackItemsState = true;
    setStacksExpanded(expandedStacks, model);
    m_restoringNotebookStackItemsState = false;
}

void NotebookItemView::setStacksExpanded(const QStringList & expandedStackNames, const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::setStacksExpanded"));

    for(auto it = expandedStackNames.constBegin(), end = expandedStackNames.constEnd(); it != end; ++it)
    {
        const QString & expandedStack = *it;
        QModelIndex index = model.indexForNotebookStack(expandedStack);
        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void NotebookItemView::restoreLastSavedSelectionOrAutoSelectNotebook(const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::restoreLastSavedSelectionOrAutoSelectNotebook"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR("Can't restore last selected notebook or auto-select one: "
                     "no selection model in the view");
        return;
    }

    QString accountKey = accountToKey(model.account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(model.account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/NotebookItemView"));
    QString lastSelectedNotebookLocalUid = appSettings.value(LAST_SELECTED_NOTEBOOK_KEY).toString();
    appSettings.endGroup();

    QNTRACE(QStringLiteral("Last selected notebook local uid: ") << lastSelectedNotebookLocalUid);

    if (!lastSelectedNotebookLocalUid.isEmpty())
    {
        QModelIndex lastSelectedNotebookIndex = model.indexForLocalUid(lastSelectedNotebookLocalUid);
        if (lastSelectedNotebookIndex.isValid()) {
            QNDEBUG(QStringLiteral("Selecting the last selected notebook item: local uid = ")
                    << lastSelectedNotebookLocalUid);
            QModelIndex left = model.index(lastSelectedNotebookIndex.row(), NotebookModel::Columns::Name,
                                           lastSelectedNotebookIndex.parent());
            QModelIndex right = model.index(lastSelectedNotebookIndex.row(), NotebookModel::Columns::NumNotesPerNotebook,
                                            lastSelectedNotebookIndex.parent());
            QItemSelection selection(left, right);
            pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }

    autoSelectNotebook(model);
}

void NotebookItemView::autoSelectNotebook(const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::autoSelectNotebook"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR("Can't auto-select notebook: no selection model in the view");
        return;
    }

    QModelIndex lastUsedNotebookIndex = model.lastUsedNotebookIndex();
    if (lastUsedNotebookIndex.isValid())
    {
        QNDEBUG(QStringLiteral("Selecting the last used notebook item"));
        QModelIndex left = model.index(lastUsedNotebookIndex.row(), NotebookModel::Columns::Name,
                                       lastUsedNotebookIndex.parent());
        QModelIndex right = model.index(lastUsedNotebookIndex.row(), NotebookModel::Columns::NumNotesPerNotebook,
                                        lastUsedNotebookIndex.parent());
        QItemSelection selection(left, right);
        pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
        return;
    }

    QModelIndex defaultNotebookIndex = model.defaultNotebookIndex();
    if (defaultNotebookIndex.isValid())
    {
        QNDEBUG(QStringLiteral("Selecting the default notebook item"));
        QModelIndex left = model.index(defaultNotebookIndex.row(), NotebookModel::Columns::Name,
                                       defaultNotebookIndex.parent());
        QModelIndex right = model.index(defaultNotebookIndex.row(), NotebookModel::Columns::NumNotesPerNotebook,
                                        defaultNotebookIndex.parent());
        QItemSelection selection(left, right);
        pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
        return;
    }

    QModelIndexList persistentIndexes = model.persistentIndexes();
    if (persistentIndexes.isEmpty()) {
        QNDEBUG(QStringLiteral("No persistent indexes returned by the notebook model, nothing to select"));
        return;
    }

    QNDEBUG(QStringLiteral("Selecting random notebook"));
    QModelIndex index = persistentIndexes.at(0);
    QModelIndex left = model.index(index.row(), NotebookModel::Columns::Name,
                                   index.parent());
    QModelIndex right = model.index(index.row(), NotebookModel::Columns::NumNotesPerNotebook,
                                    index.parent());
    QItemSelection selection(left, right);
    pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
}

} // namespace quentier
