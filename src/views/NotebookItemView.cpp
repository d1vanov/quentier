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

#include "NotebookItemView.h"
#include "../SettingsNames.h"
#include "../NoteFiltersManager.h"
#include "../models/NotebookModel.h"
#include "../models/NoteModel.h"
#include "../dialogs/AddOrEditNotebookDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScopedPointer>
#include <utility>

#define LAST_SELECTED_NOTEBOOK_KEY QStringLiteral("LastSelectedNotebookLocalUid")
#define LAST_EXPANDED_STACK_ITEMS_KEY QStringLiteral("LastExpandedStackItems")
#define LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY \
    QStringLiteral("LastExpandedLinkedNotebookItemsGuids")

#define REPORT_ERROR(error) \
    { \
        ErrorString errorDescription(error); \
        QNWARNING(errorDescription); \
        Q_EMIT notifyError(errorDescription); \
    }

namespace quentier {

NotebookItemView::NotebookItemView(QWidget * parent) :
    ItemView(parent),
    m_pNotebookItemContextMenu(Q_NULLPTR),
    m_pNotebookStackItemContextMenu(Q_NULLPTR),
    m_pNoteFiltersManager(),
    m_notebookLocalUidPendingNoteFiltersManagerReadiness(),
    m_trackingNotebookModelItemsState(false),
    m_trackingSelection(false),
    m_modelReady(false)
{
    QObject::connect(this,
                     QNSIGNAL(NotebookItemView,expanded,const QModelIndex&),
                     this,
                     QNSLOT(NotebookItemView,
                            onNotebookModelItemCollapsedOrExpanded,
                            const QModelIndex&));
    QObject::connect(this,
                     QNSIGNAL(NotebookItemView,collapsed,const QModelIndex&),
                     this,
                     QNSLOT(NotebookItemView,
                            onNotebookModelItemCollapsedOrExpanded,
                            const QModelIndex&));
}

void NotebookItemView::setNoteFiltersManager(NoteFiltersManager & noteFiltersManager)
{
    QNDEBUG(QStringLiteral("NotebookItemView::setNoteFiltersManager"));

    if (!m_pNoteFiltersManager.isNull())
    {
        if (m_pNoteFiltersManager.data() == &noteFiltersManager) {
            QNDEBUG(QStringLiteral("Already using this note filters manager"));
            return;
        }

        QObject::disconnect(m_pNoteFiltersManager.data(),
                            QNSIGNAL(NoteFiltersManager,filterChanged),
                            this,
                            QNSLOT(NotebookItemView,onNoteFilterChanged));
    }

    m_pNoteFiltersManager = &noteFiltersManager;

    QObject::connect(m_pNoteFiltersManager.data(),
                     QNSIGNAL(NoteFiltersManager,filterChanged),
                     this,
                     QNSLOT(NotebookItemView,onNoteFilterChanged),
                     Qt::UniqueConnection);
}

void NotebookItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("NotebookItemView::setModel"));

    NotebookModel * pPreviousModel = qobject_cast<NotebookModel*>(model());
    if (pPreviousModel) {
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,notifyError,ErrorString),
                            this,
                            QNSIGNAL(NotebookItemView,notifyError,ErrorString));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                            this,
                            QNSLOT(NotebookItemView,onAllNotebooksListed));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,notifyNotebookStackRenamed,
                                     QString,QString,QString),
                            this,
                            QNSLOT(NotebookItemView,onNotebookStackRenamed,
                                   QString,QString,QString));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,notifyNotebookStackChanged,
                                     QModelIndex),
                            this,
                            QNSLOT(NotebookItemView,onNotebookStackChanged,
                                   QModelIndex));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,aboutToAddNotebook),
                            this,
                            QNSLOT(NotebookItemView,onAboutToAddNotebook));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,addedNotebook,QModelIndex),
                            this,
                            QNSLOT(NotebookItemView,onAddedNotebook,QModelIndex));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,aboutToUpdateNotebook,
                                     QModelIndex),
                            this,
                            QNSLOT(NotebookItemView,onAboutToUpdateNotebook,
                                   QModelIndex));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,updatedNotebook,QModelIndex),
                            this,
                            QNSLOT(NotebookItemView,onUpdatedNotebook,QModelIndex));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,aboutToRemoveNotebooks),
                            this,
                            QNSLOT(NotebookItemView,onAboutToRemoveNotebooks));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(NotebookModel,removedNotebooks),
                            this,
                            QNSLOT(NotebookItemView,onRemovedNotebooks));
    }

    m_notebookLocalUidPendingNoteFiltersManagerReadiness.clear();
    m_modelReady = false;
    m_trackingSelection = false;
    m_trackingNotebookModelItemsState = false;

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(pModel);
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model has been set to the notebook "
                               "view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,notifyError,ErrorString),
                     this,
                     QNSIGNAL(NotebookItemView,notifyError,ErrorString));
    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,notifyNotebookStackRenamed,
                              QString,QString,QString),
                     this,
                     QNSLOT(NotebookItemView,onNotebookStackRenamed,
                            QString,QString,QString));
    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,notifyNotebookStackChanged,
                              QModelIndex),
                     this,
                     QNSLOT(NotebookItemView,onNotebookStackChanged,
                            QModelIndex));
    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,aboutToAddNotebook),
                     this,
                     QNSLOT(NotebookItemView,onAboutToAddNotebook));
    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,addedNotebook,QModelIndex),
                     this,
                     QNSLOT(NotebookItemView,onAddedNotebook,QModelIndex));
    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,aboutToUpdateNotebook,QModelIndex),
                     this,
                     QNSLOT(NotebookItemView,onAboutToUpdateNotebook,QModelIndex));
    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,updatedNotebook,QModelIndex),
                     this,
                     QNSLOT(NotebookItemView,onUpdatedNotebook,QModelIndex));
    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,aboutToRemoveNotebooks),
                     this,
                     QNSLOT(NotebookItemView,onAboutToRemoveNotebooks));
    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,removedNotebooks),
                     this,
                     QNSLOT(NotebookItemView,onRemovedNotebooks));

    ItemView::setModel(pModel);

    if (pNotebookModel->allNotebooksListed()) {
        QNDEBUG(QStringLiteral("All notebooks are already listed within "
                               "the model, need to select the appropriate one"));
        restoreNotebookModelItemsState(*pNotebookModel);
        restoreLastSavedSelectionOrAutoSelectNotebook(*pNotebookModel);
        m_modelReady = true;
        m_trackingSelection = true;
        m_trackingNotebookModelItemsState = true;
        return;
    }

    QObject::connect(pNotebookModel,
                     QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                     this,
                     QNSLOT(NotebookItemView,onAllNotebooksListed));
}

