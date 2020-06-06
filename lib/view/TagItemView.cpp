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

#include "TagItemView.h"

#include <lib/dialog/AddOrEditTagDialog.h>
#include <lib/model/tag/TagModel.h>
#include <lib/preferences/SettingsNames.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QMenu>
#include <QContextMenuEvent>

#include <memory>

#define TAG_ITEM_VIEW_GROUP_KEY QStringLiteral("TagItemView")

#define LAST_SELECTED_TAG_KEY QStringLiteral("LastSelectedTagLocalUid")

#define LAST_EXPANDED_TAG_ITEMS_KEY QStringLiteral("LastExpandedTagLocalUids")

#define LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY                                \
    QStringLiteral("LastExpandedLinkedNotebookItemsGuids")                     \
// LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY

#define ALL_TAGS_ROOT_ITEM_EXPANDED_KEY                                        \
    QStringLiteral("AllTagsRootItemExpanded")                                  \
// ALL_TAGS_ROOT_ITEM_EXPANDED_KEY

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING(errorDescription);                                           \
        Q_EMIT notifyError(errorDescription);                                  \
    }                                                                          \
// REPORT_ERROR

namespace quentier {

TagItemView::TagItemView(QWidget * parent) :
    ItemView(parent),
    m_pTagItemContextMenu(nullptr),
    m_trackingTagItemsState(false),
    m_trackingSelection(false),
    m_modelReady(false)
{
    QObject::connect(
        this,
        &TagItemView::expanded,
        this,
        &TagItemView::onTagItemCollapsedOrExpanded);

    QObject::connect(
        this,
        &TagItemView::collapsed,
        this,
        &TagItemView::onTagItemCollapsedOrExpanded);
}

void TagItemView::setNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    if (!m_pNoteFiltersManager.isNull())
    {
        if (m_pNoteFiltersManager.data() == &noteFiltersManager) {
            QNDEBUG("Already using this note filters manager");
            return;
        }

        disconnectFromNoteFiltersManagerFilterChanged();
    }

    m_pNoteFiltersManager = &noteFiltersManager;
    connectToNoteFiltersManagerFilterChanged();
}

void TagItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG("TagItemView::setModel");

    TagModel * pPreviousModel = qobject_cast<TagModel*>(model());
    if (pPreviousModel) {
        pPreviousModel->disconnect(this);
    }

    m_modelReady = false;
    m_trackingSelection = false;
    m_trackingTagItemsState = false;

    TagModel * pTagModel = qobject_cast<TagModel*>(pModel);
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model has been set to the tag item view");
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(
        pTagModel,
        &TagModel::notifyError,
        this,
        &TagItemView::notifyError);

    QObject::connect(
        pTagModel,
        &TagModel::notifyTagParentChanged,
        this,
        &TagItemView::onTagParentChanged);

    QObject::connect(
        pTagModel,
        &TagModel::aboutToAddTag,
        this,
        &TagItemView::onAboutToAddTag);

    QObject::connect(
        pTagModel,
        &TagModel::addedTag,
        this,
        &TagItemView::onAddedTag);

    QObject::connect(
        pTagModel,
        &TagModel::aboutToUpdateTag,
        this,
        &TagItemView::onAboutToUpdateTag);

    QObject::connect(
        pTagModel,
        &TagModel::updatedTag,
        this,
        &TagItemView::onUpdatedTag);

    QObject::connect(
        pTagModel,
        &TagModel::aboutToRemoveTags,
        this,
        &TagItemView::onAboutToRemoveTags);

    QObject::connect(
        pTagModel,
        &TagModel::removedTags,
        this,
        &TagItemView::onRemovedTags);

    ItemView::setModel(pModel);

    if (pTagModel->allTagsListed()) {
        QNDEBUG("All tags are already listed within the model");
        restoreTagItemsState(*pTagModel);
        restoreSelectedTag(*pTagModel);
        m_modelReady = true;
        m_trackingSelection = true;
        m_trackingTagItemsState = true;
        return;
    }

    QObject::connect(
        pTagModel,
        &TagModel::notifyAllTagsListed,
        this,
        &TagItemView::onAllTagsListed);
}

QModelIndex TagItemView::currentlySelectedItemIndex() const
{
    QNDEBUG("TagItemView::currentlySelectedItemIndex");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return QModelIndex();
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG("No selection model in the view");
        return QModelIndex();
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG("The selection contains no model indexes");
        return QModelIndex();
    }

    return singleRow(
        indexes,
        *pTagModel,
        static_cast<int>(TagModel::Column::Name));
}

