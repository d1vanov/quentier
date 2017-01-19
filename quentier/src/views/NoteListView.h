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

namespace quentier {

/**
 * @brief The NoteListView is a simple subclass of QListView which adds some bits of functionality specific to note list
 * on top of it
 */
class NoteListView: public QListView
{
    Q_OBJECT
public:
    explicit NoteListView(QWidget * parent = Q_NULLPTR);

Q_SIGNALS:
    void notifyError(QNLocalizedString errorDescription);
    void currentNoteChanged(QString noteLocalUid);

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

protected:
    virtual void currentChanged(const QModelIndex & current,
                                const QModelIndex & previous) Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_NOTE_LIST_VIEW_H
