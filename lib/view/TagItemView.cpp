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

#include "TagItemView.h"

#include <lib/preferences/SettingsNames.h>
#include <lib/model/TagModel.h>
#include <lib/dialog/AddOrEditTagDialog.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QMenu>
#include <QContextMenuEvent>
#include <QScopedPointer>

#define LAST_SELECTED_TAG_KEY QStringLiteral("LastSelectedTagLocalUid")
#define LAST_EXPANDED_TAG_ITEMS_KEY QStringLiteral("LastExpandedTagLocalUids")
#define LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY \
    QStringLiteral("LastExpandedLinkedNotebookItemsGuids")

#define REPORT_ERROR(error) \
    { \
        ErrorString errorDescription(error); \
        QNWARNING(errorDescription); \
        Q_EMIT notifyError(errorDescription); \
    }

namespace quentier {

TagItemView::TagItemView(QWidget * parent) :
    ItemView(parent),
    m_pTagItemContextMenu(Q_NULLPTR),
    m_trackingTagItemsState(false),
    m_trackingSelection(false),
    m_modelReady(false)
{
    QObject::connect(this,
                     QNSIGNAL(TagItemView,expanded,const QModelIndex&),
                     this,
                     QNSLOT(TagItemView,onTagItemCollapsedOrExpanded,
                            const QModelIndex&));
    QObject::connect(this,
                     QNSIGNAL(TagItemView,collapsed,const QModelIndex&),
                     this,
                     QNSLOT(TagItemView,onTagItemCollapsedOrExpanded,
                            const QModelIndex&));
}

void TagItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("TagItemView::setModel"));

    TagModel * pPreviousModel = qobject_cast<TagModel*>(model());
    if (pPreviousModel)
    {
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,notifyError,ErrorString),
                            this,
                            QNSIGNAL(TagItemView,notifyError,ErrorString));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,notifyAllTagsListed),
                            this,
                            QNSLOT(TagItemView,onAllTagsListed));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,notifyTagParentChanged,
                                     const QModelIndex&),
                            this,
                            QNSLOT(TagItemView,onTagParentChanged,
                                   const QModelIndex&));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,aboutToAddTag),
                            this,
                            QNSLOT(TagItemView,onAboutToAddTag));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,addedTag,const QModelIndex&),
                            this,
                            QNSLOT(TagItemView,onAddedTag,const QModelIndex&));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,aboutToUpdateTag,
                                     const QModelIndex&),
                            this,
                            QNSLOT(TagItemView,onAboutToUpdateTag,
                                   const QModelIndex&));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,updatedTag,const QModelIndex&),
                            this,
                            QNSLOT(TagItemView,onUpdatedTag,const QModelIndex&));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,aboutToRemoveTags),
                            this,
                            QNSLOT(TagItemView,onAboutToRemoveTags));
        QObject::disconnect(pPreviousModel,
                            QNSIGNAL(TagModel,removedTags),
                            this,
                            QNSLOT(TagItemView,onRemovedTags));
    }

    m_modelReady = false;
    m_trackingSelection = false;
    m_trackingTagItemsState = false;

    TagModel * pTagModel = qobject_cast<TagModel*>(pModel);
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model has been set to the tag item view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pTagModel, QNSIGNAL(TagModel,notifyError,ErrorString),
                     this, QNSIGNAL(TagItemView,notifyError,ErrorString));
    QObject::connect(pTagModel,
                     QNSIGNAL(TagModel,notifyTagParentChanged,const QModelIndex&),
                     this,
                     QNSLOT(TagItemView,onTagParentChanged,const QModelIndex&));
    QObject::connect(pTagModel,
                     QNSIGNAL(TagModel,aboutToAddTag),
                     this,
                     QNSLOT(TagItemView,onAboutToAddTag));
    QObject::connect(pTagModel,
                     QNSIGNAL(TagModel,addedTag,const QModelIndex&),
                     this,
                     QNSLOT(TagItemView,onAddedTag,const QModelIndex&));
    QObject::connect(pTagModel,
                     QNSIGNAL(TagModel,aboutToUpdateTag,const QModelIndex&),
                     this,
                     QNSLOT(TagItemView,onAboutToUpdateTag,const QModelIndex&));
    QObject::connect(pTagModel,
                     QNSIGNAL(TagModel,updatedTag,const QModelIndex&),
                     this,
                     QNSLOT(TagItemView,onUpdatedTag,const QModelIndex&));
    QObject::connect(pTagModel, QNSIGNAL(TagModel,aboutToRemoveTags),
                     this, QNSLOT(TagItemView,onAboutToRemoveTags));
    QObject::connect(pTagModel, QNSIGNAL(TagModel,removedTags),
                     this, QNSLOT(TagItemView,onRemovedTags));

    ItemView::setModel(pModel);

    if (pTagModel->allTagsListed()) {
        QNDEBUG(QStringLiteral("All tags are already listed within the model"));
        restoreTagItemsState(*pTagModel);
        restoreLastSavedSelection(*pTagModel);
        m_modelReady = true;
        m_trackingSelection = true;
        m_trackingTagItemsState = true;
        return;
    }

    QObject::connect(pTagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                     this, QNSLOT(TagItemView,onAllTagsListed));
}

