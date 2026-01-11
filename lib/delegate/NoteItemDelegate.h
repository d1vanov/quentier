/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/types/ErrorString.h>

#include <QStyledItemDelegate>

namespace quentier {

class NoteItemDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NoteItemDelegate(QObject * parent = nullptr);

public: // QStyledItemDelegate
    /**
     * @brief returns null pointer as NoteItemDelegate doesn't allow editing
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

public Q_SLOTS:
    /**
     * The slot which can watch for external changes in thumbnail display state.
     * @param showThumbnailsForAllNotes Global flag for all notes.
     * @param hideThumbnailsLocalIds    Map with local ids where the thumbails
     *                                  was manually hidden.
     */
    void setShowNoteThumbnailsState(
        bool showThumbnailsForAllNotes,
        const QSet<QString> & hideThumbnailsLocalIds);

Q_SIGNALS:
    void notifyError(             // clazy:exclude=const-signal-or-slot
        ErrorString error) const; // clazy:exclude=const-signal-or-slot

private:
    [[nodiscard]] QString timestampToString(
        const qint64 timestamp, const qint64 timePassed) const;

private:
    /**
     * Current value of "shown thumbnails for all notes".
     */
    bool m_showThumbnailsForAllNotes;
    /**
     * Set with local ids of notes where thumbnail was manually hidden.
     */
    QSet<QString> m_hideThumbnailsLocalIds;

    int m_minWidth;
    int m_minHeight;
    int m_leftMargin;
    int m_rightMargin;
    int m_topMargin;
    int m_bottomMargin;
};

} // namespace quentier
