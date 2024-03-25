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

#include "EnexImporter.h"

#include <lib/exception/Utils.h>
#include <lib/model/notebook/NotebookModel.h>
#include <lib/model/tag/TagModel.h>

#include <quentier/enml/IConverter.h>
#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <QFile>

#include <utility>

namespace quentier {

EnexImporter::EnexImporter(
    QString enexFilePath, QString notebookName,
    local_storage::ILocalStoragePtr localStorage,
    enml::IConverterPtr enmlConverter, TagModel & tagModel,
    NotebookModel & notebookModel, QObject * parent) :
    QObject{parent}, m_enexFilePath{std::move(enexFilePath)},
    m_notebookName{std::move(notebookName)},
    m_localStorage{std::move(localStorage)},
    m_enmlConverter{std::move(enmlConverter)}, m_tagModel{tagModel},
    m_notebookModel{notebookModel}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{QStringLiteral(
            "EnexImporter ctor: local storage is null")}};
    }

    if (Q_UNLIKELY(!m_enmlConverter)) {
        throw InvalidArgument{ErrorString{QStringLiteral(
            "EnexImporter ctor: enml converter is null")}};
    }

    if (Q_UNLIKELY(m_notebookName.isEmpty())) {
        throw InvalidArgument{ErrorString{QStringLiteral(
            "EnexImporter ctor: notebook name is empty")}};
    }

    ErrorString notebookNameError;
    if (Q_UNLIKELY(
            !validateNotebookName(m_notebookName, &notebookNameError))) {
        ErrorString error{QStringLiteral(
            "EnexImporter ctor: notebook name is invalid")};
        error.appendBase(notebookNameError.base());
        error.appendBase(notebookNameError.additionalBases());
        error.details() = notebookNameError.details();
        throw InvalidArgument{std::move(error)};
    }

    if (!m_tagModel.allTagsListed()) {
        QObject::connect(
            &m_tagModel, &TagModel::notifyAllTagsListed, this,
            &EnexImporter::onAllTagsListed);
    }

    if (!m_notebookModel.allNotebooksListed()) {
        QObject::connect(
            &m_notebookModel, &NotebookModel::notifyAllNotebooksListed, this,
            &EnexImporter::onAllNotebooksListed);
    }
}

bool EnexImporter::isInProgress() const
{
    QNDEBUG("enex::EnexImporter", "EnexImporter::isInProgress");

    if (!m_tagNamesPendingTagPutToLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex::EnexImporter",
            "There are " << m_tagNamesPendingTagPutToLocalStorage.size()
                << " pending put tag to local storage requests");
        return true;
    }

    if (!m_noteLocalIdsPendingNotePutToLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex::EnexImporter",
            "There are " << m_noteLocalIdsPendingNotePutToLocalStorage.size()
                         << " pending requests to put note to local storage");
        return true;
    }

    if (!m_tagModel.allTagsListed() && !m_notesPendingTagAddition.isEmpty()) {
        QNDEBUG(
            "enex::EnexImporter",
            "Not all tags were listed in the tag model yet + "
                << "there are " << m_notesPendingTagAddition.size()
                << " notes pending tag addition");
        return true;
    }

    if (!m_notebookModel.allNotebooksListed() && m_pendingNotebookModelToStart)
    {
        QNDEBUG(
            "enex::EnexImporter",
            "Not all notebooks were listed in the notebook model yet, pending "
            "them");
        return true;
    }

    QNDEBUG("enex::EnexImporter", "No signs of pending progress detected");
    return false;
}

