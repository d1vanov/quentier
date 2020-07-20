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

#include "EnexExporter.h"

#include <lib/model/tag/TagModel.h>
#include <lib/widget/NoteEditorTabsAndWindowsCoordinator.h>
#include <lib/widget/NoteEditorWidget.h>

#include <quentier/enml/ENMLConverter.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

#include <QVector>

#define QUENTIER_ENEX_VERSION QStringLiteral("Quentier")

namespace quentier {

EnexExporter::EnexExporter(
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteEditorTabsAndWindowsCoordinator & coordinator,
        TagModel & tagModel, QObject * parent) :
    QObject(parent),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_noteEditorTabsAndWindowsCoordinator(coordinator),
    m_pTagModel(&tagModel)
{
    if (!tagModel.allTagsListed()) {
        QObject::connect(
            &tagModel,
            &TagModel::notifyAllTagsListed,
            this,
            &EnexExporter::onAllTagsListed);
    }
}

void EnexExporter::setNoteLocalUids(const QStringList & noteLocalUids)
{
    QNDEBUG("enex", "EnexExporter::setNoteLocalUids: "
        << noteLocalUids.join(QStringLiteral(", ")));

    if (isInProgress()) {
        clear();
    }

    m_noteLocalUids = noteLocalUids;
}

void EnexExporter::setIncludeTags(const bool includeTags)
{
    QNDEBUG("enex", "EnexExporter::setIncludeTags: "
        << (includeTags ? "true" : "false"));

    if (m_includeTags == includeTags) {
        QNDEBUG("enex", "The setting has not changed, won't do anything");
        return;
    }

    if (isInProgress()) {
        clear();
    }

    m_includeTags = includeTags;
}

bool EnexExporter::isInProgress() const
{
    QNDEBUG("enex", "EnexExporter::isInProgress");

    if (m_noteLocalUids.isEmpty()) {
        QNDEBUG("enex", "No note local uids are set");
        return false;
    }

    if (m_findNoteRequestIds.isEmpty()) {
        QNDEBUG("enex", "No pending requests to find notes in the local "
            << "storage");
        return false;
    }

    return true;
}

void EnexExporter::start()
{
    QNDEBUG("enex", "EnexExporter::start");

    if (m_noteLocalUids.isEmpty()) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't export note to ENEX: no note local uids were "
                       "specified"));
        QNWARNING("enex", errorDescription);
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    if (m_includeTags && m_pTagModel.isNull()) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't export note to ENEX: tag model is deleted"));
        QNWARNING("enex", errorDescription);
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    m_findNoteRequestIds.clear();
    m_notesByLocalUid.clear();

    for(const auto & noteLocalUid: qAsConst(m_noteLocalUids))
    {
        auto * pNoteEditorWidget =
            m_noteEditorTabsAndWindowsCoordinator.noteEditorWidgetForNoteLocalUid(
                noteLocalUid);

        if (!pNoteEditorWidget) {
            QNTRACE("enex", "Found no note editor widget for note local uid "
                << noteLocalUid);
            findNoteInLocalStorage(noteLocalUid);
            continue;
        }

        QNTRACE("enex", "Found note editor with loaded note "
            << noteLocalUid);

        const auto * pNote = pNoteEditorWidget->currentNote();
        if (Q_UNLIKELY(!pNote)) {
            QNDEBUG("enex", "There is no note in the editor, will try to "
                << "find it in the local storage");
            findNoteInLocalStorage(noteLocalUid);
            continue;
        }

        if (!pNoteEditorWidget->isModified()) {
            QNTRACE("enex", "Fetched the unmodified note from editor: "
                << noteLocalUid);
            m_notesByLocalUid[noteLocalUid] = *pNote;
            continue;
        }

        QNTRACE("enex", "The note within the editor was modified, saving it");

        ErrorString noteSavingError;

        auto saveStatus = pNoteEditorWidget->checkAndSaveModifiedNote(
            noteSavingError);

        if (saveStatus != NoteEditorWidget::NoteSaveStatus::Ok)
        {
            QNWARNING("enex", "Could not save the note loaded into the editor: "
                << "status = " << saveStatus << ", error: " << noteSavingError
                << "; will try to find the note in the local storage");
            findNoteInLocalStorage(noteLocalUid);
            continue;
        }

        pNote = pNoteEditorWidget->currentNote();
        if (Q_UNLIKELY(!pNote))
        {
            QNWARNING("enex", "Note editor's current note has unexpectedly "
                << "become nullptr after the note has been saved; "
                << "will try to find the note in the local storage");
            findNoteInLocalStorage(noteLocalUid);
            continue;
        }

        QNTRACE("enex", "Fetched the modified & saved note from editor: "
            << noteLocalUid);

        m_notesByLocalUid[noteLocalUid] = *pNote;
    }

    if (!m_findNoteRequestIds.isEmpty()) {
        QNDEBUG("enex", "Not all requested notes were found loaded into "
            << "the editors, currently pending " << m_findNoteRequestIds.size()
            << " find note in local storage requests ");
        return;
    }

    QNDEBUG("enex", "All requested notes were found loaded into the editors "
        << "and were successfully gathered from them");

    if (m_includeTags && !m_pTagModel->allTagsListed()) {
        QNDEBUG("enex", "Waiting for the tag model to get all tags listed");
        return;
    }

    ErrorString errorDescription;
    QString enex = convertNotesToEnex(errorDescription);
    if (enex.isEmpty()) {
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    Q_EMIT notesExportedToEnex(enex);
}

