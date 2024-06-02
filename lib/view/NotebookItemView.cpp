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

#include "NotebookItemView.h"

#include "ItemSelectionModel.h"
#include "Utils.h"

#include <lib/dialog/AddOrEditNotebookDialog.h>
#include <lib/model/note/NoteModel.h>
#include <lib/model/notebook/NotebookModel.h>
#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/SidePanelsFiltering.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <memory>
#include <utility>

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorToReport(error);                                      \
        QNWARNING("view::NotebookItemView", errorToReport);                    \
        Q_EMIT notifyError(errorToReport);                                     \
    }

#define REPORT_ERROR_X(error, prefix)                                          \
    {                                                                          \
        ErrorString fullError = prefix;                                        \
        fullError.appendBase(QT_TR_NOOP("internal error"));                    \
        fullError.appendBase(error.base());                                    \
        fullError.appendBase(error.additionalBases());                         \
        fullError.details() = error.details();                                 \
        REPORT_ERROR(fullError);                                               \
    }

#define FETCH_ITEM_DATA(prefix)                                                \
    NotebookItemData itemData;                                                 \
    ErrorString error;                                                         \
    if (!fetchCurrentNotebookItemData(itemData, error)) {                      \
        if (!error.isEmpty()) {                                                \
            REPORT_ERROR_X(error, errorPrefix);                                \
        }                                                                      \
        return;                                                                \
    }

#define FETCH_STACK_DATA(prefix)                                               \
    NotebookStackData stackData;                                               \
    ErrorString error;                                                         \
    if (!fetchCurrentNotebookStackData(stackData, error)) {                    \
        if (!error.isEmpty()) {                                                \
            REPORT_ERROR_X(error, errorPrefix);                                \
        }                                                                      \
        return;                                                                \
    }

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gNotebookItemViewGroupKey = "NotebookItemView"sv;
constexpr auto gLastExpandedStackItemsKey = "LastExpandedStackItems"sv;

constexpr auto gLastExpandedLinkedNotebookItemsKey =
    "LastExpandedLinkedNotebookItemsGuids"sv;

constexpr auto gAllNotebooksRootItemExpandedKey =
    "AllNotebooksRootItemExpanded"sv;

} // namespace

NotebookItemView::NotebookItemView(QWidget * parent) :
    AbstractNoteFilteringTreeView(QStringLiteral("notebook"), parent)
{}

void NotebookItemView::setNoteModel(const NoteModel * pNoteModel)
{
    m_noteModel = pNoteModel;
}

void NotebookItemView::saveItemsState()
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::saveItemsState");

    auto * notebookModel = qobject_cast<NotebookModel *>(model());
    if (Q_UNLIKELY(!notebookModel)) {
        QNDEBUG("view::NotebookItemView", "Non-notebook model is used");
        return;
    }

    QMap<QString, QStringList> expandedStackItemsByLinkedNotebookGuid;
    QStringList expandedLinkedNotebookItemsGuids;

    const auto indexes = notebookModel->persistentIndexes();
    for (const auto & index: std::as_const(indexes)) {
        if (!isExpanded(index)) {
            continue;
        }

        const auto * modelItem = notebookModel->itemForIndex(index);
        if (Q_UNLIKELY(!modelItem)) {
            QNWARNING(
                "view::NotebookItemView",
                "Can't save the state of notebook stack "
                    << "expanded/folded state: no notebook model item "
                    << "corresponding to the persistent notebook model index");
            continue;
        }

        const auto * notebookItem = modelItem->cast<NotebookItem>();
        if (notebookItem) {
            continue;
        }

        const auto * stackItem = modelItem->cast<StackItem>();
        if (stackItem) {
            QString linkedNotebookGuid;
            const auto * parentItem = modelItem->parent();
            if (parentItem &&
                (parentItem->type() ==
                 INotebookModelItem::Type::LinkedNotebook))
            {
                auto * linkedNotebookItem =
                    parentItem->cast<LinkedNotebookRootItem>();

                if (linkedNotebookItem) {
                    linkedNotebookGuid =
                        linkedNotebookItem->linkedNotebookGuid();
                }
            }

            const QString & stackItemName = stackItem->name();
            if (Q_UNLIKELY(stackItemName.isEmpty())) {
                QNDEBUG(
                    "view::NotebookItemView",
                    "Skipping the notebook stack item without a name");
                continue;
            }

            expandedStackItemsByLinkedNotebookGuid[linkedNotebookGuid].append(
                stackItemName);

            continue;
        }

        const auto * linkedNotebookItem =
            modelItem->cast<LinkedNotebookRootItem>();

        if (linkedNotebookItem) {
            expandedLinkedNotebookItemsGuids
                << linkedNotebookItem->linkedNotebookGuid();
        }
    }

    ApplicationSettings appSettings{
        notebookModel->account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(gNotebookItemViewGroupKey);

    for (const auto it: qevercloud::toRange(
             std::as_const(expandedStackItemsByLinkedNotebookGuid)))
    {
        const QString & linkedNotebookGuid = it.key();
        const QStringList & stackItemNames = it.value();

        if (linkedNotebookGuid.isEmpty()) {
            appSettings.setValue(gLastExpandedStackItemsKey, stackItemNames);
        }
        else {
            QString key = QString::fromUtf8(
                gLastExpandedStackItemsKey.data(),
                gLastExpandedStackItemsKey.size());

            if (!linkedNotebookGuid.isEmpty()) {
                key += QStringLiteral("/") + linkedNotebookGuid;
            }

            appSettings.setValue(key, stackItemNames);
        }
    }

    appSettings.setValue(
        gLastExpandedLinkedNotebookItemsKey, expandedLinkedNotebookItemsGuids);

    saveAllItemsRootItemExpandedState(
        appSettings, gAllNotebooksRootItemExpandedKey,
        notebookModel->allItemsRootItemIndex());

    appSettings.endGroup();
}

