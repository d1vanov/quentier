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

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/ErrorString.h>

#include <qevercloud/types/Fwd.h>

#include <QObject>
#include <QPointer>

namespace quentier {

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
        local_storage::ILocalStoragePtr localStorage,
        NoteFiltersManager & noteFiltersManager, QObject * parent = nullptr);

Q_SIGNALS:
    void finished(QString createdNoteLocalUid);
    void notifyError(ErrorString errorDescription);

public Q_SLOTS:
    void start();

private:
    void createNotebook();
    void createNote(const QString & notebookLocalId);

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    QPointer<NoteFiltersManager> m_noteFiltersManager;
    bool m_pendingNotebook = false;
    bool m_pendingNote = false;
};

} // namespace quentier
