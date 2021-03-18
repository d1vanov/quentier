/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#include <memory>

#define TAG_ITEM_VIEW_GROUP_KEY QStringLiteral("TagItemView")

#define LAST_SELECTED_TAG_KEY  QStringLiteral("LastSelectedTagLocalUid")
#define LAST_SELECTED_TAGS_KEY QStringLiteral("LastSelectedTagLocalUids")

#define LAST_EXPANDED_TAG_ITEMS_KEY QStringLiteral("LastExpandedTagLocalUids")

#define LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY                                \
    QStringLiteral("LastExpandedLinkedNotebookItemsGuids")

#define ALL_TAGS_ROOT_ITEM_EXPANDED_KEY                                        \
    QStringLiteral("AllTagsRootItemExpanded")

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view:tag", errorDescription);                               \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

TagItemView::TagItemView(QWidget * parent) :
    AbstractNoteFilteringTreeView(QStringLiteral("tag"), parent)
{}

void TagItemView::saveItemsState()
{
    QNDEBUG("view:tag", "TagItemView::saveItemsState");

    const auto * pTagModel = qobject_cast<const TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    QStringList expandedTagItemsLocalIds;
    QStringList expandedLinkedNotebookItemsGuids;

    const auto indexes = pTagModel->persistentIndexes();
    for (const auto & index: qAsConst(indexes)) {
        if (!isExpanded(index)) {
            continue;
        }

        const auto * pModelItem = pTagModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING(
                "view:tag",
                "Tag model returned null pointer to tag "
                    << "model item for valid model index");
            continue;
        }

        const auto * pTagItem = pModelItem->cast<TagItem>();
        if (pTagItem) {
            expandedTagItemsLocalIds << pTagItem->localId();
            QNTRACE(
                "view:tag",
                "Found expanded tag item: local id = "
                    << pTagItem->localId());
        }

        const auto * pLinkedNotebookItem =
            pModelItem->cast<TagLinkedNotebookRootItem>();

        if (pLinkedNotebookItem) {
            expandedLinkedNotebookItemsGuids
                << pLinkedNotebookItem->linkedNotebookGuid();

            QNTRACE(
                "view:tag",
                "Found expanded tag linked notebook root "
                    << "item: linked notebook guid = "
                    << pLinkedNotebookItem->linkedNotebookGuid());
        }
    }

    ApplicationSettings appSettings{
        pTagModel->account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(TAG_ITEM_VIEW_GROUP_KEY);

    appSettings.setValue(
        LAST_EXPANDED_TAG_ITEMS_KEY, expandedTagItemsLocalIds);

    appSettings.setValue(
        LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY,
        expandedLinkedNotebookItemsGuids);

    saveAllItemsRootItemExpandedState(
        appSettings, ALL_TAGS_ROOT_ITEM_EXPANDED_KEY,
        pTagModel->allItemsRootItemIndex());

    appSettings.endGroup();
}