void NotebookItemView::restoreItemsState(const AbstractItemModel & model)
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::restoreItemsState");

    const auto * notebookModel = qobject_cast<const NotebookModel *>(&model);
    if (Q_UNLIKELY(!notebookModel)) {
        QNWARNING("view::NotebookItemView", "Non-notebook model is used");
        return;
    }

    const auto & linkedNotebookOwnerNamesByGuid =
        notebookModel->linkedNotebookOwnerNamesByGuid();

    ApplicationSettings appSettings{
        model.account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(gNotebookItemViewGroupKey);

    auto expandedStacks =
        appSettings.value(gLastExpandedStackItemsKey).toStringList();

    QHash<QString, QStringList> expandedStacksByLinkedNotebookGuid;
    for (const auto it:
         qevercloud::toRange(std::as_const(linkedNotebookOwnerNamesByGuid)))
    {
        const QString & linkedNotebookGuid = it.key();

        auto expandedStacksForLinkedNotebook =
            appSettings
                .value(
                    QString::fromUtf8(
                        gLastExpandedStackItemsKey.data(),
                        gLastExpandedStackItemsKey.size()) +
                    QStringLiteral("/") + linkedNotebookGuid)
                .toStringList();

        if (expandedStacksForLinkedNotebook.isEmpty()) {
            continue;
        }

        expandedStacksByLinkedNotebookGuid[linkedNotebookGuid] =
            expandedStacksForLinkedNotebook;
    }

    auto expandedLinkedNotebookItemsGuids =
        appSettings.value(gLastExpandedLinkedNotebookItemsKey).toStringList();

    auto allNotebooksRootItemExpandedPreference =
        appSettings.value(gAllNotebooksRootItemExpandedKey);

    appSettings.endGroup();

    const bool wasTrackingNotebookItemsState = trackItemsStateEnabled();
    setTrackItemsStateEnabled(false);

    setStacksExpanded(expandedStacks, *notebookModel, QString());

    for (const auto it:
         qevercloud::toRange(std::as_const(expandedStacksByLinkedNotebookGuid)))
    {
        setStacksExpanded(it.value(), *notebookModel, it.key());
    }

    setLinkedNotebooksExpanded(
        expandedLinkedNotebookItemsGuids, *notebookModel);

    bool allNotebooksRootItemExpanded = true;
    if (allNotebooksRootItemExpandedPreference.isValid()) {
        allNotebooksRootItemExpanded =
            allNotebooksRootItemExpandedPreference.toBool();
    }

    const auto allNotebooksRootItemIndex =
        notebookModel->allItemsRootItemIndex();
    setExpanded(allNotebooksRootItemIndex, allNotebooksRootItemExpanded);

    setTrackItemsStateEnabled(wasTrackingNotebookItemsState);
}