void EnexImporter::start()
{
    QNDEBUG("enex::EnexImporter", "EnexImporter::start");

    clear();
    connectToLocalStorageEvents();

    if (Q_UNLIKELY(!m_notebookModel.allNotebooksListed())) {
        QNDEBUG(
            "enex::EnexImporter",
            "Not all notebooks were listed in the notebook model yet, delaying "
            "the start");
        m_pendingNotebookModelToStart = true;
        return;
    }

    if (m_notebookLocalId.isEmpty()) {
        QString notebookLocalId = m_notebookModel.localIdForItemName(
            m_notebookName,
            /* linked notebook guid = */ {});

        if (notebookLocalId.isEmpty()) {
            QNDEBUG(
                "enex::EnexImporter",
                "Could not find a user's own notebook's local id for notebook "
                    << "name " << m_notebookName
                    << " within the notebook model; will create such notebook");

            putNotebookToLocalStorage(m_notebookName);
            return;
        }

        m_notebookLocalId = std::move(notebookLocalId);
    }

    QFile enexFile{m_enexFilePath};
    if (Q_UNLIKELY(!enexFile.open(QIODevice::ReadOnly))) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't import ENEX: can't open enex file for writing")};
        errorDescription.details() = m_enexFilePath;
        QNWARNING("enex::EnexImporter", errorDescription);
        Q_EMIT enexImportFailed(errorDescription);
        return;
    }

    const QString enex = QString::fromUtf8(enexFile.readAll());
    enexFile.close();

    auto res = m_enmlConverter->importEnex(enex);
    if (!res.isValid()) {
        Q_EMIT enexImportFailed(std::move(res.error()));
        return;
    }

    auto importedNotes = std::move(res.get());
    for (auto it = importedNotes.begin(); it != importedNotes.end();) {
        auto & note = *it;
        note.setNotebookLocalId(m_notebookLocalId);

        const auto tagIt =
            m_tagNamesByImportedNoteLocalIds.find(note.localId());
        if (tagIt == m_tagNamesByImportedNoteLocalIds.end()) {
            QNTRACE(
                "enex::EnexImporter",
                "Imported note doesn't have tag names assigned to it, can add "
                    << "it to local storage right away: " << note);

            putNoteToLocalStorage(note);
            it = importedNotes.erase(it);
            continue;
        }

        auto & tagNames = tagIt.value();
        for (auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end();) {
            const auto & tagName = *tagNameIt;
            if (!tagName.isEmpty()) {
                ++tagNameIt;
                continue;
            }

            QNDEBUG(
                "enex::EnexImporter",
                "Removing empty tag name from the list of tag names for note "
                    << note.localId());

            tagNameIt = tagNames.erase(tagNameIt);
        }

        if (Q_UNLIKELY(tagNames.isEmpty())) {
            QNDEBUG(
                "enex::EnexImporter",
                "No tag names are left for note "
                    << note.localId()
                    << " after the cleanup of empty tag names, can add the "
                    << "note to local storage right away");

            putNoteToLocalStorage(note);
            it = importedNotes.erase(it);
            continue;
        }

        ++it;
    }

    if (importedNotes.isEmpty()) {
        QNDEBUG(
            "enex::EnexImporter",
            "No notes had any tags, waiting for the completion "
                << "of all add note requests");
        return;
    }

    m_notesPendingTagAddition = importedNotes;
    QNDEBUG(
        "enex::EnexImporter",
        "There are " << m_notesPendingTagAddition.size()
                     << " notes which need tags assignment to them");

    if (!m_tagModel.allTagsListed()) {
        QNDEBUG(
            "enex::EnexImporter",
            "Not all tags were listed from the tag model, waiting for it");
        return;
    }

    processNotesPendingTagAddition();
}

void EnexImporter::clear()
{
    QNDEBUG("enex::EnexImporter", "EnexImporter::clear");

    m_notebookLocalId.clear();
    m_tagNamesByImportedNoteLocalIds.clear();
    m_tagNamesPendingTagPutToLocalStorage.clear();
    m_expungedTagLocalIds.clear();

    m_notesPendingTagAddition.clear();
    m_noteLocalIdsPendingNotePutToLocalStorage.clear();
    m_pendingNotebookModelToStart = false;
    m_pendingNotebookPutToLocalStorage = false;

    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }

    disconnectFromLocalStorageEvents();
}