void TagItemView::restoreItemsState(const AbstractItemModel & model)
{
    QNDEBUG("view:tag", "TagItemView::restoreItemsState");

    const auto * pTagModel = qobject_cast<const TagModel *>(&model);
    if (Q_UNLIKELY(!pTagModel)) {
        QNWARNING("view:tag", "Non-tag model is used");
        return;
    }

    ApplicationSettings appSettings{
        model.account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(TAG_ITEM_VIEW_GROUP_KEY);

    const QStringList expandedTagItemsLocalIds =
        appSettings.value(LAST_EXPANDED_TAG_ITEMS_KEY).toStringList();

    const QStringList expandedLinkedNotebookItemsGuids =
        appSettings.value(LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY)
            .toStringList();

    const auto allTagsRootItemExpandedPreference =
        appSettings.value(ALL_TAGS_ROOT_ITEM_EXPANDED_KEY);

    appSettings.endGroup();

    const bool wasTrackingTagItemsState = trackItemsStateEnabled();
    setTrackItemsStateEnabled(false);

    setTagsExpanded(expandedTagItemsLocalIds, *pTagModel);
    setLinkedNotebooksExpanded(expandedLinkedNotebookItemsGuids, *pTagModel);

    bool allTagsRootItemExpanded = true;
    if (allTagsRootItemExpandedPreference.isValid()) {
        allTagsRootItemExpanded = allTagsRootItemExpandedPreference.toBool();
    }

    const auto allTagsRootItemIndex = pTagModel->allItemsRootItemIndex();
    setExpanded(allTagsRootItemIndex, allTagsRootItemExpanded);

    setTrackItemsStateEnabled(wasTrackingTagItemsState);
}

QString TagItemView::selectedItemsGroupKey() const
{
    return TAG_ITEM_VIEW_GROUP_KEY;
}

QString TagItemView::selectedItemsArrayKey() const
{
    return LAST_SELECTED_TAGS_KEY;
}

QString TagItemView::selectedItemsKey() const
{
    return LAST_SELECTED_TAG_KEY;
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
    auto * pTagModel = qobject_cast<TagModel *>(&model);
    if (Q_UNLIKELY(!pTagModel)) {
        QNWARNING("view:tag", "Non-tag model is used");
        return;
    }

    QObject::connect(
        pTagModel, &TagModel::notifyTagParentChanged, this,
        &TagItemView::onTagParentChanged);

    QObject::connect(
        pTagModel, &TagModel::aboutToAddTag, this,
        &TagItemView::onAboutToAddTag);

    QObject::connect(
        pTagModel, &TagModel::addedTag, this, &TagItemView::onAddedTag);

    QObject::connect(
        pTagModel, &TagModel::aboutToUpdateTag, this,
        &TagItemView::onAboutToUpdateTag);

    QObject::connect(
        pTagModel, &TagModel::updatedTag, this, &TagItemView::onUpdatedTag);

    QObject::connect(
        pTagModel, &TagModel::aboutToRemoveTags, this,
        &TagItemView::onAboutToRemoveTags);

    QObject::connect(
        pTagModel, &TagModel::removedTags, this, &TagItemView::onRemovedTags);
}

void TagItemView::deleteItem(
    const QModelIndex & itemIndex, AbstractItemModel & model)
{
    QNDEBUG("view:tag", "TagItemView::deleteItem");

    const QString localId = model.localIdForItemIndex(itemIndex);
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
        QNDEBUG("view:tag", "Tag deletion was not confirmed");
        return;
    }

    if (model.removeRow(itemIndex.row(), itemIndex.parent())) {
        QNDEBUG("view:tag", "Successfully deleted tag");
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
    QNDEBUG("view:tag", "TagItemView::onAboutToAddTag");
    prepareForModelChange();
}

void TagItemView::onAddedTag(const QModelIndex & index)
{
    QNDEBUG("view:tag", "TagItemView::onAddedTag");

    Q_UNUSED(index)
    postProcessModelChange();
}

void TagItemView::onAboutToUpdateTag(const QModelIndex & index)
{
    QNDEBUG("view:tag", "TagItemView::onAboutToUpdateTag");

    Q_UNUSED(index)
    prepareForModelChange();
}

void TagItemView::onUpdatedTag(const QModelIndex & index)
{
    QNDEBUG("view:tag", "TagItemView::onUpdatedTag");

    Q_UNUSED(index)
    postProcessModelChange();
}

void TagItemView::onAboutToRemoveTags()
{
    QNDEBUG("view:tag", "TagItemView::onAboutToRemoveTags");
    prepareForModelChange();
}

void TagItemView::onRemovedTags()
{
    QNDEBUG("view:tag", "TagItemView::onRemovedTags");
    postProcessModelChange();
}

void TagItemView::onCreateNewTagAction()
{
    QNDEBUG("view:tag", "TagItemView::onCreateNewTagAction");
    Q_EMIT newTagCreationRequested();
}

void TagItemView::onRenameTagAction()
{
    QNDEBUG("view:tag", "TagItemView::onRenameTagAction");

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    const auto itemIndex = pTagModel->indexForLocalId(itemLocalId);
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
    QNDEBUG("view:tag", "TagItemView::onDeleteTagAction");

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    const auto itemIndex = pTagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag: the model "
                       "returned invalid index for the tag's local id"))
        return;
    }

    deleteItem(itemIndex, *pTagModel);
}

void TagItemView::onEditTagAction()
{
    QNDEBUG("view:tag", "TagItemView::onEditTagAction");

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't edit tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    auto pEditTagDialog =
        std::make_unique<AddOrEditTagDialog>(pTagModel, this, itemLocalId);

    pEditTagDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pEditTagDialog->exec())
}

