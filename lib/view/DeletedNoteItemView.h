/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_VIEW_DELETED_NOTE_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_DELETED_NOTE_ITEM_VIEW_H

#include "ItemView.h"

#include <quentier/types/ErrorString.h>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteModel)

class DeletedNoteItemView: public ItemView
{
    Q_OBJECT
public:
    explicit DeletedNoteItemView(QWidget * parent = nullptr);

    QModelIndex currentlySelectedItemIndex() const;

    void restoreCurrentlySelectedNote();
    void deleteCurrentlySelectedNotePermanently();
    void showCurrentlySelectedNoteInfo();

Q_SIGNALS:
    void deletedNoteInfoRequested(QString deletedNoteLocalUid);
    void notifyError(ErrorString error);

private Q_SLOTS:
    void onRestoreNoteAction();
    void onDeleteNotePermanentlyAction();
    void onShowDeletedNoteInfoAction();

private:
    void restoreNote(const QModelIndex & index, NoteModel & model);
    void deleteNotePermanently(const QModelIndex & index, NoteModel & model);

private:
    virtual void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    QMenu *     m_pDeletedNoteItemContextMenu;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_DELETED_NOTE_ITEM_VIEW_H