void EnexImporter::onAllTagsListed()
{
    QNDEBUG("enex::EnexImporter", "EnexImporter::onAllTagsListed");

    QObject::disconnect(
        &m_tagModel, &TagModel::notifyAllTagsListed, this,
        &EnexImporter::onAllTagsListed);

    if (!isInProgress()) {
        return;
    }

    processNotesPendingTagAddition();
}

void EnexImporter::onAllNotebooksListed()
{
    QNDEBUG("enex::EnexImporter", "EnexImporter::onAllNotebooksListed");

    QObject::disconnect(
        &m_notebookModel, &NotebookModel::notifyAllNotebooksListed, this,
        &EnexImporter::onAllNotebooksListed);

    if (!m_pendingNotebookModelToStart) {
        return;
    }

    m_pendingNotebookModelToStart = false;
    start();
}

void EnexImporter::onTagExpunged(
    QString tagLocalId, QStringList expungedChildTagLocalIds)
{
    QNDEBUG(
        "enex::EnexImporter",
        "EnexImporter::onTagExpunged: tag local id = "
            << tagLocalId << ", child tag local ids = "
            << expungedChildTagLocalIds.join(QStringLiteral(", ")));

    if (!isInProgress()) {
        QNDEBUG(
            "enex::EnexImporter",
            "Not in progress right now, won't do anything");
        return;
    }

    QStringList expungedTagLocalIds;
    expungedTagLocalIds.reserve(expungedChildTagLocalIds.size() + 1);
    expungedTagLocalIds = expungedChildTagLocalIds;
    expungedTagLocalIds << tagLocalId;

    for (const auto & expungedTagLocalId: std::as_const(expungedTagLocalIds)) {
        m_expungedTagLocalIds.insert(expungedTagLocalId);
    }

    // Just in case check if some of our pending notes have either of these tag
    // local ids, if so, remove them
    for (auto & note: m_notesPendingTagAddition) {
        if (note.tagLocalIds().isEmpty()) {
            continue;
        }

        auto tagLocalIds = note.tagLocalIds();
        for (const auto & expungedTagLocalId:
             std::as_const(expungedTagLocalIds))
        {
            if (tagLocalIds.contains(expungedTagLocalId)) {
                tagLocalIds.removeAll(expungedTagLocalId);
            }
        }

        note.setTagLocalIds(std::move(tagLocalIds));
    }
}

void EnexImporter::onNotebookExpunged(const QString & notebookLocalId)
{
    QNDEBUG(
        "enex::EnexImporter",
        "EnexImporter::onNotebookExpunged: " << notebookLocalId);

    if (Q_UNLIKELY(
            !m_notebookLocalId.isEmpty() &&
            notebookLocalId == m_notebookLocalId))
    {
        ErrorString error{
            QT_TR_NOOP("Can't complete ENEX import: notebook was "
                       "expunged during import")};
        QNWARNING(
            "enex::EnexImporter",
            error << ", notebook local id = " << notebookLocalId);
        clear();
        Q_EMIT enexImportFailed(error);
    }
}

