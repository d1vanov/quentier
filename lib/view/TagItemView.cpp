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

#include "TagItemView.h"

#include "ItemSelectionModel.h"
#include "Utils.h"

#include <lib/dialog/AddOrEditTagDialog.h>
#include <lib/model/tag/TagModel.h>
#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/SidePanelsFiltering.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view::TagItemView", errorDescription);                      \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gTagItemViewGroupKey = "TagItemView"sv;
constexpr auto gAllTagsRootItemExpandedKey = "AllTagsRootItemExpanded"sv;
constexpr auto gLastExpandedTagItemsKey = "LastExpandedTagLocalIds"sv;

constexpr auto gLastExpandedLinkedNotebookItemsKey =
    "LastExpandedLinkedNotebookItemsGuids"sv;

} // namespace

TagItemView::TagItemView(QWidget * parent) :
    AbstractNoteFilteringTreeView{QStringLiteral("tag"), parent}
{}

void TagItemView::saveItemsState()
{
    QNDEBUG("view::TagItemView", "TagItemView::saveItemsState");

    const auto * tagModel = qobject_cast<const TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    QStringList expandedTagItemsLocalIds;
    QStringList expandedLinkedNotebookItemsGuids;

    const auto indexes = tagModel->persistentIndexes();
    for (const auto & index: std::as_const(indexes)) {
        if (!isExpanded(index)) {
            continue;
        }

        const auto * modelItem = tagModel->itemForIndex(index);
        if (Q_UNLIKELY(!modelItem)) {
            QNWARNING(
                "view::TagItemView",
                "Tag model returned null pointer to tag "
                    << "model item for valid model index");
            continue;
        }

        const auto * tagItem = modelItem->cast<TagItem>();
        if (tagItem) {
            expandedTagItemsLocalIds << tagItem->localId();
            QNTRACE(
                "view::TagItemView",
                "Found expanded tag item: local id = " << tagItem->localId());
        }

        const auto * linkedNotebookItem =
            modelItem->cast<TagLinkedNotebookRootItem>();

        if (linkedNotebookItem) {
            expandedLinkedNotebookItemsGuids
                << linkedNotebookItem->linkedNotebookGuid();

            QNTRACE(
                "view::TagItemView",
                "Found expanded tag linked notebook root "
                    << "item: linked notebook guid = "
                    << linkedNotebookItem->linkedNotebookGuid());
        }
    }

    ApplicationSettings appSettings{
        tagModel->account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(gTagItemViewGroupKey);

    appSettings.setValue(gLastExpandedTagItemsKey, expandedTagItemsLocalIds);

    appSettings.setValue(
        gLastExpandedLinkedNotebookItemsKey, expandedLinkedNotebookItemsGuids);

    saveAllItemsRootItemExpandedState(
        appSettings, gAllTagsRootItemExpandedKey,
        tagModel->allItemsRootItemIndex());

    appSettings.endGroup();
}

void TagItemView::restoreItemsState(const AbstractItemModel & model)
{
    QNDEBUG("view::TagItemView", "TagItemView::restoreItemsState");

    const auto * tagModel = qobject_cast<const TagModel *>(&model);
    if (Q_UNLIKELY(!tagModel)) {
        QNWARNING("view::TagItemView", "Non-tag model is used");
        return;
    }

    ApplicationSettings appSettings{
        model.account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(gTagItemViewGroupKey);

    const QStringList expandedTagItemsLocalIds =
        appSettings.value(gLastExpandedTagItemsKey).toStringList();

    const QStringList expandedLinkedNotebookItemsGuids =
        appSettings.value(gLastExpandedLinkedNotebookItemsKey).toStringList();

    const auto allTagsRootItemExpandedPreference =
        appSettings.value(gAllTagsRootItemExpandedKey);

    appSettings.endGroup();

    const bool wasTrackingTagItemsState = trackItemsStateEnabled();
    setTrackItemsStateEnabled(false);

    setTagsExpanded(expandedTagItemsLocalIds, *tagModel);
    setLinkedNotebooksExpanded(expandedLinkedNotebookItemsGuids, *tagModel);

    bool allTagsRootItemExpanded = true;
    if (allTagsRootItemExpandedPreference.isValid()) {
        allTagsRootItemExpanded = allTagsRootItemExpandedPreference.toBool();
    }

    const auto allTagsRootItemIndex = tagModel->allItemsRootItemIndex();
    setExpanded(allTagsRootItemIndex, allTagsRootItemExpanded);

    setTrackItemsStateEnabled(wasTrackingTagItemsState);
}

QString TagItemView::selectedItemsGroupKey() const
{
    return QString::fromUtf8(
        gTagItemViewGroupKey.data(), gTagItemViewGroupKey.size());
}

QString TagItemView::selectedItemsArrayKey() const
{
    return QStringLiteral("LastSelectedTagLocalIds");
}

QString TagItemView::selectedItemsKey() const
{
    return QStringLiteral("LastSelectedTagLocalId");
}

bool TagItemView::shouldFilterBySelectedItems(const Account & account) const
{
    ApplicationSettings appSettings{
        account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);

    const auto filterBySelectedTag =
        appSettings.value(preferences::keys::sidePanelsFilterBySelectedTag);

    appSettings.endGroup();

    if (!filterBySelectedTag.isValid()) {
        return true;
    }

    return filterBySelectedTag.toBool();
}

QStringList TagItemView::localIdsInNoteFiltersManager(
    const NoteFiltersManager & noteFiltersManager) const
{
    return noteFiltersManager.tagLocalIdsInFilter();
}

void TagItemView::setItemLocalIdsToNoteFiltersManager(
    const QStringList & itemLocalIds, NoteFiltersManager & noteFiltersManager)
{
    noteFiltersManager.setTagsToFilter(itemLocalIds);
}

void TagItemView::removeItemLocalIdsFromNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    noteFiltersManager.removeTagsFromFilter();
}

void TagItemView::connectToModel(AbstractItemModel & model)
{
    auto * tagModel = qobject_cast<TagModel *>(&model);
    if (Q_UNLIKELY(!tagModel)) {
        QNWARNING("view::TagItemView", "Non-tag model is used");
        return;
    }

    QObject::connect(
        tagModel, &TagModel::notifyTagParentChanged, this,
        &TagItemView::onTagParentChanged);

    QObject::connect(
        tagModel, &TagModel::aboutToAddTag, this,
        &TagItemView::onAboutToAddTag);

    QObject::connect(
        tagModel, &TagModel::addedTag, this, &TagItemView::onAddedTag);

    QObject::connect(
        tagModel, &TagModel::aboutToUpdateTag, this,
        &TagItemView::onAboutToUpdateTag);

    QObject::connect(
        tagModel, &TagModel::updatedTag, this, &TagItemView::onUpdatedTag);

    QObject::connect(
        tagModel, &TagModel::aboutToRemoveTags, this,
        &TagItemView::onAboutToRemoveTags);

    QObject::connect(
        tagModel, &TagModel::removedTags, this, &TagItemView::onRemovedTags);
}

void TagItemView::deleteItem(
    const QModelIndex & itemIndex, AbstractItemModel & model)
{
    QNDEBUG("view::TagItemView", "TagItemView::deleteItem");

    QString localId = model.localIdForItemIndex(itemIndex);
    if (Q_UNLIKELY(localId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the tag item "
                       "meant to be deleted"))
        return;
    }

    const int confirm = warningMessageBox(
        this, tr("Confirm the tag deletion"),
        tr("Are you sure you want to delete this tag?"),
        tr("Note that this action is not reversible and the deletion of "
           "the tag would mean its disappearance from all the notes using it!"),
        QMessageBox::Ok | QMessageBox::No);

    if (confirm != QMessageBox::Ok) {
        QNDEBUG("view::TagItemView", "Tag deletion was not confirmed");
        return;
    }

    if (model.removeRow(itemIndex.row(), itemIndex.parent())) {
        QNDEBUG("view::TagItemView", "Successfully deleted tag");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The tag model refused to delete the tag; Check the status bar "
           "for message from the tag model explaining why the tag could not "
           "be deleted")))
}

