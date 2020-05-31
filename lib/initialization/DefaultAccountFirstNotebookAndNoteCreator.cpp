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

#include "DefaultAccountFirstNotebookAndNoteCreator.h"

#include <lib/widget/NoteFiltersManager.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Utility.h>

namespace quentier {

DefaultAccountFirstNotebookAndNoteCreator::DefaultAccountFirstNotebookAndNoteCreator(
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteFiltersManager & noteFiltersManager,
        QObject * parent) :
    QObject(parent),
    m_addNotebookRequestId(),
    m_addNoteRequestId(),
    m_pNoteFiltersManager(&noteFiltersManager)
{
    connectToLocalStorage(localStorageManagerAsync);
}

void DefaultAccountFirstNotebookAndNoteCreator::start()
{
    QNDEBUG("DefaultAccountFirstNotebookAndNoteCreator::start");

    if (Q_UNLIKELY(!m_addNotebookRequestId.isNull())) {
        QNDEBUG("Add notebook request has already been sent");
        return;
    }

    if (Q_UNLIKELY(!m_addNoteRequestId.isNull())) {
        QNDEBUG("Add note request has already been sent");
        return;
    }

    emitAddNotebookRequest();
}

void DefaultAccountFirstNotebookAndNoteCreator::onAddNotebookComplete(
    Notebook notebook, QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNDEBUG("DefaultAccountFirstNotebookAndNoteCreator::"
            << "onAddNotebookComplete: notebook = "
            << notebook << "\nRequest id = " << requestId);

    m_addNotebookRequestId = QUuid();

    if (!m_pNoteFiltersManager.isNull()) {
        m_pNoteFiltersManager->setNotebookToFilter(notebook.localUid());
    }

    emitAddNoteRequest(notebook);
}

void DefaultAccountFirstNotebookAndNoteCreator::onAddNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNWARNING("DefaultAccountFirstNotebookAndNoteCreator::"
              << "onAddNotebookFailed: notebook = " << notebook
              << "\nError description = " << errorDescription
              << "\nRequest id = " << requestId);

    m_addNotebookRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void DefaultAccountFirstNotebookAndNoteCreator::onAddNoteComplete(
    Note note, QUuid requestId)
{
    if (requestId != m_addNoteRequestId) {
        return;
    }

    QNDEBUG("DefaultAccountFirstNotebookAndNoteCreator::"
            << "onAddNoteComplete: note = " << note
            << "\nRequest id = " << requestId);

    m_addNoteRequestId = QUuid();
    Q_EMIT finished(note.localUid());
}

void DefaultAccountFirstNotebookAndNoteCreator::onAddNoteFailed(
    Note note, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_addNoteRequestId) {
        return;
    }

    QNWARNING("DefaultAccountFirstNotebookAndNoteCreator::"
              << "onAddNoteFailed: note = " << note
              << "\nError description = " << errorDescription
              << "\nRequest id = " << requestId);

    m_addNoteRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void DefaultAccountFirstNotebookAndNoteCreator::connectToLocalStorage(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG("DefaultAccountFirstNotebookAndNoteCreator::"
            "connectToLocalStorage");

    // Connect local signals to LocalStorageManagerAsync slots
    QObject::connect(this,
                     QNSIGNAL(DefaultAccountFirstNotebookAndNoteCreator,
                              addNotebook,Notebook,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onAddNotebookRequest,
                            Notebook,QUuid));
    QObject::connect(this,
                     QNSIGNAL(DefaultAccountFirstNotebookAndNoteCreator,
                              addNote,Note,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,
                            Note,QUuid));

    // Connect LocalStorageManagerAsync signals to local slots
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(DefaultAccountFirstNotebookAndNoteCreator,
                            onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(DefaultAccountFirstNotebookAndNoteCreator,
                            onAddNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,
                              Note,QUuid),
                     this,
                     QNSLOT(DefaultAccountFirstNotebookAndNoteCreator,
                            onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,
                              Note,ErrorString,QUuid),
                     this,
                     QNSLOT(DefaultAccountFirstNotebookAndNoteCreator,
                            onAddNoteFailed,Note,ErrorString,QUuid));
}

void DefaultAccountFirstNotebookAndNoteCreator::emitAddNotebookRequest()
{
    QNDEBUG("DefaultAccountFirstNotebookAndNoteCreator::"
            "emitAddNotebookRequest");

    QString notebookName;

    QString username = getCurrentUserName();
    if (!username.isEmpty()) {
        notebookName = tr("Notebook of user");
        notebookName += QStringLiteral(" ");
        notebookName += username;
    }
    else {
        notebookName = tr("Default notebook");
    }

    Notebook notebook;
    notebook.setName(notebookName);

    m_addNotebookRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to add first notebook to "
            << "the default account: " << notebook
            << "\nRequest id = " << m_addNotebookRequestId);
    Q_EMIT addNotebook(notebook, m_addNotebookRequestId);
}

void DefaultAccountFirstNotebookAndNoteCreator::emitAddNoteRequest(
    const Notebook & notebook)
{
    QNDEBUG("DefaultAccountFirstNotebookAndNoteCreator::"
            "emitAddNoteRequest");

    Note note;
    note.setTitle(tr("Welcome to Quentier"));

    note.setContent(
        QStringLiteral("<en-note><h3>") + note.title() +
        QStringLiteral("</h3><div>") +
        tr("You are currently using the default account which is local to this "
           "computer and doesn't synchronize with Evernote. At any time you can "
           "create another account, either another local one or Evernote one - "
           "for this use \"File\" menu -> \"Switch account\" -> \"Add account\".") +
        QStringLiteral("</div><br/><div>") +
        tr("The local account can be used precisely as Evernote account - you can "
           "create notebooks, notes, tags, move notes between notebooks, assign "
           "different tags to notes, perform and save searches of notes (Evernote "
           "search syntax is fully supported). The difference is that your data "
           "doesn't synchronize with Evernote. And another difference is that you "
           "have more freedom than when using Evernote account: you can permanently "
           "delete notes, notebooks, tags and saved searches. Permanently deleted "
           "data cannot be restored so use this freedom with care.") +
        QStringLiteral("</div></en-note>"));

    note.setNotebookLocalUid(notebook.localUid());

    m_addNoteRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to add first note to the default "
            << "account: " << note
            << "\nRequest id = " << m_addNoteRequestId);
    Q_EMIT addNote(note, m_addNoteRequestId);
}

} // namespace quentier