void NotebookItemView::setNoteModel(const NoteModel * pNoteModel)
{
#if QT_VERSION < 0x050000
    m_pNoteModel = const_cast<NoteModel*>(pNoteModel);
#else
    m_pNoteModel = pNoteModel;
#endif
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
                                       tr("Please select the notebook you want "
                                          "to delete")))
        return;
    }

    QModelIndex index = singleRow(indexes, *pNotebookModel,
                                  NotebookModel::Columns::Name);
    if (!index.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one notebook within the selection"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current notebook"),
                                       tr("More than one notebook is currently "
                                          "selected"),
                                       tr("Please select only the notebook you "
                                          "want to delete")))
        return;
    }

    deleteItem(index, *pNotebookModel);
}

void NotebookItemView::onAllNotebooksListed()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onAllNotebooksListed"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't cast the model set to the notebook item "
                                "view to the notebook model"))
        return;
    }

    QObject::disconnect(pNotebookModel,
                        QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                        this,
                        QNSLOT(NotebookItemView,onAllNotebooksListed));

    restoreNotebookModelItemsState(*pNotebookModel);
    restoreLastSavedSelectionOrAutoSelectNotebook(*pNotebookModel);

    m_modelReady = true;
    m_trackingSelection = true;
    m_trackingNotebookModelItemsState = true;
}

void NotebookItemView::onAboutToAddNotebook()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onAboutToAddNotebook"));
    prepareForNotebookModelChange();
}

void NotebookItemView::onAddedNotebook(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("NotebookItemView::onAddedNotebook"));

    Q_UNUSED(index)
    postProcessNotebookModelChange();
}

void NotebookItemView::onAboutToUpdateNotebook(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("NotebookItemView::onAboutToUpdateNotebook"));

    Q_UNUSED(index)
    prepareForNotebookModelChange();
}

void NotebookItemView::onUpdatedNotebook(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("NotebookItemView::onUpdatedNotebook"));

    Q_UNUSED(index)
    postProcessNotebookModelChange();
}

void NotebookItemView::onAboutToRemoveNotebooks()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onAboutToRemoveNotebooks"));
    prepareForNotebookModelChange();
}

void NotebookItemView::onRemovedNotebooks()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onRemovedNotebooks"));
    postProcessNotebookModelChange();
}

void NotebookItemView::onCreateNewNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onCreateNewNotebookAction"));
    Q_EMIT newNotebookCreationRequested();
}

void NotebookItemView::onRenameNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onRenameNotebookAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename notebook, can't "
                                "cast the slot invoker to QAction"))
        return;
    }

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename notebook, can't "
                                "get notebook's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename notebook: "
                                "the model returned invalid index for "
                                "the notebook's local uid"))
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
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete notebook, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete notebook, "
                                "can't get notebook's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete notebook: the model "
                                "returned invalid index for the notebook's local "
                                "uid"))
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
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't set notebook as default, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete notebook, "
                                "can't get notebook's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete notebook, the model "
                                "returned invalid index for the notebook's local "
                                "uid"))
        return;
    }

    itemIndex = pNotebookModel->index(itemIndex.row(),
                                      NotebookModel::Columns::Default,
                                      itemIndex.parent());
    bool res = pNotebookModel->setData(itemIndex, true);
    if (res) {
        QNDEBUG(QStringLiteral("Successfully set the notebook as default"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to set "
                                              "the notebook as default; Check "
                                              "the status bar for message from "
                                              "the notebook model explaining why "
                                              "the action was not successful")));
}

