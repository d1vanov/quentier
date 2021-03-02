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

#ifndef QUENTIER_LIB_INITIALIZATION_DEFAULT_ACCOUNT_FIRST_NOTEBOOK_AND_NOTE_CREATOR_H
#define QUENTIER_LIB_INITIALIZATION_DEFAULT_ACCOUNT_FIRST_NOTEBOOK_AND_NOTE_CREATOR_H

#include <quentier/types/ErrorString.h>

#include <qevercloud/generated/types/Note.h>
#include <qevercloud/generated/types/Notebook.h>

#include <QObject>
#include <QPointer>
#include <QUuid>

namespace quentier {

class LocalStorageManagerAsync;
class NoteFiltersManager;

/**
 * @brief The DefaultAccountFirstNotebookAndNoteCreator class encapsulates
 * a couple of asynchronous events related to setting up the default notebook
 * and note for the newly created default account
 */
class DefaultAccountFirstNotebookAndNoteCreator final : public QObject
{
    Q_OBJECT
public:
    explicit DefaultAccountFirstNotebookAndNoteCreator(
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteFiltersManager & noteFiltersManager, QObject * parent = nullptr);

    ~DefaultAccountFirstNotebookAndNoteCreator() override;

Q_SIGNALS:
    void finished(QString createdNoteLocalUid);
    void notifyError(ErrorString errorDescription);

    // private signals
    void addNotebook(qevercloud::Notebook notebook, QUuid requestId);
    void addNote(qevercloud::Note note, QUuid requestId);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onAddNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onAddNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);

    void onAddNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

private:
    void connectToLocalStorage(
        LocalStorageManagerAsync & localStorageManagerAsync);

    void emitAddNotebookRequest();
    void emitAddNoteRequest(const qevercloud::Notebook & notebook);

private:
    QUuid m_addNotebookRequestId;
    QUuid m_addNoteRequestId;

    QPointer<NoteFiltersManager> m_pNoteFiltersManager;
};

} // namespace quentier

#endif // QUENTIER_LIB_INITIALIZATION_DEFAULT_ACCOUNT_FIRST_NOTEBOOK_AND_NOTE_CREATOR_H
