/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DELEGATE_DELETED_NOTE_ITEM_DELEGATE_H
#define QUENTIER_LIB_DELEGATE_DELETED_NOTE_ITEM_DELEGATE_H

#include "AbstractStyledItemDelegate.h"

namespace quentier {

class NoteModelItem;

class DeletedNoteItemDelegate final: public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DeletedNoteItemDelegate(QObject * parent = nullptr);

    /**
     * @brief returns null pointer as DeletedNoteItemDelegate doesn't allow
     * editing
     */
    [[nodiscard]] QWidget * createEditor(
        QWidget * parent, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    void paint(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    /**
     * @brief does nothing
     */
    void setEditorData(
        QWidget * editor, const QModelIndex & index) const override;

    /**
     * @brief does nothing
     */
    void setModelData(
        QWidget * editor, QAbstractItemModel * model,
        const QModelIndex & index) const override;

    [[nodiscard]] QSize sizeHint(
        const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    /**
     * @brief does nothing
     */
    void updateEditorGeometry(
        QWidget * editor, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

private:
    void doPaint(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const;

    void drawDeletedNoteTitleOrPreviewText(
        QPainter * painter, const QStyleOptionViewItem & option,
        const NoteModelItem & item) const;

    void drawDeletionDateTime(
        QPainter * painter, const QStyleOptionViewItem & option,
        const NoteModelItem & item) const;

    [[nodiscard]] QSize doSizeHint(
        const QStyleOptionViewItem & option, const QModelIndex & index) const;

private:
    QString m_deletionDateTimeReplacementText;
};

} // namespace quentier

#endif // QUENTIER_LIB_DELEGATE_DELETED_NOTE_ITEM_DELEGATE_H
