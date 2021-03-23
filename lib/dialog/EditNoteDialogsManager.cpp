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

#include "EditNoteDialogsManager.h"
#include "EditNoteDialog.h"

#include <lib/model/notebook/NotebookModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <memory>

namespace quentier {

EditNoteDialogsManager::EditNoteDialogsManager(
    LocalStorageManagerAsync & localStorageManagerAsync, NoteCache & noteCache,
    NotebookModel * pNotebookModel, QWidget * parent) :
    QObject(parent),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_noteCache(noteCache), m_pNotebookModel(pNotebookModel)
{
    createConnections();
}

void EditNoteDialogsManager::setNotebookModel(NotebookModel * pNotebookModel)
{
    QNDEBUG("dialog", "EditNoteDialogsManager::setNotebookModel");
    m_pNotebookModel = pNotebookModel;
}

void EditNoteDialogsManager::onEditNoteDialogRequested(QString noteLocalId) // NOLINT
{
    QNDEBUG(
        "dialog",
        "EditNoteDialogsManager::onEditNoteDialogRequested: "
            << "note local id = " << noteLocalId);
    findNoteAndRaiseEditNoteDialog(noteLocalId, /* read only mode = */ false);
}

void EditNoteDialogsManager::onNoteInfoDialogRequested(QString noteLocalId) // NOLINT
{
    QNDEBUG(
        "dialog",
        "EditNoteDialogsManager::onNoteInfoDialogRequested: "
            << "note local id = " << noteLocalId);
    findNoteAndRaiseEditNoteDialog(noteLocalId, /* read only ode = */ true);
}

void EditNoteDialogsManager::onFindNoteComplete(
    qevercloud::Note note, LocalStorageManager::GetNoteOptions options, // NOLINT
    QUuid requestId)
{
    const auto it = m_findNoteRequestIds.find(requestId);
    if (it == m_findNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "dialog",
        "EditNoteDialogsManager::onFindNoteComplete: "
            << "request id = " << requestId << ", with resource metadata = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceMetadata)
                    ? "true"
                    : "false")
            << ", with resource binary data = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                    ? "true"
                    : "false")
            << ", note: " << note);

    const bool readOnlyFlag = it.value();

    Q_UNUSED(m_findNoteRequestIds.erase(it))

    m_noteCache.put(note.localId(), note);
    raiseEditNoteDialog(note, readOnlyFlag);
}

void EditNoteDialogsManager::onFindNoteFailed(
    qevercloud::Note note, LocalStorageManager::GetNoteOptions options, // NOLINT
    ErrorString errorDescription, QUuid requestId)
{
    const auto it = m_findNoteRequestIds.find(requestId);
    if (it == m_findNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "dialog",
        "EditNoteDialogsManager::onFindNoteFailed: request id = "
            << requestId << ", with resource metadata = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceMetadata)
                    ? "true"
                    : "false")
            << ", with resource binary data = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                    ? "true"
                    : "false")
            << ", error description = " << errorDescription
            << "; note: " << note);

    const bool readOnlyFlag = it.value();

    Q_UNUSED(m_findNoteRequestIds.erase(it))

    ErrorString error;
    if (readOnlyFlag) {
        error.setBase(
            QT_TR_NOOP("Can't edit note: the note to be edited was not found"));
    }
    else {
        error.setBase(
            QT_TR_NOOP("Can't show the note info: the note to be edited "
                       "was not found"));
    }

    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    QNWARNING("dialog", error);
    Q_EMIT notifyError(error);
}

void EditNoteDialogsManager::onUpdateNoteComplete(
    qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options, // NOLINT
    QUuid requestId)
{
    const auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "dialog",
        "EditNoteDialogsManager::onUpdateNoteComplete: request id = "
            << requestId << ", update resource metadata = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                    ? "true"
                    : "false")
            << ", update resource binary data = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::
                     UpdateResourceBinaryData)
                    ? "true"
                    : "false")
            << ", update tags = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                    ? "true"
                    : "false")
            << ", note: " << note);

    m_noteCache.put(note.localId(), note);
}

