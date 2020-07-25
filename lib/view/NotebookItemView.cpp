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

#include "NotebookItemView.h"

#include <lib/dialog/AddOrEditNotebookDialog.h>
#include <lib/model/NoteModel.h>
#include <lib/model/notebook/NotebookModel.h>
#include <lib/preferences/SettingsNames.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <memory>

#define NOTEBOOK_ITEM_VIEW_GROUP_KEY QStringLiteral("NotebookItemView")

#define LAST_SELECTED_NOTEBOOK_KEY                                             \
    QStringLiteral("LastSelectedNotebookLocalUid")                             \
// LAST_SELECTED_NOTEBOOK_KEY

#define LAST_EXPANDED_STACK_ITEMS_KEY QStringLiteral("LastExpandedStackItems")

#define LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY                                \
    QStringLiteral("LastExpandedLinkedNotebookItemsGuids")                     \
// LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY

#define ALL_NOTEBOOKS_ROOT_ITEM_EXPANDED_KEY                                   \
    QStringLiteral("AllNotebooksRootItemExpanded")                             \
// ALL_NOTEBOOKS_ROOT_ITEM_EXPANDED_KEY

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorToReport(error);                                      \
        QNWARNING("view:notebook", errorToReport);                             \
        Q_EMIT notifyError(errorToReport);                                     \
    }                                                                          \
// REPORT_ERROR

#define REPORT_ERROR_X(error, prefix)                                          \
    {                                                                          \
        ErrorString fullError = prefix;                                        \
        fullError.appendBase(QT_TR_NOOP("internal error"));                    \
        fullError.appendBase(error.base());                                    \
        fullError.appendBase(error.additionalBases());                         \
        fullError.details() = error.details();                                 \
        REPORT_ERROR(fullError);                                               \
    }                                                                          \
// REPORT_ERROR

#define FETCH_ITEM_DATA(prefix)                                                \
    NotebookItemData itemData;                                                 \
    ErrorString error;                                                         \
    if (!fetchCurrentNotebookItemData(itemData, error))                        \
    {                                                                          \
        if (!error.isEmpty()) {                                                \
            REPORT_ERROR_X(error, errorPrefix);                                \
        }                                                                      \
        return;                                                                \
    }                                                                          \
// FETCH_ITEM_DATA

#define FETCH_STACK_DATA(prefix)                                               \
    NotebookStackData stackData;                                               \
    ErrorString error;                                                         \
    if (!fetchCurrentNotebookStackData(stackData, error))                      \
    {                                                                          \
        if (!error.isEmpty()) {                                                \
            REPORT_ERROR_X(error, errorPrefix);                                \
        }                                                                      \
        return;                                                                \
    }                                                                          \
// FETCH_STACK_DATA

namespace quentier {

NotebookItemView::NotebookItemView(QWidget * parent) :
    ItemView(parent)
{
    QObject::connect(
        this,
        &NotebookItemView::expanded,
        this,
        &NotebookItemView::onNotebookModelItemCollapsedOrExpanded);

    QObject::connect(
        this,
        &NotebookItemView::collapsed,
        this,
        &NotebookItemView::onNotebookModelItemCollapsedOrExpanded);
}

void NotebookItemView::setNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    QNDEBUG("view:notebook", "NotebookItemView::setNoteFiltersManager");

    if (!m_pNoteFiltersManager.isNull())
    {
        if (m_pNoteFiltersManager.data() == &noteFiltersManager) {
            QNDEBUG("view:notebook", "Already using this note filters manager");
            return;
        }

        disconnectFromNoteFiltersManagerFilterChanged();
    }

    m_pNoteFiltersManager = &noteFiltersManager;
    connectToNoteFiltersManagerFilterChanged();
}

void NotebookItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG("view:notebook", "NotebookItemView::setModel");

    auto * pPreviousModel = qobject_cast<NotebookModel*>(model());
    if (pPreviousModel) {
        pPreviousModel->disconnect(this);
    }

    m_notebookLocalUidPendingNoteFiltersManagerReadiness.clear();
    m_modelReady = false;
    m_trackingSelection = false;
    m_trackingNotebookModelItemsState = false;

    auto * pNotebookModel = qobject_cast<NotebookModel*>(pModel);
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model has been set to "
            << "the notebook view");
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(
        pNotebookModel,
        &NotebookModel::notifyError,
        this,
        &NotebookItemView::notifyError);

    QObject::connect(
        pNotebookModel,
        &NotebookModel::notifyNotebookStackRenamed,
        this,
        &NotebookItemView::onNotebookStackRenamed);

    QObject::connect(
        pNotebookModel,
        &NotebookModel::notifyNotebookStackChanged,
        this,
        &NotebookItemView::onNotebookStackChanged);

    QObject::connect(
        pNotebookModel,
        &NotebookModel::aboutToAddNotebook,
        this,
        &NotebookItemView::onAboutToAddNotebook);

    QObject::connect(
        pNotebookModel,
        &NotebookModel::addedNotebook,
        this,
        &NotebookItemView::onAddedNotebook);

    QObject::connect(
        pNotebookModel,
        &NotebookModel::aboutToUpdateNotebook,
        this,
        &NotebookItemView::onAboutToUpdateNotebook);

    QObject::connect(
        pNotebookModel,
        &NotebookModel::updatedNotebook,
        this,
        &NotebookItemView::onUpdatedNotebook);

    QObject::connect(
        pNotebookModel,
        &NotebookModel::aboutToRemoveNotebooks,
        this,
        &NotebookItemView::onAboutToRemoveNotebooks);

    QObject::connect(
        pNotebookModel,
        &NotebookModel::removedNotebooks,
        this,
        &NotebookItemView::onRemovedNotebooks);

    ItemView::setModel(pModel);

    if (pNotebookModel->allNotebooksListed())
    {
        QNDEBUG("view:notebook", "All notebooks are already listed within "
            << "the model, need to select the appropriate one");

        restoreNotebookModelItemsState(*pNotebookModel);
        restoreSelectedNotebook(*pNotebookModel);

        m_modelReady = true;
        m_trackingSelection = true;
        m_trackingNotebookModelItemsState = true;
        return;
    }

    QObject::connect(
        pNotebookModel,
        &NotebookModel::notifyAllNotebooksListed,
        this,
        &NotebookItemView::onAllNotebooksListed);
}