QModelIndex TagItemView::currentlySelectedItemIndex() const
{
    QNDEBUG(QStringLiteral("TagItemView::currentlySelectedItemIndex"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
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

    return singleRow(indexes, *pTagModel, TagModel::Columns::Name);
}

void TagItemView::deleteSelectedItem()
{
    QNDEBUG(QStringLiteral("TagItemView::deleteSelectedItem"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(QStringLiteral("No tags are selected, nothing to deete"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current tag"),
                                       tr("No tag is selected currently"),
                                       tr("Please select the tag you want to "
                                          "delete")))
        return;
    }

    QModelIndex index = singleRow(indexes, *pTagModel, TagModel::Columns::Name);
    if (!index.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one tag within the selection"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current tag"),
                                       tr("More than one tag is currently selected"),
                                       tr("Please select only the tag you want "
                                          "to delete")))
        return;
    }

    deleteItem(index, *pTagModel);
}

void TagItemView::onAllTagsListed()
{
    QNDEBUG(QStringLiteral("TagItemView::onAllTagsListed"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't cast the model set to the tag item view "
                                "to the tag model"))
        return;
    }

    QObject::disconnect(pTagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                        this, QNSLOT(TagItemView,onAllTagsListed));

    restoreTagItemsState(*pTagModel);
    restoreLastSavedSelection(*pTagModel);

    m_modelReady = true;
    m_trackingSelection = true;
    m_trackingTagItemsState = true;
}

void TagItemView::onAboutToAddTag()
{
    QNDEBUG(QStringLiteral("TagItemView::onAboutToAddTag"));
    prepareForTagModelChange();
}

void TagItemView::onAddedTag(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("TagItemView::onAddedTag"));

    Q_UNUSED(index)
    postProcessTagModelChange();
}

void TagItemView::onAboutToUpdateTag(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("TagItemView::onAboutToUpdateTag"));

    Q_UNUSED(index)
    prepareForTagModelChange();
}

void TagItemView::onUpdatedTag(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("TagItemView::onUpdatedTag"));

    Q_UNUSED(index)
    postProcessTagModelChange();
}

void TagItemView::onAboutToRemoveTags()
{
    QNDEBUG(QStringLiteral("TagItemView::onAboutToRemoveTags"));
    prepareForTagModelChange();
}

void TagItemView::onRemovedTags()
{
    QNDEBUG(QStringLiteral("TagItemView::onRemovedTags"));
    postProcessTagModelChange();
}

void TagItemView::onCreateNewTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onCreateNewTagAction"));
    Q_EMIT newTagCreationRequested();
}

void TagItemView::onRenameTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onRenameTagAction"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename tag, can't cast "
                                "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename tag, can't get "
                                "tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't rename tag: the model "
                                "returned invalid index for the tag's local uid"));
        return;
    }

    edit(itemIndex);
}

void TagItemView::onDeleteTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onDeleteTagAction"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete tag, can't cast "
                                "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete tag, can't get "
                                "tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete tag: the model "
                                "returned invalid index for the tag's local uid"))
        return;
    }

    deleteItem(itemIndex, *pTagModel);
}