void TagItemView::onAboutToAddTag()
{
    QNDEBUG("view::TagItemView", "TagItemView::onAboutToAddTag");
    prepareForModelChange();
}

void TagItemView::onAddedTag(const QModelIndex & index)
{
    QNDEBUG("view::TagItemView", "TagItemView::onAddedTag");

    Q_UNUSED(index)
    postProcessModelChange();
}

void TagItemView::onAboutToUpdateTag(const QModelIndex & index)
{
    QNDEBUG("view::TagItemView", "TagItemView::onAboutToUpdateTag");

    Q_UNUSED(index)
    prepareForModelChange();
}

void TagItemView::onUpdatedTag(const QModelIndex & index)
{
    QNDEBUG("view::TagItemView", "TagItemView::onUpdatedTag");

    Q_UNUSED(index)
    postProcessModelChange();
}

void TagItemView::onAboutToRemoveTags()
{
    QNDEBUG("view::TagItemView", "TagItemView::onAboutToRemoveTags");
    prepareForModelChange();
}

void TagItemView::onRemovedTags()
{
    QNDEBUG("view::TagItemView", "TagItemView::onRemovedTags");
    postProcessModelChange();
}

void TagItemView::onCreateNewTagAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onCreateNewTagAction");
    Q_EMIT newTagCreationRequested();
}