void NotebookItemView::onShowNotebookInfoAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onShowNotebookInfoAction"));
    Q_EMIT notebookInfoRequested();
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
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't edit notebook, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't edit notebook, "
                                "can't get notebook's local uid from QAction"))
        return;
    }

    QScopedPointer<AddOrEditNotebookDialog> pEditNotebookDialog(
        new AddOrEditNotebookDialog(pNotebookModel, this, itemLocalUid));
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
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't move notebook to stack, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    QStringList itemLocalUidAndStack = pAction->data().toStringList();
    if (itemLocalUidAndStack.size() != 2) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't move notebook to stack, "
                                "can't retrieve the notebook local uid and target "
                                "stack from QAction data"))
        return;
    }

    const QString & localUid = itemLocalUidAndStack.at(0);

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(localUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't move notebook to stack, "
                                "can't get valid model index for notebook's "
                                "local uid"))
        return;
    }

    const QString & targetStack = itemLocalUidAndStack.at(1);

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;
    QModelIndex index = pNotebookModel->moveToStack(itemIndex, targetStack);
    m_trackingSelection = wasTrackingSelection;

    if (!index.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't move notebook to stack"))
        return;
    }

    QNDEBUG(QStringLiteral("Successfully moved the notebook item to the target "
                           "stack"));
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
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove the notebook from "
                                "stack, can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove the notebook from "
                                "stack, can't get notebook's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove the notebook from "
                                "stack, the model returned invalid index for "
                                "the notebook's local uid"))
        return;
    }

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;
    QModelIndex index = pNotebookModel->removeFromStack(itemIndex);
    m_trackingSelection = wasTrackingSelection;

    if (!index.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't remove the notebook from stack"))
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
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename the notebook stack, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    QStringList actionData = pAction->data().toStringList();
    if (Q_UNLIKELY(actionData.size() != 2)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename the notebook stack, "
                                "wrong size of string list embedded into QAction"))
        return;
    }

    const QString & notebookStack = actionData.at(0);
    if (Q_UNLIKELY(notebookStack.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename the notebook stack, "
                                "can't get notebook stack from QAction"))
        return;
    }

    const QString & linkedNotebookGuid = actionData.at(1);

    QModelIndex notebookStackItemIndex =
        pNotebookModel->indexForNotebookStack(notebookStack, linkedNotebookGuid);
    if (!notebookStackItemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename the notebook stack, "
                                "the model returned invalid index for "
                                "the notebook stack"))
        return;
    }

    saveNotebookModelItemsState();

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
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove all notebooks from "
                                "stack, can't cast the slot invoker to QAction"))
        return;
    }

    QStringList actionData = pAction->data().toStringList();
    if (Q_UNLIKELY(actionData.size() != 2)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove all notebooks from "
                                "stack, wrong size of string list embedded into "
                                "QAction"))
        return;
    }

    const QString & notebookStack = actionData.at(0);
    if (Q_UNLIKELY(notebookStack.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove all notebooks from "
                                "stack, can't get notebook stack from QAction"))
        return;
    }

    const QString & linkedNotebookGuid = actionData.at(1);

    while(true)
    {
        QModelIndex notebookStackItemIndex =
            pNotebookModel->indexForNotebookStack(notebookStack, linkedNotebookGuid);
        if (!notebookStackItemIndex.isValid()) {
            QNDEBUG(QStringLiteral("Notebook stack item index is invalid, "
                                   "breaking the loop"));
            break;
        }

        QModelIndex childIndex =
            notebookStackItemIndex.child(0, NotebookModel::Columns::Name);
        if (!childIndex.isValid()) {
            QNDEBUG(QStringLiteral("Detected invalid child item index for "
                                   "the notebook stack item, breaking the loop"));
            break;
        }

        bool wasTrackingSelection = m_trackingSelection;
        m_trackingSelection = false;
        Q_UNUSED(pNotebookModel->removeFromStack(childIndex))
        m_trackingSelection = wasTrackingSelection;
    }

    QNDEBUG(QStringLiteral("Successfully removed notebook items from the stack"));
}

void NotebookItemView::onFavoriteAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onFavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't favorite notebook, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, true);
}

void NotebookItemView::onUnfavoriteAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onUnfavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't unfavorite notebook, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, false);
}

void NotebookItemView::onNotebookModelItemCollapsedOrExpanded(
    const QModelIndex & index)
{
    QNTRACE(QStringLiteral("NotebookItemView::onNotebookModelItemCollapsedOrExpanded: ")
            << QStringLiteral("index: valid = ")
            << (index.isValid()
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row()
            << QStringLiteral(", column = ") << index.column()
            << QStringLiteral(", parent: valid = ")
            << (index.parent().isValid()
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.parent().row()
            << QStringLiteral(", column = ") << index.parent().column());

    if (!m_trackingNotebookModelItemsState) {
        QNDEBUG(QStringLiteral("Not tracking the expanded/collapsed state of "
                               "notebook model items at the moment"));
        return;
    }

    saveNotebookModelItemsState();
}

void NotebookItemView::onNotebookStackRenamed(const QString & previousStackName,
                                              const QString & newStackName,
                                              const QString & linkedNotebookGuid)
{
    QNDEBUG(QStringLiteral("NotebookItemView::onNotebookStackRenamed: ")
            << QStringLiteral("previous stack name = ") << previousStackName
            << QStringLiteral(", new stack name = ") << newStackName
            << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid);

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNWARNING(QStringLiteral("Received notebook stack item rename event but "
                                 "can't cast the model to the notebook one"));
        return;
    }

    ApplicationSettings appSettings(pNotebookModel->account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));
    QString key = LAST_EXPANDED_STACK_ITEMS_KEY;
    if (!linkedNotebookGuid.isEmpty()) {
        key += QStringLiteral("/") + linkedNotebookGuid;
    }

    QStringList expandedStacks = appSettings.value(key).toStringList();
    appSettings.endGroup();

    int previousStackIndex = expandedStacks.indexOf(previousStackName);
    if (previousStackIndex < 0) {
        QNDEBUG(QStringLiteral("The renamed stack item hasn't been expanded"));
    }
    else {
        expandedStacks.replace(previousStackIndex, newStackName);
    }

    setStacksExpanded(expandedStacks, *pNotebookModel, linkedNotebookGuid);

    QModelIndex newStackItemIndex =
        pNotebookModel->indexForNotebookStack(newStackName, linkedNotebookGuid);
    if (Q_UNLIKELY(!newStackItemIndex.isValid())) {
        QNWARNING(QStringLiteral("Can't select the just renamed notebook stack: "
                                 "notebook model returned invalid index for "
                                 "the new notebook stack name"));
        autoSelectNotebook(*pNotebookModel);
        return;
    }

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNWARNING(QStringLiteral("Can't select the just renamed notebook stack: "
                                 "no selection model in the view"));
        return;
    }

    pSelectionModel->select(newStackItemIndex, QItemSelectionModel::ClearAndSelect |
                                               QItemSelectionModel::Rows |
                                               QItemSelectionModel::Current);
}

