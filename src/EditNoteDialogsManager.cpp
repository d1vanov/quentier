/*
 * Copyright 2017-2019 Dmitry Ivanov
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
#include "dialogs/EditNoteDialog.h"
#include "models/NotebookModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QScopedPointer>

namespace quentier {

EditNoteDialogsManager::EditNoteDialogsManager(
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookModel * pNotebookModel,
        QWidget * parent) :
    QObject(parent),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_noteCache(noteCache),
    m_findNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_pNotebookModel(pNotebookModel)
{
    createConnections();
}

void EditNoteDialogsManager::setNotebookModel(NotebookModel * pNotebookModel)
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::setNotebookModel"));
    m_pNotebookModel = pNotebookModel;
}

void EditNoteDialogsManager::onEditNoteDialogRequested(QString noteLocalUid)
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onEditNoteDialogRequested: ")
            << QStringLiteral("note local uid = ") << noteLocalUid);
    findNoteAndRaiseEditNoteDialog(noteLocalUid, /* read only mode = */ false);
}

void EditNoteDialogsManager::onNoteInfoDialogRequested(QString noteLocalUid)
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onNoteInfoDialogRequested: ")
            << QStringLiteral("note local uid = ") << noteLocalUid);
    findNoteAndRaiseEditNoteDialog(noteLocalUid, /* read only ode = */ true);
}

void EditNoteDialogsManager::onFindNoteComplete(
    Note note, LocalStorageManager::GetNoteOptions options, QUuid requestId)
{
    auto it = m_findNoteRequestIds.find(requestId);
    if (it == m_findNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onFindNoteComplete: request id = ")
            << requestId << QStringLiteral(", with resource metadata = ")
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", with resource binary data = ")
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", note: ") << note);

    bool readOnlyFlag = it.value();

    Q_UNUSED(m_findNoteRequestIds.erase(it))

    m_noteCache.put(note.localUid(), note);
    raiseEditNoteDialog(note, readOnlyFlag);
}

void EditNoteDialogsManager::onFindNoteFailed(
    Note note, LocalStorageManager::GetNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    auto it = m_findNoteRequestIds.find(requestId);
    if (it == m_findNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onFindNoteFailed: request id = ")
            << requestId << QStringLiteral(", with resource metadata = ")
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", with resource binary data = ")
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription
            << QStringLiteral("; note: ") << note);

    bool readOnlyFlag = it.value();

    Q_UNUSED(m_findNoteRequestIds.erase(it))

    ErrorString error;
    if (readOnlyFlag) {
        error.setBase(QT_TR_NOOP("Can't edit note: the note to be edited was not "
                                 "found"));
    }
    else {
        error.setBase(QT_TR_NOOP("Can't show the note info: the note to be edited "
                                 "was not found"));
    }

    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    QNWARNING(error);
    Q_EMIT notifyError(error);
}

void EditNoteDialogsManager::onUpdateNoteComplete(
    Note note, LocalStorageManager::UpdateNoteOptions options, QUuid requestId)
{
    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onUpdateNoteComplete: request id = ")
            << requestId << QStringLiteral(", update resource metadata = ")
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", update resource binary data = ")
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceBinaryData)
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", update tags = ")
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", note: ") << note);

    m_noteCache.put(note.localUid(), note);
}

void EditNoteDialogsManager::onUpdateNoteFailed(
    Note note, LocalStorageManager::UpdateNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("EditNoteDialogsManager::onUpdateNoteFailed: ")
              << QStringLiteral("request id = ") << requestId
              << QStringLiteral(", update resource metadata = ")
              << ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                  ? QStringLiteral("true")
                  : QStringLiteral("false"))
              << QStringLiteral(", update resource binary data = ")
              << ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceBinaryData)
                  ? QStringLiteral("true")
                  : QStringLiteral("false"))
              << QStringLiteral(", update tags = ")
              << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                  ? QStringLiteral("true")
                  : QStringLiteral("false"))
              << QStringLiteral(", error: ") << errorDescription
              << QStringLiteral("; note: ") << note);

    ErrorString error(QT_TR_NOOP("Note update has failed"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);
}