QString NotebookItemView::selectedItemsGroupKey() const
{
    return QString::fromUtf8(
        gNotebookItemViewGroupKey.data(), gNotebookItemViewGroupKey.size());
}

QString NotebookItemView::selectedItemsArrayKey() const
{
    return QStringLiteral("LastSelectedNotebookLocalUids");
}

QString NotebookItemView::selectedItemsKey() const
{
    return QStringLiteral("LastSelectedNotebookLocalUid");
}

bool NotebookItemView::shouldFilterBySelectedItems(
    const Account & account) const
{
    ApplicationSettings appSettings{
        account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);

    const auto filterBySelectedNotebook = appSettings.value(
        preferences::keys::sidePanelsFilterBySelectedNotebook);

    appSettings.endGroup();

    if (!filterBySelectedNotebook.isValid()) {
        return true;
    }

    return filterBySelectedNotebook.toBool();
}

QStringList NotebookItemView::localIdsInNoteFiltersManager(
    const NoteFiltersManager & noteFiltersManager) const
{
    return noteFiltersManager.notebookLocalIdsInFilter();
}

void NotebookItemView::setItemLocalIdsToNoteFiltersManager(
    const QStringList & itemLocalIds, NoteFiltersManager & noteFiltersManager)
{
    noteFiltersManager.setNotebooksToFilter(itemLocalIds);
}

void NotebookItemView::removeItemLocalIdsFromNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    noteFiltersManager.removeNotebooksFromFilter();
}

void NotebookItemView::connectToModel(AbstractItemModel & model)
{
    auto * notebookModel = qobject_cast<NotebookModel *>(&model);
    if (Q_UNLIKELY(!notebookModel)) {
        QNWARNING("view::NotebookItemView", "Non-notebook model is used");
        return;
    }

    QObject::connect(
        notebookModel, &NotebookModel::notifyNotebookStackRenamed, this,
        &NotebookItemView::onNotebookStackRenamed);

    QObject::connect(
        notebookModel, &NotebookModel::notifyNotebookStackChanged, this,
        &NotebookItemView::onNotebookStackChanged);

    QObject::connect(
        notebookModel, &NotebookModel::aboutToAddNotebook, this,
        &NotebookItemView::onAboutToAddNotebook);

    QObject::connect(
        notebookModel, &NotebookModel::addedNotebook, this,
        &NotebookItemView::onAddedNotebook);

    QObject::connect(
        notebookModel, &NotebookModel::aboutToUpdateNotebook, this,
        &NotebookItemView::onAboutToUpdateNotebook);

    QObject::connect(
        notebookModel, &NotebookModel::updatedNotebook, this,
        &NotebookItemView::onUpdatedNotebook);

    QObject::connect(
        notebookModel, &NotebookModel::aboutToRemoveNotebooks, this,
        &NotebookItemView::onAboutToRemoveNotebooks);

    QObject::connect(
        notebookModel, &NotebookModel::removedNotebooks, this,
        &NotebookItemView::onRemovedNotebooks);
}