void TagItemView::onPromoteTagAction()
{
    QNDEBUG("view:tag", "TagItemView::onPromoteTagAction");

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    const auto itemIndex = pTagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag: the model "
                       "returned invalid index for the tag's local id"))
        return;
    }

    saveItemsState();

    const bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    const auto promotedItemIndex = pTagModel->promote(itemIndex);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (promotedItemIndex.isValid()) {
        QNDEBUG("view:tag", "Successfully promoted the tag");
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
    QNDEBUG("view:tag", "TagItemView::onDemoteTagAction");

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, can't get "
                       "tag's local id from QAction"))
        return;
    }

    const auto itemIndex = pTagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag: the model "
                       "returned invalid index for the tag's local id"))
        return;
    }

    saveItemsState();

    const bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    const auto demotedItemIndex = pTagModel->demote(itemIndex);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (demotedItemIndex.isValid()) {
        QNDEBUG("view:tag", "Successfully demoted the tag");
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
    QNDEBUG("view:tag", "TagItemView::onRemoveFromParentTagAction");

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent, "
                       "can't get tag's local id from QAction"))
        return;
    }

    const auto itemIndex = pTagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent: "
                       "the model returned invalid index for the tag's "
                       "local id"))
        return;
    }

    saveItemsState();

    const bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    const auto removedFromParentItemIndex =
        pTagModel->removeFromParent(itemIndex);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (removedFromParentItemIndex.isValid()) {
        QNDEBUG("view:tag", "Successfully removed the tag from parent");
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
    QNDEBUG("view:tag", "TagItemView::onMoveTagToParentAction");

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move tag to parent, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    const auto itemLocalIdAndParentName = pAction->data().toStringList();
    if (itemLocalIdAndParentName.size() != 2) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move tag to parent, "
                       "can't retrieve the tag local id and parent "
                       "name from QAction data"))
        return;
    }

    const QString & localId = itemLocalIdAndParentName.at(0);

    const auto itemIndex = pTagModel->indexForLocalId(localId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move tag to parent, "
                       "can't get valid model index for tag's local id"))
        return;
    }

    saveItemsState();

    const QString & parentTagName = itemLocalIdAndParentName.at(1);

    const bool wasTrackingSelection = trackSelectionEnabled();
    setTrackSelectionEnabled(false);

    const auto movedTagItemIndex =
        pTagModel->moveToParent(itemIndex, parentTagName);

    setTrackSelectionEnabled(wasTrackingSelection);

    if (!movedTagItemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't move tag to parent"))
        return;
    }

    QNDEBUG("view:tag", "Successfully moved the tag item to parent");
}

void TagItemView::onShowTagInfoAction()
{
    QNDEBUG("view:tag", "TagItemView::onShowTagInfoAction");
    Q_EMIT tagInfoRequested();
}

void TagItemView::onDeselectAction()
{
    QNDEBUG("view:tag", "TagItemView::onDeselectAction");

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
    QNDEBUG("view:tag", "TagItemView::onFavoriteAction");

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't favorite tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, true);
}

void TagItemView::onUnfavoriteAction()
{
    QNDEBUG("view:tag", "TagItemView::onUnfavoriteAction");

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite tag, can't "
                       "cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, false);
}

void TagItemView::onTagParentChanged(const QModelIndex & tagIndex)
{
    QNDEBUG("view:tag", "TagItemView::onTagParentChanged");

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    const auto * pItem = pTagModel->itemForIndex(tagIndex);
    if (Q_LIKELY(pItem && pItem->parent())) {
        const auto parentIndex = pTagModel->indexForItem(pItem->parent());

        const bool wasTrackingTagItemsState = trackItemsStateEnabled();
        setTrackItemsStateEnabled(false);

        setExpanded(parentIndex, true);

        setTrackItemsStateEnabled(wasTrackingTagItemsState);
    }

    restoreItemsState(*pTagModel);
    restoreSelectedItems(*pTagModel);
}

void TagItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("view:tag", "TagItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "view:tag",
            "Detected Qt error: tag item view received context menu event with "
                << "null pointer to the context menu event");
        return;
    }

    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used, not doing anything");
        return;
    }

    const auto clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view:tag",
            "Clicked item index is not valid, not doing "
                << "anything");
        return;
    }

    const auto * pModelItem = pTagModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the tag model item: no "
                       "item corresponding to the clicked item's index"))
        return;
    }

    if (Q_UNLIKELY(pModelItem->type() != ITagModelItem::Type::Tag)) {
        QNDEBUG(
            "view:tag",
            "Won't show the context menu for the tag model "
                << "item not of a tag type");
        return;
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the tag model "
                       "item as it contains no actual tag item even "
                       "though it is of a tag type"))

        QNWARNING("view:tag", *pModelItem);
        return;
    }

    delete m_pTagItemContextMenu;
    m_pTagItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled)               \
    {                                                                          \
        QAction * pAction = new QAction(name, menu);                           \
        pAction->setData(data);                                                \
        pAction->setEnabled(enabled);                                          \
        QObject::connect(                                                      \
            pAction, &QAction::triggered, this, &TagItemView::slot);           \
        menu->addAction(pAction);                                              \
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Create new tag") + QStringLiteral("..."), m_pTagItemContextMenu,
        onCreateNewTagAction, pTagItem->localId(), true);

    m_pTagItemContextMenu->addSeparator();

    const bool canUpdate =
        (pTagModel->flags(clickedItemIndex) & Qt::ItemIsEditable);

    ADD_CONTEXT_MENU_ACTION(
        tr("Rename"), m_pTagItemContextMenu, onRenameTagAction,
        pTagItem->localId(), canUpdate);

    const bool canDeleteTag =
        pTagItem->guid().isEmpty() &&
        !pTagModel->tagHasSynchronizedChildTags(pTagItem->localId());

    if (canDeleteTag) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Delete"), m_pTagItemContextMenu, onDeleteTagAction,
            pTagItem->localId(), true);
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Edit") + QStringLiteral("..."), m_pTagItemContextMenu,
        onEditTagAction, pTagItem->localId(), canUpdate);

    const auto clickedItemParentIndex = clickedItemIndex.parent();
    if (clickedItemParentIndex.isValid()) {
        m_pTagItemContextMenu->addSeparator();

        ADD_CONTEXT_MENU_ACTION(
            tr("Promote"), m_pTagItemContextMenu, onPromoteTagAction,
            pTagItem->localId(), canUpdate);

        ADD_CONTEXT_MENU_ACTION(
            tr("Remove from parent"), m_pTagItemContextMenu,
            onRemoveFromParentTagAction, pTagItem->localId(), canUpdate);
    }

    if (clickedItemIndex.row() != 0) {
        const auto siblingItemIndex = pTagModel->index(
            clickedItemIndex.row() - 1, clickedItemIndex.column(),
            clickedItemIndex.parent());

        const bool canUpdateItemAndSibling = canUpdate &&
            (pTagModel->flags(siblingItemIndex) & Qt::ItemIsEditable);

        ADD_CONTEXT_MENU_ACTION(
            tr("Demote"), m_pTagItemContextMenu, onDemoteTagAction,
            pTagItem->localId(), canUpdateItemAndSibling);
    }

    QStringList tagNames = pTagModel->tagNames(pTagItem->linkedNotebookGuid());

    // 1) Remove the current tag's name from the list of possible new parents
    const auto nameIt = std::lower_bound(
        tagNames.constBegin(), tagNames.constEnd(), pTagItem->name());

    if ((nameIt != tagNames.constEnd()) && (*nameIt == pTagItem->name())) {
        int pos =
            static_cast<int>(std::distance(tagNames.constBegin(), nameIt));

        tagNames.removeAt(pos);
    }

    // 2) Remove the current tag's parent tag's name from the list of possible
    // new parents
    if (clickedItemParentIndex.isValid()) {
        const auto * pParentItem =
            pTagModel->itemForIndex(clickedItemParentIndex);

        if (Q_UNLIKELY(!pParentItem)) {
            QNWARNING(
                "view:tag",
                "Can't find parent tag model item for "
                    << "valid parent tag item index");
        }
        else {
            const auto * pParentTagItem = pParentItem->cast<TagItem>();
            if (pParentTagItem) {
                const QString & parentItemName = pParentTagItem->name();

                const auto parentNameIt = std::lower_bound(
                    tagNames.constBegin(), tagNames.constEnd(), parentItemName);

                if ((parentNameIt != tagNames.constEnd()) &&
                    (*parentNameIt == parentItemName)) {
                    int pos = static_cast<int>(
                        std::distance(tagNames.constBegin(), parentNameIt));

                    tagNames.removeAt(pos);
                }
            }
        }
    }

    // 3) Remove the current tag's children names from the list of possible
    // new parents
    const int numItemChildren = pModelItem->childrenCount();
    for (int i = 0; i < numItemChildren; ++i) {
        const auto * pChildItem = pModelItem->childAtRow(i);
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING("view:tag", "Found null pointer to child tag model item");
            continue;
        }

        const auto * pChildTagItem = pChildItem->cast<TagItem>();
        if (Q_UNLIKELY(!pChildTagItem)) {
            QNWARNING(
                "view:tag",
                "Found a child tag model item under a tag "
                    << "item which contains no actual tag item: "
                    << *pChildItem);
            continue;
        }

        const auto childNameIt = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(), pChildTagItem->name());

        if ((childNameIt != tagNames.constEnd()) &&
            (*childNameIt == pChildTagItem->name()))
        {
            const int pos = static_cast<int>(
                std::distance(tagNames.constBegin(), childNameIt));

            tagNames.removeAt(pos);
        }
    }

    if (Q_LIKELY(!tagNames.isEmpty())) {
        auto * pTargetParentSubMenu =
            m_pTagItemContextMenu->addMenu(tr("Move to parent"));

        for (const auto & tagName: qAsConst(tagNames)) {
            QStringList dataPair;
            dataPair.reserve(2);
            dataPair << pTagItem->localId();
            dataPair << tagName;

            ADD_CONTEXT_MENU_ACTION(
                tagName, pTargetParentSubMenu, onMoveTagToParentAction,
                dataPair, canUpdate);
        }
    }

    if (pTagItem->isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Unfavorite"), m_pTagItemContextMenu, onUnfavoriteAction,
            pTagItem->localId(), canUpdate);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(
            tr("Favorite"), m_pTagItemContextMenu, onFavoriteAction,
            pTagItem->localId(), canUpdate);
    }

    m_pTagItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(
        tr("Clear selection"), m_pTagItemContextMenu, onDeselectAction,
        QString(), true);

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."), m_pTagItemContextMenu,
        onShowTagInfoAction, pTagItem->localId(), true);

    m_pTagItemContextMenu->show();
    m_pTagItemContextMenu->exec(pEvent->globalPos());
}

