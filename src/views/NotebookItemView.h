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

#ifndef QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H
#define QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H

#include "ItemView.h"
#include <quentier/types/ErrorString.h>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookModel)
QT_FORWARD_DECLARE_CLASS(NotebookItem)
QT_FORWARD_DECLARE_CLASS(NotebookStackItem)
QT_FORWARD_DECLARE_CLASS(NotebookModelItem)
QT_FORWARD_DECLARE_CLASS(NoteFiltersManager)

class NotebookItemView: public ItemView
{
    Q_OBJECT
public:
    explicit NotebookItemView(QWidget * parent = Q_NULLPTR);

    void setNoteFiltersManager(NoteFiltersManager & noteFiltersManager);

    virtual void setModel(QAbstractItemModel * pModel) Q_DECL_OVERRIDE;

    /**
     * @return valid model index if the selection exists and contains exactly one row and invalid model index otherwise
     */
    QModelIndex currentlySelectedItemIndex() const;

Q_SIGNALS:
    void notifyError(ErrorString error);
    void newNotebookCreationRequested();
    void notebookInfoRequested();

public Q_SLOTS:
    void deleteSelectedItem();

private Q_SLOTS:
    void onAllNotebooksListed();

    void onAboutToAddNotebook();
    void onAddedNotebook(const QModelIndex & index);

    void onAboutToUpdateNotebook(const QModelIndex & index);
    void onUpdatedNotebook(const QModelIndex & index);

    void onAboutToRemoveNotebooks();
    void onRemovedNotebooks();

    void onCreateNewNotebookAction();
    void onRenameNotebookAction();
    void onDeleteNotebookAction();
    void onSetNotebookDefaultAction();
    void onShowNotebookInfoAction();

    void onEditNotebookAction();
    void onMoveNotebookToStackAction();
    void onRemoveNotebookFromStackAction();

    void onRenameNotebookStackAction();
    void onRemoveNotebooksFromStackAction();

    void onFavoriteAction();
    void onUnfavoriteAction();

    void onNotebookModelItemCollapsedOrExpanded(const QModelIndex & index);

    void onNotebookStackRenamed(const QString & previousStackName, const QString & newStackName,
                                const QString & linkedNotebookGuid);
    void onNotebookStackChanged(const QModelIndex & notebookIndex);

    void onNoteFilterChanged();

    void onNoteFiltersManagerReady();

    virtual void selectionChanged(const QItemSelection & selected,
                                  const QItemSelection & deselected) Q_DECL_OVERRIDE;
    virtual void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void deleteItem(const QModelIndex & itemIndex, NotebookModel & model);
    void showNotebookItemContextMenu(const NotebookItem & item,
                                     const QPoint & point, NotebookModel & model);
    void showNotebookStackItemContextMenu(const NotebookStackItem & item,
                                          const NotebookModelItem & modelItem,
                                          const QPoint & point, NotebookModel & model);

    void saveNotebookModelItemsState();
    void restoreNotebookModelItemsState(const NotebookModel & model);

    void setStacksExpanded(const QStringList & expandedStackNames, const NotebookModel & model,
                           const QString & linkedNotebookGuid);
    void setLinkedNotebooksExpanded(const QStringList & expandedLinkedNotebookGuids,
                                    const NotebookModel & model);

    void restoreLastSavedSelectionOrAutoSelectNotebook(const NotebookModel & model);
    void autoSelectNotebook(const NotebookModel & model);

    void selectionChangedImpl(const QItemSelection & selected,
                              const QItemSelection & deselected);

    void persistSelectedNotebookLocalUid(const NotebookModel & notebookModel,
                                         const QString & notebookLocalUid);

    void clearSelectionImpl();

    void setFavoritedFlag(const QAction & action, const bool favorited);

    void prepareForNotebookModelChange();
    void postProcessNotebookModelChange();

    void setSelectedNotebookToNoteFilterManager(const QString & notebookLocalUid);

private:
    QMenu *     m_pNotebookItemContextMenu;
    QMenu *     m_pNotebookStackItemContextMenu;

    QPointer<NoteFiltersManager>    m_pNoteFiltersManager;

    QString     m_notebookLocalUidPendingNoteFiltersManagerReadiness;

    bool        m_trackingNotebookModelItemsState;
    bool        m_trackingSelection;
    bool        m_modelReady;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H
