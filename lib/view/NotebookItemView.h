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

#ifndef QUENTIER_LIB_VIEW_NOTEBOOK_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_NOTEBOOK_ITEM_VIEW_H

#include "ItemView.h"

#include <quentier/types/ErrorString.h>

#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(INotebookModelItem)
QT_FORWARD_DECLARE_CLASS(NoteModel)
QT_FORWARD_DECLARE_CLASS(NotebookModel)
QT_FORWARD_DECLARE_CLASS(NotebookItem)
QT_FORWARD_DECLARE_CLASS(NoteFiltersManager)
QT_FORWARD_DECLARE_CLASS(StackItem)

class NotebookItemView: public ItemView
{
    Q_OBJECT
public:
    explicit NotebookItemView(QWidget * parent = nullptr);

    void setNoteFiltersManager(NoteFiltersManager & noteFiltersManager);

    virtual void setModel(QAbstractItemModel * pModel) override;

    void setNoteModel(const NoteModel * pNoteModel);

    /**
     * @return          Valid model index if the selection exists and contains
     *                  exactly one row and invalid model index otherwise
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

    void onNotebookStackRenamed(
        const QString & previousStackName, const QString & newStackName,
        const QString & linkedNotebookGuid);

    void onNotebookStackChanged(const QModelIndex & notebookIndex);

    void onNoteFilterChanged();

    void onNoteFiltersManagerReady();

    virtual void selectionChanged(
        const QItemSelection & selected,
        const QItemSelection & deselected) override;

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    void deleteItem(const QModelIndex & itemIndex, NotebookModel & model);

    void showNotebookItemContextMenu(
        const NotebookItem & item, const QPoint & point, NotebookModel & model);

    void showNotebookStackItemContextMenu(
        const StackItem & item, const INotebookModelItem & modelItem,
        const QPoint & point, NotebookModel & model);

    void saveNotebookModelItemsState();
    void restoreNotebookModelItemsState(const NotebookModel & model);

    void setStacksExpanded(
        const QStringList & expandedStackNames, const NotebookModel & model,
        const QString & linkedNotebookGuid);

    void setLinkedNotebooksExpanded(
        const QStringList & expandedLinkedNotebookGuids,
        const NotebookModel & model);

    void saveSelectedNotebook(
        const Account & account, const QString & notebookLocalUid);

    void restoreSelectedNotebook(
        const NotebookModel & model);

    void selectionChangedImpl(
        const QItemSelection & selected, const QItemSelection & deselected);

    void handleNoSelectedNotebook(const Account & account);

    void selectAllNotebooksRootItem(const NotebookModel & notebookModel);

    void setFavoritedFlag(const QAction & action, const bool favorited);

    void prepareForNotebookModelChange();
    void postProcessNotebookModelChange();

    void setSelectedNotebookToNoteFiltersManager(
        const QString & notebookLocalUid);

    void clearNotebooksFromNoteFiltersManager();

    void disconnectFromNoteFiltersManagerFilterChanged();
    void connectToNoteFiltersManagerFilterChanged();

    bool shouldFilterBySelectedNotebook(const Account & account) const;

    // Helper structs and methods to access common data pieces in slots

    struct NotebookCommonData
    {
        NotebookModel * m_pModel = nullptr;
        QModelIndex     m_index;
        QAction *       m_pAction = nullptr;
    };

    bool fetchCurrentNotebookCommonData(
        NotebookCommonData & data, ErrorString & errorDescription) const;

    struct NotebookItemData: public NotebookCommonData
    {
        QString     m_localUid;
    };

    bool fetchCurrentNotebookItemData(
        NotebookItemData & itemData, ErrorString & errorDescription) const;

    struct NotebookStackData: public NotebookCommonData
    {
        QString     m_stack;
        QString     m_id;
    };

    bool fetchCurrentNotebookStackData(
        NotebookStackData & stackData, ErrorString & errorDescription) const;

private:
    QMenu *     m_pNotebookItemContextMenu = nullptr;
    QMenu *     m_pNotebookStackItemContextMenu = nullptr;

    QPointer<NoteFiltersManager>    m_pNoteFiltersManager;

    QPointer<const NoteModel>   m_pNoteModel;

    QString     m_notebookLocalUidPendingNoteFiltersManagerReadiness;

    bool        m_trackingNotebookModelItemsState = false;
    bool        m_trackingSelection = false;
    bool        m_modelReady = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_NOTEBOOK_ITEM_VIEW_H