void NotebookItemView::deleteItem(
    const QModelIndex & itemIndex, AbstractItemModel & model)
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::deleteItem");

    auto * notebookModel = qobject_cast<NotebookModel *>(&model);
    if (Q_UNLIKELY(!notebookModel)) {
        QNWARNING("view::NotebookItemView", "Non-notebook model is used");
        return;
    }

    const auto * modelItem = notebookModel->itemForIndex(itemIndex);
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the notebook model "
                       "item meant to be deleted"))
        return;
    }

    const auto * notebookItem = modelItem->cast<NotebookItem>();
    if (notebookItem) {
        int confirm = warningMessageBox(
            this, tr("Confirm the notebook deletion"),
            tr("Are you sure you want to delete the notebook?"),
            tr("Note that this action is not "
               "reversible and the deletion "
               "of the notebook would also mean "
               "the deletion of all the notes "
               "stored inside it!"),
            QMessageBox::Ok | QMessageBox::No);

        if (confirm != QMessageBox::Ok) {
            QNDEBUG(
                "view::NotebookItemView",
                "Notebook deletion was not confirmed");
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG("view::NotebookItemView", "Successfully deleted notebook");
            return;
        }

        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("The notebook model refused to delete the notebook; Check "
               "the status bar for message from the notebook model "
               "explaining why the notebook could not be deleted")))

        return;
    }

    const auto * stackItem = modelItem->cast<StackItem>();
    if (stackItem) {
        int confirm = warningMessageBox(
            this, tr("Confirm the notebook stack deletion"),
            tr("Are you sure you want to delete the whole stack of notebooks?"),
            tr("Note that this action is not reversible and the deletion "
               "of the whole stack of notebooks would also mean the deletion "
               "of all the notes stored inside the notebooks!"),
            QMessageBox::Ok | QMessageBox::No);

        if (confirm != QMessageBox::Ok) {
            QNDEBUG(
                "view::NotebookItemView",
                "Notebook stack deletion was not "
                    << "confirmed");
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG(
                "view::NotebookItemView",
                "Successfully deleted notebook stack");
            return;
        }

        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("The notebook model refused to delete the notebook stack; "
               "Check the status bar for message from the notebook model "
               "explaining why the notebook stack could not be deleted")));
    }
    else {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete notebook(s): "
                       "found notebook model item of unknown type"))

        QNWARNING("view::NotebookItemView", *modelItem);
    }
}

void NotebookItemView::onAboutToAddNotebook()
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::onAboutToAddNotebook");
    prepareForModelChange();
}

void NotebookItemView::onAddedNotebook(const QModelIndex & index)
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::onAddedNotebook");

    Q_UNUSED(index)
    postProcessModelChange();
}

void NotebookItemView::onAboutToUpdateNotebook(const QModelIndex & index)
{
    QNDEBUG(
        "view::NotebookItemView", "NotebookItemView::onAboutToUpdateNotebook");

    Q_UNUSED(index)
    prepareForModelChange();
}

void NotebookItemView::onUpdatedNotebook(const QModelIndex & index)
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::onUpdatedNotebook");

    Q_UNUSED(index)
    postProcessModelChange();
}

void NotebookItemView::onAboutToRemoveNotebooks()
{
    QNDEBUG(
        "view::NotebookItemView", "NotebookItemView::onAboutToRemoveNotebooks");
    prepareForModelChange();
}

void NotebookItemView::onRemovedNotebooks()
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::onRemovedNotebooks");
    postProcessModelChange();
}

void NotebookItemView::onCreateNewNotebookAction()
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::onCreateNewNotebookAction");

    Q_EMIT newNotebookCreationRequested();
}

void NotebookItemView::onRenameNotebookAction()
{
    QNDEBUG(
        "view::NotebookItemView", "NotebookItemView::onRenameNotebookAction");

    ErrorString errorPrefix{QT_TR_NOOP("Can't rename notebook")};
    FETCH_ITEM_DATA(errorPrefix)
    edit(itemData.m_index);
}

void NotebookItemView::onDeleteNotebookAction()
{
    QNDEBUG(
        "view::NotebookItemView", "NotebookItemView::onDeleteNotebookAction");

    ErrorString errorPrefix{QT_TR_NOOP("Can't delete notebook")};
    FETCH_ITEM_DATA(errorPrefix)
    deleteItem(itemData.m_index, *itemData.m_model);
}

void NotebookItemView::onSetNotebookDefaultAction()
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::onSetNotebookDefaultAction");

    ErrorString errorPrefix{QT_TR_NOOP("Can't set notebook as default")};
    FETCH_ITEM_DATA(errorPrefix)

    auto itemIndex = itemData.m_model->index(
        itemData.m_index.row(),
        static_cast<int>(NotebookModel::Column::Default),
        itemData.m_index.parent());

    if (itemData.m_model->setData(itemIndex, true)) {
        QNDEBUG(
            "view::NotebookItemView",
            "Successfully set the notebook as default");
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
    QNDEBUG(
        "view::NotebookItemView", "NotebookItemView::onShowNotebookInfoAction");
    Q_EMIT notebookInfoRequested();
}

void NotebookItemView::onEditNotebookAction()
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::onEditNotebookAction");

    ErrorString errorPrefix{QT_TR_NOOP("Can't edit notebook")};
    FETCH_ITEM_DATA(errorPrefix)

    auto editNotebookDialog = std::make_unique<AddOrEditNotebookDialog>(
        itemData.m_model, this, itemData.m_localId);

    editNotebookDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(editNotebookDialog->exec())
}

