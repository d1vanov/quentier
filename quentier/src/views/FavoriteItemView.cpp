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

#include "FavoriteItemView.h"
#include "../AccountToKey.h"
#include "../models/FavoritesModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScopedPointer>
#include <set>

#define LAST_SELECTED_FAVORITED_ITEM_KEY QStringLiteral("_LastSelectedFavoritedItemLocalUid")

#define REPORT_ERROR(error) \
    { \
        ErrorString errorDescription(error); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

FavoriteItemView::FavoriteItemView(QWidget * parent) :
    ItemView(parent),
    m_pFavoriteItemContextMenu(Q_NULLPTR),
    m_trackingSelection(false),
    m_modelReady(false)
{}

void FavoriteItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("FavoriteItemView::setModel"));

    FavoritesModel * pPreviousModel = qobject_cast<FavoritesModel*>(model());
    if (pPreviousModel)
    {
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,notifyError,ErrorString),
                            this, QNSIGNAL(FavoriteItemView,notifyError,ErrorString));
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,notifyAllItemsListed),
                            this, QNSLOT(FavoriteItemView,onAllItemsListed));
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,aboutToAddItem),
                            this, QNSLOT(FavoriteItemView,onAboutToAddItem));
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,addedItem,const QModelIndex&),
                            this, QNSLOT(FavoriteItemView,onAddedItem,const QModelIndex&));
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,aboutToUpdateItem,const QModelIndex&),
                            this, QNSLOT(FavoriteItemView,onAboutToUpdateItem,const QModelIndex&));
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,updatedItem,const QModelIndex&),
                            this, QNSLOT(FavoriteItemView,onUpdatedItem,const QModelIndex&));
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,aboutToRemoveItems),
                            this, QNSLOT(FavoriteItemView,onAboutToRemoveItems));
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,removedItems),
                            this, QNSLOT(FavoriteItemView,onRemovedItems));
    }

    m_modelReady = false;
    m_trackingSelection = false;

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(pModel);
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorites model has been set to the favorites item view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,notifyError,ErrorString),
                     this, QNSIGNAL(FavoriteItemView,notifyError,ErrorString));
    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,aboutToAddItem),
                     this, QNSLOT(FavoriteItemView,onAboutToAddItem));
    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,addedItem,const QModelIndex&),
                     this, QNSLOT(FavoriteItemView,onAddedItem,const QModelIndex&));
    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,aboutToUpdateItem,const QModelIndex&),
                     this, QNSLOT(FavoriteItemView,onAboutToUpdateItem,const QModelIndex&));
    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,updatedItem,const QModelIndex&),
                     this, QNSLOT(FavoriteItemView,onUpdatedItem,const QModelIndex&));
    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,aboutToRemoveItems),
                     this, QNSLOT(FavoriteItemView,onAboutToRemoveItems));
    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,removedItems),
                     this, QNSLOT(FavoriteItemView,onRemovedItems));

    ItemView::setModel(pModel);

    if (pFavoritesModel->allItemsListed()) {
        QNDEBUG(QStringLiteral("All favorites model's items are already listed within the model"));
        restoreLastSavedSelection(*pFavoritesModel);
        m_modelReady = true;
        m_trackingSelection = true;
        return;
    }

    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,notifyAllItemsListed),
                     this, QNSLOT(FavoriteItemView,onAllItemsListed));
}

QModelIndex FavoriteItemView::currentlySelectedItemIndex() const
{
    QNDEBUG(QStringLiteral("FavoriteItemView::currentlySelectedItemIndex"));

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorites model has been set to the favorites item view"));
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

    return singleRow(indexes, *pFavoritesModel, FavoritesModel::Columns::DisplayName);
}