#undef ADD_CONTEXT_MENU_ACTION

void TagItemView::setTagsExpanded(
    const QStringList & tagLocalIds, const TagModel & model)
{
    QNDEBUG(
        "view:tag",
        "TagItemView::setTagsExpanded: "
            << tagLocalIds.size()
            << ", tag local ids: " << tagLocalIds.join(QStringLiteral(",")));

    for (const auto & tagLocalId: qAsConst(tagLocalIds)) {
        const auto index = model.indexForLocalId(tagLocalId);
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
        "view:tag",
        "TagItemView::setLinkedNotebooksExpanded: "
            << linkedNotebookGuids.size() << ", linked notebook guids: "
            << linkedNotebookGuids.join(QStringLiteral(", ")));

    for (const auto & expandedLinkedNotebookGuid: qAsConst(linkedNotebookGuids))
    {
        const auto index =
            model.indexForLinkedNotebookGuid(expandedLinkedNotebookGuid);

        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void TagItemView::setFavoritedFlag(const QAction & action, const bool favorited)
{
    auto * pTagModel = qobject_cast<TagModel *>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("view:tag", "Non-tag model is used");
        return;
    }

    const QString itemLocalId = action.data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag for "
                       "the tag, can't get tag's local id from QAction"))
        return;
    }

    const auto itemIndex = pTagModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag for "
                       "the tag, the model returned invalid index for "
                       "the tag's local id"))
        return;
    }

    if (favorited) {
        pTagModel->favoriteTag(itemIndex);
    }
    else {
        pTagModel->unfavoriteTag(itemIndex);
    }
}

} // namespace quentier