void NotebookItemView::onMoveNotebookToStackAction()
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::onMoveNotebookToStackAction");

    ErrorString errorPrefix{QT_TR_NOOP("Can't move notebook to stack")};
    FETCH_STACK_DATA(errorPrefix)

    const auto itemIndex = stackData.m_model->indexForLocalId(stackData.m_id);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move notebook to stack, can't "
                       "get valid model index for notebook's local id"))
        return;
    }

    const bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    const auto index =
        stackData.m_model->moveToStack(itemIndex, stackData.m_stack);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (!index.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't move notebook to stack"))
        return;
    }

    QNDEBUG(
        "view::NotebookItemView",
        "Successfully moved the notebook item to the target stack");
}

void NotebookItemView::onRemoveNotebookFromStackAction()
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::onRemoveNotebookFromStackAction");

    ErrorString errorPrefix{QT_TR_NOOP("Can't remove notebook from stack")};
    FETCH_ITEM_DATA(errorPrefix)

    const bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    const auto index = itemData.m_model->removeFromStack(itemData.m_index);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (!index.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't remove the notebook from stack"))
        return;
    }

    QNDEBUG(
        "view::NotebookItemView",
        "Successfully removed the notebook item from its stack");
}

void NotebookItemView::onRenameNotebookStackAction()
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::onRenameNotebookStackAction");

    ErrorString errorPrefix{QT_TR_NOOP("Can't rename notebook stack")};
    FETCH_STACK_DATA(errorPrefix)

    const auto notebookStackItemIndex =
        stackData.m_model->indexForNotebookStack(
            stackData.m_stack, stackData.m_id);

    if (!notebookStackItemIndex.isValid()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename the notebook stack, the "
                       "model returned invalid index for the notebook stack"))
        return;
    }

    saveItemsState();
    edit(notebookStackItemIndex);
}

void NotebookItemView::onRemoveNotebooksFromStackAction()
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::onRemoveNotebooksFromStackAction");

    ErrorString errorPrefix{QT_TR_NOOP("Can't remove notebooks from stack")};
    FETCH_STACK_DATA(errorPrefix)

    while (true) {
        const auto notebookStackItemIndex =
            stackData.m_model->indexForNotebookStack(
                stackData.m_stack, stackData.m_id);

        if (!notebookStackItemIndex.isValid()) {
            QNDEBUG(
                "view::NotebookItemView",
                "Notebook stack item index is invalid, breaking the loop");
            break;
        }

        auto childIndex = notebookStackItemIndex.model()->index(
            0, static_cast<int>(NotebookModel::Column::Name));

        if (!childIndex.isValid()) {
            QNDEBUG(
                "view::NotebookItemView",
                "Detected invalid child item index for the notebook stack "
                "item, breaking the loop");
            break;
        }

        const bool wasTrackingSelection = trackSelectionEnabled();
        setTrackSelectionEnabled(false);

        Q_UNUSED(stackData.m_model->removeFromStack(childIndex))

        setTrackSelectionEnabled(wasTrackingSelection);
    }

    QNDEBUG(
        "view::NotebookItemView",
        "Successfully removed notebook items from the stack");
}

void NotebookItemView::onFavoriteAction()
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::onFavoriteAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't favorite notebook, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*action, true);
}

void NotebookItemView::onUnfavoriteAction()
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::onUnfavoriteAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite notebook, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*action, false);
}