void TagItemView::onEditTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onEditTagAction"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't edit tag, can't cast "
                                "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't promote tag, can't get "
                                "tag's local uid from QAction"))
        return;
    }

    QScopedPointer<AddOrEditTagDialog> pEditTagDialog(
        new AddOrEditTagDialog(pTagModel, this, itemLocalUid));
    pEditTagDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pEditTagDialog->exec())
}

void TagItemView::onPromoteTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onPromoteTagAction"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't promote tag, can't cast "
                                "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't promote tag, can't get "
                                "tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't promote tag: the model "
                                "returned invalid index for the tag's local uid"))
        return;
    }

    saveTagItemsState();

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;

    QModelIndex promotedItemIndex = pTagModel->promote(itemIndex);

    m_trackingSelection = wasTrackingSelection;

    if (promotedItemIndex.isValid()) {
        QNDEBUG(QStringLiteral("Successfully promoted the tag"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The tag model refused to promote "
                                              "the tag; Check the status bar for "
                                              "message from the tag model "
                                              "explaining why the tag could not "
                                              "be promoted")))
}

void TagItemView::onDemoteTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onDemoteTagAction"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't demote tag, can't cast "
                                "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't demote tag, can't get "
                                "tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't demote tag: the model "
                                "returned invalid index for the tag's local uid"))
        return;
    }

    saveTagItemsState();

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;

    QModelIndex demotedItemIndex = pTagModel->demote(itemIndex);

    m_trackingSelection = wasTrackingSelection;

    if (demotedItemIndex.isValid()) {
        QNDEBUG(QStringLiteral("Successfully demoted the tag"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The tag model refused to demote "
                                              "the tag; Check the status bar "
                                              "for message from the tag model "
                                              "explaining why the tag could not "
                                              "be demoted")))
}

void TagItemView::onRemoveFromParentTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onRemoveFromParentTagAction"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove tag from parent, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove tag from parent, "
                                "can't get tag's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't remove tag from parent: "
                                "the model returned invalid index for the tag's "
                                "local uid"))
        return;
    }

    saveTagItemsState();

    bool wasTrackingSelection = m_trackingSelection;
    m_trackingSelection = false;

    QModelIndex removedFromParentItemIndex = pTagModel->removeFromParent(itemIndex);

    m_trackingSelection = wasTrackingSelection;

    if (removedFromParentItemIndex.isValid()) {
        QNDEBUG(QStringLiteral("Successfully removed the tag from parent"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The tag model refused to remove "
                                              "the tag from parent; Check "
                                              "the status bar for message from "
                                              "the tag model explaining why "
                                              "the tag could not be removed "
                                              "from its parent")))
}

void TagItemView::onMoveTagToParentAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onMoveTagToParentAction"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't move tag to parent, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    QStringList itemLocalUidAndParentName = pAction->data().toStringList();
    if (itemLocalUidAndParentName.size() != 2) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't move tag to parent, "
                                "can't retrieve the tag local uid and parent "
                                "name from QAction data"))
        return;
    }

    const QString & localUid = itemLocalUidAndParentName.at(0);

    QModelIndex itemIndex = pTagModel->indexForLocalUid(localUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't move tag to parent, "
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

    QNDEBUG(QStringLiteral("Successfully moved the tag item to parent"));
}

void TagItemView::onShowTagInfoAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onShowTagInfoAction"));
    Q_EMIT tagInfoRequested();
}

void TagItemView::onDeselectAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onDeselectAction"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't clear the tag selection: no selection "
                                "model in the view"))
        return;
    }

    pSelectionModel->clearSelection();
}

void TagItemView::onFavoriteAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onFavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't favorite tag, can't cast "
                                "the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, true);
}

void TagItemView::onUnfavoriteAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onUnfavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't unfavorite tag, can't "
                                "cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, false);
}