void NotebookItemView::setNoteModel(const NoteModel * pNoteModel)
{
    m_pNoteModel = pNoteModel;
}

QModelIndex NotebookItemView::currentlySelectedItemIndex() const
{
    QNDEBUG("view:notebook", "NotebookItemView::currentlySelectedItemIndex");

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return {};
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG("view:notebook", "No selection model in the view");
        return {};
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG("view:notebook", "The selection contains no model indexes");
        return {};
    }

    return singleRow(
        indexes,
        *pNotebookModel,
        static_cast<int>(NotebookModel::Column::Name));
}

void NotebookItemView::deleteSelectedItem()
{
    QNDEBUG("view:notebook", "NotebookItemView::deleteSelectedItem");

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return;
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty())
    {
        QNDEBUG("view:notebook", "No notebooks are selected, nothing to deete");

        Q_UNUSED(informationMessageBox(
            this,
            tr("Cannot delete current notebook"),
            tr("No notebook is selected currently"),
            tr("Please select the notebook you want to delete")))

        return;
    }

    auto index = singleRow(
        indexes,
        *pNotebookModel,
        static_cast<int>(NotebookModel::Column::Name));

    if (!index.isValid())
    {
        QNDEBUG("view:notebook", "Not exactly one notebook within "
            << "the selection");

        Q_UNUSED(informationMessageBox(
            this,
            tr("Cannot delete current notebook"),
            tr("More than one notebook is currently selected"),
            tr("Please select only the notebook you want to delete")))

        return;
    }

    deleteItem(index, *pNotebookModel);
}

void NotebookItemView::onAllNotebooksListed()
{
    QNDEBUG("view:notebook", "NotebookItemView::onAllNotebooksListed");

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't cast the model set to the notebook item view to "
                       "the notebook model"))
        return;
    }

    QObject::disconnect(
        pNotebookModel,
        &NotebookModel::notifyAllNotebooksListed,
        this,
        &NotebookItemView::onAllNotebooksListed);

    restoreNotebookModelItemsState(*pNotebookModel);
    restoreSelectedNotebook(*pNotebookModel);

    m_modelReady = true;
    m_trackingSelection = true;
    m_trackingNotebookModelItemsState = true;
}

void NotebookItemView::onAboutToAddNotebook()
{
    QNDEBUG("view:notebook", "NotebookItemView::onAboutToAddNotebook");
    prepareForNotebookModelChange();
}

void NotebookItemView::onAddedNotebook(const QModelIndex & index)
{
    QNDEBUG("view:notebook", "NotebookItemView::onAddedNotebook");

    Q_UNUSED(index)
    postProcessNotebookModelChange();
}

void NotebookItemView::onAboutToUpdateNotebook(const QModelIndex & index)
{
    QNDEBUG("view:notebook", "NotebookItemView::onAboutToUpdateNotebook");

    Q_UNUSED(index)
    prepareForNotebookModelChange();
}

void NotebookItemView::onUpdatedNotebook(const QModelIndex & index)
{
    QNDEBUG("view:notebook", "NotebookItemView::onUpdatedNotebook");

    Q_UNUSED(index)
    postProcessNotebookModelChange();
}

void NotebookItemView::onAboutToRemoveNotebooks()
{
    QNDEBUG("view:notebook", "NotebookItemView::onAboutToRemoveNotebooks");
    prepareForNotebookModelChange();
}

void NotebookItemView::onRemovedNotebooks()
{
    QNDEBUG("view:notebook", "NotebookItemView::onRemovedNotebooks");
    postProcessNotebookModelChange();
}

void NotebookItemView::onCreateNewNotebookAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onCreateNewNotebookAction");
    Q_EMIT newNotebookCreationRequested();
}

void NotebookItemView::onRenameNotebookAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onRenameNotebookAction");

    ErrorString errorPrefix(QT_TR_NOOP("Can't rename notebook"));
    FETCH_ITEM_DATA(errorPrefix)
    edit(itemData.m_index);
}

void NotebookItemView::onDeleteNotebookAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onDeleteNotebookAction");

    ErrorString errorPrefix(QT_TR_NOOP("Can't delete notebook"));
    FETCH_ITEM_DATA(errorPrefix)
    deleteItem(itemData.m_index, *itemData.m_pModel);
}

void NotebookItemView::onSetNotebookDefaultAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onSetNotebookDefaultAction");

    ErrorString errorPrefix(QT_TR_NOOP("Can't set notebook as default"));
    FETCH_ITEM_DATA(errorPrefix)

    auto itemIndex = itemData.m_pModel->index(
        itemData.m_index.row(),
        static_cast<int>(NotebookModel::Column::Default),
        itemData.m_index.parent());

    bool res = itemData.m_pModel->setData(itemIndex, true);
    if (res) {
        QNDEBUG("view:notebook", "Successfully set the notebook as default");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The notebook model refused to set the notebook as default; Check "
           "the status bar for message from the notebook model explaining why "
           "the action was not successful")));
}

void NotebookItemView::onShowNotebookInfoAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onShowNotebookInfoAction");
    Q_EMIT notebookInfoRequested();
}

void NotebookItemView::onEditNotebookAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onEditNotebookAction");

    ErrorString errorPrefix(QT_TR_NOOP("Can't edit notebook"));
    FETCH_ITEM_DATA(errorPrefix)

    auto pEditNotebookDialog = std::make_unique<AddOrEditNotebookDialog>(
        itemData.m_pModel,
        this,
        itemData.m_localUid);

    pEditNotebookDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pEditNotebookDialog->exec())
}

void NotebookItemView::onMoveNotebookToStackAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onMoveNotebookToStackAction");

    ErrorString errorPrefix(QT_TR_NOOP("Can't move notebook to stack"));
    FETCH_STACK_DATA(errorPrefix)

    auto itemIndex = stackData.m_pModel->indexForLocalUid(stackData.m_id);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move notebook to stack, can't "
                       "get valid model index for notebook's local uid"))
        return;
    }

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;

    auto index = stackData.m_pModel->moveToStack(
        itemIndex,
        stackData.m_stack);

    m_trackingSelection = wasTrackingSelection;

    if (!index.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't move notebook to stack"))
        return;
    }

    QNDEBUG("view:notebook", "Successfully moved the notebook item to "
        << "the target stack");
}