void NotebookItemView::onNotebookStackRenamed(
    const QString & previousStackName, const QString & newStackName,
    const QString & linkedNotebookGuid)
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::onNotebookStackRenamed: previous stack name = "
            << previousStackName << ", new stack name = " << newStackName
            << ", linked notebook guid = " << linkedNotebookGuid);

    auto * notebookModel = qobject_cast<NotebookModel *>(model());
    if (Q_UNLIKELY(!notebookModel)) {
        QNWARNING(
            "view::NotebookItemView",
            "Received notebook stack item rename event but can't cast the "
            "model to the notebook one");
        return;
    }

    ApplicationSettings appSettings{
        notebookModel->account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(gNotebookItemViewGroupKey);
    QString key = QString::fromUtf8(
        gLastExpandedStackItemsKey.data(), gLastExpandedStackItemsKey.size());
    if (!linkedNotebookGuid.isEmpty()) {
        key += QStringLiteral("/") + linkedNotebookGuid;
    }

    QStringList expandedStacks = appSettings.value(key).toStringList();
    appSettings.endGroup();

    const auto previousStackIndex = expandedStacks.indexOf(previousStackName);
    if (previousStackIndex < 0) {
        QNDEBUG(
            "view::NotebookItemView",
            "The renamed stack item hasn't been expanded");
    }
    else {
        expandedStacks.replace(previousStackIndex, newStackName);
    }

    setStacksExpanded(expandedStacks, *notebookModel, linkedNotebookGuid);

    const auto newStackItemIndex =
        notebookModel->indexForNotebookStack(newStackName, linkedNotebookGuid);

    if (Q_UNLIKELY(!newStackItemIndex.isValid())) {
        QNWARNING(
            "view::NotebookItemView",
            "Can't select the just renamed notebook stack: notebook model "
            "returned invalid index for the new notebook stack name");
        return;
    }

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        QNWARNING(
            "view::NotebookItemView",
            "Can't select the just renamed notebook stack: no selection model "
            "in the view");
        return;
    }

    selectionModel->select(
        newStackItemIndex,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows |
            QItemSelectionModel::Current);
}

void NotebookItemView::onNotebookStackChanged(const QModelIndex & index)
{
    QNDEBUG(
        "view::NotebookItemView", "NotebookItemView::onNotebookStackChanged");

    Q_UNUSED(index)

    auto * notebookModel = qobject_cast<NotebookModel *>(model());
    if (Q_UNLIKELY(!notebookModel)) {
        QNDEBUG("view::NotebookItemView", "Non-notebook model is used");
        return;
    }

    restoreItemsState(*notebookModel);
    restoreSelectedItems(*notebookModel);
}

void NotebookItemView::contextMenuEvent(QContextMenuEvent * event)
{
    QNDEBUG("view::NotebookItemView", "NotebookItemView::contextMenuEvent");

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "view::NotebookItemView",
            "Detected Qt error: notebook item view received context menu event "
            "with null pointer to the context menu event");
        return;
    }

    auto * notebookModel = qobject_cast<NotebookModel *>(model());
    if (Q_UNLIKELY(!notebookModel)) {
        QNDEBUG(
            "view::NotebookItemView",
            "Non-notebook model is used, not doing anything");
        return;
    }

    auto clickedItemIndex = indexAt(event->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view::NotebookItemView",
            "Clicked item index is not valid, not doing anything");
        return;
    }

    const auto * modelItem = notebookModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the notebook model "
                       "item: no item corresponding to the clicked item's "
                       "index"))
        return;
    }

    const auto * notebookItem = modelItem->cast<NotebookItem>();
    if (notebookItem) {
        showNotebookItemContextMenu(
            *notebookItem, event->globalPos(), *notebookModel);
        return;
    }

    const auto * stackItem = modelItem->cast<StackItem>();
    if (stackItem) {
        showNotebookStackItemContextMenu(
            *stackItem, *modelItem, event->globalPos(), *notebookModel);
    }
}

