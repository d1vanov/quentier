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

#include <quentier/enml/Factory.h>
#include <quentier/enml/IConverter.h>
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
    m_noteEditorTabsAndWindowsCoordinator{coordinator}, m_tagModel{&tagModel}
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
        "enex::EnexExporter",
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
        "enex::EnexExporter",
        "EnexExporter::setIncludeTags: " << (includeTags ? "true" : "false"));

    if (m_includeTags == includeTags) {
        QNDEBUG(
            "enex::EnexExporter",
            "The setting has not changed, won't do anything");
        return;
    }

    if (isInProgress()) {
        clear();
    }

    m_includeTags = includeTags;
}

bool EnexExporter::isInProgress() const
{
    QNDEBUG("enex::EnexExporter", "EnexExporter::isInProgress");

    if (m_noteLocalIds.isEmpty()) {
        QNDEBUG("enex::EnexExporter", "No note local ids are set");
        return false;
    }

    if (m_noteLocalIdsPendingFindInLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex::EnexExporter",
            "No pending requests to find notes in the local storage");
        return false;
    }

    return true;
}

void EnexExporter::start()
{
    QNDEBUG("enex::EnexExporter", "EnexExporter::start");

    if (m_noteLocalIds.isEmpty()) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't export note to ENEX: no note local ids were "
                       "specified")};
        QNWARNING("enex::EnexExporter", errorDescription);
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    if (m_includeTags && m_tagModel.isNull()) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't export note to ENEX: tag model is deleted")};
        QNWARNING("enex::EnexExporter", errorDescription);
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
        auto * noteEditorWidget =
            m_noteEditorTabsAndWindowsCoordinator
                .noteEditorWidgetForNoteLocalId(noteLocalId);

        if (!noteEditorWidget) {
            QNTRACE(
                "enex::EnexExporter",
                "Found no note editor widget for note local id "
                    << noteLocalId);
            findNoteInLocalStorage(noteLocalId);
            continue;
        }

        QNTRACE(
            "enex::EnexExporter",
            "Found note editor with loaded note " << noteLocalId);

        auto note = noteEditorWidget->currentNote();
        if (Q_UNLIKELY(!note)) {
            QNDEBUG(
                "enex::EnexExporter",
                "There is no note in the editor, will try to "
                    << "find it in the local storage");
            findNoteInLocalStorage(noteLocalId);
            continue;
        }

        if (!noteEditorWidget->isModified()) {
            QNTRACE(
                "enex::EnexExporter",
                "Fetched the unmodified note from editor: " << noteLocalId);
            m_notesByLocalId[noteLocalId] = *note;
            continue;
        }

        QNTRACE(
            "enex::EnexExporter",
            "The note within the editor was modified, saving it");

        ErrorString noteSavingError;

        const auto saveStatus =
            noteEditorWidget->checkAndSaveModifiedNote(noteSavingError);

        if (saveStatus != NoteEditorWidget::NoteSaveStatus::Ok) {
            QNWARNING(
                "enex::EnexExporter",
                "Could not save the note loaded into the editor: status = "
                    << saveStatus << ", error: " << noteSavingError
                    << "; will try to find the note in the local storage");
            findNoteInLocalStorage(noteLocalId);
            continue;
        }

        note = noteEditorWidget->currentNote();
        if (Q_UNLIKELY(!note)) {
            QNWARNING(
                "enex::EnexExporter",
                "Note editor's current note has unexpectedly "
                    << "become nullopt after saving it; will try to find note "
                    << "in the local storage");
            findNoteInLocalStorage(noteLocalId);
            continue;
        }

        QNTRACE(
            "enex::EnexExporter",
            "Fetched the modified & saved note from editor: " << noteLocalId);

        m_notesByLocalId[noteLocalId] = *note;
    }

    if (!m_noteLocalIdsPendingFindInLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex::EnexExporter",
            "Not all requested notes were found loaded into the editors, "
                << "pending "
                << m_noteLocalIdsPendingFindInLocalStorage.size()
                << " find note in local storage requests");
        return;
    }

    QNDEBUG(
        "enex::EnexExporter",
        "All requested notes were found loaded into the editors "
            << "and were successfully gathered from them");

    if (m_includeTags && !m_tagModel->allTagsListed()) {
        QNDEBUG(
            "enex::EnexExporter",
            "Waiting for the tag model to get all tags listed");
        return;
    }

    ErrorString errorDescription;
    QString enex = convertNotesToEnex(errorDescription);
    if (enex.isEmpty()) {
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    Q_EMIT notesExportedToEnex(std::move(enex));
}