void TagItemView::onTagItemCollapsedOrExpanded(const QModelIndex & index)
{
    QNTRACE(QStringLiteral("TagItemView::onTagItemCollapsedOrExpanded: ")
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

    if (!m_trackingTagItemsState) {
        QNDEBUG(QStringLiteral("Not tracking the tag items state at this moment"));
        return;
    }

    saveTagItemsState();
}

void TagItemView::onTagParentChanged(const QModelIndex & tagIndex)
{
    QNDEBUG(QStringLiteral("TagItemView::onTagParentChanged"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    const TagModelItem * pItem = pTagModel->itemForIndex(tagIndex);
    if (Q_LIKELY(pItem && pItem->parent()))
    {
        QModelIndex parentIndex = pTagModel->indexForItem(pItem->parent());

        bool wasTrackingTagItemsState = m_trackingTagItemsState;
        m_trackingTagItemsState = false;

        setExpanded(parentIndex, true);

        m_trackingTagItemsState = wasTrackingTagItemsState;
    }

    restoreTagItemsState(*pTagModel);
    restoreLastSavedSelection(*pTagModel);
}

void TagItemView::selectionChanged(const QItemSelection & selected,
                                   const QItemSelection & deselected)
{
    QNDEBUG(QStringLiteral("TagItemView::selectionChanged"));

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void TagItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("TagItemView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: tag item view received "
                                 "context menu event with null pointer "
                                 "to the context menu event"));
        return;
    }

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used, not doing anything"));
        return;
    }

    QModelIndex clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing "
                               "anything"));
        return;
    }

    const TagModelItem * pModelItem = pTagModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't show the context menu for the tag model "
                                "item: no item corresponding to the clicked "
                                "item's index"))
        return;
    }

    if (Q_UNLIKELY(pModelItem->type() != TagModelItem::Type::Tag)) {
        QNDEBUG(QStringLiteral("Won't show the context menu for the tag model "
                               "item not of a tag type"));
        return;
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't show the context menu for the tag model "
                                "item as it contains no actual tag item even "
                                "though it is of a tag type"))
        QNWARNING(*pModelItem);
        return;
    }

    delete m_pTagItemContextMenu;
    m_pTagItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), \
                         this, QNSLOT(TagItemView,slot)); \
        menu->addAction(pAction); \
    }

    ADD_CONTEXT_MENU_ACTION(tr("Create new tag") + QStringLiteral("..."),
                            m_pTagItemContextMenu, onCreateNewTagAction,
                            pTagItem->localUid(), true);

    m_pTagItemContextMenu->addSeparator();

    bool canUpdate = (pTagModel->flags(clickedItemIndex) & Qt::ItemIsEditable);
    ADD_CONTEXT_MENU_ACTION(tr("Rename"), m_pTagItemContextMenu,
                            onRenameTagAction, pTagItem->localUid(), canUpdate);

    bool canDeleteTag = pTagItem->guid().isEmpty();
    if (canDeleteTag) {
        canDeleteTag = !pTagModel->tagHasSynchronizedChildTags(pTagItem->localUid());
    }

    if (canDeleteTag) {
        ADD_CONTEXT_MENU_ACTION(tr("Delete"), m_pTagItemContextMenu,
                                onDeleteTagAction, pTagItem->localUid(),
                                true);
    }

    ADD_CONTEXT_MENU_ACTION(tr("Edit") + QStringLiteral("..."),
                            m_pTagItemContextMenu, onEditTagAction,
                            pTagItem->localUid(), canUpdate);

    QModelIndex clickedItemParentIndex = clickedItemIndex.parent();
    if (clickedItemParentIndex.isValid())
    {
        m_pTagItemContextMenu->addSeparator();

        ADD_CONTEXT_MENU_ACTION(tr("Promote"), m_pTagItemContextMenu,
                                onPromoteTagAction, pTagItem->localUid(),
                                canUpdate);
        ADD_CONTEXT_MENU_ACTION(tr("Remove from parent"), m_pTagItemContextMenu,
                                onRemoveFromParentTagAction, pTagItem->localUid(),
                                canUpdate);
    }

    if (clickedItemIndex.row() != 0)
    {
        QModelIndex siblingItemIndex =
            pTagModel->index(clickedItemIndex.row() - 1,
                             clickedItemIndex.column(),
                             clickedItemIndex.parent());
        bool canUpdateItemAndSibling =
            canUpdate && (pTagModel->flags(siblingItemIndex) & Qt::ItemIsEditable);
        ADD_CONTEXT_MENU_ACTION(tr("Demote"), m_pTagItemContextMenu,
                                onDemoteTagAction, pTagItem->localUid(),
                                canUpdateItemAndSibling);
    }

    QStringList tagNames = pTagModel->tagNames(pTagItem->linkedNotebookGuid());

    // 1) Remove the current tag's name from the list of possible new parents
    auto nameIt = std::lower_bound(tagNames.constBegin(),
                                   tagNames.constEnd(),
                                   pTagItem->name());
    if ((nameIt != tagNames.constEnd()) && (*nameIt == pTagItem->name())) {
        int pos = static_cast<int>(std::distance(tagNames.constBegin(), nameIt));
        tagNames.removeAt(pos);
    }

    // 2) Remove the current tag's parent tag's name from the list of possible
    // new parents
    if (clickedItemParentIndex.isValid())
    {
        const TagModelItem * pParentItem =
            pTagModel->itemForIndex(clickedItemParentIndex);
        if (Q_UNLIKELY(!pParentItem))
        {
            QNWARNING(QStringLiteral("Can't find parent tag model item for "
                                     "valid parent tag item index"));
        }
        else if (pParentItem->tagItem())
        {
            const QString & parentItemName = pParentItem->tagItem()->name();
            auto parentNameIt = std::lower_bound(tagNames.constBegin(),
                                                 tagNames.constEnd(),
                                                 parentItemName);
            if ((parentNameIt != tagNames.constEnd()) &&
                (*parentNameIt == parentItemName))
            {
                int pos = static_cast<int>(std::distance(tagNames.constBegin(),
                                                         parentNameIt));
                tagNames.removeAt(pos);
            }
        }
    }

    // 3) Remove the current tag's children names from the list of possible
    // new parents
    int numItemChildren = pModelItem->numChildren();
    for(int i = 0; i < numItemChildren; ++i)
    {
        const TagModelItem * pChildItem = pModelItem->childAtRow(i);
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING(QStringLiteral("Found null pointer to child tag model item"));
            continue;
        }

        const TagItem * pChildTagItem = pChildItem->tagItem();
        if (Q_UNLIKELY(!pChildTagItem)) {
            QNWARNING(QStringLiteral("Found a child tag model item under a tag ")
                      << QStringLiteral("item which contains no actual tag item: ")
                      << *pChildItem);
            continue;
        }

        auto childNameIt = std::lower_bound(tagNames.constBegin(),
                                            tagNames.constEnd(),
                                            pChildTagItem->name());
        if ((childNameIt != tagNames.constEnd()) &&
            (*childNameIt == pChildTagItem->name()))
        {
            int pos = static_cast<int>(std::distance(tagNames.constBegin(),
                                                     childNameIt));
            tagNames.removeAt(pos);
        }
    }

    if (Q_LIKELY(!tagNames.isEmpty()))
    {
        QMenu * pTargetParentSubMenu =
            m_pTagItemContextMenu->addMenu(tr("Move to parent"));
        for(auto it = tagNames.constBegin(),
            end = tagNames.constEnd(); it != end; ++it)
        {
            QStringList dataPair;
            dataPair.reserve(2);
            dataPair << pTagItem->localUid();
            dataPair << *it;
            ADD_CONTEXT_MENU_ACTION(*it, pTargetParentSubMenu,
                                    onMoveTagToParentAction,
                                    dataPair, canUpdate);
        }
    }

    if (pTagItem->isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(tr("Unfavorite"), m_pTagItemContextMenu,
                                onUnfavoriteAction, pTagItem->localUid(),
                                canUpdate);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(tr("Favorite"), m_pTagItemContextMenu,
                                onFavoriteAction, pTagItem->localUid(),
                                canUpdate);
    }

    m_pTagItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Clear selection"), m_pTagItemContextMenu,
                            onDeselectAction, QString(), true);

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."),
                            m_pTagItemContextMenu, onShowTagInfoAction,
                            pTagItem->localUid(), true);

    m_pTagItemContextMenu->show();
    m_pTagItemContextMenu->exec(pEvent->globalPos());
}

