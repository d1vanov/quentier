/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/Note.h>
#include <quentier/local_storage/LocalStorageManager.h>

#include <QObject>
#include <QStringList>
#include <QSet>
#include <QHash>
#include <QUuid>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)
QT_FORWARD_DECLARE_CLASS(NoteEditorTabsAndWindowsCoordinator)
QT_FORWARD_DECLARE_CLASS(TagModel)

class EnexExporter: public QObject
{
    Q_OBJECT
public:
    explicit EnexExporter(
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteEditorTabsAndWindowsCoordinator & coordinator,
        TagModel & tagModel, QObject * parent = nullptr);

    const QString & targetEnexFilePath() const
    { return m_targetEnexFilePath; }

    void setTargetEnexFilePath(const QString & path)
    { m_targetEnexFilePath = path; }

    const QStringList & noteLocalUids() const
    { return m_noteLocalUids; }

    void setNoteLocalUids(const QStringList & noteLocalUids);

    bool includeTags() const
    { return m_includeTags; }

    void setIncludeTags(const bool includeTags);

    bool isInProgress() const;
    void start();

    void clear();

Q_SIGNALS:
    void notesExportedToEnex(QString enex);
    void failedToExportNotesToEnex(ErrorString errorDescription);

// private signals:
    void findNote(Note note, LocalStorageManager::GetNoteOptions options,
                  QUuid requestId);

private Q_SLOTS:
    void onFindNoteComplete(Note note, LocalStorageManager::GetNoteOptions options,
                            QUuid requestId);
    void onFindNoteFailed(Note note, LocalStorageManager::GetNoteOptions options,
                          ErrorString errorDescription, QUuid requestId);

    void onAllTagsListed();

private:
    void findNoteInLocalStorage(const QString & noteLocalUid);
    QString convertNotesToEnex(ErrorString & errorDescription);

    void connectToLocalStorage();
    void disconnectFromLocalStorage();

private:
    LocalStorageManagerAsync &              m_localStorageManagerAsync;
    NoteEditorTabsAndWindowsCoordinator &   m_noteEditorTabsAndWindowsCoordinator;
    QPointer<TagModel>                      m_pTagModel;
    QString                                 m_targetEnexFilePath;
    QStringList                             m_noteLocalUids;
    QSet<QUuid>                             m_findNoteRequestIds;
    QHash<QString, Note>                    m_notesByLocalUid;
    bool                                    m_includeTags;
    bool                                    m_connectedToLocalStorage;
};

} // namespace quentier

#endif // QUENTIER_LIB_ENEX_ENEX_EXPORTER_H
