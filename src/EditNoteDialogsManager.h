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

#ifndef QUENTIER_EDIT_NOTE_DIALOGS_MANAGER_H
#define QUENTIER_EDIT_NOTE_DIALOGS_MANAGER_H

#include "models/NoteCache.h"
#include <quentier/types/Note.h>
#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <QObject>
#include <QUuid>
#include <QSet>
#include <QHash>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)
QT_FORWARD_DECLARE_CLASS(NotebookModel)

class EditNoteDialogsManager: public QObject
{
    Q_OBJECT
public:
    // NOTE: as dialogs need a widget to be their parent, this class' parent must be a QWidget instance
    explicit EditNoteDialogsManager(LocalStorageManagerAsync & localStorageManagerAsync,
                                    NoteCache & noteCache, NotebookModel * pNotebookModel,
                                    QWidget * parent = Q_NULLPTR);

    void setNotebookModel(NotebookModel * pNotebookModel);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    // private signals:
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId);
    void updateNote(Note note, bool updateResources, bool updateTags, QUuid requestId);

public Q_SLOTS:
    void onEditNoteDialogRequested(QString noteLocalUid);
    void onNoteInfoDialogRequested(QString noteLocalUid);

private Q_SLOTS:
    void onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, ErrorString errorDescription, QUuid requestId);
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            ErrorString errorDescription, QUuid requestId);

private:
    void createConnections();
    void findNoteAndRaiseEditNoteDialog(const QString & noteLocalUid, const bool readOnlyFlag);
    void raiseEditNoteDialog(const Note & note, const bool readOnlyFlag);

private:
    Q_DISABLE_COPY(EditNoteDialogsManager)

private:
    LocalStorageManagerAsync &          m_localStorageManagerAsync;
    NoteCache &                         m_noteCache;

    // NOTE: the bool value in this hash is a "read only" flag for the dialog
    // which should be raised on the found note
    typedef QHash<QUuid, bool> FindNoteRequestIdToReadOnlyFlagHash;
    FindNoteRequestIdToReadOnlyFlagHash m_findNoteRequestIds;

    QSet<QUuid>                         m_updateNoteRequestIds;

    QPointer<NotebookModel>             m_pNotebookModel;
};

} // namespace quentier

#endif // QUENTIER_EDIT_NOTE_DIALOGS_MANAGER_H
