/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_ENEX_ENEX_IMPORTER_H
#define QUENTIER_LIB_ENEX_ENEX_IMPORTER_H

#include <quentier/types/ErrorString.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Tag.h>
#include <quentier/utility/Macros.h>
#include <quentier/utility/SuppressWarnings.h>

#include <QHash>
#include <QObject>
#include <QUuid>

SAVE_WARNINGS
GCC_SUPPRESS_WARNING(-Wdeprecated - declarations)

#include <boost/bimap.hpp>

RESTORE_WARNINGS

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)
QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(NotebookModel)

class EnexImporter : public QObject
{
    Q_OBJECT
public:
    explicit EnexImporter(
        const QString & enexFilePath, const QString & notebookName,
        LocalStorageManagerAsync & localStorageManagerAsync,
        TagModel & tagModel, NotebookModel & notebookModel,
        QObject * parent = nullptr);

    bool isInProgress() const;
    void start();

    void clear();

Q_SIGNALS:
    void enexImportedSuccessfully(QString enexFilePath);
    void enexImportFailed(ErrorString errorDescription);

    // private signals:
    void addTag(Tag tag, QUuid requestId);
    void addNotebook(Notebook notebook, QUuid requestId);
    void addNote(Note note, QUuid requestId);

private Q_SLOTS:
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

    void onExpungeTagComplete(
        Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId);

    void onAddNotebookComplete(Notebook notebook, QUuid requestId);

    void onAddNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onAddNoteComplete(Note note, QUuid requestId);

    void onAddNoteFailed(
        Note note, ErrorString errorDescription, QUuid requestId);

    void onAllTagsListed();
    void onAllNotebooksListed();

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    void processNotesPendingTagAddition();

    void addNoteToLocalStorage(const Note & note);
    void addTagToLocalStorage(const QString & tagName);
    void addNotebookToLocalStorage(const QString & notebookName);

private:
    LocalStorageManagerAsync & m_localStorageManagerAsync;
    TagModel & m_tagModel;
    NotebookModel & m_notebookModel;
    QString m_enexFilePath;
    QString m_notebookName;
    QString m_notebookLocalUid;

    QHash<QString, QStringList> m_tagNamesByImportedNoteLocalUid;

    using AddTagRequestIdByTagNameBimap = boost::bimap<QString, QUuid>;
    AddTagRequestIdByTagNameBimap m_addTagRequestIdByTagNameBimap;

    QSet<QString> m_expungedTagLocalUids;

    QUuid m_addNotebookRequestId;

    QVector<Note> m_notesPendingTagAddition;
    QSet<QUuid> m_addNoteRequestIds;

    bool m_pendingNotebookModelToStart = false;
    bool m_connectedToLocalStorage = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_ENEX_ENEX_IMPORTER_H