void TagItemView::deleteSelectedItem()
{
    QNDEBUG("TagItemView::deleteSelectedItem");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG("No tags are selected, nothing to deete");
        Q_UNUSED(informationMessageBox(
            this,
            tr("Cannot delete current tag"),
            tr("No tag is selected currently"),
            tr("Please select the tag you want to delete")))
        return;
    }

    QModelIndex index = singleRow(
        indexes,
        *pTagModel,
        static_cast<int>(TagModel::Column::Name));

    if (!index.isValid()) {
        QNDEBUG("Not exactly one tag within the selection");
        Q_UNUSED(informationMessageBox(
            this,
            tr("Cannot delete current tag"),
            tr("More than one tag is currently selected"),
            tr("Please select only the tag you want to delete")))
        return;
    }

    deleteItem(index, *pTagModel);
}

void TagItemView::onAllTagsListed()
{
    QNDEBUG("TagItemView::onAllTagsListed");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't cast the model set to the tag item view "
                                "to the tag model"))
        return;
    }

    QObject::disconnect(
        pTagModel,
        &TagModel::notifyAllTagsListed,
        this,
        &TagItemView::onAllTagsListed);

    restoreTagItemsState(*pTagModel);
    restoreSelectedTag(*pTagModel);

    m_modelReady = true;
    m_trackingSelection = true;
    m_trackingTagItemsState = true;
}

void TagItemView::onAboutToAddTag()
{
    QNDEBUG("TagItemView::onAboutToAddTag");
    prepareForTagModelChange();
}

void TagItemView::onAddedTag(const QModelIndex & index)
{
    QNDEBUG("TagItemView::onAddedTag");

    Q_UNUSED(index)
    postProcessTagModelChange();
}

void TagItemView::onAboutToUpdateTag(const QModelIndex & index)
{
    QNDEBUG("TagItemView::onAboutToUpdateTag");

    Q_UNUSED(index)
    prepareForTagModelChange();
}

void TagItemView::onUpdatedTag(const QModelIndex & index)
{
    QNDEBUG("TagItemView::onUpdatedTag");

    Q_UNUSED(index)
    postProcessTagModelChange();
}

void TagItemView::onAboutToRemoveTags()
{
    QNDEBUG("TagItemView::onAboutToRemoveTags");
    prepareForTagModelChange();
}

void TagItemView::onRemovedTags()
{
    QNDEBUG("TagItemView::onRemovedTags");
    postProcessTagModelChange();
}

void TagItemView::onCreateNewTagAction()
{
    QNDEBUG("TagItemView::onCreateNewTagAction");
    Q_EMIT newTagCreationRequested();
}

void TagItemView::onRenameTagAction()
{
    QNDEBUG("TagItemView::onRenameTagAction");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename tag, can't get "
                       "tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename tag: the model "
                       "returned invalid index for the tag's local uid"));
        return;
    }

    edit(itemIndex);
}

void TagItemView::onDeleteTagAction()
{
    QNDEBUG("TagItemView::onDeleteTagAction");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag, can't get "
                       "tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete tag: the model "
                       "returned invalid index for the tag's local uid"))
        return;
    }

    deleteItem(itemIndex, *pTagModel);
}

void TagItemView::onEditTagAction()
{
    QNDEBUG("TagItemView::onEditTagAction");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't edit tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't get "
                       "tag's local uid from QAction"))
        return;
    }

    auto pEditTagDialog = std::make_unique<AddOrEditTagDialog>(
        pTagModel,
        this,
        itemLocalUid);

    pEditTagDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pEditTagDialog->exec())
}