void NotebookItemView::onNotebookStackChanged(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("NotebookItemView::onNotebookStackChanged"));

    Q_UNUSED(index)

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    restoreNotebookModelItemsState(*pNotebookModel);
    restoreLastSavedSelectionOrAutoSelectNotebook(*pNotebookModel);
}

void NotebookItemView::onNoteFilterChanged()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onNoteFilterChanged"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG(QStringLiteral("Note filters manager is null"));
        return;
    }

    if (m_pNoteFiltersManager->isFilterBySearchStringActive()) {
        QNDEBUG(QStringLiteral("Filter by search string is active"));
        clearSelectionImpl();
        return;
    }

    if (!m_pNoteFiltersManager->savedSearchLocalUidInFilter().isEmpty()) {
        QNDEBUG(QStringLiteral("Filter by saved search is active"));
        clearSelectionImpl();
        return;
    }

    const QStringList & notebookLocalUids =
        m_pNoteFiltersManager->notebookLocalUidsInFilter();
    if (notebookLocalUids.size() != 1) {
        QNDEBUG(QStringLiteral("Not exactly one notebook local uid is within ")
                << QStringLiteral("the filter: ")
                << notebookLocalUids.join(QStringLiteral(", ")));
        clearSelectionImpl();
        return;
    }

    QModelIndex notebookIndex =
        pNotebookModel->indexForLocalUid(notebookLocalUids[0]);
    if (Q_UNLIKELY(!notebookIndex.isValid())) {
        QNWARNING(QStringLiteral("The filtered notebook local uid's index is "
                                 "invalid"));
        clearSelectionImpl();
        return;
    }

    setCurrentIndex(notebookIndex);
}

void NotebookItemView::onNoteFiltersManagerReady()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onNoteFiltersManagerReady"));

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG(QStringLiteral("Note filters manager is null"));
        return;
    }

    QObject::disconnect(m_pNoteFiltersManager.data(),
                        QNSIGNAL(NoteFiltersManager,ready),
                        this,
                        QNSLOT(NotebookItemView,onNoteFiltersManagerReady));

    if (m_notebookLocalUidPendingNoteFiltersManagerReadiness.isEmpty()) {
        QNDEBUG(QStringLiteral("No notebook local uid to set to the filter"));
        return;
    }

    QString notebookLocalUid = m_notebookLocalUidPendingNoteFiltersManagerReadiness;
    m_notebookLocalUidPendingNoteFiltersManagerReadiness.clear();
    setSelectedNotebookToNoteFilterManager(notebookLocalUid);
}

void NotebookItemView::selectionChanged(const QItemSelection & selected,
                                        const QItemSelection & deselected)
{
    QNDEBUG(QStringLiteral("NotebookItemView::selectionChanged"));

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void NotebookItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("NotebookItemView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: notebook item view received "
                                 "context menu event with null pointer "
                                 "to the context menu event"));
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

    const NotebookModelItem * pModelItem =
        pNotebookModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't show the context menu for the notebook "
                                "model item: no item corresponding to the clicked "
                                "item's index"))
        return;
    }

    if (pModelItem->type() == NotebookModelItem::Type::Notebook)
    {
        const NotebookItem * pNotebookItem = pModelItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem)) {
            REPORT_ERROR(QT_TR_NOOP("Can't show the context menu for the notebook "
                                    "item: the model item reported that it points "
                                    "to the notebook item but the pointer to "
                                    "the notebook item is null"))
            return;
        }

        showNotebookItemContextMenu(*pNotebookItem, pEvent->globalPos(),
                                    *pNotebookModel);
    }
    else if (pModelItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * pNotebookStackItem =
            pModelItem->notebookStackItem();
        if (Q_UNLIKELY(!pNotebookStackItem)) {
            REPORT_ERROR(QT_TR_NOOP("Can't show the context menu for the notebook "
                                    "stack item: the model item reported that it "
                                    "points to the notebook stack item but "
                                    "the pointer to the notebook stack item is "
                                    "null"))
            return;
        }

        showNotebookStackItemContextMenu(*pNotebookStackItem, *pModelItem,
                                         pEvent->globalPos(), *pNotebookModel);
    }
}

