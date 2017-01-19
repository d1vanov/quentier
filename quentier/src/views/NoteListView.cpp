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

#include "NoteListView.h"
#include "../models/NoteFilterModel.h"
#include "../models/NoteModel.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

NoteListView::NoteListView(QWidget * parent) :
    QListView(parent)
{}

void NoteListView::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                               )
#else
                               , const QVector<int> & roles)
#endif
{
    QListView::dataChanged(topLeft, bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                               );
#else
                               , roles);
#endif
}


void NoteListView::currentChanged(const QModelIndex & current,
                                  const QModelIndex & previous)
{
    QNTRACE(QStringLiteral("NoteListView::currentChanged"));

    Q_UNUSED(previous)

    if (!current.isValid()) {
        QNTRACE(QStringLiteral("Current index is invalid"));
        return;
    }

    const NoteFilterModel * pNoteFilterModel = qobject_cast<const NoteFilterModel*>(current.model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNLocalizedString error = QNLocalizedString("Wrong model connected to the note list view", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNLocalizedString error = QNLocalizedString("Can't get the source model from the note filter model "
                                                    "connected to the note list view", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QModelIndex sourceIndex = pNoteFilterModel->mapToSource(current);
    const NoteModelItem * pItem = pNoteModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pItem)) {
        QNLocalizedString error = QNLocalizedString("Can't retrieve the new current note item for note model index", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit currentNoteChanged(pItem->localUid());
}

} // namespace quentier
