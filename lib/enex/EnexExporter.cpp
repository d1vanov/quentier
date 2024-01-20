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

#include "EnexExporter.h"

#include <lib/model/tag/TagModel.h>
#include <lib/widget/NoteEditorTabsAndWindowsCoordinator.h>
#include <lib/widget/NoteEditorWidget.h>

#include <quentier/enml/ENMLConverter.h>
#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <utility>

namespace quentier {

EnexExporter::EnexExporter(
    local_storage::ILocalStoragePtr localStorage,
    NoteEditorTabsAndWindowsCoordinator & coordinator, TagModel & tagModel,
    QObject * parent) :
    QObject{parent},
    m_localStorage{std::move(localStorage)},
    m_noteEditorTabsAndWindowsCoordinator{coordinator}, m_pTagModel{&tagModel}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{QStringLiteral(
            "EnexExporter ctor: local storage is null")}};
    }

    if (!tagModel.allTagsListed()) {
        QObject::connect(
            &tagModel, &TagModel::notifyAllTagsListed, this,
            &EnexExporter::onAllTagsListed);
    }
}

void EnexExporter::setNoteLocalIds(QStringList noteLocalIds)
{
    QNDEBUG(
        "enex",
        "EnexExporter::setNoteLocalIds: "
            << noteLocalIds.join(QStringLiteral(", ")));

    if (isInProgress()) {
        clear();
    }

    m_noteLocalIds = std::move(noteLocalIds);
}

void EnexExporter::setIncludeTags(const bool includeTags)
{
    QNDEBUG(
        "enex",
        "EnexExporter::setIncludeTags: " << (includeTags ? "true" : "false"));

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

    if (m_noteLocalIds.isEmpty()) {
        QNDEBUG("enex", "No note local uids are set");
        return false;
    }

    if (m_noteLocalIdsPendingFindInLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex",
            "No pending requests to find notes in the local storage");
        return false;
    }

    return true;
}

void EnexExporter::start()
{
    QNDEBUG("enex", "EnexExporter::start");

    if (m_noteLocalIds.isEmpty()) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't export note to ENEX: no note local ids were "
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

    if (m_findNotesInLocalStorageCanceler) {
        m_findNotesInLocalStorageCanceler->cancel();
        m_findNotesInLocalStorageCanceler.reset();
    }

    m_notesByLocalId.clear();
    m_noteLocalIdsPendingFindInLocalStorage.clear();

    for (const auto & noteLocalId: std::as_const(m_noteLocalIds)) {
        auto * pNoteEditorWidget =
            m_noteEditorTabsAndWindowsCoordinator
                .noteEditorWidgetForNoteLocalUid(noteLocalId);

        if (!pNoteEditorWidget) {
            QNTRACE(
                "enex",
                "Found no note editor widget for note local id "
                    << noteLocalId);
            findNoteInLocalStorage(noteLocalId);
            continue;
        }

        QNTRACE("enex", "Found note editor with loaded note " << noteLocalId);

        const auto * pNote = pNoteEditorWidget->currentNote();
        if (Q_UNLIKELY(!pNote)) {
            QNDEBUG(
                "enex",
                "There is no note in the editor, will try to "
                    << "find it in the local storage");
            findNoteInLocalStorage(noteLocalId);
            continue;
        }

        if (!pNoteEditorWidget->isModified()) {
            QNTRACE(
                "enex",
                "Fetched the unmodified note from editor: " << noteLocalId);
            m_notesByLocalId[noteLocalId] = *pNote;
            continue;
        }

        QNTRACE("enex", "The note within the editor was modified, saving it");

        ErrorString noteSavingError;

        auto saveStatus =
            pNoteEditorWidget->checkAndSaveModifiedNote(noteSavingError);

        if (saveStatus != NoteEditorWidget::NoteSaveStatus::Ok) {
            QNWARNING(
                "enex",
                "Could not save the note loaded into the editor: "
                    << "status = " << saveStatus
                    << ", error: " << noteSavingError
                    << "; will try to find the note in the local storage");
            findNoteInLocalStorage(noteLocalId);
            continue;
        }

        pNote = pNoteEditorWidget->currentNote();
        if (Q_UNLIKELY(!pNote)) {
            QNWARNING(
                "enex",
                "Note editor's current note has unexpectedly "
                    << "become nullptr after the note has been saved; "
                    << "will try to find the note in the local storage");
            findNoteInLocalStorage(noteLocalId);
            continue;
        }

        QNTRACE(
            "enex",
            "Fetched the modified & saved note from editor: " << noteLocalId);

        m_notesByLocalId[noteLocalId] = *pNote;
    }

    if (!m_noteLocalIdsPendingFindInLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex",
            "Not all requested notes were found loaded into the editors, "
                << "pending "
                << m_noteLocalIdsPendingFindInLocalStorage.size()
                << " find note in local storage requests");
        return;
    }

    QNDEBUG(
        "enex",
        "All requested notes were found loaded into the editors "
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
    m_noteLocalIds.clear();

    if (m_findNotesInLocalStorageCanceler) {
        m_findNotesInLocalStorageCanceler->cancel();
        m_findNotesInLocalStorageCanceler.reset();
    }

    m_notesByLocalId.clear();
    m_noteLocalIdsPendingFindInLocalStorage.clear();
}