void TagItemView::onPromoteTagAction()
{
    QNDEBUG("TagItemView::onPromoteTagAction");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, can't get "
                       "tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag: the model "
                       "returned invalid index for the tag's local uid"))
        return;
    }

    saveTagItemsState();

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;

    QModelIndex promotedItemIndex = pTagModel->promote(itemIndex);

    m_trackingSelection = wasTrackingSelection;

    if (promotedItemIndex.isValid()) {
        QNDEBUG("Successfully promoted the tag");
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
    QNDEBUG("TagItemView::onDemoteTagAction");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, can't get "
                       "tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag: the model "
                       "returned invalid index for the tag's local uid"))
        return;
    }

    saveTagItemsState();

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;

    QModelIndex demotedItemIndex = pTagModel->demote(itemIndex);

    m_trackingSelection = wasTrackingSelection;

    if (demotedItemIndex.isValid()) {
        QNDEBUG("Successfully demoted the tag");
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
    QNDEBUG("TagItemView::onRemoveFromParentTagAction");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent, "
                       "can't get tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't remove tag from parent: "
                       "the model returned invalid index for the tag's "
                       "local uid"))
        return;
    }

    saveTagItemsState();

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;

    QModelIndex removedFromParentItemIndex = pTagModel->removeFromParent(
        itemIndex);

    m_trackingSelection = wasTrackingSelection;

    if (removedFromParentItemIndex.isValid()) {
        QNDEBUG("Successfully removed the tag from parent");
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
    QNDEBUG("TagItemView::onMoveTagToParentAction");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't move tag to parent, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    auto itemLocalUidAndParentName = pAction->data().toStringList();
    if (itemLocalUidAndParentName.size() != 2) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move tag to parent, "
                       "can't retrieve the tag local uid and parent "
                       "name from QAction data"))
        return;
    }

    const QString & localUid = itemLocalUidAndParentName.at(0);

    QModelIndex itemIndex = pTagModel->indexForLocalUid(localUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't move tag to parent, "
                       "can't get valid model index for tag's local uid"))
        return;
    }

    saveTagItemsState();

    const QString & parentTagName = itemLocalUidAndParentName.at(1);

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;

    QModelIndex movedTagItemIndex =
        pTagModel->moveToParent(itemIndex, parentTagName);

    m_trackingSelection = wasTrackingSelection;

    if (!movedTagItemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't move tag to parent"))
        return;
    }

    QNDEBUG("Successfully moved the tag item to parent");
}

void TagItemView::onShowTagInfoAction()
{
    QNDEBUG("TagItemView::onShowTagInfoAction");
    Q_EMIT tagInfoRequested();
}

void TagItemView::onDeselectAction()
{
    QNDEBUG("TagItemView::onDeselectAction");

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
    QNDEBUG("TagItemView::onFavoriteAction");

    auto * pAction = qobject_cast<QAction*>(sender());
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
    QNDEBUG("TagItemView::onUnfavoriteAction");

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite tag, can't "
                       "cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, false);
}

void TagItemView::onTagItemCollapsedOrExpanded(const QModelIndex & index)
{
    QNTRACE("TagItemView::onTagItemCollapsedOrExpanded: index: valid = "
        << (index.isValid() ? "true" : "false")
        << ", row = " << index.row() << ", column = " << index.column()
        << ", parent: valid = "
        << (index.parent().isValid() ? "true" : "false")
        << ", row = " << index.parent().row()
        << ", column = " << index.parent().column());

    if (!m_trackingTagItemsState) {
        QNDEBUG("Not tracking the tag items state at this moment");
        return;
    }

    saveTagItemsState();
}

void TagItemView::onTagParentChanged(const QModelIndex & tagIndex)
{
    QNDEBUG("TagItemView::onTagParentChanged");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    const auto * pItem = pTagModel->itemForIndex(tagIndex);
    if (Q_LIKELY(pItem && pItem->parent()))
    {
        QModelIndex parentIndex = pTagModel->indexForItem(pItem->parent());

        bool wasTrackingTagItemsState = m_trackingTagItemsState;
        m_trackingTagItemsState = false;

        setExpanded(parentIndex, true);

        m_trackingTagItemsState = wasTrackingTagItemsState;
    }

    restoreTagItemsState(*pTagModel);
    restoreSelectedTag(*pTagModel);
}

void TagItemView::onNoteFilterChanged()
{
    QNDEBUG("TagItemView::onNoteFilterChanged");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG("Note filters manager is null");
        return;
    }

    if (m_pNoteFiltersManager->isFilterBySearchStringActive()) {
        QNDEBUG("Filter by search string is active");
        selectAllTagsRootItem(*pTagModel);
        return;
    }

    if (!m_pNoteFiltersManager->savedSearchLocalUidInFilter().isEmpty()) {
        QNDEBUG("Filter by saved search is active");
        selectAllTagsRootItem(*pTagModel);
        return;
    }

    const auto & tagLocalUids =
        m_pNoteFiltersManager->tagLocalUidsInFilter();

    if (tagLocalUids.size() != 1) {
        QNDEBUG("Not exactly one tag local uid is within the filter: "
            << tagLocalUids.join(QStringLiteral(", ")));
        selectAllTagsRootItem(*pTagModel);
        return;
    }

    auto tagIndex = pTagModel->indexForLocalUid(tagLocalUids[0]);
    if (Q_UNLIKELY(!tagIndex.isValid())) {
        QNWARNING("The filtered tag local uid's index is invalid");
        selectAllTagsRootItem(*pTagModel);
        return;
    }

    setCurrentIndex(tagIndex);
}