void NotebookItemView::deleteItem(const QModelIndex & itemIndex,
                                  NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::deleteItem"));

    const NotebookModelItem * pModelItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't find the notebook model "
                                "item meant to be deleted"))
        return;
    }

    if (pModelItem->type() == NotebookModelItem::Type::Notebook)
    {
        const NotebookItem * pNotebookItem = pModelItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem)) {
            REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete notebook: "
                                    "the model item reported that it points to "
                                    "the notebook item but the pointer to "
                                    "the notebook item is null"))
            return;
        }

        int confirm = warningMessageBox(this, tr("Confirm the notebook deletion"),
                                        tr("Are you sure you want to delete "
                                           "the notebook?"),
                                        tr("Note that this action is not "
                                           "reversible and the deletion "
                                           "of the notebook would also mean "
                                           "the deletion of all the notes "
                                           "stored inside it!"),
                                        QMessageBox::Ok | QMessageBox::No);
        if (confirm != QMessageBox::Ok) {
            QNDEBUG(QStringLiteral("Notebook deletion was not confirmed"));
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG(QStringLiteral("Successfully deleted notebook"));
            autoSelectNotebook(model);
            return;
        }

        Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to "
                                                  "delete the notebook; Check "
                                                  "the status bar for message "
                                                  "from the notebook model "
                                                  "explaining why the notebook "
                                                  "could not be deleted")))
    }
    else if (pModelItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * pNotebookStackItem =
            pModelItem->notebookStackItem();
        if (Q_UNLIKELY(!pNotebookStackItem)) {
            REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete notebook stack: "
                                    "the model item reported that it points to "
                                    "the notebook stack but the pointer to "
                                    "the notebook stack item is null"))
            return;
        }

        int confirm = warningMessageBox(this,
                                        tr("Confirm the notebook stack deletion"),
                                        tr("Are you sure you want to delete "
                                           "the whole stack of notebooks?"),
                                        tr("Note that this action is not "
                                           "reversible and the deletion "
                                           "of the whole stack of notebooks "
                                           "would also mean the deletion of all "
                                           "the notes stored inside the notebooks!"),
                                        QMessageBox::Ok | QMessageBox::No);
        if (confirm != QMessageBox::Ok) {
            QNDEBUG(QStringLiteral("Notebook stack deletion was not confirmed"));
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG(QStringLiteral("Successfully deleted notebook stack"));
            autoSelectNotebook(model);
            return;
        }

        Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to "
                                                  "delete the notebook stack; "
                                                  "Check the status bar for message "
                                                  "from the notebook model "
                                                  "explaining why the notebook "
                                                  "stack could not be deleted")));
    }
    else
    {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete notebook(s): "
                                "found notebook model item of unknown type"))
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
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), \
                         this, QNSLOT(NotebookItemView,slot)); \
        menu->addAction(pAction); \
    }

    ADD_CONTEXT_MENU_ACTION(tr("Create new notebook") + QStringLiteral("..."),
                            m_pNotebookItemContextMenu, onCreateNewNotebookAction,
                            item.localUid(), true);

    m_pNotebookItemContextMenu->addSeparator();

    bool canRename = (item.isUpdatable() && item.nameIsUpdatable());
    ADD_CONTEXT_MENU_ACTION(tr("Rename"), m_pNotebookItemContextMenu,
                            onRenameNotebookAction, item.localUid(), canRename);

    bool canDeleteNotebook = !m_pNoteModel.isNull() && item.guid().isEmpty();
    if (canDeleteNotebook) {
        ADD_CONTEXT_MENU_ACTION(tr("Delete"), m_pNotebookItemContextMenu,
                                onDeleteNotebookAction, item.localUid(),
                                true);
    }

    ADD_CONTEXT_MENU_ACTION(tr("Edit") + QStringLiteral("..."),
                            m_pNotebookItemContextMenu, onEditNotebookAction,
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
        QMenu * pTargetStackSubMenu =
            m_pNotebookItemContextMenu->addMenu(tr("Move to stack"));
        for(auto it = stacks.constBegin(),
            end = stacks.constEnd(); it != end; ++it)
        {
            QStringList dataPair;
            dataPair.reserve(2);
            dataPair << item.localUid();
            dataPair << *it;
            ADD_CONTEXT_MENU_ACTION(*it, pTargetStackSubMenu,
                                    onMoveNotebookToStackAction,
                                    dataPair, item.isUpdatable());
        }
    }

    if (!stack.isEmpty())
    {
        ADD_CONTEXT_MENU_ACTION(tr("Remove from stack"),
                                m_pNotebookItemContextMenu,
                                onRemoveNotebookFromStackAction, item.localUid(),
                                item.isUpdatable());
    }

    m_pNotebookItemContextMenu->addSeparator();

    if (!item.isDefault()) {
        ADD_CONTEXT_MENU_ACTION(tr("Set default"), m_pNotebookItemContextMenu,
                                onSetNotebookDefaultAction, item.localUid(),
                                item.isUpdatable());
    }

    if (item.isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(tr("Unfavorite"), m_pNotebookItemContextMenu,
                                onUnfavoriteAction, item.localUid(),
                                item.isUpdatable());
    }
    else {
        ADD_CONTEXT_MENU_ACTION(tr("Favorite"), m_pNotebookItemContextMenu,
                                onFavoriteAction, item.localUid(),
                                item.isUpdatable());
    }

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."),
                            m_pNotebookItemContextMenu,
                            onShowNotebookInfoAction, item.localUid(), true);

    m_pNotebookItemContextMenu->show();
    m_pNotebookItemContextMenu->exec(point);
}

void NotebookItemView::showNotebookStackItemContextMenu(
    const NotebookStackItem & item,
    const NotebookModelItem & modelItem,
    const QPoint & point, NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::showNotebookStackItemContextMenu: (")
            << point.x() << QStringLiteral(", ") << point.y()
            << QStringLiteral(", item: ") << item);

    delete m_pNotebookStackItemContextMenu;
    m_pNotebookStackItemContextMenu = new QMenu(this);

    ADD_CONTEXT_MENU_ACTION(tr("Create new notebook") + QStringLiteral("..."),
                            m_pNotebookStackItemContextMenu,
                            onCreateNewNotebookAction, item.name(), true);

    m_pNotebookStackItemContextMenu->addSeparator();

    bool allChildrenUpdatable = false;

    QString linkedNotebookGuid;
    const NotebookModelItem * pParentItem = modelItem.parent();
    if (pParentItem &&
        (pParentItem->type() == NotebookModelItem::Type::LinkedNotebook) &&
        pParentItem->notebookLinkedNotebookItem())
    {
        linkedNotebookGuid =
            pParentItem->notebookLinkedNotebookItem()->linkedNotebookGuid();
    }

    QModelIndex stackItemIndex = model.indexForNotebookStack(item.name(),
                                                             linkedNotebookGuid);
    const NotebookModelItem * pModelItem = model.itemForIndex(stackItemIndex);
    if (Q_LIKELY(pModelItem))
    {
        allChildrenUpdatable = true;

        QList<const NotebookModelItem*> children = pModelItem->children();
        for(auto it = children.constBegin(),
            end = children.constEnd(); it != end; ++it)
        {
            const NotebookModelItem * pChildItem = *it;
            if (Q_UNLIKELY(!pChildItem)) {
                QNWARNING(QStringLiteral("Found null child at stack item: ")
                          << *pModelItem);
                continue;
            }

            if (Q_UNLIKELY(pChildItem->type() != NotebookModelItem::Type::Notebook)) {
                QNWARNING(QStringLiteral("Found child of non-notebook type at ")
                          << QStringLiteral("stack item: ") << *pModelItem
                          << QStringLiteral("\nChild item: ") << *pChildItem);
                continue;
            }

            const NotebookItem * pNotebookItem = pChildItem->notebookItem();
            if (Q_UNLIKELY(!pNotebookItem)) {
                QNWARNING(QStringLiteral("Found child of notebook type but with ")
                          << QStringLiteral("null notebook item at stack item: ")
                          << *pModelItem << QStringLiteral("\nChild item: ")
                          << *pChildItem);
                continue;
            }

            if (!pNotebookItem->isUpdatable()) {
                allChildrenUpdatable = false;
                break;
            }
        }
    }

    ADD_CONTEXT_MENU_ACTION(tr("Rename"), m_pNotebookStackItemContextMenu,
                            onRenameNotebookStackAction,
                            QStringList() << item.name() << linkedNotebookGuid,
                            allChildrenUpdatable);

    ADD_CONTEXT_MENU_ACTION(tr("Remove stack"), m_pNotebookStackItemContextMenu,
                            onRemoveNotebooksFromStackAction,
                            QStringList() << item.name() << linkedNotebookGuid,
                            allChildrenUpdatable);

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."),
                            m_pNotebookStackItemContextMenu,
                            onShowNotebookInfoAction, item.name(), true);

    m_pNotebookStackItemContextMenu->show();
    m_pNotebookStackItemContextMenu->exec(point);
}

