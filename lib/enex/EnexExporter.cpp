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

#include "EnexExporter.h"

#include <lib/model/TagModel.h>
#include <lib/widget/NoteEditorTabsAndWindowsCoordinator.h>
#include <lib/widget/NoteEditorWidget.h>

#include <quentier/enml/ENMLConverter.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

#include <QVector>

#define QUENTIER_ENEX_VERSION QStringLiteral("Quentier")

namespace quentier {

EnexExporter::EnexExporter(LocalStorageManagerAsync & localStorageManagerAsync,
                           NoteEditorTabsAndWindowsCoordinator & coordinator,
                           TagModel & tagModel, QObject * parent) :
    QObject(parent),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_noteEditorTabsAndWindowsCoordinator(coordinator),
    m_pTagModel(&tagModel),
    m_noteLocalUids(),
    m_findNoteRequestIds(),
    m_notesByLocalUid(),
    m_includeTags(),
    m_connectedToLocalStorage(false)
{
    if (!tagModel.allTagsListed()) {
        QObject::connect(&tagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                         this, QNSLOT(EnexExporter,onAllTagsListed));
    }
}

void EnexExporter::setNoteLocalUids(const QStringList & noteLocalUids)
{
    QNDEBUG(QStringLiteral("EnexExporter::setNoteLocalUids: ")
            << noteLocalUids.join(QStringLiteral(", ")));

    if (isInProgress()) {
        clear();
    }

    m_noteLocalUids = noteLocalUids;
}

void EnexExporter::setIncludeTags(const bool includeTags)
{
    QNDEBUG(QStringLiteral("EnexExporter::setIncludeTags: ")
            << (includeTags ? QStringLiteral("true") : QStringLiteral("false")));

    if (m_includeTags == includeTags) {
        QNDEBUG(QStringLiteral("The setting has not changed, won't do anything"));
        return;
    }

    if (isInProgress()) {
        clear();
    }

    m_includeTags = includeTags;
}

bool EnexExporter::isInProgress() const
{
    QNDEBUG(QStringLiteral("EnexExporter::isInProgress"));

    if (m_noteLocalUids.isEmpty()) {
        QNDEBUG(QStringLiteral("No note local uids are set"));
        return false;
    }

    if (m_findNoteRequestIds.isEmpty()) {
        QNDEBUG(QStringLiteral("No pending requests to find notes in the local "
                               "storage"));
        return false;
    }

    return true;
}

void EnexExporter::start()
{
    QNDEBUG(QStringLiteral("EnexExporter::start"));

    if (m_noteLocalUids.isEmpty()) {
        ErrorString errorDescription(QT_TR_NOOP("Can't export note to ENEX: no "
                                                "note local uids were specified"));
        QNWARNING(errorDescription);
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    if (m_includeTags && m_pTagModel.isNull()) {
        ErrorString errorDescription(QT_TR_NOOP("Can't export note to ENEX: "
                                                "the tag model has expired"));
        QNWARNING(errorDescription);
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    m_findNoteRequestIds.clear();
    m_notesByLocalUid.clear();

    for(auto it = m_noteLocalUids.constBegin(),
        end = m_noteLocalUids.constEnd(); it != end; ++it)
    {
        const QString & noteLocalUid = *it;

        NoteEditorWidget * pNoteEditorWidget =
            m_noteEditorTabsAndWindowsCoordinator.noteEditorWidgetForNoteLocalUid(
                noteLocalUid);
        if (!pNoteEditorWidget) {
            QNTRACE(QStringLiteral("Found no note editor widget for note local uid ")
                    << noteLocalUid);
            findNoteInLocalStorage(noteLocalUid);
            continue;
        }

        QNTRACE(QStringLiteral("Found note editor with loaded note ")
                << noteLocalUid);

        const Note * pNote = pNoteEditorWidget->currentNote();
        if (Q_UNLIKELY(!pNote)) {
            QNDEBUG(QStringLiteral("There is no note in the editor, will try to "
                                   "find it in the local storage"));
            findNoteInLocalStorage(noteLocalUid);
            continue;
        }

        if (!pNoteEditorWidget->isModified()) {
            QNTRACE(QStringLiteral("Fetched the unmodified note from editor: ")
                    << noteLocalUid);
            m_notesByLocalUid[noteLocalUid] = *pNote;
            continue;
        }

        QNTRACE(QStringLiteral("The note within the editor was modified, saving it"));

        ErrorString noteSavingError;
        NoteEditorWidget::NoteSaveStatus::type saveStatus =
            pNoteEditorWidget->checkAndSaveModifiedNote(noteSavingError);
        if (saveStatus != NoteEditorWidget::NoteSaveStatus::Ok)
        {
            QNWARNING(QStringLiteral("Could not save the note loaded into ")
                      << QStringLiteral("the editor: status = ") << saveStatus
                      << QStringLiteral(", error: ") << noteSavingError
                      << QStringLiteral("; will try to find the note in the local ")
                      << QStringLiteral("storage"));
            findNoteInLocalStorage(noteLocalUid);
            continue;
        }

        pNote = pNoteEditorWidget->currentNote();
        if (Q_UNLIKELY(!pNote))
        {
            QNWARNING(QStringLiteral("Note editor's current note has unexpectedly "
                                     "become nullptr after the note has been saved; "
                                     "will try to find the note in the local storage"));
            findNoteInLocalStorage(noteLocalUid);
            continue;
        }

        QNTRACE(QStringLiteral("Fetched the modified & saved note from editor: ")
                << noteLocalUid);
        m_notesByLocalUid[noteLocalUid] = *pNote;
    }

    if (!m_findNoteRequestIds.isEmpty()) {
        QNDEBUG(QStringLiteral("Not all requested notes were found loaded into ")
                << QStringLiteral("the editors, currently pending ")
                << m_findNoteRequestIds.size()
                << QStringLiteral(" find note in local storage requests "));
        return;
    }

    QNDEBUG(QStringLiteral("All requested notes were found loaded into the editors "
                           "and were successfully gathered from them"));

    if (m_includeTags && !m_pTagModel->allTagsListed()) {
        QNDEBUG(QStringLiteral("Waiting for the tag model to get all tags listed"));
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
    QNDEBUG(QStringLiteral("EnexExporter::clear"));

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

    QNDEBUG(QStringLiteral("EnexExporter::onFindNoteComplete: request id = ")
            << requestId << QStringLiteral(", note: ") << note);

    Q_UNUSED(options)

    m_notesByLocalUid[note.localUid()] = note;
    m_findNoteRequestIds.erase(it);

    if (!m_findNoteRequestIds.isEmpty()) {
        QNDEBUG(QStringLiteral("Still pending  ") << m_findNoteRequestIds.size()
                << QStringLiteral(" find note in local storage requests "));
        return;
    }

    if (m_includeTags)
    {
        if (Q_UNLIKELY(m_pTagModel.isNull())) {
            ErrorString errorDescription(QT_TR_NOOP("Can't export note(s) to ENEX: "
                                                    "the tag model has expired"));
            QNWARNING(errorDescription);
            clear();
            Q_EMIT failedToExportNotesToEnex(errorDescription);
            return;
        }

        if (!m_pTagModel->allTagsListed()) {
            QNDEBUG(QStringLiteral("Not all tags were listed within the tag "
                                   "model yet"));
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

    QNDEBUG(QStringLiteral("EnexExporter::onFindNoteFailed: request id = ")
            << requestId << QStringLiteral(", error: ") << errorDescription
            << QStringLiteral(", note: ") << note);

    Q_UNUSED(options)

    ErrorString error(QT_TR_NOOP("Can't export note(s) to ENEX: can't find one "
                                 "of notes in the local storage"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    QNWARNING(error);

    clear();
    Q_EMIT failedToExportNotesToEnex(error);
}

void EnexExporter::onAllTagsListed()
{
    QNDEBUG(QStringLiteral("EnexExporter::onAllTagsListed"));

    QObject::disconnect(m_pTagModel.data(), QNSIGNAL(TagModel,notifyAllTagsListed),
                        this, QNSLOT(EnexExporter,onAllTagsListed));

    if (m_noteLocalUids.isEmpty()) {
        QNDEBUG(QStringLiteral("No note local uids are specified, won't do anything"));
        return;
    }

    if (!m_findNoteRequestIds.isEmpty()) {
        QNDEBUG(QStringLiteral("Still pending  ") << m_findNoteRequestIds.size()
                << QStringLiteral(" find note in local storage requests "));
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
    QNDEBUG(QStringLiteral("EnexExporter::findNoteInLocalStorage: ")
            << noteLocalUid);

    Note dummyNote;
    dummyNote.setLocalUid(noteLocalUid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteRequestIds.insert(requestId))

    connectToLocalStorage();

    QNTRACE(QStringLiteral("Emitting the request to find note in the local storage: ")
            << QStringLiteral("note local uid = ") << noteLocalUid
            << QStringLiteral(", request id = ") << requestId);
    LocalStorageManager::GetNoteOptions options(
        LocalStorageManager::GetNoteOption::WithResourceMetadata |
        LocalStorageManager::GetNoteOption::WithResourceBinaryData);
    Q_EMIT findNote(dummyNote, options, requestId);
}

QString EnexExporter::convertNotesToEnex(ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("EnexExporter::convertNotesToEnex"));

    if (m_notesByLocalUid.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Can't export notes to ENEX: no notes "
                                            "were specified or found"));
        QNWARNING(errorDescription);
        return QString();
    }

    if (m_includeTags && m_pTagModel.isNull()) {
        errorDescription.setBase(QT_TR_NOOP("Can't export notes to ENEX: "
                                            "the tag model has expired"));
        QNWARNING(errorDescription);
        return QString();
    }

    QVector<Note> notes;
    notes.reserve(m_notesByLocalUid.size());

    QHash<QString, QString> tagNameByTagLocalUid;
    for(auto it = m_notesByLocalUid.constBegin(),
        end = m_notesByLocalUid.constEnd(); it != end; ++it)
    {
        const Note & currentNote = it.value();

        if (m_includeTags && currentNote.hasTagLocalUids())
        {
            const QStringList & tagLocalUids = currentNote.tagLocalUids();
            for(auto tagIt = tagLocalUids.constBegin(),
                tagEnd = tagLocalUids.constEnd(); tagIt != tagEnd; ++tagIt)
            {
                const TagModelItem * pModelItem =
                    m_pTagModel->itemForLocalUid(*tagIt);
                if (Q_UNLIKELY(!pModelItem))
                {
                    errorDescription.setBase(
                        QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                                   "detected note with tag local uid for which "
                                   "no tag model item was found"));
                    QNWARNING(errorDescription
                              << QStringLiteral(", tag local uid = ") << *tagIt
                              << QStringLiteral(", note: ") << currentNote);
                    return QString();
                }

                if (Q_UNLIKELY(pModelItem->type() != TagModelItem::Type::Tag))
                {
                    errorDescription.setBase(
                        QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                                   "detected tag model item corresponding to tag "
                                   "local uid but not of a tag type"));
                    QNWARNING(errorDescription
                              << QStringLiteral(", tag local uid = ") << *tagIt
                              << QStringLiteral(", tag model item: ")
                              << *pModelItem << QStringLiteral("\nNote: ")
                              << currentNote);
                    return QString();
                }

                const TagItem * pTagItem = pModelItem->tagItem();
                if (Q_UNLIKELY(!pTagItem))
                {
                    errorDescription.setBase(
                        QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                                   "detected tag model item corresponding to tag "
                                   "local uid and of a tag type but containing "
                                   "no actual tag item"));
                    QNWARNING(errorDescription
                              << QStringLiteral(", tag local uid = ") << *tagIt
                              << QStringLiteral(", tag model item: ")
                              << *pModelItem << QStringLiteral("\nNote: ")
                              << currentNote);
                    return QString();
                }

                tagNameByTagLocalUid[*tagIt] = pTagItem->name();
            }
        }

        notes << currentNote;
    }

    QString enex;
    ENMLConverter converter;
    ENMLConverter::EnexExportTags::type exportTagsOption =
        (m_includeTags
         ? ENMLConverter::EnexExportTags::Yes
         : ENMLConverter::EnexExportTags::No);
    bool res = converter.exportNotesToEnex(notes, tagNameByTagLocalUid,
                                           exportTagsOption, enex,
                                           errorDescription,
                                           QUENTIER_ENEX_VERSION);
    if (!res) {
        return QString();
    }

    QNDEBUG(QStringLiteral("Successfully exported note(s) to ENEX"));
    return enex;
}

void EnexExporter::connectToLocalStorage()
{
    QNDEBUG(QStringLiteral("EnexExporter::connectToLocalStorage"));

    if (m_connectedToLocalStorage) {
        QNTRACE(QStringLiteral("Already connected to local storage"));
        return;
    }

    QObject::connect(this,
                     QNSIGNAL(EnexExporter,findNote,
                              Note,LocalStorageManager::GetNoteOptions,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,
                            Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,
                              Note,LocalStorageManager::GetNoteOptions,QUuid),
                     this,
                     QNSLOT(EnexExporter,onFindNoteComplete,
                            Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,
                              Note,LocalStorageManager::GetNoteOptions,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(EnexExporter,onFindNoteFailed,
                            Note,LocalStorageManager::GetNoteOptions,
                            ErrorString,QUuid));

    m_connectedToLocalStorage = true;
}

void EnexExporter::disconnectFromLocalStorage()
{
    QNDEBUG(QStringLiteral("EnexExporter::disconnectFromLocalStorage"));

    if (!m_connectedToLocalStorage) {
        QNTRACE(QStringLiteral("Not connected to local storage at the moment"));
        return;
    }

    QObject::disconnect(this,
                        QNSIGNAL(EnexExporter,findNote,
                                 Note,LocalStorageManager::GetNoteOptions,QUuid),
                        &m_localStorageManagerAsync,
                        QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,
                               Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,
                                 Note,LocalStorageManager::GetNoteOptions,QUuid),
                        this,
                        QNSLOT(EnexExporter,onFindNoteComplete,
                               Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,
                                 Note,LocalStorageManager::GetNoteOptions,
                                 ErrorString,QUuid),
                        this,
                        QNSLOT(EnexExporter,onFindNoteFailed,
                               Note,LocalStorageManager::GetNoteOptions,
                               ErrorString,QUuid));

    m_connectedToLocalStorage = false;
}

} // namespace quentier
