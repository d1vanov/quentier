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

#include "TagItemView.h"
#include "AccountToKey.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScopedPointer>

#ifndef QT_MOC_RUN
#include <boost/scope_exit.hpp>
#endif

#define LAST_SELECTED_TAG_KEY QStringLiteral("_LastSelectedTagLocalUid")
#define LAST_EXPANDED_TAG_ITEMS_KEY QStringLiteral("_LastExpandedTagLocalUids")

#define REPORT_ERROR(error) \
    { \
        QNLocalizedString errorDescription = QNLocalizedString(error, this); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

TagItemView::TagItemView(QWidget * parent) :
    ItemView(parent),
    m_pTagItemContextMenu(Q_NULLPTR),
    m_restoringTagItemsState(false)
{
    QObject::connect(this, QNSIGNAL(TagItemView,expanded,const QModelIndex&),
                     this, QNSLOT(TagItemView,onTagItemCollapsedOrExpanded,const QModelIndex&));
    QObject::connect(this, QNSIGNAL(TagItemView,collapsed,const QModelIndex&),
                     this, QNSLOT(TagItemView,onTagItemCollapsedOrExpanded,const QModelIndex&));
}

void TagItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("TagItemView::setModel"));

    TagModel * pPreviousModel = qobject_cast<TagModel*>(model());
    if (pPreviousModel) {
        QObject::disconnect(pPreviousModel, QNSIGNAL(TagModel,notifyError,QNLocalizedString),
                            this, QNSIGNAL(TagItemView,notifyError,QNLocalizedString));
    }

    TagModel * pTagModel = qobject_cast<TagModel*>(pModel);
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model has been set to the tag view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pTagModel, QNSIGNAL(TagModel,notifyError,QNLocalizedString),
                     this, QNSIGNAL(TagItemView,notifyError,QNLocalizedString));

    ItemView::setModel(pModel);

    if (pTagModel->allTagsListed()) {
        QNDEBUG(QStringLiteral("All tags are already listed within the model"));
        restoreTagItemsState(*pTagModel);
        restoreLastSavedSelection(*pTagModel);
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

void TagItemView::onAllTagsListed()
{
    QNDEBUG(QStringLiteral("TagItemView::onAllTagsListed"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        REPORT_ERROR("Can't cast the model set to the tag item view "
                     "to the tag model");
        return;
    }

    QObject::disconnect(pTagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                        this, QNSLOT(TagItemView,onAllTagsListed));

    restoreTagItemsState(*pTagModel);
    restoreLastSavedSelection(*pTagModel);
}

void TagItemView::onCreateNewTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onCreateNewTagAction"));
    emit newTagCreationRequested();
}

void TagItemView::onRenameTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onRenameTagAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't rename tag, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't rename tag, "
                     "can't get tag's local uid from QAction")
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't rename tag: the model returned invalid "
                     "index for the tag's local uid");
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
        REPORT_ERROR("Internal error: can't delete tag, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't delete tag, "
                     "can't get tag's local uid from QAction")
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't delete tag: the model returned invalid "
                     "index for the tag's local uid");
        return;
    }

    deleteItem(itemIndex, *pTagModel);
}

void TagItemView::onEditTagAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onEditTagAction"));

    // TODO: implement
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
        REPORT_ERROR("Internal error: can't promote tag, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't promote tag, "
                     "can't get tag's local uid from QAction")
        return;
    }

    QModelIndex itemIndex = pTagModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't promote tag: the model returned invalid "
                     "index for the tag's local uid");
        return;
    }

    QModelIndex promotedItemIndex = pTagModel->promote(itemIndex);
    if (promotedItemIndex.isValid()) {
        QNDEBUG(QStringLiteral("Successfully promoted the tag"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The tag model refused to promote the tag; "
                                              "Check the status bar for message from the tag model "
                                              "explaining why the tag could not be promoted")))
}

void TagItemView::onShowTagInfoAction()
{
    QNDEBUG(QStringLiteral("TagItemView::onShowTagInfoAction"));
    emit tagInfoRequested();
}

