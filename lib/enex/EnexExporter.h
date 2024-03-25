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

#include <qevercloud/types/Note.h>
#include <quentier/utility/cancelers/Fwd.h>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QStringList>

namespace quentier {

class NoteEditorTabsAndWindowsCoordinator;
class TagModel;

class EnexExporter final : public QObject
{
    Q_OBJECT
public:
    EnexExporter(
        local_storage::ILocalStoragePtr localStorage,
        NoteEditorTabsAndWindowsCoordinator & coordinator,
        TagModel & tagModel,
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

    void setNoteLocalIds(QStringList noteLocalIds);

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

private Q_SLOTS:
    void onAllTagsListed();

private:
    void findNoteInLocalStorage(const QString & noteLocalId);
    void onNoteFoundInLocalStorage(const qevercloud::Note & note);
    void onNoteNotFoundInLocalStorage(QString noteLocalId);
    void onFailedToFindNoteInLocalStorage(ErrorString errorDescription);

    [[nodiscard]] QString convertNotesToEnex(ErrorString & errorDescription);

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    NoteEditorTabsAndWindowsCoordinator & m_noteEditorTabsAndWindowsCoordinator;
    QPointer<TagModel> m_tagModel;
    QString m_targetEnexFilePath;
    QStringList m_noteLocalIds;
    QHash<QString, qevercloud::Note> m_notesByLocalId;
    QSet<QString> m_noteLocalIdsPendingFindInLocalStorage;
    utility::cancelers::ManualCancelerPtr m_findNotesInLocalStorageCanceler;
    bool m_includeTags = false;
};

} // namespace quentier
