/*
 * Copyright 2017-2025 Dmitry Ivanov
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

#include <lib/exception/Utils.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/PlatformUtils.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>

#include <QTextStream>

namespace quentier {

DefaultAccountFirstNotebookAndNoteCreator::
    DefaultAccountFirstNotebookAndNoteCreator(
        local_storage::ILocalStoragePtr localStorage,
        NoteFiltersManager & noteFiltersManager, QObject * parent) :
    QObject{parent}, m_localStorage{std::move(localStorage)},
    m_noteFiltersManager{&noteFiltersManager}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{
            ErrorString{"DefaultAccountFirstNotebookAndNoteCreator: local "
                        "storage is null"}};
    }
}

void DefaultAccountFirstNotebookAndNoteCreator::start()
{
    QNDEBUG(
        "initialization::DefaultAccountFirstNotebookAndNoteCreator",
        "DefaultAccountFirstNotebookAndNoteCreator::start");

    if (Q_UNLIKELY(m_pendingNotebook)) {
        QNDEBUG(
            "initialization::DefaultAccountFirstNotebookAndNoteCreator",
            "Already pending notebook creation");
        return;
    }

    if (Q_UNLIKELY(m_pendingNote)) {
        QNDEBUG(
            "initialization::DefaultAccountFirstNotebookAndNoteCreator",
            "Already pending note creation");
        return;
    }

    createNotebook();
}

void DefaultAccountFirstNotebookAndNoteCreator::createNotebook()
{
    QNDEBUG(
        "initialization::DefaultAccountFirstNotebookAndNoteCreator",
        "DefaultAccountFirstNotebookAndNoteCreator::createNotebook");

    Q_ASSERT(!m_pendingNotebook);

    QString notebookName;

    QString username = utils::getCurrentUserName();
    if (!username.isEmpty()) {
        notebookName = tr("Notebook of user");
        notebookName += QStringLiteral(" ");
        notebookName += username;
    }
    else {
        notebookName = tr("Default notebook");
    }

    qevercloud::Notebook notebook;
    notebook.setName(notebookName);

    QNDEBUG(
        "initialization::DefaultAccountFirstNotebookAndNoteCreator",
        "Trying to create first notebook for the default account: "
            << notebook);

    m_pendingNotebook = true;

    QString notebookLocalId = notebook.localId();
    auto putNotebookFuture = m_localStorage->putNotebook(std::move(notebook));
    auto putNotebookThenFuture = threading::then(
        std::move(putNotebookFuture), this, [this, notebookLocalId] {
            QNDEBUG(
                "initialization::DefaultAccountFirstNotebookAndNoteCreator",
                "Successfully created default notebook, local id = "
                    << notebookLocalId);

            m_pendingNotebook = false;

            if (!m_noteFiltersManager.isNull()) {
                m_noteFiltersManager->setNotebooksToFilter(
                    QStringList{} << notebookLocalId);
            }

            createNote(notebookLocalId);
        });

    threading::onFailed(
        std::move(putNotebookThenFuture), this,
        [this,
         notebookLocalId = std::move(notebookLocalId)](const QException & e) {
            m_pendingNotebook = false;

            auto message = exceptionMessage(e);

            ErrorString errorDescription{
                QT_TR_NOOP("Failed to create default notebook")};
            errorDescription.appendBase(message.base());
            errorDescription.appendBase(message.additionalBases());
            errorDescription.details() = message.details();

            QNWARNING(
                "initialization::DefaultAccountFirstNotebookAndNoteCreator",
                errorDescription << ", notebook local id = "
                                 << notebookLocalId);

            Q_EMIT notifyError(std::move(errorDescription));
        });
}

void DefaultAccountFirstNotebookAndNoteCreator::createNote(
    const QString & notebookLocalId)
{
    QNDEBUG(
        "initialization::DefaultAccountFirstNotebookAndNoteCreator",
        "DefaultAccountFirstNotebookAndNoteCreator::createNote: notebook local "
            << "id = " << notebookLocalId);

    Q_ASSERT(!m_pendingNote);

    qevercloud::Note note;
    note.setTitle(tr("Welcome to Quentier"));

    QString content;
    {
        QTextStream strm{&content};
        strm << "<en-note><h3>" << *note.title() << "</h3><div>"
             << tr("You are currently using the default account which is local "
                   "to this computer and doesn't synchronize with Evernote. At "
                   "any time you can create another account, either another "
                   "local one or Evernote one - for this use \"File\" menu -> "
                   "\"Switch account\" -> \"Add account\".")
             << "</div><br/><div>"
             << tr("The local account can be used precisely as Evernote "
                   "account - you can create notebooks, notes, tags, move "
                   "notes between notebooks, assign different tags to notes, "
                   "perform and save searches of notes (Evernote search "
                   "syntax is fully supported). The difference is that your "
                   "data doesn't synchronize with Evernote. And another "
                   "difference is that you have more freedom than when using "
                   "Evernote account: you can permanently delete notes, "
                   "notebooks, tags and saved searches. Permanently deleted "
                   "data cannot be restored so use this freedom with care.")
             << "</div></en-note>";

        strm.flush();
    }

    note.setContent(std::move(content));
    note.setNotebookLocalId(notebookLocalId);

    const auto now = QDateTime::currentMSecsSinceEpoch();
    note.setCreated(now);
    note.setUpdated(now);

    QNDEBUG(
        "initialization::DefaultAccountFirstNotebookAndNoteCreator",
        "Trying to create first note for the default account: note local id = "
            << note.localId());

    QString noteLocalId = note.localId();
    auto putNoteFuture = m_localStorage->putNote(std::move(note));
    auto putNoteThenFuture =
        threading::then(std::move(putNoteFuture), this, [this, noteLocalId] {
            QNDEBUG(
                "initialization::DefaultAccountFirstNotebookAndNoteCreator",
                "Successfully created default note, local id = "
                    << noteLocalId);

            m_pendingNote = false;
            Q_EMIT finished(noteLocalId);
        });

    threading::onFailed(
        std::move(putNoteThenFuture), this,
        [this, noteLocalId](const QException & e) {
            m_pendingNote = false;

            auto message = exceptionMessage(e);

            ErrorString errorDescription{
                QT_TR_NOOP("Failed to create default note")};
            errorDescription.appendBase(message.base());
            errorDescription.appendBase(message.additionalBases());
            errorDescription.details() = message.details();

            QNWARNING(
                "initialization::DefaultAccountFirstNotebookAndNoteCreator",
                errorDescription << ", note local id = " << noteLocalId);

            Q_EMIT notifyError(std::move(errorDescription));
        });
}

} // namespace quentier
