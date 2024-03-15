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

#include "EditNoteDialogsManager.h"
#include "EditNoteDialog.h"

#include <lib/exception/Utils.h>
#include <lib/model/notebook/NotebookModel.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>

#include <memory>

namespace quentier {

EditNoteDialogsManager::EditNoteDialogsManager(
    local_storage::ILocalStoragePtr localStorage, NoteCache & noteCache,
    NotebookModel * notebookModel, QWidget * parent) :
    QObject{parent},
    m_localStorage{std::move(localStorage)}, m_noteCache{noteCache},
    m_notebookModel{notebookModel}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{
            ErrorString{"EditNoteDialogsManager ctor: local storage is null"}};
    }
}

void EditNoteDialogsManager::setNotebookModel(NotebookModel * notebookModel)
{
    QNDEBUG(
        "dialog::EditNoteDialogsManager",
        "EditNoteDialogsManager::setNotebookModel: "
            << (notebookModel ? "not null" : "null"));

    m_notebookModel = notebookModel;
}

void EditNoteDialogsManager::onEditNoteDialogRequested(QString noteLocalId)
{
    QNDEBUG(
        "dialog::EditNoteDialogsManager",
        "EditNoteDialogsManager::onEditNoteDialogRequested: "
            << "note local id = " << noteLocalId);

    findNoteAndRaiseEditNoteDialog(noteLocalId, /* read only mode = */ false);
}

void EditNoteDialogsManager::onNoteInfoDialogRequested(QString noteLocalId)
{
    QNDEBUG(
        "dialog::EditNoteDialogsManager",
        "EditNoteDialogsManager::onNoteInfoDialogRequested: "
            << "note local id = " << noteLocalId);
    findNoteAndRaiseEditNoteDialog(noteLocalId, /* read only ode = */ true);
}

void EditNoteDialogsManager::findNoteAndRaiseEditNoteDialog(
    const QString & noteLocalId, const bool readOnlyFlag)
{
    if (const auto * cachedNote = m_noteCache.get(noteLocalId)) {
        raiseEditNoteDialog(*cachedNote, readOnlyFlag);
        return;
    }

    // Otherwise need to find the note in the local storage
    QNTRACE(
        "dialog::EditNoteDialogsManager",
        "Requesting note from local storage: note local id = " << noteLocalId);

    auto findNoteFuture = m_localStorage->findNoteByLocalId(
        noteLocalId,
        local_storage::ILocalStorage::FetchNoteOptions{} |
            local_storage::ILocalStorage::FetchNoteOption::
                WithResourceMetadata);

    auto findNoteThenFuture = threading::then(
        std::move(findNoteFuture), this,
        [this, noteLocalId,
         readOnlyFlag](const std::optional<qevercloud::Note> & note) {
            if (Q_UNLIKELY(!note)) {
                ErrorString error{
                    QT_TR_NOOP("Cannot show/edit note: note was not found in "
                               "local storage")};
                error.details() = noteLocalId;
                QNWARNING("dialog::EditNoteDialogsManager", error);
                Q_EMIT notifyError(error);
                return;
            }

            m_noteCache.put(noteLocalId, *note);
            raiseEditNoteDialog(*note, readOnlyFlag);
        });

    threading::onFailed(
        std::move(findNoteThenFuture), this,
        [this, noteLocalId](const QException & e) {
            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP("Cannot show/edit note")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.setDetails(message.details());
            QNWARNING("dialog::EditNoteDialogsManager", error);
            Q_EMIT notifyError(error);
        });
}

void EditNoteDialogsManager::raiseEditNoteDialog(
    const qevercloud::Note & note, const bool readOnlyFlag)
{
    QNDEBUG(
        "dialog::EditNoteDialogsManager",
        "EditNoteDialogsManager::raiseEditNoteDialog: "
            << "note local id = " << note.localId()
            << ", read only flag = " << (readOnlyFlag ? "true" : "false"));

    auto * widget = qobject_cast<QWidget *>(parent());
    if (Q_UNLIKELY(!widget)) {
        ErrorString error{QT_TR_NOOP(
            "Can't raise the note editing dialog: no parent widget")};
        QNWARNING(
            "dialog::EditNoteDialogsManager",
            error << ", parent = " << parent());
        Q_EMIT notifyError(error);
        return;
    }

    QNTRACE(
        "dialog::EditNoteDialogsManager",
        "Note before raising the edit dialog: " << note);

    auto editNoteDialog = std::make_unique<EditNoteDialog>(
        note, m_notebookModel.data(), widget, readOnlyFlag);

    editNoteDialog->setWindowModality(Qt::WindowModal);
    const int res = editNoteDialog->exec();

    if (readOnlyFlag) {
        QNTRACE(
            "dialog::EditNoteDialogsManager",
            "Don't care about the result, the dialog was read-only anyway");
        return;
    }

    if (res == QDialog::Rejected) {
        QNTRACE("dialog::EditNoteDialogsManager", "Note editing rejected");
        return;
    }

    qevercloud::Note editedNote = editNoteDialog->note();
    QNTRACE(
        "dialog::EditNoteDialogsManager",
        "Note after possible editing: " << editedNote);

    if (editedNote == note) {
        QNTRACE(
            "dialog::EditNoteDialogsManager",
            "Note hasn't changed after the editing, nothing to do");
        return;
    }

    QNTRACE(
        "dialog::EditNoteDialogsManager", "Updating the note: " << editedNote);

    auto updateNoteFuture = m_localStorage->updateNote(
        editedNote, local_storage::ILocalStorage::UpdateNoteOptions{});

    auto updateNoteThenFuture =
        threading::then(std::move(updateNoteFuture), this, [this, editedNote] {
            QNDEBUG(
                "dialog::EditNoteDialogsManager",
                "Successfully updated note with local id "
                    << editedNote.localId());
            m_noteCache.put(editedNote.localId(), editedNote);
        });

    threading::onFailed(
        std::move(updateNoteThenFuture), this,
        [this, editedNote](const QException & e) {
            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP("Cannot apply note edits")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.setDetails(message.details());
            QNWARNING("dialog::EditNoteDialogsManager", error);
            Q_EMIT notifyError(error);
        });
}

} // namespace quentier
