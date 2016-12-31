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

#ifndef QUENTIER_VIEWS_FAVORITE_ITEM_VIEW_H
#define QUENTIER_VIEWS_FAVORITE_ITEM_VIEW_H

#include "ItemView.h"
#include <quentier/utility/QNLocalizedString.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FavoritesModel)

class FavoriteItemView: public ItemView
{
    Q_OBJECT
public:
    explicit FavoriteItemView(QWidget * parent = Q_NULLPTR);

    virtual void setModel(QAbstractItemModel * pModel) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(QNLocalizedString error);
    void favoritedItemInfoRequested();

private Q_SLOTS:
    void onAllItemsListed();

private:
    QMenu *     m_pFavoriteItemContextMenu;
    bool        m_trackingSelection;
    bool        m_modelReady;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_FAVORITE_ITEM_VIEW_H