void EditNoteDialogsManager::createConnections()
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::createConnections"));

    QObject::connect(this,
                     QNSIGNAL(EditNoteDialogsManager,findNote,
                              Note,LocalStorageManager::GetNoteOptions,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,
                            Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::connect(this,
                     QNSIGNAL(EditNoteDialogsManager,updateNote,
                              Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onUpdateNoteRequest,
                            Note,LocalStorageManager::UpdateNoteOptions,QUuid));

    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,
                              Note,LocalStorageManager::GetNoteOptions,QUuid),
                     this,
                     QNSLOT(EditNoteDialogsManager,onFindNoteComplete,
                            Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,
                              Note,LocalStorageManager::GetNoteOptions,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(EditNoteDialogsManager,onFindNoteFailed,
                            Note,LocalStorageManager::GetNoteOptions,
                            ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,
                              Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     this,
                     QNSLOT(EditNoteDialogsManager,onUpdateNoteComplete,
                            Note,LocalStorageManager::UpdateNoteOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNoteFailed,
                              Note,LocalStorageManager::UpdateNoteOptions,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(EditNoteDialogsManager,onUpdateNoteFailed,
                            Note,LocalStorageManager::UpdateNoteOptions,
                            ErrorString,QUuid));
}

void EditNoteDialogsManager::findNoteAndRaiseEditNoteDialog(
    const QString & noteLocalUid, const bool readOnlyFlag)
{
    const Note * pCachedNote = m_noteCache.get(noteLocalUid);
    if (pCachedNote) {
        raiseEditNoteDialog(*pCachedNote, readOnlyFlag);
        return;
    }

    // Otherwise need to find the note in the local storage
    QUuid requestId = QUuid::createUuid();
    m_findNoteRequestIds[requestId] = readOnlyFlag;

    Note note;
    note.setLocalUid(noteLocalUid);
    QNTRACE(QStringLiteral("Emitting the request to find note: requets id = ")
            << requestId << QStringLiteral(", note local uid = ") << noteLocalUid);
    LocalStorageManager::GetNoteOptions options(
        LocalStorageManager::GetNoteOption::WithResourceMetadata);
    Q_EMIT findNote(note, options, requestId);
}

void EditNoteDialogsManager::raiseEditNoteDialog(const Note & note,
                                                 const bool readOnlyFlag)
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::raiseEditNoteDialog: ")
            << QStringLiteral("note local uid = ") << note.localUid()
            << QStringLiteral(", read only flag = ")
            << (readOnlyFlag
                ? QStringLiteral("true")
                : QStringLiteral("false")));

    QWidget * pWidget = qobject_cast<QWidget*>(parent());
    if (Q_UNLIKELY(!pWidget)) {
        ErrorString error(QT_TR_NOOP("Can't raise the note editing dialog: "
                                     "no parent widget"));
        QNWARNING(error << QStringLiteral(", parent = ") << parent());
        Q_EMIT notifyError(error);
        return;
    }

    QNTRACE(QStringLiteral("Note before raising the edit dialog: ") << note);

    QScopedPointer<EditNoteDialog> pEditNoteDialog(
        new EditNoteDialog(note, m_pNotebookModel.data(), pWidget, readOnlyFlag));
    pEditNoteDialog->setWindowModality(Qt::WindowModal);
    int res = pEditNoteDialog->exec();

    if (readOnlyFlag) {
        QNTRACE(QStringLiteral("Don't care about the result, the dialog was "
                               "read-only anyway"));
        return;
    }

    if (res == QDialog::Rejected) {
        QNTRACE(QStringLiteral("Note editing rejected"));
        return;
    }

    Note editedNote = pEditNoteDialog->note();
    QNTRACE(QStringLiteral("Note after possible editing: ") << editedNote);

    if (editedNote == note) {
        QNTRACE(QStringLiteral("Note hasn't changed after the editing, nothing "
                               "to do"));
        return;
    }

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to update note: request id = ")
            << requestId);
    Q_EMIT updateNote(editedNote, LocalStorageManager::UpdateNoteOptions(0),
                      requestId);
}

} // namespace quentier
