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

#define LAST_SELECTED_FAVORITED_ITEM_KEY QStringLiteral("_LastSelectedFavoritedItemLocalUid")

#define REPORT_ERROR(error) \
    { \
        QNLocalizedString errorDescription = QNLocalizedString(error, this); \
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
        QObject::disconnect(pPreviousModel, QNSIGNAL(FavoritesModel,notifyError,QNLocalizedString),
                            this, QNSIGNAL(FavoriteItemView,notifyError,QNLocalizedString));
        // TODO; disconnect from other signals
    }

    m_modelReady = false;
    m_trackingSelection = false;

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(pModel);
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(QStringLiteral("Non-favorites model has been set to the favorites item view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,notifyError,QNLocalizedString),
                     this, QNSIGNAL(FavoriteItemView,notifyError,QNLocalizedString));
    // TODO: connect to other signals

    ItemView::setModel(pModel);

    if (pFavoritesModel->allItemsListed()) {
        QNDEBUG(QStringLiteral("All favorites model's items are already listed within the model"));
        // TODO: restore last saved selection
        m_modelReady = true;
        m_trackingSelection = true;
        return;
    }

    QObject::connect(pFavoritesModel, QNSIGNAL(FavoritesModel,notifyAllItemsListed),
                     this, QNSLOT(FavoriteItemView,onAllItemsListed));
}

void FavoriteItemView::onAllItemsListed()
{
    QNDEBUG(QStringLiteral("FavoriteItemView::onAllItemsListed"));

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        REPORT_ERROR("Can't cast the model set to the favorites item view "
                     "to the favorites model");
        return;
    }

    QObject::disconnect(pFavoritesModel, QNSIGNAL(FavoritesModel,notifyAllItemsListed),
                        this, QNSLOT(FavoriteItemView,onAllItemsListed));

    // TODO: restore the last saved selection
    m_modelReady = true;
    m_trackingSelection = true;
}

} // namespace quentier