void EnexExporter::onAllTagsListed()
{
    QNDEBUG("enex", "EnexExporter::onAllTagsListed");

    QObject::disconnect(
        m_pTagModel.data(), &TagModel::notifyAllTagsListed, this,
        &EnexExporter::onAllTagsListed);

    if (m_noteLocalIds.isEmpty()) {
        QNDEBUG("enex", "No note local ids are specified, won't do anything");
        return;
    }

    if (!m_noteLocalIdsPendingFindInLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex",
            "Still pending " << m_noteLocalIdsPendingFindInLocalStorage.size()
                << " find note in local storage requests");
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

void EnexExporter::findNoteInLocalStorage(const QString & noteLocalId)
{
    QNDEBUG("enex", "EnexExporter::findNoteInLocalStorage: " << noteLocalId);

    if (m_noteLocalIdsPendingFindInLocalStorage.contains(noteLocalId)) {
        QNDEBUG("enex", "Already pending find request for this note");
        return;
    }

    m_noteLocalIdsPendingFindInLocalStorage.insert(noteLocalId);

    if (m_findNotesInLocalStorageCanceler) {
        m_findNotesInLocalStorageCanceler->cancel();
    }

    m_findNotesInLocalStorageCanceler =
        std::make_shared<utility::cancelers::ManualCanceler>();

    const auto options = local_storage::ILocalStorage::FetchNoteOptions{}
        | local_storage::ILocalStorage::FetchNoteOption::WithResourceMetadata
        | local_storage::ILocalStorage::FetchNoteOption::WithResourceBinaryData;

    auto future = m_localStorage->findNoteByLocalId(noteLocalId, options);
    auto thenFuture = threading::then(
        std::move(future), this,
        [this, noteLocalId, canceler = m_findNotesInLocalStorageCanceler](
            std::optional<qevercloud::Note> note) {
            if (canceler->isCanceled()) {
                return;
            }

            m_noteLocalIdsPendingFindInLocalStorage.remove(noteLocalId);
            if (note) {
                onNoteFoundInLocalStorage(std::move(*note));
            }
            else {
                onNoteNotFoundInLocalStorage(noteLocalId);
            }
        });

    threading::onFailed(
        std::move(thenFuture), this,
        [this, noteLocalId](const QException & e) {
            ErrorString error{
                QT_TR_NOOP("Can't export note(s) to ENEX: error while trying "
                           "to find note in the local storage")};
            error.details() = e.what();
            onFailedToFindNoteInLocalStorage(noteLocalId, std::move(error));
        });
}

void EnexExporter::onNoteFoundInLocalStorage(qevercloud::Note note)
{
    QNDEBUG(
        "enex",
        "EnexExporter::onNoteFoundInLocalStorage: " << note);

    if (!m_noteLocalIdsPendingFindInLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex",
            "Still pending " << m_noteLocalIdsPendingFindInLocalStorage.size()
                << " find note in local storage requests");
        return;
    }

    if (m_includeTags) {
        if (Q_UNLIKELY(m_pTagModel.isNull())) {
            ErrorString errorDescription(
                QT_TR_NOOP("Can't export note(s) to ENEX: tag model is "
                           "deleted"));
            QNWARNING("enex", errorDescription);
            clear();
            Q_EMIT failedToExportNotesToEnex(errorDescription);
            return;
        }

        if (!m_pTagModel->allTagsListed()) {
            QNDEBUG(
                "enex", "Not all tags were listed within the tag model yet");
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

void EnexExporter::onNoteNotFoundInLocalStorage(QString noteLocalId)
{
    QNDEBUG(
        "enex",
        "EnexExporter::onNoteNotFoundInLocalStorage: note local id = "
            << noteLocalId);

    ErrorString error{
        QT_TR_NOOP("Can't export note(s) to ENEX: can't find one "
                   "of notes in the local storage")};
    error.details() = noteLocalId;
    QNWARNING("enex", error);

    clear();
    Q_EMIT failedToExportNotesToEnex(error);
}

void EnexExporter::onFailedToFindNoteInLocalStorage(
    QString noteLocalId, ErrorString errorDescription)
{
    clear();
    Q_EMIT failedToExportNotesToEnex(std::move(errorDescription));
}

QString EnexExporter::convertNotesToEnex(ErrorString & errorDescription)
{
    QNDEBUG("enex", "EnexExporter::convertNotesToEnex");

    if (m_notesByLocalId.isEmpty()) {
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

    QList<Note> notes;
    notes.reserve(m_notesByLocalId.size());

    QHash<QString, QString> tagNameByTagLocalId;
    for (auto it = m_notesByLocalId.constBegin(),
              end = m_notesByLocalId.constEnd();
         it != end; ++it)
    {
        const auto & currentNote = it.value();

        if (m_includeTags && !currentNote.tagLocalIds().isEmpty()) {
            const auto & tagLocalIds = currentNote.tagLocalIds();
            for (auto tagIt = tagLocalIds.constBegin(),
                      tagEnd = tagLocalIds.constEnd();
                 tagIt != tagEnd; ++tagIt)
            {
                const auto * pModelItem = m_pTagModel->itemForLocalUid(*tagIt);
                if (Q_UNLIKELY(!pModelItem)) {
                    errorDescription.setBase(QT_TR_NOOP(
                        "Can't export notes to ENEX: internal error, "
                        "detected note with tag local id for which "
                        "no tag model item was found"));

                    QNWARNING(
                        "enex",
                        errorDescription << ", tag local id = " << *tagIt
                                         << ", note: " << currentNote);
                    return {};
                }

                const auto * pTagItem = pModelItem->cast<TagItem>();
                if (Q_UNLIKELY(!pTagItem)) {
                    errorDescription.setBase(
                        QT_TR_NOOP("Can't export notes to ENEX: internal "
                                   "error, detected tag model item "
                                   "corresponding to tag local id but not of "
                                   "a tag type"));

                    QNWARNING(
                        "enex",
                        errorDescription << ", tag local uid = " << *tagIt
                                         << ", tag model item: " << *pModelItem
                                         << "\nNote: " << currentNote);
                    return {};
                }

                tagNameByTagLocalId[*tagIt] = pTagItem->name();
            }
        }

        notes << currentNote;
    }

    QString enex;
    ENMLConverter converter;

    const auto exportTagsOption =
        (m_includeTags ? ENMLConverter::EnexExportTags::Yes
                       : ENMLConverter::EnexExportTags::No);

    bool res = converter.exportNotesToEnex(
        notes, tagNameByTagLocalId, exportTagsOption, enex, errorDescription,
        QStringLiteral("Quentier"));

    if (!res) {
        return {};
    }

    QNDEBUG("enex", "Successfully exported note(s) to ENEX");
    return enex;
}

} // namespace quentier