void TagItemView::onRenameTagAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onRenameTagAction");

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    const auto itemIndex = tagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename tag: the model "
                       "returned invalid index for the tag's local id"));
        return;
    }

    edit(itemIndex);
}

void TagItemView::onDeleteTagAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onDeleteTagAction");

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    const auto itemIndex = tagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag: the model "
                       "returned invalid index for the tag's local id"))
        return;
    }

    deleteItem(itemIndex, *tagModel);
}

void TagItemView::onEditTagAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onEditTagAction");

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't edit tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    auto pEditTagDialog =
        std::make_unique<AddOrEditTagDialog>(tagModel, this, itemLocalId);

    pEditTagDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pEditTagDialog->exec())
}

void TagItemView::onPromoteTagAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onPromoteTagAction");

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    auto itemIndex = tagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag: the model "
                       "returned invalid index for the tag's local id"))
        return;
    }

    saveItemsState();

    bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    auto promotedItemIndex = tagModel->promote(itemIndex);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (promotedItemIndex.isValid()) {
        QNDEBUG("view::TagItemView", "Successfully promoted the tag");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The tag model refused to promote the tag; Check the status bar for "
           "message from the tag model explaining why the tag could not "
           "be promoted")))
}

void TagItemView::onDemoteTagAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onDemoteTagAction");

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    auto itemIndex = tagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag: the model "
                       "returned invalid index for the tag's local id"))
        return;
    }

    saveItemsState();

    bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    auto demotedItemIndex = tagModel->demote(itemIndex);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (demotedItemIndex.isValid()) {
        QNDEBUG("view::TagItemView", "Successfully demoted the tag");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The tag model refused to demote the tag; Check the status bar "
           "for message from the tag model explaining why the tag could not "
           "be demoted")))
}

void TagItemView::onRemoveFromParentTagAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onRemoveFromParentTagAction");

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent, "
                       "can't get tag's local id from QAction"))
        return;
    }

    auto itemIndex = tagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent: "
                       "the model returned invalid index for the tag's "
                       "local id"))
        return;
    }

    saveItemsState();

    bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    auto removedFromParentItemIndex = tagModel->removeFromParent(itemIndex);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (removedFromParentItemIndex.isValid()) {
        QNDEBUG(
            "view::TagItemView", "Successfully removed the tag from parent");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The tag model refused to remove the tag from parent; Check "
           "the status bar for message from the tag model explaining why "
           "the tag could not be removed from its parent")))
}

