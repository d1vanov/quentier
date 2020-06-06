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

#ifndef QUENTIER_LIB_VIEW_TAG_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_TAG_ITEM_VIEW_H

#include "ItemView.h"

#include <quentier/types/ErrorString.h>

#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(NoteFiltersManager)
QT_FORWARD_DECLARE_CLASS(TagModel)

class TagItemView: public ItemView
{
    Q_OBJECT
public:
    explicit TagItemView(QWidget * parent = nullptr);

    void setNoteFiltersManager(NoteFiltersManager & noteFiltersManager);

    virtual void setModel(QAbstractItemModel * pModel) override;

    /**
     * @return      Valid model index if the selection exists and contains
     *              exactly one row and invalid model index otherwise
     */
    QModelIndex currentlySelectedItemIndex() const;

Q_SIGNALS:
    void notifyError(ErrorString error);
    void newTagCreationRequested();
    void tagInfoRequested();

public Q_SLOTS:
    void deleteSelectedItem();

private Q_SLOTS:
    void onAllTagsListed();

    void onAboutToAddTag();
    void onAddedTag(const QModelIndex & index);

    void onAboutToUpdateTag(const QModelIndex & index);
    void onUpdatedTag(const QModelIndex & index);

    void onAboutToRemoveTags();
    void onRemovedTags();

    void onCreateNewTagAction();
    void onRenameTagAction();
    void onDeleteTagAction();

    void onEditTagAction();
    void onPromoteTagAction();
    void onDemoteTagAction();
    void onRemoveFromParentTagAction();
    void onMoveTagToParentAction();
    void onShowTagInfoAction();
    void onDeselectAction();

    void onFavoriteAction();
    void onUnfavoriteAction();

    void onTagItemCollapsedOrExpanded(const QModelIndex & index);
    void onTagParentChanged(const QModelIndex & index);

    void onNoteFilterChanged();

    void onNoteFiltersManagerReady();

    virtual void selectionChanged(
        const QItemSelection & selected,
        const QItemSelection & deselected) override;

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    void deleteItem(const QModelIndex & itemIndex, TagModel & model);

    void saveTagItemsState();
    void restoreTagItemsState(const TagModel & model);
    void setTagsExpanded(const QStringList & tagLocalUids, const TagModel & model);

    void setLinkedNotebooksExpanded(
        const QStringList & linkedNotebookGuids, const TagModel & model);

    void saveSelectedTag(const Account & account, const QString & tagLocalUid);
    void restoreSelectedTag(const TagModel & model);

    void selectionChangedImpl(
        const QItemSelection & selected, const QItemSelection & deselected);

    void handleNoSelectedTag(const Account & account);

    void selectAllTagsRootItem(const TagModel & model);

    void setFavoritedFlag(const QAction & action, const bool favorited);

    void prepareForTagModelChange();
    void postProcessTagModelChange();

    void setSelectedTagToNoteFiltersManager(
        const QString & tagLocalUid);

    void clearTagsFromNoteFiltersManager();

    void disconnectFromNoteFiltersManagerFilterChanged();
    void connectToNoteFiltersManagerFilterChanged();

    bool shouldFilterBySelectedTag(const Account & account) const;

private:
    QMenu *     m_pTagItemContextMenu;
    bool        m_trackingTagItemsState;
    bool        m_trackingSelection;
    bool        m_modelReady;

    QPointer<NoteFiltersManager>    m_pNoteFiltersManager;

    QString     m_tagLocalUidPendingNoteFiltersManagerReadiness;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_TAG_ITEM_VIEW_H
