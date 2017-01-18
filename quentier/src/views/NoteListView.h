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

protected:
    virtual void currentChanged(const QModelIndex & current,
                                const QModelIndex & previous) Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_NOTE_LIST_VIEW_H