void NotebookItemView::onRemoveNotebookFromStackAction()
{
    QNDEBUG(
        "view:notebook",
        "NotebookItemView::onRemoveNotebookFromStackAction");

    ErrorString errorPrefix(QT_TR_NOOP("Can't remove notebook from stack"));
    FETCH_ITEM_DATA(errorPrefix)

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;
    auto index = itemData.m_pModel->removeFromStack(itemData.m_index);
    m_trackingSelection = wasTrackingSelection;

    if (!index.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't remove the notebook from stack"))
        return;
    }

    QNDEBUG("view:notebook", "Successfully removed the notebook item from its "
        << "stack");
}

void NotebookItemView::onRenameNotebookStackAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onRenameNotebookStackAction");

    ErrorString errorPrefix(QT_TR_NOOP("Can't rename notebook stack"));
    FETCH_STACK_DATA(errorPrefix)

    auto notebookStackItemIndex = stackData.m_pModel->indexForNotebookStack(
        stackData.m_stack,
        stackData.m_id);

    if (!notebookStackItemIndex.isValid()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename the notebook stack, "
                       "the model returned invalid index for "
                       "the notebook stack"))
        return;
    }

    saveNotebookModelItemsState();
    edit(notebookStackItemIndex);
}

void NotebookItemView::onRemoveNotebooksFromStackAction()
{
    QNDEBUG(
        "view:notebook",
        "NotebookItemView::onRemoveNotebooksFromStackAction");

    ErrorString errorPrefix(QT_TR_NOOP("Can't remove notebooks from stack"));
    FETCH_STACK_DATA(errorPrefix)

    while(true)
    {
        auto notebookStackItemIndex = stackData.m_pModel->indexForNotebookStack(
            stackData.m_stack,
            stackData.m_id);

        if (!notebookStackItemIndex.isValid()) {
            QNDEBUG("view:notebook", "Notebook stack item index is invalid, "
                << "breaking the loop");
            break;
        }

        auto childIndex = notebookStackItemIndex.model()->index(
            0,
            static_cast<int>(NotebookModel::Column::Name));

        if (!childIndex.isValid()) {
            QNDEBUG("view:notebook", "Detected invalid child item index for "
                << "the notebook stack item, breaking the loop");
            break;
        }

        bool wasTrackingSelection = m_trackingSelection;
        m_trackingSelection = false;
        Q_UNUSED(stackData.m_pModel->removeFromStack(childIndex))
        m_trackingSelection = wasTrackingSelection;
    }

    QNDEBUG("view:notebook", "Successfully removed notebook items from "
        << "the stack");
}

void NotebookItemView::onFavoriteAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onFavoriteAction");

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't favorite notebook, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, true);
}

void NotebookItemView::onUnfavoriteAction()
{
    QNDEBUG("view:notebook", "NotebookItemView::onUnfavoriteAction");

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite notebook, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, false);
}

void NotebookItemView::onNotebookModelItemCollapsedOrExpanded(
    const QModelIndex & index)
{
    QNTRACE(
        "view:notebook",
        "NotebookItemView::onNotebookModelItemCollapsedOrExpanded: "
            << "index: valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column()
            << ", parent: valid = "
            << (index.parent().isValid() ? "true" : "false")
            << ", row = " << index.parent().row()
            << ", column = " << index.parent().column());

    if (!m_trackingNotebookModelItemsState) {
        QNDEBUG("view:notebook", "Not tracking the expanded/collapsed state of "
            << "notebook model items at the moment");
        return;
    }

    saveNotebookModelItemsState();
}

void NotebookItemView::onNotebookStackRenamed(
    const QString & previousStackName, const QString & newStackName,
    const QString & linkedNotebookGuid)
{
    QNDEBUG("view:notebook", "NotebookItemView::onNotebookStackRenamed: "
        << "previous stack name = " << previousStackName
        << ", new stack name = " << newStackName
        << ", linked notebook guid = " << linkedNotebookGuid);

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNWARNING("view:notebook", "Received notebook stack item rename event "
            << "but can't cast the model to the notebook one");
        return;
    }

    ApplicationSettings appSettings(
        pNotebookModel->account(),
        QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(QStringLiteral("NotebookItemView"));
    QString key = LAST_EXPANDED_STACK_ITEMS_KEY;
    if (!linkedNotebookGuid.isEmpty()) {
        key += QStringLiteral("/") + linkedNotebookGuid;
    }

    QStringList expandedStacks = appSettings.value(key).toStringList();
    appSettings.endGroup();

    int previousStackIndex = expandedStacks.indexOf(previousStackName);
    if (previousStackIndex < 0) {
        QNDEBUG("view:notebook", "The renamed stack item hasn't been expanded");
    }
    else {
        expandedStacks.replace(previousStackIndex, newStackName);
    }

    setStacksExpanded(expandedStacks, *pNotebookModel, linkedNotebookGuid);

    auto newStackItemIndex = pNotebookModel->indexForNotebookStack(
        newStackName,
        linkedNotebookGuid);

    if (Q_UNLIKELY(!newStackItemIndex.isValid())) {
        QNWARNING("view:notebook", "Can't select the just renamed notebook "
            << "stack: notebook model returned invalid index for the new "
            << "notebook stack name");
        return;
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNWARNING("view:notebook", "Can't select the just renamed notebook "
            << "stack: no selection model in the view");
        return;
    }

    pSelectionModel->select(
        newStackItemIndex,
        QItemSelectionModel::ClearAndSelect |
        QItemSelectionModel::Rows |
        QItemSelectionModel::Current);
}

void NotebookItemView::onNotebookStackChanged(const QModelIndex & index)
{
    QNDEBUG("view:notebook", "NotebookItemView::onNotebookStackChanged");

    Q_UNUSED(index)

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return;
    }

    restoreNotebookModelItemsState(*pNotebookModel);
    restoreSelectedNotebook(*pNotebookModel);
}

