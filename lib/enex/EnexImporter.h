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

#include <quentier/enml/Fwd.h>
#include <quentier/local_storage/Fwd.h>
#include <quentier/types/ErrorString.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/Tag.h>

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>

namespace quentier {

class TagModel;
class NotebookModel;

class EnexImporter final : public QObject
{
    Q_OBJECT
public:
    EnexImporter(
        QString enexFilePath, QString notebookName,
        local_storage::ILocalStoragePtr localStorage,
        enml::IConverterPtr enmlConverter,
        TagModel & tagModel, NotebookModel & notebookModel,
        QObject * parent = nullptr);

    [[nodiscard]] bool isInProgress() const;
    void start();
    void clear();

Q_SIGNALS:
    void enexImportedSuccessfully(QString enexFilePath);
    void enexImportFailed(ErrorString errorDescription);

private Q_SLOTS:
    void onAllTagsListed();
    void onAllNotebooksListed();

    void onTagExpunged(
        QString tagLocalId, QStringList expungedChildTagLocalIds);

    void onTargetNotebookExpunged();

private:
    void onTagPutToLocalStorage(const QString & tagName);
    void onFailedToPutTagToLocalStorage(
        QString tagName, ErrorString errorDescription);

    void onNotebookPutToLocalStorage(const QString & notebookLocalId);
    void onFailedToPutNotebookToLocalStorage(ErrorString errorDescription);

    void onNotePutToLocalStorage(const QString & noteLocalId);
    void onFailedToPutNoteToLocalStorage(ErrorString errorDescription);

private:
    void processNotesPendingTagAddition();

    void putNoteToLocalStorage(qevercloud::Note note);
    void putTagToLocalStorage(const QString & tagName);
    void putNotebookToLocalStorage();

private:
    const QString m_enexFilePath;
    const QString m_notebookName;
    const local_storage::ILocalStoragePtr m_localStorage;
    const enml::IConverterPtr m_enmlConverter;
    TagModel & m_tagModel;
    NotebookModel & m_notebookModel;

    QString m_notebookLocalId;

    QHash<QString, QStringList> m_tagNamesByImportedNoteLocalIds;
    QSet<QString> m_tagNamesPendingTagPutToLocalStorage;
    QSet<QString> m_expungedTagLocalIds;

    QList<qevercloud::Note> m_notesPendingTagAddition;
    QSet<QString> m_noteLocalIdsPendingNotePutToLocalStorage;

    bool m_pendingNotebookModelToStart = false;
    bool m_pendingNotebookPutToLocalStorage = false;
};

} // namespace quentier