#undef ADD_CONTEXT_MENU_ACTION

void TagItemView::deleteItem(const QModelIndex & itemIndex, TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::deleteItem"));

    const TagModelItem * pItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't find the tag model item "
                                "meant to be deleted"))
        return;
    }

    int confirm = warningMessageBox(this, tr("Confirm the tag deletion"),
                                    tr("Are you sure you want to delete this tag?"),
                                    tr("Note that this action is not reversible "
                                       "and the deletion of the tag would mean "
                                       "its disappearance from all the notes "
                                       "using it!"),
                                    QMessageBox::Ok | QMessageBox::No);
    if (confirm != QMessageBox::Ok) {
        QNDEBUG(QStringLiteral("Tag deletion was not confirmed"));
        return;
    }

    bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
    if (res) {
        QNDEBUG(QStringLiteral("Successfully deleted tag"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The tag model refused to delete "
                                              "the tag; Check the status bar "
                                              "for message from the tag model "
                                              "explaining why the tag could not "
                                              "be deleted")))
}

void TagItemView::saveTagItemsState()
{
    QNDEBUG(QStringLiteral("TagItemView::saveTagItemsState"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QStringList expandedTagItemsLocalUids;
    QStringList expandedLinkedNotebookItemsGuids;

    QModelIndexList indexes = pTagModel->persistentIndexes();
    for(auto it = indexes.constBegin(),
        end = indexes.constEnd(); it != end; ++it)
    {
        const QModelIndex & index = *it;

        if (!isExpanded(index)) {
            continue;
        }

        const TagModelItem * pModelItem = pTagModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING(QStringLiteral("Tag model returned null pointer to tag "
                                     "model item for valid model index"));
            continue;
        }

        if ((pModelItem->type() == TagModelItem::Type::Tag) &&
            pModelItem->tagItem())
        {
            expandedTagItemsLocalUids << pModelItem->tagItem()->localUid();
            QNTRACE(QStringLiteral("Found expanded tag item: local uid = ")
                    << pModelItem->tagItem()->localUid());
        }
        else if ((pModelItem->type() == TagModelItem::Type::LinkedNotebook) &&
                 pModelItem->tagLinkedNotebookItem())
        {
            expandedLinkedNotebookItemsGuids
                << pModelItem->tagLinkedNotebookItem()->linkedNotebookGuid();
            QNTRACE(QStringLiteral("Found expanded tag linked notebook root ")
                    << QStringLiteral("item: linked notebook guid = ")
                    << pModelItem->tagLinkedNotebookItem()->linkedNotebookGuid());
        }
    }

    ApplicationSettings appSettings(pTagModel->account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("TagItemView"));
    appSettings.setValue(LAST_EXPANDED_TAG_ITEMS_KEY, expandedTagItemsLocalUids);
    appSettings.setValue(LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY,
                         expandedLinkedNotebookItemsGuids);
    appSettings.endGroup();
}

void TagItemView::restoreTagItemsState(const TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::restoreTagItemsState"));

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("TagItemView"));
    QStringList expandedTagItemsLocalUids =
        appSettings.value(LAST_EXPANDED_TAG_ITEMS_KEY).toStringList();
    QStringList expandedLinkedNotebookItemsGuids =
        appSettings.value(LAST_EXPANDED_LINKED_NOTEBOOK_ITEMS_KEY).toStringList();
    appSettings.endGroup();

    bool wasTrackingTagItemsState = m_trackingTagItemsState;
    m_trackingTagItemsState = false;

    setTagsExpanded(expandedTagItemsLocalUids, model);
    setLinkedNotebooksExpanded(expandedLinkedNotebookItemsGuids, model);

    m_trackingTagItemsState = wasTrackingTagItemsState;
}