void NotebookItemView::onNoteFilterChanged()
{
    QNDEBUG("view:notebook", "NotebookItemView::onNoteFilterChanged");

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return;
    }

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG("view:notebook", "Note filters manager is null");
        return;
    }

    if (m_pNoteFiltersManager->isFilterBySearchStringActive()) {
        QNDEBUG("view:notebook", "Filter by search string is active");
        selectAllNotebooksRootItem(*pNotebookModel);
        return;
    }

    if (!m_pNoteFiltersManager->savedSearchLocalUidInFilter().isEmpty()) {
        QNDEBUG("view:notebook", "Filter by saved search is active");
        selectAllNotebooksRootItem(*pNotebookModel);
        return;
    }

    const auto & notebookLocalUids =
        m_pNoteFiltersManager->notebookLocalUidsInFilter();

    if (notebookLocalUids.size() != 1) {
        QNDEBUG("view:notebook", "Not exactly one notebook local uid is within "
            << "the filter: " << notebookLocalUids.join(QStringLiteral(", ")));
        selectAllNotebooksRootItem(*pNotebookModel);
        return;
    }

    auto notebookIndex = pNotebookModel->indexForLocalUid(notebookLocalUids[0]);
    if (Q_UNLIKELY(!notebookIndex.isValid())) {
        QNWARNING("view:notebook", "The filtered notebook local uid's index is "
            << "invalid");
        selectAllNotebooksRootItem(*pNotebookModel);
        return;
    }

    setCurrentIndex(notebookIndex);
}

void NotebookItemView::onNoteFiltersManagerReady()
{
    QNDEBUG("view:notebook", "NotebookItemView::onNoteFiltersManagerReady");

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG("view:notebook", "Note filters manager is null");
        return;
    }

    QObject::disconnect(
        m_pNoteFiltersManager.data(),
        &NoteFiltersManager::ready,
        this,
        &NotebookItemView::onNoteFiltersManagerReady);

    QString notebookLocalUid =
        m_notebookLocalUidPendingNoteFiltersManagerReadiness;

    m_notebookLocalUidPendingNoteFiltersManagerReadiness.clear();

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return;
    }

    saveSelectedNotebook(pNotebookModel->account(), notebookLocalUid);

    if (shouldFilterBySelectedNotebook(pNotebookModel->account()))
    {
        if (!notebookLocalUid.isEmpty()) {
            setSelectedNotebookToNoteFiltersManager(notebookLocalUid);
        }
        else {
            clearNotebooksFromNoteFiltersManager();
        }
    }
}

void NotebookItemView::selectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG("view:notebook", "NotebookItemView::selectionChanged");

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void NotebookItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("view:notebook", "NotebookItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING("view:notebook", "Detected Qt error: notebook item view "
            << "received context menu event with null pointer to the context "
            << "menu event");
        return;
    }

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used, not doing "
            << "anything");
        return;
    }

    auto clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG("view:notebook", "Clicked item index is not valid, not doing "
            << "anything");
        return;
    }

    const auto * pModelItem = pNotebookModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the notebook model "
                       "item: no item corresponding to the clicked item's "
                       "index"))
        return;
    }

    const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (pNotebookItem)
    {
        showNotebookItemContextMenu(
            *pNotebookItem,
            pEvent->globalPos(),
            *pNotebookModel);

        return;
    }

    const auto * pStackItem = pModelItem->cast<StackItem>();
    if (pStackItem) {
        showNotebookStackItemContextMenu(
            *pStackItem,
            *pModelItem,
            pEvent->globalPos(),
            *pNotebookModel);
    }
}

void NotebookItemView::deleteItem(
    const QModelIndex & itemIndex, NotebookModel & model)
{
    QNDEBUG("view:notebook", "NotebookItemView::deleteItem");

    const auto * pModelItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the notebook model "
                       "item meant to be deleted"))
        return;
    }

    const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (pNotebookItem)
    {
        int confirm = warningMessageBox(
            this,
            tr("Confirm the notebook deletion"),
            tr("Are you sure you want to delete the notebook?"),
            tr("Note that this action is not "
               "reversible and the deletion "
               "of the notebook would also mean "
               "the deletion of all the notes "
               "stored inside it!"),
            QMessageBox::Ok | QMessageBox::No);

        if (confirm != QMessageBox::Ok) {
            QNDEBUG("view:notebook", "Notebook deletion was not confirmed");
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG("view:notebook", "Successfully deleted notebook");
            return;
        }

        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("The notebook model refused to delete the notebook; Check "
               "the status bar for message from the notebook model "
               "explaining why the notebook could not be deleted")))

        return;
    }

    const auto * pStackItem = pModelItem->cast<StackItem>();
    if (pStackItem)
    {
        int confirm = warningMessageBox(
            this,
            tr("Confirm the notebook stack deletion"),
            tr("Are you sure you want to delete the whole stack of notebooks?"),
            tr("Note that this action is not reversible and the deletion "
               "of the whole stack of notebooks would also mean the deletion "
               "of all the notes stored inside the notebooks!"),
            QMessageBox::Ok | QMessageBox::No);

        if (confirm != QMessageBox::Ok) {
            QNDEBUG("view:notebook", "Notebook stack deletion was not "
                << "confirmed");
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG("view:notebook", "Successfully deleted notebook stack");
            return;
        }

        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("The notebook model refused to delete the notebook stack; "
               "Check the status bar for message from the notebook model "
               "explaining why the notebook stack could not be deleted")));
    }
    else
    {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete notebook(s): "
                       "found notebook model item of unknown type"))

        QNWARNING("view:notebook", *pModelItem);
    }
}

void NotebookItemView::showNotebookItemContextMenu(
    const NotebookItem & item, const QPoint & point, NotebookModel & model)
{
    QNDEBUG("view:notebook", "NotebookItemView::showNotebookItemContextMenu: ("
        << point.x() << ", " << point.y() << "), item: " << item);

    delete m_pNotebookItemContextMenu;
    m_pNotebookItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled)               \
    {                                                                          \
        QAction * pAction = new QAction(name, menu);                           \
        pAction->setData(data);                                                \
        pAction->setEnabled(enabled);                                          \
        QObject::connect(                                                      \
            pAction,                                                           \
            &QAction::triggered,                                               \
            this,                                                              \
            &NotebookItemView::slot);                                          \
        menu->addAction(pAction);                                              \
    }                                                                          \
