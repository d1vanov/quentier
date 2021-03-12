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

#ifndef QUENTIER_LIB_ENEX_ENEX_EXPORTER_H
#define QUENTIER_LIB_ENEX_ENEX_EXPORTER_H

#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/ErrorString.h>

#include <qevercloud/generated/types/Note.h>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QUuid>

namespace quentier {

class LocalStorageManagerAsync;
class NoteEditorTabsAndWindowsCoordinator;
class TagModel;

class EnexExporter final : public QObject
{
    Q_OBJECT
public:
    explicit EnexExporter(
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteEditorTabsAndWindowsCoordinator & coordinator, TagModel & tagModel,
        QObject * parent = nullptr);

    [[nodiscard]] const QString & targetEnexFilePath() const noexcept
    {
        return m_targetEnexFilePath;
    }

    void setTargetEnexFilePath(QString path)
    {
        m_targetEnexFilePath = std::move(path);
    }

    [[nodiscard]] const QStringList & noteLocalIds() const noexcept
    {
        return m_noteLocalIds;
    }

    void setNoteLocalIds(const QStringList & noteLocalUids);

    [[nodiscard]] bool includeTags() const noexcept
    {
        return m_includeTags;
    }

    void setIncludeTags(bool includeTags);

    [[nodiscard]] bool isInProgress() const;

    void start();

    void clear();

Q_SIGNALS:
    void notesExportedToEnex(QString enex);
    void failedToExportNotesToEnex(ErrorString errorDescription);

    // private signals:
    void findNote(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

private Q_SLOTS:
    void onFindNoteComplete(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void onFindNoteFailed(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onAllTagsListed();

private:
    void findNoteInLocalStorage(const QString & noteLocalId);
    [[nodiscard]] QString convertNotesToEnex(ErrorString & errorDescription);

    void connectToLocalStorage();
    void disconnectFromLocalStorage();

private:
    LocalStorageManagerAsync & m_localStorageManagerAsync;
    NoteEditorTabsAndWindowsCoordinator & m_noteEditorTabsAndWindowsCoordinator;
    QPointer<TagModel> m_pTagModel;
    QString m_targetEnexFilePath;
    QStringList m_noteLocalIds;
    QSet<QUuid> m_findNoteRequestIds;
    QHash<QString, qevercloud::Note> m_notesByLocalId;
    bool m_includeTags = false;
    bool m_connectedToLocalStorage = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_ENEX_ENEX_EXPORTER_H