void TagItemView::onMoveTagToParentAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onMoveTagToParentAction");

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move tag to parent, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    auto itemLocalIdAndParentName = action->data().toStringList();
    if (itemLocalIdAndParentName.size() != 2) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move tag to parent, "
                       "can't retrieve the tag local id and parent "
                       "name from QAction data"))
        return;
    }

    const QString & localId = itemLocalIdAndParentName.at(0);

    auto itemIndex = tagModel->indexForLocalId(localId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move tag to parent, "
                       "can't get valid model index for tag's local id"))
        return;
    }

    saveItemsState();

    const QString & parentTagName = itemLocalIdAndParentName.at(1);

    bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    auto movedTagItemIndex = tagModel->moveToParent(itemIndex, parentTagName);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (!movedTagItemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't move tag to parent"))
        return;
    }

    QNDEBUG("view::TagItemView", "Successfully moved the tag item to parent");
}

void TagItemView::onShowTagInfoAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onShowTagInfoAction");
    Q_EMIT tagInfoRequested();
}

void TagItemView::onDeselectAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onDeselectAction");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't clear the tag selection: no selection "
                       "model in the view"))
        return;
    }

    pSelectionModel->clearSelection();
}

void TagItemView::onFavoriteAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onFavoriteAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't favorite tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*action, true);
}