// ADD_CONTEXT_MENU_ACTION

    ADD_CONTEXT_MENU_ACTION(
        tr("Create new notebook") + QStringLiteral("..."),
        m_pNotebookItemContextMenu,
        onCreateNewNotebookAction,
        item.localUid(),
        true);

    m_pNotebookItemContextMenu->addSeparator();
    bool canRename = (item.isUpdatable() && item.nameIsUpdatable());

    ADD_CONTEXT_MENU_ACTION(
        tr("Rename"),
        m_pNotebookItemContextMenu,
        onRenameNotebookAction,
        item.localUid(),
        canRename);

    bool canDeleteNotebook = !m_pNoteModel.isNull() && item.guid().isEmpty();
    if (canDeleteNotebook) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Delete"),
            m_pNotebookItemContextMenu,
            onDeleteNotebookAction,
            item.localUid(),
            true);
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Edit") + QStringLiteral("..."),
        m_pNotebookItemContextMenu,
        onEditNotebookAction,
        item.localUid(),
        canRename);

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
        auto * pTargetStackSubMenu = m_pNotebookItemContextMenu->addMenu(
            tr("Move to stack"));

        for(const auto & stack: qAsConst(stacks))
        {
            QStringList dataPair;
            dataPair.reserve(2);
            dataPair << stack;
            dataPair << item.localUid();

            ADD_CONTEXT_MENU_ACTION(
                stack,
                pTargetStackSubMenu,
                onMoveNotebookToStackAction,
                dataPair,
                item.isUpdatable());
        }
    }

    if (!stack.isEmpty())
    {
        ADD_CONTEXT_MENU_ACTION(
            tr("Remove from stack"),
            m_pNotebookItemContextMenu,
            onRemoveNotebookFromStackAction,
            item.localUid(),
            item.isUpdatable());
    }

    m_pNotebookItemContextMenu->addSeparator();

    if (!item.isDefault()) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Set default"),
            m_pNotebookItemContextMenu,
            onSetNotebookDefaultAction,
            item.localUid(),
            item.isUpdatable());
    }

    if (item.isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Unfavorite"),
            m_pNotebookItemContextMenu,
            onUnfavoriteAction,
            item.localUid(),
            item.isUpdatable());
    }
    else {
        ADD_CONTEXT_MENU_ACTION(
            tr("Favorite"),
            m_pNotebookItemContextMenu,
            onFavoriteAction,
            item.localUid(),
            item.isUpdatable());
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."),
        m_pNotebookItemContextMenu,
        onShowNotebookInfoAction,
        item.localUid(),
        true);

    m_pNotebookItemContextMenu->show();
    m_pNotebookItemContextMenu->exec(point);
}

void NotebookItemView::showNotebookStackItemContextMenu(
    const StackItem & item, const INotebookModelItem & modelItem,
    const QPoint & point, NotebookModel & model)
{
    QNDEBUG(
        "view:notebook",
        "NotebookItemView::showNotebookStackItemContextMenu: ("
            << point.x() << ", " << point.y() << ", item: " << item);

    delete m_pNotebookStackItemContextMenu;
    m_pNotebookStackItemContextMenu = new QMenu(this);

    ADD_CONTEXT_MENU_ACTION(
        tr("Create new notebook") + QStringLiteral("..."),
        m_pNotebookStackItemContextMenu,
        onCreateNewNotebookAction,
        item.name(),
        true);

    m_pNotebookStackItemContextMenu->addSeparator();

    bool allChildrenUpdatable = false;

    QString linkedNotebookGuid;
    const auto * pParentItem = modelItem.parent();
    if (pParentItem &&
        (pParentItem->type() == INotebookModelItem::Type::LinkedNotebook))
    {
        const auto * pLinkedNotebookItem =
            pParentItem->cast<LinkedNotebookRootItem>();
        if (pLinkedNotebookItem) {
            linkedNotebookGuid =
                pLinkedNotebookItem->linkedNotebookGuid();
        }
    }

    auto stackItemIndex = model.indexForNotebookStack(
        item.name(),
        linkedNotebookGuid);

    const auto * pModelItem = model.itemForIndex(stackItemIndex);
    if (Q_LIKELY(pModelItem))
    {
        allChildrenUpdatable = true;

        auto children = pModelItem->children();
        for(const auto * pChildItem: qAsConst(children))
        {
            if (Q_UNLIKELY(!pChildItem)) {
                QNWARNING("view:notebook", "Found null child at stack item: "
                    << *pModelItem);
                continue;
            }

            const auto * pNotebookItem = pChildItem->cast<NotebookItem>();
            if (Q_UNLIKELY(!pNotebookItem)) {
                QNWARNING("view:notebook", "Found child of non-notebook type "
                    << "at stack item: " << *pModelItem << "\nChild item: "
                    << *pChildItem);
                continue;
            }

            if (!pNotebookItem->isUpdatable()) {
                allChildrenUpdatable = false;
                break;
            }
        }
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Rename"),
        m_pNotebookStackItemContextMenu,
        onRenameNotebookStackAction,
        QStringList() << item.name() << linkedNotebookGuid,
        allChildrenUpdatable);

    ADD_CONTEXT_MENU_ACTION(
        tr("Remove stack"),
        m_pNotebookStackItemContextMenu,
        onRemoveNotebooksFromStackAction,
        QStringList() << item.name() << linkedNotebookGuid,
        allChildrenUpdatable);

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."),
        m_pNotebookStackItemContextMenu,
        onShowNotebookInfoAction,
        item.name(),
        true);

    m_pNotebookStackItemContextMenu->show();
    m_pNotebookStackItemContextMenu->exec(point);
}

#undef ADD_CONTEXT_MENU_ACTION

