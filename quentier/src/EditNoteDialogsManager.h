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
#include <quentier/utility/QNLocalizedString.h>
#include <QObject>
#include <QUuid>
#include <QSet>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(NotebookModel)

class EditNoteDialogsManager: public QObject
{
    Q_OBJECT
public:
    // NOTE: as dialogs need a widget to be their parent, this class' parent must be a QWidget instance
    explicit EditNoteDialogsManager(LocalStorageManagerThreadWorker & localStorageWorker,
                                    NoteCache & noteCache, NotebookModel * pNotebookModel,
                                    QWidget * parent = Q_NULLPTR);

    void setNotebookModel(NotebookModel * pNotebookModel);

Q_SIGNALS:
    void notifyError(QNLocalizedString errorDescription);

    // private signals:
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId);
    void updateNote(Note note, bool updateResources, bool updateTags, QUuid requestId);

public Q_SLOTS:
    void onEditNoteDialogRequested(QString noteLocalUid);

private Q_SLOTS:
    void onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            QNLocalizedString errorDescription, QUuid requestId);

private:
    void createConnections();
    void raiseEditNoteDialog(const Note & note);

private:
    Q_DISABLE_COPY(EditNoteDialogsManager)

private:
    LocalStorageManagerThreadWorker &   m_localStorageWorker;
    NoteCache &                         m_noteCache;

    QSet<QUuid>                         m_findNoteRequestIds;
    QSet<QUuid>                         m_updateNoteRequestIds;

    QPointer<NotebookModel>             m_pNotebookModel;
};

} // namespace quentier

#endif // QUENTIER_EDIT_NOTE_DIALOGS_MANAGER_H