#undef ADD_CONTEXT_MENU_ACTION

void NotebookItemView::saveNotebookModelItemsState()
{
    QNDEBUG(QStringLiteral("NotebookItemView::saveNotebookModelItemsState"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QMap<QString, QStringList> expandedStackItemsByLinkedNotebookGuid;
    QStringList expandedLinkedNotebookItemsGuids;

    QModelIndexList indexes = pNotebookModel->persistentIndexes();
    for(auto it = indexes.constBegin(),
        end = indexes.constEnd(); it != end; ++it)
    {
        const QModelIndex & index = *it;
        if (!isExpanded(index)) {
            continue;
        }

        const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING(QStringLiteral("Can't save the state of notebook stack "
                                     "expanded/folded state: no notebook model "
                                     "item corresponding to the persistent "
                                     "notebook model index"));
            continue;
        }

        if (pModelItem->type() == NotebookModelItem::Type::Notebook) {
            continue;
        }

        if (pModelItem->type() == NotebookModelItem::Type::Stack)
        {
            QString linkedNotebookGuid;
            const NotebookModelItem * pParentItem = pModelItem->parent();
            if (pParentItem &&
                (pParentItem->type() == NotebookModelItem::Type::LinkedNotebook) &&
                pParentItem->notebookLinkedNotebookItem())
            {
                linkedNotebookGuid =
                    pParentItem->notebookLinkedNotebookItem()->linkedNotebookGuid();
            }

            const NotebookStackItem * pStackItem = pModelItem->notebookStackItem();
            if (Q_UNLIKELY(!pStackItem)) {
                QNWARNING(QStringLiteral("Can't save the state of notebook stack "
                                         "expanded/folded state: the model item "
                                         "of stack type has no actual notebook "
                                         "stack item"));
                continue;
            }

            const QString & stackItemName = pStackItem->name();
            if (Q_UNLIKELY(stackItemName.isEmpty())) {
                QNDEBUG(QStringLiteral("Skipping the notebook stack item without "
                                       "a name"));
                continue;
            }

            expandedStackItemsByLinkedNotebookGuid[linkedNotebookGuid].append(stackItemName);
        }
        else if (pModelItem->type() == NotebookModelItem::Type::LinkedNotebook)
        {
            const NotebookLinkedNotebookRootItem * pLinkedNotebookItem =
                pModelItem->notebookLinkedNotebookItem();
            if (Q_UNLIKELY(!pLinkedNotebookItem)) {
                QNWARNING(QStringLiteral("Can't save the state of linked notebook "
                                         "expanded/folded state: the model item "
                                         "of linked notebook type has no actual "
                                         "linked notebook item"));
                continue;
            }

            expandedLinkedNotebookItemsGuids << pLinkedNotebookItem->linkedNotebookGuid();
        }
    }

    ApplicationSettings appSettings(pNotebookModel->account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));

    for(auto it = expandedStackItemsByLinkedNotebookGuid.constBegin(),
        end = expandedStackItemsByLinkedNotebookGuid.constEnd();
        it != end; ++it)
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

    appSettings.setValue(LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY,
                         expandedLinkedNotebookItemsGuids);
    appSettings.endGroup();
}

void NotebookItemView::restoreNotebookModelItemsState(const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::restoreNotebookModelItemsState"));

    const QHash<QString,QString> & linkedNotebookOwnerNamesByGuid =
        model.linkedNotebookOwnerNamesByGuid();

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));
    QStringList expandedStacks =
        appSettings.value(LAST_EXPANDED_STACK_ITEMS_KEY).toStringList();

    QHash<QString,QStringList> expandedStacksByLinkedNotebookGuid;
    for(auto it = linkedNotebookOwnerNamesByGuid.constBegin(),
        end = linkedNotebookOwnerNamesByGuid.constEnd(); it != end; ++it)
    {
        const QString & linkedNotebookGuid = it.key();
        QStringList expandedStacksForLinkedNotebook =
            appSettings.value(LAST_EXPANDED_STACK_ITEMS_KEY + QStringLiteral("/") +
                              linkedNotebookGuid).toStringList();
        if (expandedStacksForLinkedNotebook.isEmpty()) {
            continue;
        }

        expandedStacksByLinkedNotebookGuid[linkedNotebookGuid] =
            expandedStacksForLinkedNotebook;
    }

    QStringList expandedLinkedNotebookItemsGuids =
        appSettings.value(LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY).toStringList();
    appSettings.endGroup();

    bool wasTrackingNotebookItemsState = m_trackingNotebookModelItemsState;
    m_trackingNotebookModelItemsState = false;
    setStacksExpanded(expandedStacks, model, QString());

    for(auto it = expandedStacksByLinkedNotebookGuid.constBegin(),
        end = expandedStacksByLinkedNotebookGuid.constEnd(); it != end; ++it)
    {
        setStacksExpanded(it.value(), model, it.key());
    }

    setLinkedNotebooksExpanded(expandedLinkedNotebookItemsGuids, model);

    m_trackingNotebookModelItemsState = wasTrackingNotebookItemsState;
}