void FavoriteItemView::deleteSelectedItems()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::deleteSelectedItems"));

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorites model has been set to the favorites item view"));
        return;
    }

    QModelIndexList indexes = selectedIndexes();
    std::set<int> rows;
    for(int i = 0, size = indexes.size(); i < size; ++i)
    {
        const QModelIndex & currentIndex = indexes.at(i);
        if (Q_UNLIKELY(!currentIndex.isValid())) {
            continue;
        }

        int row = currentIndex.row();
        auto it = rows.find(row);
        if (it != rows.end()) {
            continue;
        }

        Q_UNUSED(rows.insert(row))
    }

    if (rows.empty()) {
        QNDEBUG(QStringLiteral("No favorited items are selected, nothing to unfavorite"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot unfavorite the favorited items"),
                                       tr("No favorited items are selected currently"),
                                       tr("Please select the favorited items you want to unfavorite")))
        return;
    }

    int numRows = static_cast<int>(rows.size());
    bool res = pFavoritesModel->removeRows(*(rows.begin()), numRows);
    if (res) {
        QNDEBUG(QStringLiteral("Successfully unfavorited the selected items"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The favorited items model refused to unfavorite the selected item(s); "
                                              "Check the status bar for message from the favorites model "
                                              "explaining why the items could not be unfavorited")))
}

void FavoriteItemView::onAllItemsListed()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onAllItemsListed"));

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't cast the model set to the favorites item view "
                                       "to the favorites model"))
        return;
    }

    QObject::disconnect(pFavoritesModel, QNSIGNAL(FavoritesModel,notifyAllItemsListed),
                        this, QNSLOT(FavoriteItemView,onAllItemsListed));

    restoreLastSavedSelection(*pFavoritesModel);
    m_modelReady = true;
    m_trackingSelection = true;
}

void FavoriteItemView::onAboutToAddItem()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onAboutToAddItem"));
    prepareForFavoritesModelChange();
}

void FavoriteItemView::onAddedItem(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onAddedItem"));

    Q_UNUSED(index)
    postProcessFavoritedModelChange();
}

void FavoriteItemView::onAboutToUpdateItem(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onAboutToUpdateItem"));

    Q_UNUSED(index)
    prepareForFavoritesModelChange();
}

void FavoriteItemView::onUpdatedItem(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onUpdatedItem"));

    Q_UNUSED(index)
    postProcessFavoritedModelChange();
}

void FavoriteItemView::onAboutToRemoveItems()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onAboutToRemoveItems"));
    prepareForFavoritesModelChange();
}

void FavoriteItemView::onRemovedItems()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onRemovedItems"));
    postProcessFavoritedModelChange();
}

void FavoriteItemView::onRenameFavoritedItemAction()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onRenameFavoritedItemAction"));

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorites model has been set to the favorites item view"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't rename favorited item, "
                                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't rename favorited item, "
                                       "can't get favorited item's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pFavoritesModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't rename favorited item: the model returned invalid "
                                       "index for the favorited item's local uid"))
        return;
    }

    edit(itemIndex);
}

void FavoriteItemView::onUnfavoriteItemAction()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onUnfavoriteItemAction"));

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorites model has been set to the favorites item view"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't unfavorite item, "
                                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't unfavorite item, "
                                       "can't get favorited item's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pFavoritesModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't unfavorite item: the model returned invalid "
                                       "index for the favorited item's local uid"))
        return;
    }

    Q_UNUSED(pFavoritesModel->removeRow(itemIndex.row(), itemIndex.parent()))
}

void FavoriteItemView::onDeselectAction()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onDeselectAction"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't clear the favorited items selection: no selection model in the view"))
        return;
    }

    pSelectionModel->clearSelection();
}

void FavoriteItemView::onShowFavoritedItemInfoAction()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onShowFavoritedItemInfoAction"));
    emit favoritedItemInfoRequested();
}