void EnexExporter::clear()
{
    QNDEBUG("enex", "EnexExporter::clear");

    m_targetEnexFilePath.clear();
    m_noteLocalUids.clear();
    m_findNoteRequestIds.clear();
    m_notesByLocalUid.clear();

    disconnectFromLocalStorage();
    m_connectedToLocalStorage = false;
}

void EnexExporter::onFindNoteComplete(
    Note note, LocalStorageManager::GetNoteOptions options, QUuid requestId)
{
    auto it = m_findNoteRequestIds.find(requestId);
    if (it == m_findNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("enex", "EnexExporter::onFindNoteComplete: request id = "
        << requestId << ", note: " << note);

    Q_UNUSED(options)

    m_notesByLocalUid[note.localUid()] = note;
    m_findNoteRequestIds.erase(it);

    if (!m_findNoteRequestIds.isEmpty()) {
        QNDEBUG("enex", "Still pending  " << m_findNoteRequestIds.size()
            << " find note in local storage requests ");
        return;
    }

    if (m_includeTags)
    {
        if (Q_UNLIKELY(m_pTagModel.isNull()))
        {
            ErrorString errorDescription(
                QT_TR_NOOP("Can't export note(s) to ENEX: tag model is "
                           "deleted"));
            QNWARNING("enex", errorDescription);
            clear();
            Q_EMIT failedToExportNotesToEnex(errorDescription);
            return;
        }

        if (!m_pTagModel->allTagsListed()) {
            QNDEBUG("enex", "Not all tags were listed within the tag model yet");
            return;
        }
    }

    ErrorString errorDescription;
    QString enex = convertNotesToEnex(errorDescription);
    if (enex.isEmpty()) {
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    Q_EMIT notesExportedToEnex(enex);
}

void EnexExporter::onFindNoteFailed(
    Note note, LocalStorageManager::GetNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    auto it = m_findNoteRequestIds.find(requestId);
    if (it == m_findNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("enex", "EnexExporter::onFindNoteFailed: request id = "
        << requestId << ", error: " << errorDescription
        << ", note: " << note);

    Q_UNUSED(options)

    ErrorString error(
        QT_TR_NOOP("Can't export note(s) to ENEX: can't find one "
                   "of notes in the local storage"));

    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    QNWARNING("enex", error);

    clear();
    Q_EMIT failedToExportNotesToEnex(error);
}

void EnexExporter::onAllTagsListed()
{
    QNDEBUG("enex", "EnexExporter::onAllTagsListed");

    QObject::disconnect(
        m_pTagModel.data(),
        &TagModel::notifyAllTagsListed,
        this,
        &EnexExporter::onAllTagsListed);

    if (m_noteLocalUids.isEmpty()) {
        QNDEBUG("enex", "No note local uids are specified, won't do anything");
        return;
    }

    if (!m_findNoteRequestIds.isEmpty()) {
        QNDEBUG("enex", "Still pending  " << m_findNoteRequestIds.size()
            << " find note in local storage requests ");
        return;
    }

    ErrorString errorDescription;
    QString enex = convertNotesToEnex(errorDescription);
    if (enex.isEmpty()) {
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    Q_EMIT notesExportedToEnex(enex);
}

void EnexExporter::findNoteInLocalStorage(const QString & noteLocalUid)
{
    QNDEBUG("enex", "EnexExporter::findNoteInLocalStorage: " << noteLocalUid);

    Note dummyNote;
    dummyNote.setLocalUid(noteLocalUid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteRequestIds.insert(requestId))

    connectToLocalStorage();

    QNTRACE("enex", "Emitting the request to find note in the local storage: "
        << "note local uid = " << noteLocalUid
        << ", request id = " << requestId);

    LocalStorageManager::GetNoteOptions options(
        LocalStorageManager::GetNoteOption::WithResourceMetadata |
        LocalStorageManager::GetNoteOption::WithResourceBinaryData);

    Q_EMIT findNote(dummyNote, options, requestId);
}

QString EnexExporter::convertNotesToEnex(ErrorString & errorDescription)
{
    QNDEBUG("enex", "EnexExporter::convertNotesToEnex");

    if (m_notesByLocalUid.isEmpty()) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't export notes to ENEX: no notes "
                       "were specified or found"));
        QNWARNING("enex", errorDescription);
        return QString();
    }

    if (m_includeTags && m_pTagModel.isNull()) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't export notes to ENEX: "
                       "tag model is deleted"));
        QNWARNING("enex", errorDescription);
        return QString();
    }

    QVector<Note> notes;
    notes.reserve(m_notesByLocalUid.size());

    QHash<QString, QString> tagNameByTagLocalUid;
    for(auto it = m_notesByLocalUid.constBegin(),
        end = m_notesByLocalUid.constEnd(); it != end; ++it)
    {
        const auto & currentNote = it.value();

        if (m_includeTags && currentNote.hasTagLocalUids())
        {
            const auto & tagLocalUids = currentNote.tagLocalUids();
            for(auto tagIt = tagLocalUids.constBegin(),
                tagEnd = tagLocalUids.constEnd(); tagIt != tagEnd; ++tagIt)
            {
                const auto * pModelItem = m_pTagModel->itemForLocalUid(*tagIt);
                if (Q_UNLIKELY(!pModelItem))
                {
                    errorDescription.setBase(
                        QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                                   "detected note with tag local uid for which "
                                   "no tag model item was found"));

                    QNWARNING("enex", errorDescription
                        << ", tag local uid = " << *tagIt
                        << ", note: " << currentNote);
                    return {};
                }

                const auto * pTagItem = pModelItem->cast<TagItem>();
                if (Q_UNLIKELY(!pTagItem))
                {
                    errorDescription.setBase(
                        QT_TR_NOOP("Can't export notes to ENEX: internal "
                                   "error, detected tag model item "
                                   "corresponding to tag local uid but not of "
                                   "a tag type"));

                    QNWARNING("enex", errorDescription
                        << ", tag local uid = " << *tagIt
                        << ", tag model item: " << *pModelItem
                        << "\nNote: " << currentNote);
                    return {};
                }

                tagNameByTagLocalUid[*tagIt] = pTagItem->name();
            }
        }

        notes << currentNote;
    }

    QString enex;
    ENMLConverter converter;

    auto exportTagsOption = (m_includeTags
        ? ENMLConverter::EnexExportTags::Yes
        : ENMLConverter::EnexExportTags::No);

    bool res = converter.exportNotesToEnex(
        notes,
        tagNameByTagLocalUid,
        exportTagsOption,
        enex,
        errorDescription,
        QUENTIER_ENEX_VERSION);

    if (!res) {
        return QString();
    }

    QNDEBUG("enex", "Successfully exported note(s) to ENEX");
    return enex;
}

void EnexExporter::connectToLocalStorage()
{
    QNDEBUG("enex", "EnexExporter::connectToLocalStorage");

    if (m_connectedToLocalStorage) {
        QNTRACE("enex", "Already connected to local storage");
        return;
    }

    QObject::connect(
        this,
        &EnexExporter::findNote,
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNoteRequest);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteComplete,
        this,
        &EnexExporter::onFindNoteComplete);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteFailed,
        this,
        &EnexExporter::onFindNoteFailed);

    m_connectedToLocalStorage = true;
}

void EnexExporter::disconnectFromLocalStorage()
{
    QNDEBUG("enex", "EnexExporter::disconnectFromLocalStorage");

    if (!m_connectedToLocalStorage) {
        QNTRACE("enex", "Not connected to local storage at the moment");
        return;
    }

    QObject::disconnect(
        this,
        &EnexExporter::findNote,
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNoteRequest);

    QObject::disconnect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteComplete,
        this,
        &EnexExporter::onFindNoteComplete);

    QObject::disconnect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteFailed,
        this,
        &EnexExporter::onFindNoteFailed);

    m_connectedToLocalStorage = false;
}

} // namespace quentier