void TagItemView::onTagItemCollapsedOrExpanded(const QModelIndex & index)
{
    QNTRACE(QStringLiteral("TagItemView::onTagItemCollapsedOrExpanded: index: valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row() << QStringLiteral(", column = ") << index.column()
            << QStringLiteral(", parent: valid = ") << (index.parent().isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.parent().row() << QStringLiteral(", column = ") << index.parent().column());

    if (m_restoringTagItemsState) {
        QNDEBUG(QStringLiteral("Ignoring the event as the tag items are being restored currently"));
        return;
    }

    saveTagItemsState();
}

void TagItemView::selectionChanged(const QItemSelection & selected,
                                   const QItemSelection & deselected)
{
    QNDEBUG(QStringLiteral("TagItemView::selectionChanged"));

    BOOST_SCOPE_EXIT(this_, &selected, &deselected) {
        this_->ItemView::selectionChanged(selected, deselected);
    } BOOST_SCOPE_EXIT_END

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
    QModelIndex sourceIndex = singleRow(selectedIndexes, *pTagModel, TagModel::Columns::Name);
    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        return;
    }

    const TagModelItem * pTagItem = pTagModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR("Internal error: can't find the tag model item corresponding "
                     "to the selected index");
        return;
    }

    QNTRACE(QStringLiteral("Currently selected tag item: ") << *pTagItem);

    QString accountKey = accountToKey(pTagModel->account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(pTagModel->account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/TagItemView"));
    appSettings.setValue(LAST_SELECTED_TAG_KEY, pTagItem->localUid());
    appSettings.endGroup();

    QNDEBUG(QStringLiteral("Persisted the currently selected tag local uid: ")
            << pTagItem->localUid());
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
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    const TagModelItem * pItem = pTagModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR("Can't show the context menu for the tag model item: "
                     "no item corresponding to the clicked item's index");
        return;
    }

    delete m_pTagItemContextMenu;
    m_pTagItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(TagItemView,slot)); \
        menu->addAction(pAction); \
    }

    ADD_CONTEXT_MENU_ACTION(tr("Create new tag") + QStringLiteral("..."),
                            m_pTagItemContextMenu, onCreateNewTagAction,
                            pItem->localUid(), true);

    m_pTagItemContextMenu->addSeparator();

    bool canUpdate = (pTagModel->flags(clickedItemIndex) & Qt::ItemIsEditable);
    ADD_CONTEXT_MENU_ACTION(tr("Rename"), m_pTagItemContextMenu,
                            onRenameTagAction, pItem->localUid(), canUpdate);

    ADD_CONTEXT_MENU_ACTION(tr("Delete"), m_pTagItemContextMenu,
                            onDeleteTagAction, pItem->localUid(),
                            !pItem->isSynchronizable());

    ADD_CONTEXT_MENU_ACTION(tr("Edit") + QStringLiteral("..."),
                            m_pTagItemContextMenu, onEditTagAction,
                            pItem->localUid(), canUpdate);

    if (clickedItemIndex.parent().isValid()) {
        ADD_CONTEXT_MENU_ACTION(tr("Promote"), m_pTagItemContextMenu,
                                onPromoteTagAction, pItem->localUid(),
                                canUpdate);
    }

    m_pTagItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pTagItemContextMenu,
                            onShowTagInfoAction, pItem->localUid(), true);

    m_pTagItemContextMenu->show();
    m_pTagItemContextMenu->exec(pEvent->globalPos());
}

#undef ADD_CONTEXT_MENU_ACTION

void TagItemView::deleteItem(const QModelIndex & itemIndex, TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::deleteItem"));

    const TagModelItem * pItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR("Internal error: can't find the tag model item meant to be deleted");
        return;
    }

    int confirm = warningMessageBox(this, tr("Confirm the tag deletion"),
                                    tr("Are you sure you want to delete this tag?"),
                                    tr("Note that this action is not reversible and the deletion "
                                       "of the tag would mean its disappearance from all the notes "
                                       "using it!"), QMessageBox::Ok | QMessageBox::No);
    if (confirm != QMessageBox::Ok) {
        QNDEBUG(QStringLiteral("Tag deletion was not confirmed"));
        return;
    }

    bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
    if (res) {
        QNDEBUG(QStringLiteral("Successfully deleted tag"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The tag model refused to delete the tag; "
                                              "Check the status bar for message from the tag model "
                                              "explaining why the tag could not be deleted")))
}

void TagItemView::saveTagItemsState()
{
    QNDEBUG(QStringLiteral("TagItemView::saveTagItemsState"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QString accountKey = accountToKey(pTagModel->account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(pTagModel->account());
        return;
    }

    QStringList result;

    QModelIndexList indexes = pTagModel->persistentIndexes();
    for(auto it = indexes.constBegin(), end = indexes.constEnd(); it != end; ++it)
    {
        const QModelIndex & index = *it;

        if (!isExpanded(index)) {
            continue;
        }

        const TagModelItem * pItem = pTagModel->itemForIndex(index);
        if (Q_UNLIKELY(pItem)) {
            continue;
        }

        result << pItem->localUid();
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/TagItemView"));
    appSettings.setValue(LAST_EXPANDED_TAG_ITEMS_KEY, result);
    appSettings.endGroup();
}

void TagItemView::restoreTagItemsState(const TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::restoreTagItemsState"));

    QString accountKey = accountToKey(model.account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(model.account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/TagItemView"));
    QStringList expandedTagLocalUids = appSettings.value(LAST_EXPANDED_TAG_ITEMS_KEY).toStringList();
    appSettings.endGroup();

    m_restoringTagItemsState = true;
    setTagsExpanded(expandedTagLocalUids, model);
    m_restoringTagItemsState = false;
}

void TagItemView::setTagsExpanded(const QStringList & tagLocalUids, const TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::setTagsExpanded"));

    for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it)
    {
        const QString & tagLocalUid = *it;
        QModelIndex index = model.indexForLocalUid(tagLocalUid);
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
        REPORT_ERROR("Can't restore the last selected tag: no selection model in the view");
        return;
    }

    QString accountKey = accountToKey(model.account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(model.account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/TagItemView"));
    QString lastSelectedTagLocalUid = appSettings.value(LAST_SELECTED_TAG_KEY).toString();
    appSettings.endGroup();

    if (lastSelectedTagLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("Found no last selected tag local uid"));
        return;
    }

    QNTRACE(QStringLiteral("Last selected tag local uid: ") << lastSelectedTagLocalUid);

    QModelIndex lastSelectedTagIndex = model.indexForLocalUid(lastSelectedTagLocalUid);
    if (!lastSelectedTagIndex.isValid()) {
        QNDEBUG(QStringLiteral("Tag model returned invalid index for the sast selected tag local uid"));
        return;
    }

    QModelIndex left = model.index(lastSelectedTagIndex.row(), TagModel::Columns::Name,
                                   lastSelectedTagIndex.parent());
    QModelIndex right = model.index(lastSelectedTagIndex.row(), TagModel::Columns::NumNotesPerTag,
                                    lastSelectedTagIndex.parent());
    QItemSelection selection(left, right);
    pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
}

} // namespace quentier