void TagItemView::onUnfavoriteAction()
{
    QNDEBUG("view::TagItemView", "TagItemView::onUnfavoriteAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite tag, can't "
                       "cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*action, false);
}

void TagItemView::onTagParentChanged(const QModelIndex & tagIndex)
{
    QNDEBUG("view::TagItemView", "TagItemView::onTagParentChanged");

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    const auto * pItem = tagModel->itemForIndex(tagIndex);
    if (Q_LIKELY(pItem && pItem->parent())) {
        auto parentIndex = tagModel->indexForItem(pItem->parent());

        bool wasTrackingTagItemsState = trackItemsStateEnabled();
        setTrackItemsStateEnabled(false);

        setExpanded(parentIndex, true);

        setTrackItemsStateEnabled(wasTrackingTagItemsState);
    }

    restoreItemsState(*tagModel);
    restoreSelectedItems(*tagModel);
}

void TagItemView::contextMenuEvent(QContextMenuEvent * event)
{
    QNDEBUG("view::TagItemView", "TagItemView::contextMenuEvent");

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "view::TagItemView",
            "Detected Qt error: tag item view received "
                << "context menu event with null pointer to the context menu "
                << "event");
        return;
    }

    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG(
            "view::TagItemView", "Non-tag model is used, not doing anything");
        return;
    }

    auto clickedItemIndex = indexAt(event->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view::TagItemView",
            "Clicked item index is not valid, not doing "
                << "anything");
        return;
    }

    const auto * modelItem = tagModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the tag model item: no "
                       "item corresponding to the clicked item's index"))
        return;
    }

    if (Q_UNLIKELY(modelItem->type() != ITagModelItem::Type::Tag)) {
        QNDEBUG(
            "view::TagItemView",
            "Won't show the context menu for the tag model "
                << "item not of a tag type");
        return;
    }

    const auto * tagItem = modelItem->cast<TagItem>();
    if (Q_UNLIKELY(!tagItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the tag model "
                       "item as it contains no actual tag item even "
                       "though it is of a tag type"))

        QNWARNING("view::TagItemView", *modelItem);
        return;
    }

    delete m_tagItemContextMenu;
    m_tagItemContextMenu = new QMenu(this);

    addContextMenuAction(
        tr("Create new tag") + QStringLiteral("..."), *m_tagItemContextMenu,
        this, [this] { onCreateNewTagAction(); }, tagItem->localId(),
        ActionState::Enabled);

    m_tagItemContextMenu->addSeparator();

    const bool canUpdate =
        (tagModel->flags(clickedItemIndex) & Qt::ItemIsEditable);

    addContextMenuAction(
        tr("Rename"), *m_tagItemContextMenu, this,
        [this] { onRenameTagAction(); }, tagItem->localId(),
        canUpdate ? ActionState::Enabled : ActionState::Disabled);

    bool canDeleteTag = tagItem->guid().isEmpty();
    if (canDeleteTag) {
        canDeleteTag =
            !tagModel->tagHasSynchronizedChildTags(tagItem->localId());
    }

    if (canDeleteTag) {
        addContextMenuAction(
            tr("Delete"), *m_tagItemContextMenu, this,
            [this] { onDeleteTagAction(); }, tagItem->localId(),
            ActionState::Enabled);
    }

    addContextMenuAction(
        tr("Edit") + QStringLiteral("..."), *m_tagItemContextMenu, this,
        [this] { onEditTagAction(); }, tagItem->localId(),
        canUpdate ? ActionState::Enabled : ActionState::Disabled);

    const QModelIndex clickedItemParentIndex = clickedItemIndex.parent();
    if (clickedItemParentIndex.isValid()) {
        m_tagItemContextMenu->addSeparator();

        addContextMenuAction(
            tr("Promote"), *m_tagItemContextMenu, this,
            [this] { onPromoteTagAction(); }, tagItem->localId(),
            canUpdate ? ActionState::Enabled : ActionState::Disabled);

        addContextMenuAction(
            tr("Remove from parent"), *m_tagItemContextMenu, this,
            [this] { onRemoveFromParentTagAction(); }, tagItem->localId(),
            canUpdate ? ActionState::Enabled : ActionState::Disabled);
    }

    if (clickedItemIndex.row() != 0) {
        const QModelIndex siblingItemIndex = tagModel->index(
            clickedItemIndex.row() - 1, clickedItemIndex.column(),
            clickedItemIndex.parent());

        const bool canUpdateItemAndSibling = canUpdate &&
            (tagModel->flags(siblingItemIndex) & Qt::ItemIsEditable);

        addContextMenuAction(
            tr("Demote"), *m_tagItemContextMenu, this,
            [this] { onDemoteTagAction(); }, tagItem->localId(),
            canUpdateItemAndSibling ? ActionState::Enabled
                                    : ActionState::Disabled);
    }

    QStringList tagNames = tagModel->tagNames(tagItem->linkedNotebookGuid());

    // 1) Remove the current tag's name from the list of possible new parents
    const auto nameIt = std::lower_bound(
        tagNames.constBegin(), tagNames.constEnd(), tagItem->name());

    if ((nameIt != tagNames.constEnd()) && (*nameIt == tagItem->name())) {
        const int pos =
            static_cast<int>(std::distance(tagNames.constBegin(), nameIt));

        tagNames.removeAt(pos);
    }

    // 2) Remove the current tag's parent tag's name from the list of possible
    // new parents
    if (clickedItemParentIndex.isValid()) {
        const auto * parentItem =
            tagModel->itemForIndex(clickedItemParentIndex);

        if (Q_UNLIKELY(!parentItem)) {
            QNWARNING(
                "view::TagItemView",
                "Can't find parent tag model item for valid parent tag item "
                "index");
        }
        else {
            const auto * parentTagItem = parentItem->cast<TagItem>();
            if (parentTagItem) {
                const QString & parentItemName = parentTagItem->name();

                const auto parentNameIt = std::lower_bound(
                    tagNames.constBegin(), tagNames.constEnd(), parentItemName);

                if ((parentNameIt != tagNames.constEnd()) &&
                    (*parentNameIt == parentItemName))
                {
                    const int pos = static_cast<int>(
                        std::distance(tagNames.constBegin(), parentNameIt));

                    tagNames.removeAt(pos);
                }
            }
        }
    }

    // 3) Remove the current tag's children names from the list of possible
    // new parents
    const int numItemChildren = modelItem->childrenCount();
    for (int i = 0; i < numItemChildren; ++i) {
        const auto * childItem = modelItem->childAtRow(i);
        if (Q_UNLIKELY(!childItem)) {
            QNWARNING(
                "view::TagItemView",
                "Found null pointer to child tag model item");
            continue;
        }

        const auto * childTagItem = childItem->cast<TagItem>();
        if (Q_UNLIKELY(!childTagItem)) {
            QNWARNING(
                "view::TagItemView",
                "Found a child tag model item under a tag item which contains "
                    << "no actual tag item: " << *childItem);
            continue;
        }

        const auto childNameIt = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(), childTagItem->name());

        if ((childNameIt != tagNames.constEnd()) &&
            (*childNameIt == childTagItem->name()))
        {
            const int pos = static_cast<int>(
                std::distance(tagNames.constBegin(), childNameIt));

            tagNames.removeAt(pos);
        }
    }

    if (Q_LIKELY(!tagNames.isEmpty())) {
        auto * targetParentSubMenu =
            m_tagItemContextMenu->addMenu(tr("Move to parent"));

        for (const auto & tagName: std::as_const(tagNames)) {
            QStringList dataPair;
            dataPair.reserve(2);
            dataPair << tagItem->localId();
            dataPair << tagName;

            addContextMenuAction(
                tagName, *targetParentSubMenu, this,
                [this] { onMoveTagToParentAction(); }, dataPair,
                canUpdate ? ActionState::Enabled : ActionState::Disabled);
        }
    }

    if (tagItem->isFavorited()) {
        addContextMenuAction(
            tr("Unfavorite"), *m_tagItemContextMenu, this,
            [this] { onUnfavoriteAction(); }, tagItem->localId(),
            canUpdate ? ActionState::Enabled : ActionState::Disabled);
    }
    else {
        addContextMenuAction(
            tr("Favorite"), *m_tagItemContextMenu, this,
            [this] { onFavoriteAction(); }, tagItem->localId(),
            canUpdate ? ActionState::Enabled : ActionState::Disabled);
    }

    m_tagItemContextMenu->addSeparator();

    addContextMenuAction(
        tr("Clear selection"), *m_tagItemContextMenu, this,
        [this] { onDeselectAction(); }, QString{}, ActionState::Enabled);

    addContextMenuAction(
        tr("Info") + QStringLiteral("..."), *m_tagItemContextMenu, this,
        [this] { onShowTagInfoAction(); }, tagItem->localId(),
        ActionState::Enabled);

    m_tagItemContextMenu->show();
    m_tagItemContextMenu->exec(event->globalPos());
}