void NotebookItemView::setStacksExpanded(const QStringList & expandedStackNames,
                                         const NotebookModel & model,
                                         const QString & linkedNotebookGuid)
{
    QNDEBUG(QStringLiteral("NotebookItemView::setStacksExpanded: ")
            << QStringLiteral("linked notebook guid = ") << linkedNotebookGuid);

    for(auto it = expandedStackNames.constBegin(),
        end = expandedStackNames.constEnd(); it != end; ++it)
    {
        const QString & expandedStack = *it;

        QModelIndex index = model.indexForNotebookStack(expandedStack,
                                                        linkedNotebookGuid);
        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void NotebookItemView::setLinkedNotebooksExpanded(
    const QStringList & expandedLinkedNotebookGuids, const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::setLinkedNotebooksExpanded: ")
            << expandedLinkedNotebookGuids.join(QStringLiteral(", ")));

    for(auto it = expandedLinkedNotebookGuids.constBegin(),
        end = expandedLinkedNotebookGuids.constEnd(); it != end; ++it)
    {
        const QString & expandedLinkedNotebookGuid = *it;

        QModelIndex index =
            model.indexForLinkedNotebookGuid(expandedLinkedNotebookGuid);
        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void NotebookItemView::restoreLastSavedSelectionOrAutoSelectNotebook(
    const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::"
                           "restoreLastSavedSelectionOrAutoSelectNotebook"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't restore the last selected notebook or "
                                "auto-select one: no selection model in the view"))
        return;
    }

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));
    QString lastSelectedNotebookLocalUid =
        appSettings.value(LAST_SELECTED_NOTEBOOK_KEY).toString();
    appSettings.endGroup();

    QNTRACE(QStringLiteral("Last selected notebook local uid: ")
            << lastSelectedNotebookLocalUid);

    if (!lastSelectedNotebookLocalUid.isEmpty())
    {
        QModelIndex lastSelectedNotebookIndex =
            model.indexForLocalUid(lastSelectedNotebookLocalUid);
        if (lastSelectedNotebookIndex.isValid())
        {
            QNDEBUG(QStringLiteral("Selecting the last selected notebook item: ")
                    << QStringLiteral("local uid = ")
                    << lastSelectedNotebookLocalUid
                    << QStringLiteral(", index: row = ")
                    << lastSelectedNotebookIndex.row()
                    << QStringLiteral(", column = ")
                    << lastSelectedNotebookIndex.column()
                    << QStringLiteral(", internal id = ")
                    << lastSelectedNotebookIndex.internalId());
            pSelectionModel->select(lastSelectedNotebookIndex,
                                    QItemSelectionModel::ClearAndSelect |
                                    QItemSelectionModel::Rows |
                                    QItemSelectionModel::Current);
            return;
        }

        QNTRACE(QStringLiteral("The notebook model returned invalid index for ")
                << QStringLiteral("local uid ") << lastSelectedNotebookLocalUid);
    }

    autoSelectNotebook(model);
}

void NotebookItemView::autoSelectNotebook(const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::autoSelectNotebook"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't auto-select notebook: no selection model "
                                "in the view"))
        return;
    }

    QModelIndex lastUsedNotebookIndex = model.lastUsedNotebookIndex();
    if (lastUsedNotebookIndex.isValid())
    {
        QNDEBUG(QStringLiteral("Selecting the last used notebook item"));
        pSelectionModel->select(lastUsedNotebookIndex,
                                QItemSelectionModel::ClearAndSelect |
                                QItemSelectionModel::Rows |
                                QItemSelectionModel::Current);
        if (!m_trackingSelection)
        {
            const NotebookModelItem * pItem =
                model.itemForIndex(lastUsedNotebookIndex);
            if (pItem && pItem->notebookItem())
            {
                setSelectedNotebookToNoteFilterManager(
                    pItem->notebookItem()->localUid());
            }
        }
        return;
    }

    QModelIndex defaultNotebookIndex = model.defaultNotebookIndex();
    if (defaultNotebookIndex.isValid())
    {
        QNDEBUG(QStringLiteral("Selecting the default notebook item"));
        pSelectionModel->select(defaultNotebookIndex,
                                QItemSelectionModel::ClearAndSelect |
                                QItemSelectionModel::Rows |
                                QItemSelectionModel::Current);
        if (!m_trackingSelection)
        {
            const NotebookModelItem * pItem =
                model.itemForIndex(defaultNotebookIndex);
            if (pItem && pItem->notebookItem())
            {
                setSelectedNotebookToNoteFilterManager(
                    pItem->notebookItem()->localUid());
            }
        }
        return;
    }

    QModelIndexList persistentIndexes = model.persistentIndexes();
    if (persistentIndexes.isEmpty()) {
        QNDEBUG(QStringLiteral("No persistent indexes returned by the notebook "
                               "model, nothing to select"));
        return;
    }

    QNDEBUG(QStringLiteral("Selecting random notebook"));
    QModelIndex index = persistentIndexes.at(0);
    pSelectionModel->select(index, QItemSelectionModel::ClearAndSelect |
                                   QItemSelectionModel::Rows |
                                   QItemSelectionModel::Current);

    if (!m_trackingSelection)
    {
        const NotebookModelItem * pItem = model.itemForIndex(index);
        if (pItem && pItem->notebookItem()) {
            setSelectedNotebookToNoteFilterManager(pItem->notebookItem()->localUid());
        }
    }
}