void NotebookItemView::saveNotebookModelItemsState()
{
    QNDEBUG("view:notebook", "NotebookItemView::saveNotebookModelItemsState");

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return;
    }

    QMap<QString, QStringList> expandedStackItemsByLinkedNotebookGuid;
    QStringList expandedLinkedNotebookItemsGuids;

    auto indexes = pNotebookModel->persistentIndexes();
    for(const auto & index: qAsConst(indexes))
    {
        if (!isExpanded(index)) {
            continue;
        }

        const auto * pModelItem = pNotebookModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING("view:notebook", "Can't save the state of notebook stack "
                << "expanded/folded state: no notebook model item "
                << "corresponding to the persistent notebook model index");
            continue;
        }

        const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
        if (pNotebookItem) {
            continue;
        }

        const auto * pStackItem = pModelItem->cast<StackItem>();
        if (pStackItem)
        {
            QString linkedNotebookGuid;
            const auto * pParentItem = pModelItem->parent();
            if (pParentItem &&
                (pParentItem->type() ==
                 INotebookModelItem::Type::LinkedNotebook))
            {
                auto * pLinkedNotebookItem =
                    pParentItem->cast<LinkedNotebookRootItem>();

                if (pLinkedNotebookItem) {
                    linkedNotebookGuid =
                        pLinkedNotebookItem->linkedNotebookGuid();
                }
            }

            const QString & stackItemName = pStackItem->name();
            if (Q_UNLIKELY(stackItemName.isEmpty())) {
                QNDEBUG("view:notebook", "Skipping the notebook stack item "
                    << "without a name");
                continue;
            }

            expandedStackItemsByLinkedNotebookGuid[linkedNotebookGuid].append(
                stackItemName);

            continue;
        }

        const auto * pLinkedNotebookItem =
            pModelItem->cast<LinkedNotebookRootItem>();

        if (pLinkedNotebookItem) {
            expandedLinkedNotebookItemsGuids
                << pLinkedNotebookItem->linkedNotebookGuid();
        }
    }

    ApplicationSettings appSettings(
        pNotebookModel->account(),
        QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(QStringLiteral("NotebookItemView"));

    for(const auto & it:
        qevercloud::toRange(qAsConst(expandedStackItemsByLinkedNotebookGuid)))
    {
        const QString & linkedNotebookGuid = it.key();
        const QStringList & stackItemNames = it.value();

        if (linkedNotebookGuid.isEmpty())
        {
            appSettings.setValue(LAST_EXPANDED_STACK_ITEMS_KEY, stackItemNames);
        }
        else
        {
            QString key = LAST_EXPANDED_STACK_ITEMS_KEY;

            if (!linkedNotebookGuid.isEmpty()) {
                key += QStringLiteral("/") + linkedNotebookGuid;
            }

            appSettings.setValue(key, stackItemNames);
        }
    }

    appSettings.setValue(
        LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY,
        expandedLinkedNotebookItemsGuids);

    auto allNotebooksRootItemIndex = pNotebookModel->index(0, 0);

    appSettings.setValue(
        ALL_NOTEBOOKS_ROOT_ITEM_EXPANDED_KEY,
        isExpanded(allNotebooksRootItemIndex));

    appSettings.endGroup();
}

void NotebookItemView::restoreNotebookModelItemsState(
    const NotebookModel & model)
{
    QNDEBUG(
        "view:notebook",
        "NotebookItemView::restoreNotebookModelItemsState");

    const auto & linkedNotebookOwnerNamesByGuid =
        model.linkedNotebookOwnerNamesByGuid();

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));

    auto expandedStacks = appSettings.value(
        LAST_EXPANDED_STACK_ITEMS_KEY).toStringList();

    QHash<QString,QStringList> expandedStacksByLinkedNotebookGuid;
    for(const auto & it:
        qevercloud::toRange(qAsConst(linkedNotebookOwnerNamesByGuid)))
    {
        const QString & linkedNotebookGuid = it.key();

        auto expandedStacksForLinkedNotebook = appSettings.value(
            LAST_EXPANDED_STACK_ITEMS_KEY + QStringLiteral("/") +
            linkedNotebookGuid).toStringList();

        if (expandedStacksForLinkedNotebook.isEmpty()) {
            continue;
        }

        expandedStacksByLinkedNotebookGuid[linkedNotebookGuid] =
            expandedStacksForLinkedNotebook;
    }

    auto expandedLinkedNotebookItemsGuids = appSettings.value(
        LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY).toStringList();

    auto allNotebooksRootItemExpandedPreference = appSettings.value(
        ALL_NOTEBOOKS_ROOT_ITEM_EXPANDED_KEY);

    appSettings.endGroup();

    bool wasTrackingNotebookItemsState = m_trackingNotebookModelItemsState;
    m_trackingNotebookModelItemsState = false;
    setStacksExpanded(expandedStacks, model, QString());

    for(const auto & it:
        qevercloud::toRange(qAsConst(expandedStacksByLinkedNotebookGuid)))
    {
        setStacksExpanded(it.value(), model, it.key());
    }

    setLinkedNotebooksExpanded(expandedLinkedNotebookItemsGuids, model);

    bool allNotebooksRootItemExpanded = true;
    if (allNotebooksRootItemExpandedPreference.isValid()) {
        allNotebooksRootItemExpanded =
            allNotebooksRootItemExpandedPreference.toBool();
    }

    auto allNotebooksRootItemIndex = model.index(0, 0);
    setExpanded(allNotebooksRootItemIndex, allNotebooksRootItemExpanded);

    m_trackingNotebookModelItemsState = wasTrackingNotebookItemsState;
}