void TagItemView::setTagsExpanded(
    const QStringList & tagLocalIds, const TagModel & model)
{
    QNDEBUG(
        "view::TagItemView",
        "TagItemView::setTagsExpanded: "
            << tagLocalIds.size()
            << ", tag local ids: " << tagLocalIds.join(QStringLiteral(",")));

    for (const auto & tagLocalId: std::as_const(tagLocalIds)) {
        QModelIndex index = model.indexForLocalId(tagLocalId);
        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void TagItemView::setLinkedNotebooksExpanded(
    const QStringList & linkedNotebookGuids, const TagModel & model)
{
    QNDEBUG(
        "view::TagItemView",
        "TagItemView::setLinkedNotebooksExpanded: "
            << linkedNotebookGuids.size() << ", linked notebook guids: "
            << linkedNotebookGuids.join(QStringLiteral(", ")));

    for (const auto & expandedLinkedNotebookGuid:
         std::as_const(linkedNotebookGuids))
    {
        QModelIndex index =
            model.indexForLinkedNotebookGuid(expandedLinkedNotebookGuid);

        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void TagItemView::setFavoritedFlag(const QAction & action, const bool favorited)
{
    auto * tagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG("view::TagItemView", "Non-tag model is used");
        return;
    }

    const QString itemLocalId = action.data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag for "
                       "the tag, can't get tag's local id from QAction"))
        return;
    }

    const QModelIndex itemIndex = tagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag for "
                       "the tag, the model returned invalid index for "
                       "the tag's local id"))
        return;
    }

    if (favorited) {
        tagModel->favoriteTag(itemIndex);
    }
    else {
        tagModel->unfavoriteTag(itemIndex);
    }
}

} // namespace quentier