void TagItemView::onNoteFiltersManagerReady()
{
    QNDEBUG("TagItemView::onNoteFiltersManagerReady");

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG("Note filters manager is null");
        return;
    }

    QObject::disconnect(
        m_pNoteFiltersManager.data(),
        &NoteFiltersManager::ready,
        this,
        &TagItemView::onNoteFiltersManagerReady);

    QString tagLocalUid = m_tagLocalUidPendingNoteFiltersManagerReadiness;
    m_tagLocalUidPendingNoteFiltersManagerReadiness.clear();

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    saveSelectedTag(pTagModel->account(), tagLocalUid);

    if (shouldFilterBySelectedTag(pTagModel->account()))
    {
        if (!tagLocalUid.isEmpty()) {
            setSelectedTagToNoteFiltersManager(tagLocalUid);
        }
        else {
            clearTagsFromNoteFiltersManager();
        }
    }
}

void TagItemView::selectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG("TagItemView::selectionChanged");

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void TagItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("TagItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING("Detected Qt error: tag item view received context menu "
                  "event with null pointer to the context menu event");
        return;
    }

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used, not doing anything");
        return;
    }

    QModelIndex clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG("Clicked item index is not valid, not doing anything");
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
        QNDEBUG("Won't show the context menu for the tag model "
            << "item not of a tag type");
        return;
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem))
    {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the tag model "
                       "item as it contains no actual tag item even "
                       "though it is of a tag type"))
        QNWARNING(*pModelItem);
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
            pAction,                                                           \
            &QAction::triggered,                                               \
            this,                                                              \
            &TagItemView::slot);                                               \
        menu->addAction(pAction);                                              \
    }                                                                          \
