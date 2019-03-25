/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DELEGATE_NOTE_ITEM_DELEGATE_H
#define QUENTIER_LIB_DELEGATE_NOTE_ITEM_DELEGATE_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>

#include <QStyledItemDelegate>

namespace quentier {

class NoteItemDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NoteItemDelegate(QObject * parent = Q_NULLPTR);

    /**
     * @brief returns null pointer as NoteItemDelegate doesn't allow editing
     */
    virtual QWidget * createEditor(QWidget * parent,
                                   const QStyleOptionViewItem & option,
                                   const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual void paint(QPainter * painter,
                       const QStyleOptionViewItem & option,
                       const QModelIndex & index) const Q_DECL_OVERRIDE;

    /**
     * @brief does nothing
     */
    virtual void setEditorData(QWidget * editor,
                               const QModelIndex & index) const Q_DECL_OVERRIDE;

    /**
     * @brief does nothing
     */
    virtual void setModelData(QWidget * editor, QAbstractItemModel * model,
                              const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual QSize sizeHint(const QStyleOptionViewItem & option,
                           const QModelIndex & index) const Q_DECL_OVERRIDE;

    /**
     * @brief does nothing
     */
    virtual void updateEditorGeometry(QWidget * editor,
                                      const QStyleOptionViewItem & option,
                                      const QModelIndex & index) const Q_DECL_OVERRIDE;

public Q_SLOTS:
    /**
     * The slot which can watch for external changes in thumbnail display state.
     * @param showThumbnailsForAllNotes Global flag for all notes.
     * @param hideThumbnailsLocalUids   Map with local uids where the thumbails
     *                                  was manually hidden.
     */
    void setShowNoteThumbnailsState(bool showThumbnailsForAllNotes,
                                    const QSet<QString> & hideThumbnailsLocalUids);


Q_SIGNALS:
    void notifyError(ErrorString error) const;  // clazy:exclude=const-signal-or-slot

private:
    QString timestampToString(const qint64 timestamp, const qint64 timePassed) const;

private:
    /**
     * Current value of "shown thumbnails for all notes".
     */
    bool                    m_showThumbnailsForAllNotes;
    /**
     * Set with local uids of notes where thumbnail was manually hidden.
     */
    QSet<QString>           m_hideThumbnailsLocalUids;

    int                     m_minWidth;
    int                     m_minHeight;
    int                     m_leftMargin;
    int                     m_rightMargin;
    int                     m_topMargin;
    int                     m_bottomMargin;
};

} // namespace quentier

#endif // QUENTIER_LIB_DELEGATE_NOTE_ITEM_DELEGATE_H
