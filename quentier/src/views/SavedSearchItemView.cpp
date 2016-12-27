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

#include "SavedSearchItemView.h"
#include "AccountToKey.h"
#include "../models/SavedSearchModel.h"
#include "../dialogs/AddOrEditSavedSearchDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScopedPointer>

#define LAST_SELECTED_SAVED_SEARCH_KEY QStringLiteral("_LastSelectedSavedSearchLocalUid")

#define REPORT_ERROR(error) \
    { \
        QNLocalizedString errorDescription = QNLocalizedString(error, this); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

SavedSearchItemView::SavedSearchItemView(QWidget * parent) :
    ItemView(parent),
    m_pSavedSearchItemContextMenu(Q_NULLPTR),
    m_trackingSelection(false),
    m_modelReady(false)
{}

void SavedSearchItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::setModel"));

    SavedSearchModel * pPreviousModel = qobject_cast<SavedSearchModel*>(model());
    if (pPreviousModel)
    {
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,notifyError,QNLocalizedString),
                            this, QNSIGNAL(SavedSearchItemView,notifyError,QNLocalizedString));
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,notifyAllSavedSearchesListed),
                            this, QNSLOT(SavedSearchItemView,onAllSavedSearchesListed));
        // TODO: disconnect from other signals/slots
    }

    m_modelReady = false;
    m_trackingSelection = false;

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(pModel);
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model has been set to the saved search item view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,notifyError,QNLocalizedString),
                     this, QNSIGNAL(SavedSearchItemView,notifyError,QNLocalizedString));
    // TODO: connect to other signlals from the model

    ItemView::setModel(pModel);

    if (pSavedSearchModel->allSavedSearchesListed()) {
        QNDEBUG(QStringLiteral("All saved searches are already listed within the model"));
        // TODO: restore the last saved selection
        m_modelReady = true;
        m_trackingSelection = true;
        return;
    }

    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,notifyAllSavedSearchesListed),
                     this, QNSLOT(SavedSearchItemView,onAllSavedSearchesListed));
}

QModelIndex SavedSearchItemView::currentlySelectedItemIndex() const
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::currentlySelectedItemIndex"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
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

    return singleRow(indexes, *pSavedSearchModel, SavedSearchModel::Columns::Name);
}

void SavedSearchItemView::deleteSelectedItem()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::deleteSelectedItem"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(QStringLiteral("No saved searches are selected, nothing to deete"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current saved search"),
                                       tr("No saved search is selected currently"),
                                       tr("Please select the saved seaarch you want to delete")))
        return;
    }

    QModelIndex index = singleRow(indexes, *pSavedSearchModel, SavedSearchModel::Columns::Name);
    if (!index.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one saved search within the selection"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current saved search"),
                                       tr("More than one saved search is currently selected"),
                                       tr("Please select only the saved search you want to delete")))
        return;
    }

    deleteItem(index, *pSavedSearchModel);
}

void SavedSearchItemView::onAllSavedSearchesListed()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onAllSavedSearchesListed"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        REPORT_ERROR("Can't cast the model set to the saved search item view "
                     "to the saved search model");
        return;
    }

    QObject::disconnect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,notifyAllSavedSearchesListed),
                        this, QNSLOT(SavedSearchItemView,onAllSavedSearchesListed));

    // TODO: restore the last saved selection
    m_modelReady = true;
    m_trackingSelection = true;
}

void SavedSearchItemView::deleteItem(const QModelIndex & itemIndex, SavedSearchModel & model)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::deleteItem"));

    const SavedSearchModelItem * pItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR("Internal error: can't find the saved search model item meant to be deleted");
        return;
    }

    int confirm = warningMessageBox(this, tr("Confirm the saved search deletion"),
                                    tr("Are you sure you want to delete this saved search?"),
                                    tr("Note that this action is not reversible!"),
                                    QMessageBox::Ok | QMessageBox::No);
    if (confirm != QMessageBox::Ok) {
        QNDEBUG(QStringLiteral("Saved search deletion was not confirmed"));
        return;
    }

    bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
    if (res) {
        QNDEBUG(QStringLiteral("Successfully deleted saved search"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The saved search model refused to delete the saved search; "
                                              "Check the status bar for message from the saved search model "
                                              "explaining why the saved search could not be deleted")))
}

} // namespace quentier