void NotebookItemView::showNotebookItemContextMenu(
    const NotebookItem & item, const QPoint & point, NotebookModel & model)
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::showNotebookItemContextMenu: ("
            << point.x() << ", " << point.y() << "), item: " << item);

    delete m_notebookItemContextMenu;
    m_notebookItemContextMenu = new QMenu(this);

    addContextMenuAction(
        tr("Create new notebook") + QStringLiteral("..."),
        *m_notebookItemContextMenu, this,
        [this] { onCreateNewNotebookAction(); }, item.localId(),
        ActionState::Enabled);

    m_notebookItemContextMenu->addSeparator();
    const bool canRename = (item.isUpdatable() && item.nameIsUpdatable());

    addContextMenuAction(
        tr("Rename"), *m_notebookItemContextMenu, this,
        [this] { onRenameNotebookAction(); }, item.localId(),
        canRename ? ActionState::Enabled : ActionState::Disabled);

    const bool canDeleteNotebook =
        !m_noteModel.isNull() && item.guid().isEmpty();

    if (canDeleteNotebook) {
        addContextMenuAction(
            tr("Delete"), *m_notebookItemContextMenu, this,
            [this] { onDeleteNotebookAction(); }, item.localId(),
            ActionState::Enabled);
    }

    addContextMenuAction(
        tr("Edit") + QStringLiteral("..."), *m_notebookItemContextMenu, this,
        [this] { onEditNotebookAction(); }, item.localId(),
        canRename ? ActionState::Enabled : ActionState::Disabled);

    const QString & stack = item.stack();
    QStringList stacks = model.stacks();
    if (!stack.isEmpty()) {
        Q_UNUSED(stacks.removeAll(stack))
    }

    if (!stacks.isEmpty() || !stack.isEmpty()) {
        m_notebookItemContextMenu->addSeparator();
    }

    if (!stacks.isEmpty()) {
        auto * targetStackSubMenu =
            m_notebookItemContextMenu->addMenu(tr("Move to stack"));
        Q_ASSERT(targetStackSubMenu);

        for (const auto & stack: std::as_const(stacks)) {
            QStringList dataPair;
            dataPair.reserve(2);
            dataPair << stack;
            dataPair << item.localId();

            addContextMenuAction(
                stack, *targetStackSubMenu, this,
                [this] { onMoveNotebookToStackAction(); }, dataPair,
                item.isUpdatable() ? ActionState::Enabled
                                   : ActionState::Disabled);
        }
    }

    if (!stack.isEmpty()) {
        addContextMenuAction(
            tr("Remove from stack"), *m_notebookItemContextMenu, this,
            [this] { onRemoveNotebookFromStackAction(); }, item.localId(),
            item.isUpdatable() ? ActionState::Enabled : ActionState::Disabled);
    }

    m_notebookItemContextMenu->addSeparator();

    if (!item.isDefault()) {
        addContextMenuAction(
            tr("Set default"), *m_notebookItemContextMenu, this,
            [this] { onSetNotebookDefaultAction(); }, item.localId(),
            item.isUpdatable() ? ActionState::Enabled : ActionState::Disabled);
    }

    if (item.isFavorited()) {
        addContextMenuAction(
            tr("Unfavorite"), *m_notebookItemContextMenu, this,
            [this] { onUnfavoriteAction(); }, item.localId(),
            item.isUpdatable() ? ActionState::Enabled : ActionState::Disabled);
    }
    else {
        addContextMenuAction(
            tr("Favorite"), *m_notebookItemContextMenu, this,
            [this] { onFavoriteAction(); }, item.localId(),
            item.isUpdatable() ? ActionState::Enabled : ActionState::Disabled);
    }

    addContextMenuAction(
        tr("Info") + QStringLiteral("..."), *m_notebookItemContextMenu, this,
        [this] { onShowNotebookInfoAction(); }, item.localId(),
        ActionState::Enabled);

    m_notebookItemContextMenu->exec(point);
}