void FavoriteItemView::selectionChanged(const QItemSelection & selected,
                                        const QItemSelection & deselected)
{
    QNDEBUG(QStringLiteral("FavoriteItemView::selectionChanged"));

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void FavoriteItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("FavoriteItemView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: favorite item view received "
                                 "context menu event with null pointer "
                                 "to the context menu event"));
        return;
    }

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorites model has been set to the favorites item view"));
        return;
    }

    QModelIndex clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    const FavoritesModelItem * pItem = pFavoritesModel->itemAtRow(clickedItemIndex.row());
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't show the context menu for the favorites model item: "
                                       "no item corresponding to the clicked item's index"))
        return;
    }

    delete m_pFavoriteItemContextMenu;
    m_pFavoriteItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(FavoriteItemView,slot)); \
        menu->addAction(pAction); \
    }

    bool canUpdate = (pFavoritesModel->flags(clickedItemIndex) & Qt::ItemIsEditable);
    ADD_CONTEXT_MENU_ACTION(tr("Rename"), m_pFavoriteItemContextMenu,
                            onRenameFavoritedItemAction, pItem->localUid(), canUpdate);
    ADD_CONTEXT_MENU_ACTION(tr("Unfavorite"), m_pFavoriteItemContextMenu,
                            onUnfavoriteItemAction, pItem->localUid(), true);

    m_pFavoriteItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Clear selection"), m_pFavoriteItemContextMenu,
                            onDeselectAction, QString(), true);

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pFavoriteItemContextMenu,
                            onShowFavoritedItemInfoAction, pItem->localUid(), true);

    m_pFavoriteItemContextMenu->show();
    m_pFavoriteItemContextMenu->exec(pEvent->globalPos());
}

void FavoriteItemView::restoreLastSavedSelection(const FavoritesModel & model)
{
    QNDEBUG(QStringLiteral("FavoriteItemView::restoreLastSavedSelection"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't restore the last selected favorited model item: no selection model in the view"))
        return;
    }

    QString accountKey = accountToKey(model.account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't create application settings key from account"))
        QNWARNING(model.account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/FavoriteItemView"));
    QString lastSelectedItemLocalUid = appSettings.value(LAST_SELECTED_FAVORITED_ITEM_KEY).toString();
    appSettings.endGroup();

    if (lastSelectedItemLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("Found no last selected favorited item local uid"));
        return;
    }

    QNTRACE(QStringLiteral("Last selected item local uid: ") << lastSelectedItemLocalUid);

    QModelIndex lastSelectedItemIndex = model.indexForLocalUid(lastSelectedItemLocalUid);
    if (!lastSelectedItemIndex.isValid()) {
        QNDEBUG(QStringLiteral("Favorites model returned invalid index for the last selected favorited item local uid"));
        return;
    }

    pSelectionModel->select(lastSelectedItemIndex, QItemSelectionModel::ClearAndSelect |
                                                   QItemSelectionModel::Rows |
                                                   QItemSelectionModel::Current);
}

void FavoriteItemView::selectionChangedImpl(const QItemSelection & selected,
                                            const QItemSelection & deselected)
{
    QNTRACE(QStringLiteral("FavoriteItemView::selectionChangedImpl"));

    if (!m_trackingSelection) {
        QNTRACE(QStringLiteral("Not tracking selection at this time, skipping"));
        return;
    }

    Q_UNUSED(deselected)

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorited items model is used"));
        return;
    }

    QModelIndexList selectedIndexes = selected.indexes();

    if (selectedIndexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The new selection is empty"));
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    QModelIndex sourceIndex = singleRow(selectedIndexes, *pFavoritesModel, FavoritesModel::Columns::DisplayName);
    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        return;
    }

    const FavoritesModelItem * pItem = pFavoritesModel->itemAtRow(sourceIndex.row());
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't find the favorited model item corresponding to the selected index"))
        return;
    }

    QString accountKey = accountToKey(pFavoritesModel->account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't create application settings key from account"))
        QNWARNING(pFavoritesModel->account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/FavoriteItemView"));
    appSettings.setValue(LAST_SELECTED_FAVORITED_ITEM_KEY, pItem->localUid());
    appSettings.endGroup();

    QNDEBUG(QStringLiteral("Persisted the currently selected favorited item's local uid: ")
            << pItem->localUid());
}

void FavoriteItemView::prepareForFavoritesModelChange()
{
    if (!m_modelReady) {
        QNDEBUG(QStringLiteral("The model is not ready yet"));
        return;
    }

    m_trackingSelection = false;
}

void FavoriteItemView::postProcessFavoritedModelChange()
{
    if (!m_modelReady) {
        QNDEBUG(QStringLiteral("The model is not ready yet"));
        return;
    }

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorited items model is used"));
        return;
    }

    restoreLastSavedSelection(*pFavoritesModel);
    m_trackingSelection = true;
}

} // namespace quentier