void EditNoteDialogsManager::onUpdateNoteFailed(
    qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options, // NOLINT
    ErrorString errorDescription, QUuid requestId)
{
    const auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNWARNING(
        "dialog",
        "EditNoteDialogsManager::onUpdateNoteFailed: "
            << "request id = " << requestId << ", update resource metadata = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                    ? "true"
                    : "false")
            << ", update resource binary data = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::
                     UpdateResourceBinaryData)
                    ? "true"
                    : "false")
            << ", update tags = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                    ? "true"
                    : "false")
            << ", error: " << errorDescription << "; note: " << note);

    ErrorString error(QT_TR_NOOP("Note update has failed"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);
}

void EditNoteDialogsManager::createConnections()
{
    QNDEBUG("dialog", "EditNoteDialogsManager::createConnections");

    QObject::connect(
        this, &EditNoteDialogsManager::findNote, &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNoteRequest);

    QObject::connect(
        this, &EditNoteDialogsManager::updateNote, &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onUpdateNoteRequest);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteComplete, this,
        &EditNoteDialogsManager::onFindNoteComplete);

    QObject::connect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::findNoteFailed,
        this, &EditNoteDialogsManager::onFindNoteFailed);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteComplete, this,
        &EditNoteDialogsManager::onUpdateNoteComplete);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteFailed, this,
        &EditNoteDialogsManager::onUpdateNoteFailed);
}

void EditNoteDialogsManager::findNoteAndRaiseEditNoteDialog(
    const QString & noteLocalId, const bool readOnlyFlag)
{
    const auto * pCachedNote = m_noteCache.get(noteLocalId);
    if (pCachedNote) {
        raiseEditNoteDialog(*pCachedNote, readOnlyFlag);
        return;
    }

    // Otherwise need to find the note in the local storage
    const auto requestId = QUuid::createUuid();
    m_findNoteRequestIds[requestId] = readOnlyFlag;

    qevercloud::Note note;
    note.setLocalId(noteLocalId);

    QNTRACE(
        "dialog",
        "Emitting the request to find note: requets id = "
            << requestId << ", note local id = " << noteLocalId);

    LocalStorageManager::GetNoteOptions options(
        LocalStorageManager::GetNoteOption::WithResourceMetadata);

    Q_EMIT findNote(note, options, requestId);
}

void EditNoteDialogsManager::raiseEditNoteDialog(
    const qevercloud::Note & note, bool readOnlyFlag)
{
    QNDEBUG(
        "dialog",
        "EditNoteDialogsManager::raiseEditNoteDialog: "
            << "note local id = " << note.localId()
            << ", read only flag = " << (readOnlyFlag ? "true" : "false"));

    auto * pWidget = qobject_cast<QWidget *>(parent());
    if (Q_UNLIKELY(!pWidget)) {
        ErrorString error(
            QT_TR_NOOP("Can't raise the note editing dialog: "
                       "no parent widget"));
        QNWARNING("dialog", error << ", parent = " << parent());
        Q_EMIT notifyError(error);
        return;
    }

    QNTRACE("dialog", "Note before raising the edit dialog: " << note);

    auto pEditNoteDialog = std::make_unique<EditNoteDialog>(
        note, m_pNotebookModel.data(), pWidget, readOnlyFlag);

    pEditNoteDialog->setWindowModality(Qt::WindowModal);
    const int res = pEditNoteDialog->exec();

    if (readOnlyFlag) {
        QNTRACE(
            "dialog",
            "Don't care about the result, the dialog was "
                << "read-only anyway");
        return;
    }

    if (res == QDialog::Rejected) {
        QNTRACE("dialog", "Note editing rejected");
        return;
    }

    auto editedNote = pEditNoteDialog->note();
    QNTRACE("dialog", "Note after possible editing: " << editedNote);

    if (editedNote == note) {
        QNTRACE(
            "dialog",
            "Note hasn't changed after the editing, nothing to "
                << "do");
        return;
    }

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

    QNTRACE(
        "dialog",
        "Emitting the request to update note: request id = " << requestId);

    Q_EMIT updateNote(
        editedNote,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        LocalStorageManager::UpdateNoteOptions{},
#else
        LocalStorageManager::UpdateNoteOptions(0), // NOLINT
#endif
        requestId);
}

} // namespace quentier