// ADD_CONTEXT_MENU_ACTION

    ADD_CONTEXT_MENU_ACTION(
        tr("Create new tag") + QStringLiteral("..."),
        m_pTagItemContextMenu,
        onCreateNewTagAction,
        pTagItem->localUid(),
        true);

    m_pTagItemContextMenu->addSeparator();

    bool canUpdate = (pTagModel->flags(clickedItemIndex) & Qt::ItemIsEditable);

    ADD_CONTEXT_MENU_ACTION(
        tr("Rename"),
        m_pTagItemContextMenu,
        onRenameTagAction,
        pTagItem->localUid(),
        canUpdate);

    bool canDeleteTag = pTagItem->guid().isEmpty();
    if (canDeleteTag) {
        canDeleteTag = !pTagModel->tagHasSynchronizedChildTags(
            pTagItem->localUid());
    }

    if (canDeleteTag) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Delete"),
            m_pTagItemContextMenu,
            onDeleteTagAction,
            pTagItem->localUid(),
            true);
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Edit") + QStringLiteral("..."),
        m_pTagItemContextMenu,
        onEditTagAction,
        pTagItem->localUid(),
        canUpdate);

    QModelIndex clickedItemParentIndex = clickedItemIndex.parent();
    if (clickedItemParentIndex.isValid())
    {
        m_pTagItemContextMenu->addSeparator();

        ADD_CONTEXT_MENU_ACTION(
            tr("Promote"),
            m_pTagItemContextMenu,
            onPromoteTagAction,
            pTagItem->localUid(),
            canUpdate);

        ADD_CONTEXT_MENU_ACTION(
            tr("Remove from parent"),
            m_pTagItemContextMenu,
            onRemoveFromParentTagAction,
            pTagItem->localUid(),
            canUpdate);
    }

    if (clickedItemIndex.row() != 0)
    {
        QModelIndex siblingItemIndex = pTagModel->index(
            clickedItemIndex.row() - 1,
            clickedItemIndex.column(),
            clickedItemIndex.parent());

        bool canUpdateItemAndSibling =
            canUpdate &&
            (pTagModel->flags(siblingItemIndex) & Qt::ItemIsEditable);

        ADD_CONTEXT_MENU_ACTION(
            tr("Demote"),
            m_pTagItemContextMenu,
            onDemoteTagAction,
            pTagItem->localUid(),
            canUpdateItemAndSibling);
    }

    QStringList tagNames = pTagModel->tagNames(pTagItem->linkedNotebookGuid());

    // 1) Remove the current tag's name from the list of possible new parents
    auto nameIt = std::lower_bound(
        tagNames.constBegin(),
        tagNames.constEnd(),
        pTagItem->name());

    if ((nameIt != tagNames.constEnd()) && (*nameIt == pTagItem->name()))
    {
        int pos = static_cast<int>(
            std::distance(tagNames.constBegin(), nameIt));

        tagNames.removeAt(pos);
    }

    // 2) Remove the current tag's parent tag's name from the list of possible
    // new parents
    if (clickedItemParentIndex.isValid())
    {
        const auto * pParentItem = pTagModel->itemForIndex(
            clickedItemParentIndex);

        if (Q_UNLIKELY(!pParentItem))
        {
            QNWARNING("Can't find parent tag model item for "
                << "valid parent tag item index");
        }
        else
        {
            const auto * pParentTagItem = pParentItem->cast<TagItem>();
            if (pParentTagItem)
            {
                const QString & parentItemName = pParentTagItem->name();

                auto parentNameIt = std::lower_bound(
                    tagNames.constBegin(),
                    tagNames.constEnd(),
                    parentItemName);

                if ((parentNameIt != tagNames.constEnd()) &&
                    (*parentNameIt == parentItemName))
                {
                    int pos = static_cast<int>(std::distance(
                        tagNames.constBegin(),
                        parentNameIt));

                    tagNames.removeAt(pos);
                }
            }
        }
    }

    // 3) Remove the current tag's children names from the list of possible
    // new parents
    int numItemChildren = pModelItem->childrenCount();
    for(int i = 0; i < numItemChildren; ++i)
    {
        const auto * pChildItem = pModelItem->childAtRow(i);
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING("Found null pointer to child tag model item");
            continue;
        }

        const auto * pChildTagItem = pChildItem->cast<TagItem>();
        if (Q_UNLIKELY(!pChildTagItem)) {
            QNWARNING("Found a child tag model item under a tag "
                << "item which contains no actual tag item: "
                << *pChildItem);
            continue;
        }

        auto childNameIt = std::lower_bound(
            tagNames.constBegin(),
            tagNames.constEnd(),
            pChildTagItem->name());

        if ((childNameIt != tagNames.constEnd()) &&
            (*childNameIt == pChildTagItem->name()))
        {
            int pos = static_cast<int>(std::distance(
                tagNames.constBegin(),
                childNameIt));

            tagNames.removeAt(pos);
        }
    }

    if (Q_LIKELY(!tagNames.isEmpty()))
    {
        auto * pTargetParentSubMenu = m_pTagItemContextMenu->addMenu(
            tr("Move to parent"));

        for(const auto & tagName: qAsConst(tagNames))
        {
            QStringList dataPair;
            dataPair.reserve(2);
            dataPair << pTagItem->localUid();
            dataPair << tagName;

            ADD_CONTEXT_MENU_ACTION(
                tagName,
                pTargetParentSubMenu,
                onMoveTagToParentAction,
                dataPair,
                canUpdate);
        }
    }

    if (pTagItem->isFavorited())
    {
        ADD_CONTEXT_MENU_ACTION(
            tr("Unfavorite"),
            m_pTagItemContextMenu,
            onUnfavoriteAction,
            pTagItem->localUid(),
            canUpdate);
    }
    else
    {
        ADD_CONTEXT_MENU_ACTION(
            tr("Favorite"),
            m_pTagItemContextMenu,
            onFavoriteAction,
            pTagItem->localUid(),
            canUpdate);
    }

    m_pTagItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(
        tr("Clear selection"),
        m_pTagItemContextMenu,
        onDeselectAction,
        QString(),
        true);

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."),
        m_pTagItemContextMenu,
        onShowTagInfoAction,
        pTagItem->localUid(),
        true);

    m_pTagItemContextMenu->show();
    m_pTagItemContextMenu->exec(pEvent->globalPos());
}

#undef ADD_CONTEXT_MENU_ACTION

void TagItemView::deleteItem(const QModelIndex & itemIndex, TagModel & model)
{
    QNDEBUG("TagItemView::deleteItem");

    const auto * pItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the tag model item "
                       "meant to be deleted"))
        return;
    }

    int confirm = warningMessageBox(
        this,
        tr("Confirm the tag deletion"),
        tr("Are you sure you want to delete this tag?"),
        tr("Note that this action is not reversible and the deletion of "
           "the tag would mean its disappearance from all the notes using it!"),
        QMessageBox::Ok | QMessageBox::No);

    if (confirm != QMessageBox::Ok) {
        QNDEBUG("Tag deletion was not confirmed");
        return;
    }

    bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
    if (res) {
        QNDEBUG("Successfully deleted tag");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The tag model refused to delete the tag; Check the status bar "
           "for message from the tag model explaining why the tag could not "
           "be deleted")))
}