void EnexImporter::onTagPutToLocalStorage(
    const QString & tagLocalId, const QString & tagName)
{
    m_tagNamesPendingTagPutToLocalStorage.remove(tagName);

    for (auto noteIt = m_notesPendingTagAddition.begin();
         noteIt != m_notesPendingTagAddition.end();)
    {
        auto & note = *noteIt;

        const auto tagIt =
            m_tagNamesByImportedNoteLocalIds.find(note.localId());
        if (Q_UNLIKELY(tagIt == m_tagNamesByImportedNoteLocalIds.end())) {
            QNWARNING(
                "enex::EnexImporter",
                "Detected note within the list of those pending tags addition "
                    << "which doesn't really wait for tags addition");

            putNoteToLocalStorage(note);
            noteIt = m_notesPendingTagAddition.erase(noteIt);
            continue;
        }

        auto & tagNames = tagIt.value();
        for (auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end();) {
            const auto & currentTagName = *tagNameIt;
            if (currentTagName.toLower() != tagName) {
                ++tagNameIt;
                continue;
            }

            QNDEBUG(
                "enex::EnexImporter",
                "Found tag for note " << note.localId()
                                      << ": name = " << tagName
                                      << ", local id = " << tagLocalId);

            if (!note.tagLocalIds().contains(tagLocalId)) {
                note.mutableTagLocalIds().append(tagLocalId);
            }

            tagNames.erase(tagNameIt);
            break;
        }

        if (!tagNames.isEmpty()) {
            QNTRACE(
                "enex::EnexImporter",
                "Still pending " << tagNames.size() << " tag names for note "
                                 << note.localId());

            ++noteIt;
            continue;
        }

        QNDEBUG(
            "enex::EnexImporter",
            "Added the last missing tag for note "
                << note.localId()
                << ", can send it to the local storage right away");

        m_tagNamesByImportedNoteLocalIds.erase(tagIt);

        putNoteToLocalStorage(note);
        noteIt = m_notesPendingTagAddition.erase(noteIt);
    }
}

void EnexImporter::onFailedToPutTagToLocalStorage(ErrorString errorDescription)
{
    clear();

    ErrorString error{QT_TR_NOOP("Can't import ENEX")};
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(std::move(error));
}

void EnexImporter::onNotebookPutToLocalStorage(QString notebookLocalId)
{
    m_notebookLocalId = std::move(notebookLocalId);
    start();
}

void EnexImporter::onFailedToPutNotebookToLocalStorage(
    ErrorString errorDescription)
{
    clear();

    ErrorString error{QT_TR_NOOP("Can't import ENEX")};
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(std::move(error));
}

void EnexImporter::onNotePutToLocalStorage(const QString & noteLocalId)
{
    m_noteLocalIdsPendingNotePutToLocalStorage.remove(noteLocalId);

    if (!m_noteLocalIdsPendingNotePutToLocalStorage.isEmpty()) {
        QNDEBUG(
            "enex::EnexImporter",
            "Still pending "
                << m_noteLocalIdsPendingNotePutToLocalStorage.size()
                << " put note to local storage requests");
        return;
    }

    if (!m_notesPendingTagAddition.isEmpty()) {
        QNDEBUG(
            "enex::EnexImporter",
            "There are still " << m_notesPendingTagAddition.size()
                               << " notes pending tag addition");
        return;
    }

    QNDEBUG(
        "enex::EnexImporter",
        "There are no pending put note requests and no notes pending tags "
            << "addition => it looks like the import has finished");

    Q_EMIT enexImportedSuccessfully(m_enexFilePath);
}

void EnexImporter::onFailedToPutNoteToLocalStorage(ErrorString errorDescription)
{
    clear();

    ErrorString error{QT_TR_NOOP("Can't import ENEX")};
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(std::move(error));
}

void EnexImporter::connectToLocalStorageEvents()
{
    QNDEBUG("enex::EnexImporter", "EnexImporter::connectToLocalStorageEvents");

    if (m_connectedToLocalStorage) {
        QNDEBUG("enex::EnexImporter", "Already connected to local storage");
        return;
    }

    auto * notifier = m_localStorage->notifier();

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::tagExpunged, this,
        &EnexImporter::onTagExpunged);

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::notebookExpunged, this,
        &EnexImporter::onNotebookExpunged);

    m_connectedToLocalStorage = true;
}

void EnexImporter::disconnectFromLocalStorageEvents()
{
    QNDEBUG(
        "enex::EnexImporter", "EnexImporter::disconnectFromLocalStorageEvents");

    if (!m_connectedToLocalStorage) {
        QNDEBUG(
            "enex::EnexImporter",
            "Not connected to local storage at the moment");
        return;
    }

    auto * notifier = m_localStorage->notifier();
    Q_ASSERT(notifier);
    notifier->disconnect(this);

    m_connectedToLocalStorage = false;
}

