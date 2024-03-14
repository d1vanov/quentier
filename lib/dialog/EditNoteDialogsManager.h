/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include <lib/model/note/NoteCache.h>

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/ErrorString.h>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QUuid>

namespace quentier {

class NotebookModel;

class EditNoteDialogsManager : public QObject
{
    Q_OBJECT
public:
    // NOTE: as dialogs need a widget to be their parent, this class' parent
    // must be a QWidget instance
    explicit EditNoteDialogsManager(
        local_storage::ILocalStoragePtr localStorage, NoteCache & noteCache,
        NotebookModel * notebookModel, QWidget * parent = nullptr);

    void setNotebookModel(NotebookModel * notebookModel);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

public Q_SLOTS:
    void onEditNoteDialogRequested(QString noteLocalId);
    void onNoteInfoDialogRequested(QString noteLocalId);

private:
    void findNoteAndRaiseEditNoteDialog(
        const QString & noteLocalId, const bool readOnlyFlag);

    void raiseEditNoteDialog(const qevercloud::Note & note, bool readOnlyFlag);

private:
    Q_DISABLE_COPY(EditNoteDialogsManager)

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    NoteCache & m_noteCache;

    QPointer<NotebookModel> m_notebookModel;
};

} // namespace quentier