void TagItemView::saveTagItemsState()
{
    QNDEBUG("TagItemView::saveTagItemsState");

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    QStringList expandedTagItemsLocalUids;
    QStringList expandedLinkedNotebookItemsGuids;

    auto indexes = pTagModel->persistentIndexes();
    for(const auto & index: qAsConst(indexes))
    {
        if (!isExpanded(index)) {
            continue;
        }

        const auto * pModelItem = pTagModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING("Tag model returned null pointer to tag "
                << "model item for valid model index");
            continue;
        }

        const auto * pTagItem = pModelItem->cast<TagItem>();
        if (pTagItem) {
            expandedTagItemsLocalUids << pTagItem->localUid();
            QNTRACE("Found expanded tag item: local uid = "
                << pTagItem->localUid());
        }

        const auto * pLinkedNotebookItem =
            pModelItem->cast<TagLinkedNotebookRootItem>();

        if (pLinkedNotebookItem) {
            expandedLinkedNotebookItemsGuids
                << pLinkedNotebookItem->linkedNotebookGuid();

            QNTRACE("Found expanded tag linked notebook root "
                << "item: linked notebook guid = "
                << pLinkedNotebookItem->linkedNotebookGuid());
        }
    }

    ApplicationSettings appSettings(pTagModel->account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("TagItemView"));

    appSettings.setValue(
        LAST_EXPANDED_TAG_ITEMS_KEY,
        expandedTagItemsLocalUids);

    appSettings.setValue(
        LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY,
        expandedLinkedNotebookItemsGuids);

    auto allTagsRootItemIndex = pTagModel->index(0, 0);

    appSettings.setValue(
        ALL_TAGS_ROOT_ITEM_EXPANDED_KEY,
        isExpanded(allTagsRootItemIndex));

    appSettings.endGroup();
}

void TagItemView::restoreTagItemsState(const TagModel & model)
{
    QNDEBUG("TagItemView::restoreTagItemsState");

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("TagItemView"));

    QStringList expandedTagItemsLocalUids = appSettings.value(
        LAST_EXPANDED_TAG_ITEMS_KEY).toStringList();

    QStringList expandedLinkedNotebookItemsGuids = appSettings.value(
        LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY).toStringList();

    auto allTagsRootItemExpandedPreference = appSettings.value(
        ALL_TAGS_ROOT_ITEM_EXPANDED_KEY);

    appSettings.endGroup();

    bool wasTrackingTagItemsState = m_trackingTagItemsState;
    m_trackingTagItemsState = false;

    setTagsExpanded(expandedTagItemsLocalUids, model);
    setLinkedNotebooksExpanded(expandedLinkedNotebookItemsGuids, model);

    bool allTagsRootItemExpanded = true;
    if (allTagsRootItemExpandedPreference.isValid()) {
        allTagsRootItemExpanded =
            allTagsRootItemExpandedPreference.toBool();
    }

    auto allTagsRootItemIndex = model.index(0, 0);
    setExpanded(allTagsRootItemIndex, allTagsRootItemExpanded);

    m_trackingTagItemsState = wasTrackingTagItemsState;
}