void EnexExporter::clear()
{
    QNDEBUG("enex::EnexExporter", "EnexExporter::clear");

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
    QNDEBUG("enex::EnexExporter", "EnexExporter::onAllTagsListed");

    QObject::disconnect(
        m_tagModel.data(), &TagModel::notifyAllTagsListed, this,
        &EnexExporter::onAllTagsListed);

    if (m_noteLocalIds.isEmpty()) {
        QNDEBUG(
            "enex::EnexExporter",
            "No note local ids are specified, won't do anything");
        return;
    }

    if (!m_noteLocalIdsPendingFindInLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex::EnexExporter",
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

    Q_EMIT notesExportedToEnex(std::move(enex));
}

void EnexExporter::findNoteInLocalStorage(const QString & noteLocalId)
{
    QNDEBUG(
        "enex::EnexExporter",
        "EnexExporter::findNoteInLocalStorage: " << noteLocalId);

    if (m_noteLocalIdsPendingFindInLocalStorage.contains(noteLocalId)) {
        QNDEBUG(
            "enex::EnexExporter", "Already pending find request for this note");
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
            const std::optional<qevercloud::Note> & note) {
            if (canceler->isCanceled()) {
                return;
            }

            m_noteLocalIdsPendingFindInLocalStorage.remove(noteLocalId);
            if (note) {
                onNoteFoundInLocalStorage(*note);
            }
            else {
                onNoteNotFoundInLocalStorage(noteLocalId);
            }
        });

    threading::onFailed(
        std::move(thenFuture), this,
        [this](const QException & e) {
            ErrorString error{
                QT_TR_NOOP("Can't export note(s) to ENEX: error while trying "
                           "to find note in the local storage")};
            error.details() = QString::fromUtf8(e.what());
            onFailedToFindNoteInLocalStorage(std::move(error));
        });
}

void EnexExporter::onNoteFoundInLocalStorage(const qevercloud::Note & note)
{
    QNDEBUG(
        "enex::EnexExporter",
        "EnexExporter::onNoteFoundInLocalStorage: " << note);

    if (!m_noteLocalIdsPendingFindInLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex::EnexExporter",
            "Still pending " << m_noteLocalIdsPendingFindInLocalStorage.size()
                << " find note in local storage requests");
        return;
    }

    if (m_includeTags) {
        if (Q_UNLIKELY(m_tagModel.isNull())) {
            ErrorString errorDescription{
                QT_TR_NOOP("Can't export note(s) to ENEX: tag model is "
                           "deleted")};
            QNWARNING("enex::EnexExporter", errorDescription);
            clear();
            Q_EMIT failedToExportNotesToEnex(errorDescription);
            return;
        }

        if (!m_tagModel->allTagsListed()) {
            QNDEBUG(
                "enex::EnexExporter",
                "Not all tags were listed within the tag model yet");
            return;
        }
    }

    ErrorString errorDescription;
    QString enex = convertNotesToEnex(errorDescription);
    if (enex.isEmpty()) {
        Q_EMIT failedToExportNotesToEnex(errorDescription);
        return;
    }

    Q_EMIT notesExportedToEnex(std::move(enex));
}

void EnexExporter::onNoteNotFoundInLocalStorage(QString noteLocalId)
{
    QNDEBUG(
        "enex::EnexExporter",
        "EnexExporter::onNoteNotFoundInLocalStorage: note local id = "
            << noteLocalId);

    ErrorString error{
        QT_TR_NOOP("Can't export note(s) to ENEX: can't find one "
                   "of notes in the local storage")};
    error.details() = std::move(noteLocalId);
    QNWARNING("enex::EnexExporter", error);

    clear();
    Q_EMIT failedToExportNotesToEnex(std::move(error));
}

void EnexExporter::onFailedToFindNoteInLocalStorage(
    ErrorString errorDescription)
{
    clear();
    Q_EMIT failedToExportNotesToEnex(std::move(errorDescription));
}

QString EnexExporter::convertNotesToEnex(ErrorString & errorDescription)
{
    QNDEBUG("enex::EnexExporter", "EnexExporter::convertNotesToEnex");

    if (m_notesByLocalId.isEmpty()) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't export notes to ENEX: no notes "
                       "were specified or found"));
        QNWARNING("enex::EnexExporter", errorDescription);
        return {};
    }

    if (m_includeTags && m_tagModel.isNull()) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't export notes to ENEX: "
                       "tag model is deleted"));
        QNWARNING("enex::EnexExporter", errorDescription);
        return {};
    }

    QList<qevercloud::Note> notes;
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
                const auto * modelItem = m_tagModel->itemForLocalId(*tagIt);
                if (Q_UNLIKELY(!modelItem)) {
                    errorDescription.setBase(QT_TR_NOOP(
                        "Can't export notes to ENEX: internal error, "
                        "detected note with tag local id for which "
                        "no tag model item was found"));

                    QNWARNING(
                        "enex::EnexExporter",
                        errorDescription << ", tag local id = " << *tagIt
                                         << ", note: " << currentNote);
                    return {};
                }

                const auto * tagItem = modelItem->cast<TagItem>();
                if (Q_UNLIKELY(!tagItem)) {
                    errorDescription.setBase(
                        QT_TR_NOOP("Can't export notes to ENEX: internal "
                                   "error, detected tag model item "
                                   "corresponding to tag local id but not of "
                                   "a tag type"));

                    QNWARNING(
                        "enex::EnexExporter",
                        errorDescription << ", tag local id = " << *tagIt
                                         << ", tag model item: " << *modelItem
                                         << "\nNote: " << currentNote);
                    return {};
                }

                tagNameByTagLocalId[*tagIt] = tagItem->name();
            }
        }

        notes << currentNote;
    }

    auto converter = enml::createConverter();

    const auto exportTagsOption =
        (m_includeTags ? enml::IConverter::EnexExportTags::Yes
                       : enml::IConverter::EnexExportTags::No);

    auto res = converter->exportNotesToEnex(
        notes, tagNameByTagLocalId, exportTagsOption,
        QStringLiteral("Quentier"));

    if (!res.isValid()) {
        return {};
    }

    QNDEBUG("enex::EnexExporter", "Successfully exported note(s) to ENEX");
    return res.get();
}

} // namespace quentier