void TagItemView::setTagsExpanded(const QStringList & tagLocalUids,
                                  const TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::setTagsExpanded: ") << tagLocalUids.size()
            << QStringLiteral(", tag local uids: ")
            << tagLocalUids.join(QStringLiteral(",")));

    for(auto it = tagLocalUids.constBegin(),
        end = tagLocalUids.constEnd(); it != end; ++it)
    {
        const QString & tagLocalUid = *it;
        QModelIndex index = model.indexForLocalUid(tagLocalUid);
        if (!index.isValid()) {
            continue;
        }

        setExpanded(index, true);
    }
}

void TagItemView::setLinkedNotebooksExpanded(const QStringList & linkedNotebookGuids,
                                             const TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::setLinkedNotebooksExpanded: ")
            << linkedNotebookGuids.size()
            << QStringLiteral(", linked notebook guids: ")
            << linkedNotebookGuids.join(QStringLiteral(", ")));

    for(auto it = linkedNotebookGuids.constBegin(),
        end = linkedNotebookGuids.constEnd(); it != end; ++it)
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

void TagItemView::restoreLastSavedSelection(const TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::restoreLastSavedSelection"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't restore the last selected tag: "
                                "no selection model in the view"))
        return;
    }

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("TagItemView"));
    QString lastSelectedTagLocalUid =
        appSettings.value(LAST_SELECTED_TAG_KEY).toString();
    appSettings.endGroup();

    if (lastSelectedTagLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("Found no last selected tag local uid"));
        return;
    }

    QNTRACE(QStringLiteral("Last selected tag local uid: ")
            << lastSelectedTagLocalUid);

    QModelIndex lastSelectedTagIndex =
        model.indexForLocalUid(lastSelectedTagLocalUid);
    if (!lastSelectedTagIndex.isValid()) {
        QNDEBUG(QStringLiteral("Tag model returned invalid index for the last "
                               "selected tag local uid"));
        return;
    }

    pSelectionModel->select(lastSelectedTagIndex,
                            QItemSelectionModel::ClearAndSelect |
                            QItemSelectionModel::Rows |
                            QItemSelectionModel::Current);
}