void NotebookItemView::showNotebookStackItemContextMenu(
    const StackItem & item, const INotebookModelItem & modelItem,
    const QPoint & point, NotebookModel & model)
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::showNotebookStackItemContextMenu: ("
            << point.x() << ", " << point.y() << ", item: " << item);

    delete m_notebookStackItemContextMenu;
    m_notebookStackItemContextMenu = new QMenu(this);

    addContextMenuAction(
        tr("Create new notebook") + QStringLiteral("..."),
        *m_notebookStackItemContextMenu, this,
        [this] { onCreateNewNotebookAction(); }, item.name(),
        ActionState::Enabled);

    m_notebookStackItemContextMenu->addSeparator();

    bool allChildrenUpdatable = false;

    QString linkedNotebookGuid;
    const auto * parentItem = modelItem.parent();
    if (parentItem &&
        (parentItem->type() == INotebookModelItem::Type::LinkedNotebook))
    {
        const auto * linkedNotebookItem =
            parentItem->cast<LinkedNotebookRootItem>();
        if (linkedNotebookItem) {
            linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();
        }
    }

    auto stackItemIndex =
        model.indexForNotebookStack(item.name(), linkedNotebookGuid);

    const auto * stackModelItem = model.itemForIndex(stackItemIndex);
    if (Q_LIKELY(stackModelItem)) {
        allChildrenUpdatable = true;

        const auto children = stackModelItem->children();
        for (const auto * childItem: std::as_const(children)) {
            if (Q_UNLIKELY(!childItem)) {
                QNWARNING(
                    "view::NotebookItemView",
                    "Found null child at stack item: " << *stackModelItem);
                continue;
            }

            const auto * notebookItem = childItem->cast<NotebookItem>();
            if (Q_UNLIKELY(!notebookItem)) {
                QNWARNING(
                    "view::NotebookItemView",
                    "Found child of non-notebook type at stack item: "
                        << *stackModelItem << "\nChild item: " << *childItem);
                continue;
            }

            if (!notebookItem->isUpdatable()) {
                allChildrenUpdatable = false;
                break;
            }
        }
    }

    addContextMenuAction(
        tr("Rename"), *m_notebookStackItemContextMenu, this,
        [this] { onRenameNotebookStackAction(); },
        QStringList{} << item.name() << linkedNotebookGuid,
        allChildrenUpdatable ? ActionState::Enabled : ActionState::Disabled);

    addContextMenuAction(
        tr("Remove stack"), *m_notebookStackItemContextMenu, this,
        [this] { onRemoveNotebooksFromStackAction(); },
        QStringList{} << item.name() << linkedNotebookGuid,
        allChildrenUpdatable ? ActionState::Enabled : ActionState::Disabled);

    addContextMenuAction(
        tr("Info") + QStringLiteral("..."), *m_notebookStackItemContextMenu,
        this, [this] { onShowNotebookInfoAction(); }, item.name(),
        ActionState::Enabled);

    m_notebookStackItemContextMenu->exec(point);
}

void NotebookItemView::setStacksExpanded(
    const QStringList & expandedStackNames, const NotebookModel & model,
    const QString & linkedNotebookGuid)
{
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::setStacksExpanded: linked notebook guid = "
            << linkedNotebookGuid);

    for (const auto & expandedStack: std::as_const(expandedStackNames)) {
        const auto index =
            model.indexForNotebookStack(expandedStack, linkedNotebookGuid);

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
    QNDEBUG(
        "view::NotebookItemView",
        "NotebookItemView::setLinkedNotebooksExpanded: "
            << expandedLinkedNotebookGuids.join(QStringLiteral(", ")));

    for (const auto & expandedLinkedNotebookGuid:
         std::as_const(expandedLinkedNotebookGuids))
    {
        const auto index =
            model.indexForLinkedNotebookGuid(expandedLinkedNotebookGuid);

        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void NotebookItemView::setFavoritedFlag(
    const QAction & action, const bool favorited)
{
    auto * notebookModel = qobject_cast<NotebookModel *>(model());
    if (Q_UNLIKELY(!notebookModel)) {
        QNDEBUG("view::NotebookItemView", "Non-notebook model is used");
        return;
    }

    const QString itemLocalId = action.data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag "
                       "for the notebook, can't get notebook's local "
                       "id from QAction"))
        return;
    }

    const auto itemIndex = notebookModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag "
                       "for the notebook, the model returned invalid "
                       "index for the notebook's local id"))
        return;
    }

    if (favorited) {
        notebookModel->favoriteNotebook(itemIndex);
    }
    else {
        notebookModel->unfavoriteNotebook(itemIndex);
    }
}

bool NotebookItemView::fetchCurrentNotebookCommonData(
    NotebookCommonData & data, ErrorString & errorDescription) const
{
    data.m_action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!data.m_action)) {
        errorDescription.appendBase(
            QT_TR_NOOP("can't cast the slot invoker to QAction"));
        return false;
    }

    data.m_model = qobject_cast<NotebookModel *>(model());
    if (Q_UNLIKELY(!data.m_model)) {
        QNDEBUG("view::NotebookItemView", "Non-notebook model is used");
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

    itemData.m_localId = itemData.m_action->data().toString();
    if (Q_UNLIKELY(itemData.m_localId.isEmpty())) {
        errorDescription.appendBase(
            QT_TR_NOOP("can't get notebook local id from QAction"));
        return false;
    }

    itemData.m_index = itemData.m_model->indexForLocalId(itemData.m_localId);
    if (Q_UNLIKELY(!itemData.m_index.isValid())) {
        errorDescription.appendBase(
            QT_TR_NOOP("the model returned invalid index for "
                       "the notebook's local id"));
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

    auto stackAndId = stackData.m_action->data().toStringList();
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