void TagItemView::setTagsExpanded(
    const QStringList & tagLocalUids, const TagModel & model)
{
    QNDEBUG("TagItemView::setTagsExpanded: " << tagLocalUids.size()
        << ", tag local uids: " << tagLocalUids.join(QStringLiteral(",")));

    for(const auto & tagLocalUid: qAsConst(tagLocalUids))
    {
        QModelIndex index = model.indexForLocalUid(tagLocalUid);
        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void TagItemView::setLinkedNotebooksExpanded(
    const QStringList & linkedNotebookGuids, const TagModel & model)
{
    QNDEBUG("TagItemView::setLinkedNotebooksExpanded: "
        << linkedNotebookGuids.size() << ", linked notebook guids: "
        << linkedNotebookGuids.join(QStringLiteral(", ")));

    for(const auto & expandedLinkedNotebookGuid: qAsConst(linkedNotebookGuids))
    {
        QModelIndex index = model.indexForLinkedNotebookGuid(
            expandedLinkedNotebookGuid);

        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void TagItemView::saveSelectedTag(
    const Account & account, const QString & tagLocalUid)
{
    QNDEBUG("TagItemView::saveSelectedTag: " << tagLocalUid);

    ApplicationSettings appSettings(account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(TAG_ITEM_VIEW_GROUP_KEY);
    appSettings.setValue(LAST_SELECTED_TAG_KEY, tagLocalUid);
    appSettings.endGroup();
}

void TagItemView::restoreSelectedTag(const TagModel & model)
{
    QNDEBUG("TagItemView::restoreSelectedTag");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't restore the last selected tag: "
                       "no selection model in the view"))
        return;
    }

    QString selectedTagLocalUid;

    if (shouldFilterBySelectedTag(model.account()))
    {
        if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
            QNDEBUG("Note filters manager is null");
            return;
        }

        auto filteredTagLocalUids =
            m_pNoteFiltersManager->tagLocalUidsInFilter();

        if (filteredTagLocalUids.isEmpty()) {
            QNDEBUG("No tags within filter, selecting all tags "
                << "root item");
            selectAllTagsRootItem(model);
            return;
        }

        if (filteredTagLocalUids.size() != 1) {
            QNDEBUG("Not exactly one tag local uid within filter: "
                << filteredTagLocalUids.join(QStringLiteral(",")));
            return;
        }

        selectedTagLocalUid = filteredTagLocalUids.at(0);
    }
    else
    {
        QNDEBUG("Filtering by selected tag is switched off");

        ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
        appSettings.beginGroup(TAG_ITEM_VIEW_GROUP_KEY);

        selectedTagLocalUid = appSettings.value(
            LAST_SELECTED_TAG_KEY).toString();

        appSettings.endGroup();
    }

    QNTRACE("Selecting tag local uid: " << selectedTagLocalUid);

    if (selectedTagLocalUid.isEmpty()) {
        QNDEBUG("Found no last selected tag local uid");
        return;
    }

    QModelIndex selectedTagIndex = model.indexForLocalUid(
        selectedTagLocalUid);

    if (!selectedTagIndex.isValid()) {
        QNDEBUG("Tag model returned invalid index for the last "
            << "selected tag local uid");
        return;
    }

    pSelectionModel->select(
        selectedTagIndex,
        QItemSelectionModel::ClearAndSelect |
        QItemSelectionModel::Rows |
        QItemSelectionModel::Current);
}

void TagItemView::selectionChangedImpl(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNTRACE("TagItemView::selectionChangedImpl");

    if (!m_trackingSelection) {
        QNTRACE("Not tracking selection at this time, skipping");
        return;
    }

    Q_UNUSED(deselected)

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    auto selectedIndexes = selected.indexes();

    if (selectedIndexes.isEmpty()) {
        QNDEBUG("The new selection is empty");
        handleNoSelectedTag(pTagModel->account());
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    QModelIndex sourceIndex = singleRow(
        selectedIndexes,
        *pTagModel,
        static_cast<int>(TagModel::Column::Name));

    if (!sourceIndex.isValid()) {
        QNDEBUG("Not exactly one row is selected");
        handleNoSelectedTag(pTagModel->account());
        return;
    }

    const auto * pModelItem = pTagModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the tag model item "
                       "corresponding to the selected index"))
        handleNoSelectedTag(pTagModel->account());
        return;
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem)) {
        QNDEBUG("The selected tag model item is not of a tag type");
        handleNoSelectedTag(pTagModel->account());
        return;
    }

    if (!pTagItem->linkedNotebookGuid().isEmpty()) {
        QNDEBUG("Tag from the linked notebook is selected, "
            << "won't do anything");
        handleNoSelectedTag(pTagModel->account());
        return;
    }

    saveSelectedTag(pTagModel->account(), pTagItem->localUid());

    if (shouldFilterBySelectedTag(pTagModel->account())) {
        setSelectedTagToNoteFiltersManager(pTagItem->localUid());
    }
    else {
        QNDEBUG("Filtering by selected tag is switched off");
    }
}

void TagItemView::handleNoSelectedTag(const Account & account)
{
    saveSelectedTag(account, QString());

    if (shouldFilterBySelectedTag(account)) {
        clearTagsFromNoteFiltersManager();
    }
}

void TagItemView::selectAllTagsRootItem(const TagModel & model)
{
    QNDEBUG("TagItemView::selectAllTagsRootItem");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't select all tags root item: "
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

    handleNoSelectedTag(model.account());
}