void NotebookItemView::selectionChangedImpl(const QItemSelection & selected,
                                            const QItemSelection & deselected)
{
    QNTRACE(QStringLiteral("NotebookItemView::selectionChangedImpl"));

    if (!m_trackingSelection) {
        QNTRACE(QStringLiteral("Not tracking selection at this time, skipping"));
        return;
    }

    Q_UNUSED(deselected)

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
    QModelIndex sourceIndex = singleRow(selectedIndexes, *pNotebookModel,
                                        NotebookModel::Columns::Name);
    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        return;
    }

    const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't find the notebook model "
                                "item corresponging to the selected index"))
        return;
    }

    QNTRACE(QStringLiteral("Currently selected notebook item: ") << *pModelItem);

    if (pModelItem->type() != NotebookModelItem::Type::Notebook) {
        QNDEBUG(QStringLiteral("Non-notebook item is selected"));
        return;
    }

    const NotebookItem * pNotebookItem = pModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: selected notebook model item "
                                "seems to point to notebook (not to stack) but "
                                "returns null pointer to the notebook item"))
        return;
    }

    if (!pNotebookItem->linkedNotebookGuid().isEmpty()) {
        QNDEBUG(QStringLiteral("Notebook from the linked notebook is selected, "
                               "won't do anything"));
        return;
    }

    persistSelectedNotebookLocalUid(*pNotebookModel, pNotebookItem->localUid());
    setSelectedNotebookToNoteFilterManager(pNotebookItem->localUid());
}

void NotebookItemView::persistSelectedNotebookLocalUid(
    const NotebookModel & notebookModel, const QString & notebookLocalUid)
{
    QNDEBUG(QStringLiteral("NotebookItemView::persistSelectedNotebookLocalUid: ")
            << notebookLocalUid);

    ApplicationSettings appSettings(notebookModel.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));
    appSettings.setValue(LAST_SELECTED_NOTEBOOK_KEY, notebookLocalUid);
    appSettings.endGroup();
}

void NotebookItemView::clearSelectionImpl()
{
    QNDEBUG(QStringLiteral("NotebookItemView::clearSelectionImpl"));

    m_trackingSelection = false;
    clearSelection();
    m_trackingSelection = true;

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    persistSelectedNotebookLocalUid(*pNotebookModel, QString());
}

void NotebookItemView::setFavoritedFlag(const QAction & action,
                                        const bool favorited)
{
    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QString itemLocalUid = action.data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't set the favorited flag "
                                "for the notebook, can't get notebook's local "
                                "uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't set the favorited flag "
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
        QNDEBUG(QStringLiteral("The model is not ready yet"));
        return;
    }

    saveNotebookModelItemsState();
    m_trackingSelection = false;
    m_trackingNotebookModelItemsState = false;
}

void NotebookItemView::postProcessNotebookModelChange()
{
    if (!m_modelReady) {
        QNDEBUG(QStringLiteral("The model is not ready yet"));
        return;
    }

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    restoreNotebookModelItemsState(*pNotebookModel);
    m_trackingNotebookModelItemsState = true;

    restoreLastSavedSelectionOrAutoSelectNotebook(*pNotebookModel);
    m_trackingSelection = true;
}

void NotebookItemView::setSelectedNotebookToNoteFilterManager(
    const QString & notebookLocalUid)
{
    QNDEBUG(QStringLiteral("NotebookItemView::setSelectedNotebookToNoteFilterManager: ")
            << notebookLocalUid);

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG(QStringLiteral("Note filters manager is null"));
        return;
    }

    if (!m_pNoteFiltersManager->isReady())
    {
        QNDEBUG(QStringLiteral("Note filters manager is not ready yet, will ")
                << QStringLiteral("postpone setting the notebook to it: ")
                << notebookLocalUid);

        QObject::connect(m_pNoteFiltersManager.data(),
                         QNSIGNAL(NoteFiltersManager,ready),
                         this,
                         QNSLOT(NotebookItemView,onNoteFiltersManagerReady),
                         Qt::UniqueConnection);

        m_notebookLocalUidPendingNoteFiltersManagerReadiness = notebookLocalUid;
        return;
    }

    if (m_pNoteFiltersManager->isFilterBySearchStringActive()) {
        QNDEBUG(QStringLiteral("Filter by search string is active, won't reset "
                               "filter to seleted notebook"));
        return;
    }

    const QString & savedSearchLocalUidInFilter =
        m_pNoteFiltersManager->savedSearchLocalUidInFilter();
    if (!savedSearchLocalUidInFilter.isEmpty()) {
        QNDEBUG(QStringLiteral("Filter by saved search is active, won't reset "
                               "filter to selected notebook"));
        return;
    }

    QObject::disconnect(m_pNoteFiltersManager.data(),
                        QNSIGNAL(NoteFiltersManager,filterChanged),
                        this, QNSLOT(NotebookItemView,onNoteFilterChanged));

    m_pNoteFiltersManager->resetFilterToNotebookLocalUid(notebookLocalUid);

    QObject::connect(m_pNoteFiltersManager.data(),
                     QNSIGNAL(NoteFiltersManager,filterChanged),
                     this, QNSLOT(NotebookItemView,onNoteFilterChanged),
                     Qt::UniqueConnection);
}

} // namespace quentier
