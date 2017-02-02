/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef QUENTIER_VIEWS_NOTE_LIST_VIEW_H
#define QUENTIER_VIEWS_NOTE_LIST_VIEW_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QListView>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookItemView)
QT_FORWARD_DECLARE_CLASS(NotebookItem)

/**
 * @brief The NoteListView is a simple subclass of QListView which adds some bits of functionality specific to note list
 * on top of it
 */
class NoteListView: public QListView
{
    Q_OBJECT
public:
    explicit NoteListView(QWidget * parent = Q_NULLPTR);

    void setNotebookItemView(NotebookItemView * pNotebookItemView);

Q_SIGNALS:
    void notifyError(QNLocalizedString errorDescription);
    void currentNoteChanged(QString noteLocalUid);

    void newNoteCreationRequested();
    void editNoteDialogRequested(QString noteLocalUid);
    void noteInfoDialogRequested(QString noteLocalUid);

public Q_SLOTS:
    /**
     * @brief The dataChanged method is redefined in NoteListView for the sole reason of being a public slot
     * instead of protected; it calls the implementation of QListView's dataChanged protected slot
     */
    virtual void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            )
#else
                            , const QVector<int> & roles = QVector<int>())
#endif
                            Q_DECL_OVERRIDE;

    virtual void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end) Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void onCreateNewNoteAction();
    void onDeleteNoteAction();
    void onEditNoteAction();
    void onMoveToOtherNotebookAction();

    void onUnfavoriteAction();
    void onFavoriteAction();

    void onShowNoteInfoAction();

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;

protected:
    virtual void currentChanged(const QModelIndex & current,
                                const QModelIndex & previous) Q_DECL_OVERRIDE;

    const NotebookItem * currentNotebookItem();

protected:
    QMenu *             m_pNoteItemContextMenu;
    NotebookItemView *  m_pNotebookItemView;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_NOTE_LIST_VIEW_H
