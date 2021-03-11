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

#ifndef QUENTIER_LIB_DIALOG_EDIT_NOTE_DIALOGS_MANAGER_H
#define QUENTIER_LIB_DIALOG_EDIT_NOTE_DIALOGS_MANAGER_H

#include <lib/model/note/NoteCache.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>

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
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookModel * pNotebookModel,
        QWidget * parent = nullptr);

    void setNotebookModel(NotebookModel * pNotebookModel);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    // private signals:
    void findNote(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void updateNote(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

public Q_SLOTS:
    void onEditNoteDialogRequested(QString noteLocalId);
    void onNoteInfoDialogRequested(QString noteLocalId);

private Q_SLOTS:
    void onFindNoteComplete(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void onFindNoteFailed(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onUpdateNoteComplete(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void onUpdateNoteFailed(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

private:
    void createConnections();

    void findNoteAndRaiseEditNoteDialog(
        const QString & noteLocalId, bool readOnlyFlag);

    void raiseEditNoteDialog(const qevercloud::Note & note, bool readOnlyFlag);

private:
    Q_DISABLE_COPY(EditNoteDialogsManager)

private:
    LocalStorageManagerAsync & m_localStorageManagerAsync;
    NoteCache & m_noteCache;

    // NOTE: the bool value in this hash is a "read only" flag for the dialog
    // which should be raised on the found note
    QHash<QUuid, bool> m_findNoteRequestIds;

    QSet<QUuid> m_updateNoteRequestIds;

    QPointer<NotebookModel> m_pNotebookModel;
};

} // namespace quentier

#endif // QUENTIER_LIB_DIALOG_EDIT_NOTE_DIALOGS_MANAGER_H