void EnexImporter::processNotesPendingTagAddition()
{
    QNDEBUG("enex::EnexImporter", "EnexImporter::processNotesPendingTagAddition");

    for (auto it = m_notesPendingTagAddition.begin();
         it != m_notesPendingTagAddition.end();)
    {
        auto & note = *it;

        const auto tagIt = m_tagNamesByImportedNoteLocalIds.find(note.localId());
        if (Q_UNLIKELY(tagIt == m_tagNamesByImportedNoteLocalIds.end())) {
            QNWARNING(
                "enex::EnexImporter",
                "Detected note within the list of those pending tags addition "
                "which doesn't really wait for tags addition");
            putNoteToLocalStorage(note);
            it = m_notesPendingTagAddition.erase(it);
            continue;
        }

        auto & tagNames = tagIt.value();
        QStringList nonexistentTagNames;
        nonexistentTagNames.reserve(tagNames.size());

        for (auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end();) {
            const auto & tagName = *tagNameIt;

            QString tagLocalId = m_tagModel.localIdForItemName(
                tagName,
                /* linked notebook guid = */ {});

            if (tagLocalId.isEmpty()) {
                nonexistentTagNames << tagName;
                QNDEBUG(
                    "enex::EnexImporter",
                    "No tag called \""
                        << tagName << "\" exists, it would need to be created");
                ++tagNameIt;
                continue;
            }

            if (m_expungedTagLocalIds.contains(tagLocalId)) {
                nonexistentTagNames << tagName;
                QNDEBUG(
                    "enex::EnexImporter",
                    "Tag local id "
                        << tagLocalId
                        << " found within the model has been marked as the one "
                        << "of expunged tag; will need to create a new tag "
                        << "with such name");

                ++tagNameIt;
                continue;
            }

            QNTRACE(
                "enex::EnexImporter",
                "Local id for tag name " << tagName << " is " << tagLocalId);

            if (!note.tagLocalIds().contains(tagLocalId)) {
                note.mutableTagLocalIds().append(tagLocalId);
            }

            tagNameIt = tagNames.erase(tagNameIt);
        }

        if (tagNames.isEmpty()) {
            QNTRACE(
                "enex::EnexImporter",
                "Found all tags for note with local id " << note.localId());
            m_tagNamesByImportedNoteLocalIds.erase(tagIt);
        }

        if (nonexistentTagNames.isEmpty()) {
            QNDEBUG(
                "enex::EnexImporter",
                "Found no nonexistent tags, can send this note to "
                    << "the local storage right away");
            putNoteToLocalStorage(note);
            it = m_notesPendingTagAddition.erase(it);
            continue;
        }

        for (const auto & nonexistentTag: std::as_const(nonexistentTagNames)) {
            if (const auto tagNameIt =
                    m_tagNamesPendingTagPutToLocalStorage.constFind(
                        nonexistentTag);
                tagNameIt != m_tagNamesPendingTagPutToLocalStorage.constEnd())
            {
                QNTRACE(
                    "enex::EnexImporter",
                    "Nonexistent tag with name "
                        << nonexistentTag
                        << " is already being added to the local storage");
            }
            else {
                putTagToLocalStorage(nonexistentTag);
            }
        }

        ++it;
    }
}

void EnexImporter::putNoteToLocalStorage(qevercloud::Note note)
{
    QNDEBUG(
        "enex::EnexImporter",
        "EnexImporter::putNoteToLocalStorage: note local id = "
            << note.localId());

    QNTRACE("enex::EnexImporter", "Note: " << note);
    
    QString noteLocalId = note.localId();
    m_noteLocalIdsPendingNotePutToLocalStorage.insert(noteLocalId);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto putNoteFuture = m_localStorage->putNote(std::move(note));
    auto putNoteThenFuture = threading::then(
        std::move(putNoteFuture), this,
        [this, noteLocalId, canceler] {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "enex::EnexImporter",
                "Successfully put note to local storage, note local id = "
                    << noteLocalId);
            onNotePutToLocalStorage(noteLocalId);
        });

    threading::onFailed(
        std::move(putNoteThenFuture), this,
        [this, noteLocalId = std::move(noteLocalId), canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            ErrorString errorDescription{QT_TR_NOOP(
                "Failed to put note to local storage")};
            errorDescription.appendBase(message.base());
            errorDescription.appendBase(message.additionalBases());
            errorDescription.details() = message.details();
            QNWARNING(
                "enex::EnexImporter",
                errorDescription << ", note local id = " << noteLocalId);

            onFailedToPutNoteToLocalStorage(errorDescription);
        });
}