void NotebookItemView::setStacksExpanded(
    const QStringList & expandedStackNames, const NotebookModel & model,
    const QString & linkedNotebookGuid)
{
    QNDEBUG("view:notebook", "NotebookItemView::setStacksExpanded: "
        << "linked notebook guid = " << linkedNotebookGuid);

    for(const auto & expandedStack: qAsConst(expandedStackNames))
    {
        auto index = model.indexForNotebookStack(
            expandedStack,
            linkedNotebookGuid);

        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void NotebookItemView::setLinkedNotebooksExpanded(
    const QStringList & expandedLinkedNotebookGuids,
    const NotebookModel & model)
{
    QNDEBUG("view:notebook", "NotebookItemView::setLinkedNotebooksExpanded: "
        << expandedLinkedNotebookGuids.join(QStringLiteral(", ")));

    for(const auto & expandedLinkedNotebookGuid:
        qAsConst(expandedLinkedNotebookGuids))
    {
        auto index = model.indexForLinkedNotebookGuid(
            expandedLinkedNotebookGuid);

        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void NotebookItemView::saveSelectedNotebook(
    const Account & account, const QString & notebookLocalUid)
{
    QNDEBUG("view:notebook", "NotebookItemView::saveSelectedNotebook: "
        << notebookLocalUid);

    ApplicationSettings appSettings(account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTEBOOK_ITEM_VIEW_GROUP_KEY);
    appSettings.setValue(LAST_SELECTED_NOTEBOOK_KEY, notebookLocalUid);
    appSettings.endGroup();
}

void NotebookItemView::restoreSelectedNotebook(
    const NotebookModel & model)
{
    QNDEBUG("view:notebook", "NotebookItemView::restoreSelectedNotebook");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't restore the last selected notebook: "
                       "no selection model in the view"))
        return;
    }

    QString selectedNotebookLocalUid;

    if (shouldFilterBySelectedNotebook(model.account()))
    {
        if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
            QNDEBUG("view:notebook", "Note filters manager is null");
            return;
        }

        auto filteredNotebookLocalUids =
            m_pNoteFiltersManager->notebookLocalUidsInFilter();

        if (filteredNotebookLocalUids.isEmpty()) {
            QNDEBUG("view:notebook", "No notebooks within filter, selecting "
                << "all notebooks root item");
            selectAllNotebooksRootItem(model);
            return;
        }

        if (filteredNotebookLocalUids.size() != 1) {
            QNDEBUG("view:notebook", "Not exactly one notebook local uid "
                << "within filter: "
                << filteredNotebookLocalUids.join(QStringLiteral(",")));
            return;
        }

        selectedNotebookLocalUid = filteredNotebookLocalUids.at(0);
    }
    else
    {
        ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
        appSettings.beginGroup(NOTEBOOK_ITEM_VIEW_GROUP_KEY);

        selectedNotebookLocalUid = appSettings.value(
            LAST_SELECTED_NOTEBOOK_KEY).toString();

        appSettings.endGroup();
    }

    QNTRACE("view:notebook", "Selecting notebook local uid: "
        << selectedNotebookLocalUid);

    if (selectedNotebookLocalUid.isEmpty()) {
        QNDEBUG("view:notebook", "Found no last selected notebook local uid");
        return;
    }

    auto selectedNotebookIndex = model.indexForLocalUid(
        selectedNotebookLocalUid);

    if (!selectedNotebookIndex.isValid()) {
        QNDEBUG("view:notebook", "Notebook model returned invalid index for "
            << "the last selected notebook local uid");
        return;
    }

    pSelectionModel->select(
        selectedNotebookIndex,
        QItemSelectionModel::ClearAndSelect |
        QItemSelectionModel::Rows |
        QItemSelectionModel::Current);
}

void NotebookItemView::selectionChangedImpl(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNTRACE("view:notebook", "NotebookItemView::selectionChangedImpl");

    if (!m_trackingSelection) {
        QNTRACE("view:notebook", "Not tracking selection at this time, "
            << "skipping");
        return;
    }

    Q_UNUSED(deselected)

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return;
    }

    auto selectedIndexes = selected.indexes();
    if (selectedIndexes.isEmpty()) {
        QNDEBUG("view:notebook", "The new selection is empty");
        handleNoSelectedNotebook(pNotebookModel->account());
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    auto sourceIndex = singleRow(
        selectedIndexes,
        *pNotebookModel,
        static_cast<int>(NotebookModel::Column::Name));

    if (!sourceIndex.isValid()) {
        QNDEBUG("view:notebook", "Not exactly one row is selected");
        handleNoSelectedNotebook(pNotebookModel->account());
        return;
    }

    const auto * pModelItem = pNotebookModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pModelItem))
    {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the notebook model "
                       "item corresponging to the selected index"))

        handleNoSelectedNotebook(pNotebookModel->account());
        return;
    }

    QNTRACE("view:notebook", "Currently selected notebook model item: "
        << *pModelItem);

    if (pModelItem->type() == INotebookModelItem::Type::AllNotebooksRoot) {
        QNDEBUG("view:notebook", "All notebooks root item has been selected");
        handleNoSelectedNotebook(pNotebookModel->account());
        return;
    }

    const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (!pNotebookItem) {
        QNDEBUG("view:notebook", "Non-notebook item is selected");
        handleNoSelectedNotebook(pNotebookModel->account());
        return;
    }

    saveSelectedNotebook(pNotebookModel->account(), pNotebookItem->localUid());

    if (shouldFilterBySelectedNotebook(pNotebookModel->account())) {
        setSelectedNotebookToNoteFiltersManager(pNotebookItem->localUid());
    }
    else {
        QNDEBUG("view:notebook", "Filtering by selected notebook is switched "
            << "off");
    }
}

void NotebookItemView::handleNoSelectedNotebook(const Account & account)
{
    saveSelectedNotebook(account, QString());

    if (shouldFilterBySelectedNotebook(account)) {
        clearNotebooksFromNoteFiltersManager();
    }
}

void NotebookItemView::selectAllNotebooksRootItem(const NotebookModel & model)
{
    QNDEBUG("view:notebook", "NotebookItemView::selectAllNotebooksRootItem");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't select all notebooks root item: "
                       "no selection model in the view"))
        return;
    }

    m_trackingSelection = false;

    pSelectionModel->select(
        model.index(0, 0),
        QItemSelectionModel::ClearAndSelect |
        QItemSelectionModel::Rows |
        QItemSelectionModel::Current);

    m_trackingSelection = true;

    handleNoSelectedNotebook(model.account());
}

void NotebookItemView::setFavoritedFlag(
    const QAction & action, const bool favorited)
{
    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return;
    }

    QString itemLocalUid = action.data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag "
                       "for the notebook, can't get notebook's local "
                       "uid from QAction"))
        return;
    }

    auto itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag "
                       "for the notebook, the model returned invalid "
                       "index for the notebook's local uid"))
        return;
    }

    if (favorited) {
        pNotebookModel->favoriteNotebook(itemIndex);
    }
    else {
        pNotebookModel->unfavoriteNotebook(itemIndex);
    }
}

void NotebookItemView::prepareForNotebookModelChange()
{
    if (!m_modelReady) {
        QNDEBUG("view:notebook", "The model is not ready yet");
        return;
    }

    saveNotebookModelItemsState();
    m_trackingSelection = false;
    m_trackingNotebookModelItemsState = false;
}