void TagItemView::setFavoritedFlag(const QAction & action, const bool favorited)
{
    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    QString itemLocalUid = action.data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag for "
                       "the tag, can't get tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag for "
                       "the tag, the model returned invalid index for "
                       "the tag's local uid"))
        return;
    }

    if (favorited) {
        pTagModel->favoriteTag(itemIndex);
    }
    else {
        pTagModel->unfavoriteTag(itemIndex);
    }
}

void TagItemView::prepareForTagModelChange()
{
    if (!m_modelReady) {
        QNDEBUG("The model is not ready yet");
        return;
    }

    saveTagItemsState();
    m_trackingSelection = false;
    m_trackingTagItemsState = false;
}

void TagItemView::postProcessTagModelChange()
{
    if (!m_modelReady) {
        QNDEBUG("The model is not ready yet");
        return;
    }

    auto * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG("Non-tag model is used");
        return;
    }

    restoreTagItemsState(*pTagModel);
    m_trackingTagItemsState = true;

    restoreSelectedTag(*pTagModel);
    m_trackingSelection = true;
}

void TagItemView::setSelectedTagToNoteFiltersManager(
    const QString & tagLocalUid)
{
    QNDEBUG("TagItemView::setSelectedTagToNoteFiltersManager: "
        << tagLocalUid);

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG("Note filters manager is null");
        return;
    }

    if (!m_pNoteFiltersManager->isReady())
    {
        QNDEBUG("Note filters manager is not ready yet, will "
            << "postpone setting the tag to it: "
            << tagLocalUid);

        QObject::connect(
            m_pNoteFiltersManager.data(),
            &NoteFiltersManager::ready,
            this,
            &TagItemView::onNoteFiltersManagerReady,
            Qt::UniqueConnection);

        m_tagLocalUidPendingNoteFiltersManagerReadiness = tagLocalUid;
        return;
    }

    if (m_pNoteFiltersManager->isFilterBySearchStringActive()) {
        QNDEBUG("Filter by search string is active, won't set "
            << "the seleted tag to filter");
        return;
    }

    const QString & savedSearchLocalUidInFilter =
        m_pNoteFiltersManager->savedSearchLocalUidInFilter();

    if (!savedSearchLocalUidInFilter.isEmpty()) {
        QNDEBUG("Filter by saved search is active, won't set "
            << "the selected tag to filter");
        return;
    }

    disconnectFromNoteFiltersManagerFilterChanged();
    m_pNoteFiltersManager->setTagToFilter(tagLocalUid);
    connectToNoteFiltersManagerFilterChanged();
}

void TagItemView::clearTagsFromNoteFiltersManager()
{
    QNDEBUG("TagItemView::clearTagsFromNoteFiltersManager");

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        QNDEBUG("Note filters manager is null");
        return;
    }

    if (!m_pNoteFiltersManager->isReady())
    {
        QNDEBUG("Note filters manager is not ready yet, will "
            << "postpone clearing tags from it");

        m_tagLocalUidPendingNoteFiltersManagerReadiness.clear();

        QObject::connect(
            m_pNoteFiltersManager.data(),
            &NoteFiltersManager::ready,
            this,
            &TagItemView::onNoteFiltersManagerReady,
            Qt::UniqueConnection);

        return;
    }

    disconnectFromNoteFiltersManagerFilterChanged();
    m_pNoteFiltersManager->removeTagsFromFilter();
    connectToNoteFiltersManagerFilterChanged();
}

void TagItemView::disconnectFromNoteFiltersManagerFilterChanged()
{
    QObject::disconnect(
        m_pNoteFiltersManager.data(),
        &NoteFiltersManager::filterChanged,
        this,
        &TagItemView::onNoteFilterChanged);
}

void TagItemView::connectToNoteFiltersManagerFilterChanged()
{
    QObject::connect(
        m_pNoteFiltersManager.data(),
        &NoteFiltersManager::filterChanged,
        this,
        &TagItemView::onNoteFilterChanged,
        Qt::UniqueConnection);
}

bool TagItemView::shouldFilterBySelectedTag(const Account & account) const
{
    ApplicationSettings appSettings(account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(SIDE_PANELS_FILTER_BY_SELECTION_SETTINGS_GROUP_NAME);

    auto filterBySelectedTag = appSettings.value(
        FILTER_BY_SELECTED_TAG_SETTINGS_KEY);

    appSettings.endGroup();

    if (!filterBySelectedTag.isValid()) {
        return true;
    }

    return filterBySelectedTag.toBool();
}

} // namespace quentier