void EnexImporter::putTagToLocalStorage(const QString & tagName)
{
    QNDEBUG(
        "enex::EnexImporter",
        "EnexImporter::putTagToLocalStorage: " << tagName);

    qevercloud::Tag newTag;
    newTag.setName(tagName);

    QString tagLocalId = newTag.localId();
    m_tagNamesPendingTagPutToLocalStorage.insert(tagName);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto putTagFuture = m_localStorage->putTag(std::move(newTag));
    auto putTagThenFuture = threading::then(
        std::move(putTagFuture), this,
        [this, tagLocalId, tagName, canceler] {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "enex::EnexImporter",
                "Successfully put tag to local storage, tag local id = "
                    << tagLocalId << ", tag name = " << tagName);
            onTagPutToLocalStorage(tagLocalId, tagName);
        });

    threading::onFailed(
        std::move(putTagThenFuture), this,
        [this, tagLocalId = std::move(tagLocalId),
         tagName, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            ErrorString errorDescription{
                QT_TR_NOOP("Failed to put tag to local storage")};
            errorDescription.appendBase(message.base());
            errorDescription.appendBase(message.additionalBases());
            errorDescription.details() = message.details();
            QNWARNING(
                "enex::EnexImporter",
                errorDescription << ", tag local id = " << tagLocalId
                                 << ", tag name = " << tagName);

            onFailedToPutTagToLocalStorage(std::move(errorDescription));
        });
}

void EnexImporter::putNotebookToLocalStorage(const QString & notebookName)
{
    QNDEBUG(
        "enex::EnexImporter",
        "EnexImporter::putNotebookToLocalStorage: notebook name = "
            << notebookName);

    qevercloud::Notebook newNotebook;
    newNotebook.setName(notebookName);

    QString notebookLocalId = newNotebook.localId();
    m_pendingNotebookPutToLocalStorage = true;

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto putNotebookFuture =
        m_localStorage->putNotebook(std::move(newNotebook));

    auto putNotebookThenFuture = threading::then(
        std::move(putNotebookFuture), this,
        [this, notebookLocalId = notebookLocalId, notebookName,
         canceler]() mutable {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "enex::EnexImporter",
                "Successfully put notebook to local storage, notebook local "
                    << "id = " << notebookLocalId << ", notebook name = "
                    << notebookName);
            onNotebookPutToLocalStorage(std::move(notebookLocalId));
        });

    threading::onFailed(
        std::move(putNotebookThenFuture), this,
        [this, notebookLocalId = std::move(notebookLocalId), notebookName,
         canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            ErrorString errorDescription{
                QT_TR_NOOP("Failed to put notebook to local storage")};
            errorDescription.appendBase(message.base());
            errorDescription.appendBase(message.additionalBases());
            errorDescription.details() = message.details();
            QNWARNING(
                "enex::EnexImporter",
                errorDescription << ", notebook local id = " << notebookLocalId
                                 << ", notebook name = " << notebookName);

            onFailedToPutNotebookToLocalStorage(std::move(errorDescription));
        });
}

utility::cancelers::ICancelerPtr EnexImporter::setupCanceler()
{
    if (!m_canceler) {
        m_canceler = std::make_shared<utility::cancelers::ManualCanceler>();
    }

    return m_canceler;
}

} // namespace quentier
