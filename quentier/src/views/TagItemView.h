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

#ifndef QUENTIER_VIEWS_TAG_ITEM_VIEW_H
#define QUENTIER_VIEWS_TAG_ITEM_VIEW_H

#include "ItemView.h"
#include <quentier/utility/QNLocalizedString.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)

class TagItemView: public ItemView
{
    Q_OBJECT
public:
    TagItemView(QWidget * parent = Q_NULLPTR);

    virtual void setModel(QAbstractItemModel * pModel) Q_DECL_OVERRIDE;

    /**
     * @return valid model index if the selection exists and contains exactly one row and invalid model index otherwise
     */
    QModelIndex currentlySelectedItemIndex() const;

Q_SIGNALS:
    void notifyError(QNLocalizedString error);
    void newTagCreationRequested();
    void tagInfoRequested();

private Q_SLOTS:
    void onAllTagsListed();

    void onCreateNewTagAction();
    void onRenameTagAction();
    void onDeleteTagAction();

    void onEditTagAction();
    void onPromoteTagAction();
    void onShowTagInfoAction();

    void onTagItemCollapsedOrExpanded(const QModelIndex & index);



    virtual void selectionChanged(const QItemSelection & selected,
                                  const QItemSelection & deselected) Q_DECL_OVERRIDE;

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void deleteItem(const QModelIndex & itemIndex, TagModel & model);

    void saveTagItemsState();
    void restoreTagItemsState(const TagModel & model);
    void setTagsExpanded(const QStringList & tagLocalUids, const TagModel & model);

    void restoreLastSavedSelection(const TagModel & model);

private:
    QMenu *     m_pTagItemContextMenu;
    bool        m_restoringTagItemsState;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_TAG_ITEM_VIEW_H
