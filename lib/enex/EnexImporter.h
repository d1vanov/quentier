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

#ifndef QUENTIER_LIB_ENEX_ENEX_IMPORTER_H
#define QUENTIER_LIB_ENEX_ENEX_IMPORTER_H

#include <quentier/types/ErrorString.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/Tag.h>

#include <QHash>
#include <QObject>
#include <QUuid>

#include <boost/bimap.hpp>

namespace quentier {

class LocalStorageManagerAsync;
class TagModel;
class NotebookModel;

class EnexImporter final : public QObject
{
    Q_OBJECT
public:
    explicit EnexImporter(
        QString enexFilePath, QString notebookName,
        LocalStorageManagerAsync & localStorageManagerAsync,
        TagModel & tagModel, NotebookModel & notebookModel,
        QObject * parent = nullptr);

    ~EnexImporter() override;

    [[nodiscard]] bool isInProgress() const;
    void start();

    void clear();

Q_SIGNALS:
    void enexImportedSuccessfully(QString enexFilePath);
    void enexImportFailed(ErrorString errorDescription);

    // private signals:
    void addTag(qevercloud::Tag tag, QUuid requestId);
    void addNotebook(qevercloud::Notebook notebook, QUuid requestId);
    void addNote(qevercloud::Note note, QUuid requestId);

private Q_SLOTS:
    void onAddTagComplete(qevercloud::Tag tag, QUuid requestId);
    void onAddTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onExpungeTagComplete(
        qevercloud::Tag tag, QStringList expungedChildTagLocalIds,
        QUuid requestId);

    void onAddNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onAddNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);

    void onAddNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

    void onAllTagsListed();
    void onAllNotebooksListed();

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    void processNotesPendingTagAddition();

    void addNoteToLocalStorage(const qevercloud::Note & note);
    void addTagToLocalStorage(const QString & tagName);
    void addNotebookToLocalStorage(const QString & notebookName);

private:
    LocalStorageManagerAsync & m_localStorageManagerAsync;
    TagModel & m_tagModel;
    NotebookModel & m_notebookModel;
    QString m_enexFilePath;
    QString m_notebookName;
    QString m_notebookLocalId;

    QHash<QString, QStringList> m_tagNamesByImportedNoteLocalId;

    using AddTagRequestIdByTagNameBimap = boost::bimap<QString, QUuid>;
    AddTagRequestIdByTagNameBimap m_addTagRequestIdByTagNameBimap;

    QSet<QString> m_expungedTagLocalIds;

    QUuid m_addNotebookRequestId;

    QList<qevercloud::Note> m_notesPendingTagAddition;
    QSet<QUuid> m_addNoteRequestIds;

    bool m_pendingNotebookModelToStart = false;
    bool m_connectedToLocalStorage = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_ENEX_ENEX_IMPORTER_H
