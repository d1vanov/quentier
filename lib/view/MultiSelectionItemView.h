/*
 * Copyright 2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_VIEW_MULTI_SELECTION_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_MULTI_SELECTION_ITEM_VIEW_H

#include "ItemView.h"

#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(ItemModel)
QT_FORWARD_DECLARE_CLASS(NoteFiltersManager)

/**
 * @brief The MultiSelectionItemView class is an abstract base class for tree
 * views supporting selection of multiple items simultaneously. Subclasses of
 * MultiSelectionItemView are intended to be used along with ItemModel
 * subclasses.
 */
class MultiSelectionItemView : public ItemView
{
    Q_OBJECT
public:
    explicit MultiSelectionItemView(
        const QString & modelTypeName, QWidget * parent = nullptr);

    virtual ~MultiSelectionItemView() override;

    void setNoteFiltersManager(NoteFiltersManager & noteFiltersManager);

protected:
    /**
     * @brief saveItemsState method should persist expanded/collapsed states
     * of model items so that they can be restored later when needed
     */
    virtual void saveItemsState() = 0;

    virtual bool shouldFilterBySelectedItems(const Account & account) const = 0;

    virtual const QStringList & localUidsFromNoteFiltersManager(
        const NoteFiltersManager & noteFiltersManager) const = 0;

private Q_SLOTS:
    void onItemCollapsedOrExpanded(const QModelIndex & index);
    void onNoteFilterChanged();

private:
    void disconnectFromNoteFiltersManagerFilterChanged();
    void connectToNoteFiltersManagerFilterChanged();

    void selectAllItemsRootItem(const ItemModel & model);

private:
    const QString   m_modelTypeName;

    QPointer<NoteFiltersManager>    m_pNoteFiltersManager;

    bool            m_trackingItemsState = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_MULTI_SELECTION_ITEM_VIEW_H