void NotebookItemView::postProcessNotebookModelChange()
{
    if (!m_modelReady) {
        QNDEBUG("view:notebook", "The model is not ready yet");
        return;
    }

    auto * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return;
    }

    restoreNotebookModelItemsState(*pNotebookModel);
    m_trackingNotebookModelItemsState = true;

    restoreSelectedNotebook(*pNotebookModel);
    m_trackingSelection = true;
}

void NotebookItemView::setSelectedNotebookToNoteFiltersManager(
    const QString & notebookLocalUid)
{
    QNDEBUG(
        "view:notebook",
        "NotebookItemView::setSelectedNotebookToNoteFiltersManager: "
            << notebookLocalUid);

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG("view:notebook", "Note filters manager is null");
        return;
    }

    if (!m_pNoteFiltersManager->isReady())
    {
        QNDEBUG("view:notebook", "Note filters manager is not ready yet, will "
            << "postpone setting the notebook to it: "
            << notebookLocalUid);

        QObject::connect(
            m_pNoteFiltersManager.data(),
            &NoteFiltersManager::ready,
            this,
            &NotebookItemView::onNoteFiltersManagerReady,
            Qt::UniqueConnection);

        m_notebookLocalUidPendingNoteFiltersManagerReadiness = notebookLocalUid;
        return;
    }

    if (m_pNoteFiltersManager->isFilterBySearchStringActive()) {
        QNDEBUG("view:notebook", "Filter by search string is active, won't set "
            << "the seleted notebook to filter");
        return;
    }

    const QString & savedSearchLocalUidInFilter =
        m_pNoteFiltersManager->savedSearchLocalUidInFilter();

    if (!savedSearchLocalUidInFilter.isEmpty()) {
        QNDEBUG("view:notebook", "Filter by saved search is active, won't set "
            << "the selected notebook to filter");
        return;
    }

    disconnectFromNoteFiltersManagerFilterChanged();
    m_pNoteFiltersManager->setNotebookToFilter(notebookLocalUid);
    connectToNoteFiltersManagerFilterChanged();
}

void NotebookItemView::clearNotebooksFromNoteFiltersManager()
{
    QNDEBUG(
        "view:notebook",
        "NotebookItemView::clearNotebooksFromNoteFiltersManager");

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG("view:notebook", "Note filters manager is null");
        return;
    }

    if (!m_pNoteFiltersManager->isReady())
    {
        QNDEBUG("view:notebook", "Note filters manager is not ready yet, will "
            << "postpone clearing notebooks from it");

        m_notebookLocalUidPendingNoteFiltersManagerReadiness.clear();

        QObject::connect(
            m_pNoteFiltersManager.data(),
            &NoteFiltersManager::ready,
            this,
            &NotebookItemView::onNoteFiltersManagerReady,
            Qt::UniqueConnection);

        return;
    }

    disconnectFromNoteFiltersManagerFilterChanged();
    m_pNoteFiltersManager->removeNotebooksFromFilter();
    connectToNoteFiltersManagerFilterChanged();
}

void NotebookItemView::disconnectFromNoteFiltersManagerFilterChanged()
{
    QObject::disconnect(
        m_pNoteFiltersManager.data(),
        &NoteFiltersManager::filterChanged,
        this,
        &NotebookItemView::onNoteFilterChanged);
}

void NotebookItemView::connectToNoteFiltersManagerFilterChanged()
{
    QObject::connect(
        m_pNoteFiltersManager.data(),
        &NoteFiltersManager::filterChanged,
        this,
        &NotebookItemView::onNoteFilterChanged,
        Qt::UniqueConnection);
}

bool NotebookItemView::shouldFilterBySelectedNotebook(
    const Account & account) const
{
    ApplicationSettings appSettings(account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(SIDE_PANELS_FILTER_BY_SELECTION_SETTINGS_GROUP_NAME);

    auto filterBySelectedNotebook = appSettings.value(
        FILTER_BY_SELECTED_NOTEBOOK_SETTINGS_KEY);

    appSettings.endGroup();

    if (!filterBySelectedNotebook.isValid()) {
        return true;
    }

    return filterBySelectedNotebook.toBool();
}

bool NotebookItemView::fetchCurrentNotebookCommonData(
    NotebookCommonData & data, ErrorString & errorDescription) const
{
    data.m_pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!data.m_pAction)) {
        errorDescription.appendBase(
            QT_TR_NOOP("can't cast the slot invoker to QAction"));
        return false;
    }

    data.m_pModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!data.m_pModel)) {
        QNDEBUG("view:notebook", "Non-notebook model is used");
        return false;
    }

    return true;
}

bool NotebookItemView::fetchCurrentNotebookItemData(
    NotebookItemData & itemData, ErrorString & errorDescription) const
{
    if (!fetchCurrentNotebookCommonData(itemData, errorDescription)) {
        return false;
    }

    itemData.m_localUid = itemData.m_pAction->data().toString();
    if (Q_UNLIKELY(itemData.m_localUid.isEmpty())) {
        errorDescription.appendBase(
            QT_TR_NOOP("can't get notebook local uid from QAction"));
        return false;
    }

    itemData.m_index = itemData.m_pModel->indexForLocalUid(itemData.m_localUid);
    if (Q_UNLIKELY(!itemData.m_index.isValid())) {
        errorDescription.appendBase(
            QT_TR_NOOP("the model returned invalid index for "
                       "the notebook's local uid"));
        return false;
    }

    return true;
}

bool NotebookItemView::fetchCurrentNotebookStackData(
    NotebookStackData & stackData, ErrorString & errorDescription) const
{
    if (!fetchCurrentNotebookCommonData(stackData, errorDescription)) {
        return false;
    }

    auto stackAndId = stackData.m_pAction->data().toStringList();
    if (stackAndId.size() != 2) {
        errorDescription.setBase(
            QT_TR_NOOP("can't retrieve stack data from QAction"));
        return false;
    }

    stackData.m_stack = std::move(stackAndId[0]);
    if (stackData.m_stack.isEmpty()) {
        errorDescription.setBase(
            QT_TR_NOOP("empty notebook stack in QAction data"));
        return false;
    }

    stackData.m_id = std::move(stackAndId[1]);
    return true;
}

} // namespace quentier