void TagItemView::selectionChangedImpl(const QItemSelection & selected,
                                       const QItemSelection & deselected)
{
    QNTRACE(QStringLiteral("TagItemView::selectionChangedImpl"));

    if (!m_trackingSelection) {
        QNTRACE(QStringLiteral("Not tracking selection at this time, skipping"));
        return;
    }

    Q_UNUSED(deselected)

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QModelIndexList selectedIndexes = selected.indexes();

    if (selectedIndexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The new selection is empty"));
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    QModelIndex sourceIndex = singleRow(selectedIndexes, *pTagModel,
                                        TagModel::Columns::Name);
    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        return;
    }

    const TagModelItem * pModelItem = pTagModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't find the tag model item "
                                "corresponding to the selected index"))
        return;
    }

    if (Q_UNLIKELY(pModelItem->type() != TagModelItem::Type::Tag)) {
        QNDEBUG(QStringLiteral("The selected tag model item is not of a tag type"));
        return;
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: the tag model item corresponding "
                                "to the selected index contains no tag item even "
                                "though it is of a tag type"))
        return;
    }

    QNTRACE(QStringLiteral("Currently selected tag item: ") << *pTagItem);

    ApplicationSettings appSettings(pTagModel->account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("TagItemView"));
    appSettings.setValue(LAST_SELECTED_TAG_KEY, pTagItem->localUid());
    appSettings.endGroup();

    QNDEBUG(QStringLiteral("Persisted the currently selected tag local uid: ")
            << pTagItem->localUid());
}

void TagItemView::setFavoritedFlag(const QAction & action, const bool favorited)
{
    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QString itemLocalUid = action.data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't set the favorited flag "
                                "for the tag, can't get tag's local uid from "
                                "QAction"))
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't set the favorited flag "
                                "for the tag, the model returned invalid index "
                                "for the tag's local uid"))
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
        QNDEBUG(QStringLiteral("The model is not ready yet"));
        return;
    }

    saveTagItemsState();
    m_trackingSelection = false;
    m_trackingTagItemsState = false;
}

void TagItemView::postProcessTagModelChange()
{
    if (!m_modelReady) {
        QNDEBUG(QStringLiteral("The model is not ready yet"));
        return;
    }

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    restoreTagItemsState(*pTagModel);
    m_trackingTagItemsState = true;

    restoreLastSavedSelection(*pTagModel);
    m_trackingSelection = true;
}

} // namespace quentier
