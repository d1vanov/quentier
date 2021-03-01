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

#include "EnexImporter.h"

#include <lib/model/notebook/NotebookModel.h>
#include <lib/model/tag/TagModel.h>

#include <quentier/enml/ENMLConverter.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Validation.h>

#include <QFile>

namespace quentier {

EnexImporter::EnexImporter(
    const QString & enexFilePath, const QString & notebookName,
    LocalStorageManagerAsync & localStorageManagerAsync, TagModel & tagModel,
    NotebookModel & notebookModel, QObject * parent) :
    QObject(parent),
    m_localStorageManagerAsync(localStorageManagerAsync), m_tagModel(tagModel),
    m_notebookModel(notebookModel), m_enexFilePath(enexFilePath),
    m_notebookName(notebookName)
{
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

EnexImporter::~EnexImporter() = default;

bool EnexImporter::isInProgress() const
{
    QNDEBUG("enex", "EnexImporter::isInProgress");

    if (!m_addTagRequestIdByTagNameBimap.empty()) {
        QNDEBUG(
            "enex",
            "There are " << m_addTagRequestIdByTagNameBimap.size()
                         << " pending requests to add tag");
        return true;
    }

    if (!m_addNoteRequestIds.isEmpty()) {
        QNDEBUG(
            "enex",
            "There are " << m_addNoteRequestIds.size()
                         << " pending requests to add note");
        return true;
    }

    if (!m_tagModel.allTagsListed() && !m_notesPendingTagAddition.isEmpty()) {
        QNDEBUG(
            "enex",
            "Not all tags were listed in the tag model yet + there are "
                << m_notesPendingTagAddition.size()
                << " notes pending tag addition");
        return true;
    }

    if (!m_notebookModel.allNotebooksListed() && m_pendingNotebookModelToStart)
    {
        QNDEBUG(
            "enex",
            "Not all notebooks were listed in the notebook model yet, pending "
                << "them");
        return true;
    }

    QNDEBUG("enex", "No signs of pending progress detected");
    return false;
}

void EnexImporter::start()
{
    QNDEBUG("enex", "EnexImporter::start");

    clear();

    if (Q_UNLIKELY(!m_notebookModel.allNotebooksListed())) {
        QNDEBUG(
            "enex",
            "Not all notebooks were listed in the notebook model yet, delaying "
                << "the start");
        m_pendingNotebookModelToStart = true;
        return;
    }

    if (m_notebookLocalId.isEmpty()) {
        if (Q_UNLIKELY(m_notebookName.isEmpty())) {
            ErrorString errorDescription(
                QT_TR_NOOP("Can't import ENEX: the notebook name is empty"));

            QNWARNING("enex", errorDescription);
            Q_EMIT enexImportFailed(errorDescription);
            return;
        }

        ErrorString notebookNameError;
        if (Q_UNLIKELY(
                !validateNotebookName(m_notebookName, &notebookNameError))) {
            ErrorString errorDescription(
                QT_TR_NOOP("Can't import ENEX: the notebook name is invalid"));
            errorDescription.appendBase(notebookNameError.base());
            errorDescription.appendBase(notebookNameError.additionalBases());
            errorDescription.details() = notebookNameError.details();
            QNWARNING("enex", errorDescription);
            Q_EMIT enexImportFailed(errorDescription);
            return;
        }

        const QString notebookLocalId = m_notebookModel.localIdForItemName(
            m_notebookName,
            /* linked notebook guid = */ {});

        if (notebookLocalId.isEmpty()) {
            QNDEBUG(
                "enex",
                "Could not find a user's own notebook's local "
                    << "id for notebook name " << m_notebookName
                    << " within the notebook model; will create such notebook");

            if (m_addNotebookRequestId.isNull()) {
                addNotebookToLocalStorage(m_notebookName);
            }
            else {
                QNDEBUG(
                    "enex",
                    "Already pending the notebook addition, request id = "
                        << m_addNotebookRequestId);
            }

            return;
        }

        m_notebookLocalId = notebookLocalId;
    }

    QFile enexFile{m_enexFilePath};
    if (Q_UNLIKELY(!enexFile.open(QIODevice::ReadOnly))) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't import ENEX: can't open enex file for writing"));
        errorDescription.details() = m_enexFilePath;
        QNWARNING("enex", errorDescription);
        Q_EMIT enexImportFailed(errorDescription);
        return;
    }

    const QString enex = QString::fromUtf8(enexFile.readAll());
    enexFile.close();

    QVector<qevercloud::Note> importedNotes;
    ErrorString errorDescription;
    ENMLConverter converter;

    bool res = converter.importEnex(
        enex, importedNotes, m_tagNamesByImportedNoteLocalId,
        errorDescription);

    if (!res) {
        Q_EMIT enexImportFailed(errorDescription);
        return;
    }

    for (auto it = importedNotes.begin(); it != importedNotes.end();) {
        auto & note = *it;
        note.setNotebookLocalId(m_notebookLocalId);

        auto tagIt = m_tagNamesByImportedNoteLocalId.find(note.localId());
        if (tagIt == m_tagNamesByImportedNoteLocalId.end()) {
            QNTRACE(
                "enex",
                "Imported note doesn't have tag names assigned "
                    << "to it, can add it to local storage right away: "
                    << note);

            addNoteToLocalStorage(note);
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
                "enex",
                "Removing empty tag name from the list of tag "
                    << "names for note " << note.localId());

            tagNameIt = tagNames.erase(tagNameIt);
        }

        if (Q_UNLIKELY(tagNames.isEmpty())) {
            QNDEBUG(
                "enex",
                "No tag names are left for note "
                    << note.localId()
                    << " after the cleanup of empty tag names, can add the "
                    << "note to local storage right away");

            addNoteToLocalStorage(note);
            it = importedNotes.erase(it);
            continue;
        }

        ++it;
    }

    if (importedNotes.isEmpty()) {
        QNDEBUG(
            "enex",
            "No notes had any tags, waiting for the completion of all add note "
                << "requests");
        return;
    }

    m_notesPendingTagAddition = importedNotes;
    QNDEBUG(
        "enex",
        "There are " << m_notesPendingTagAddition.size()
                     << " notes which need tags assignment to them");

    if (!m_tagModel.allTagsListed()) {
        QNDEBUG(
            "enex",
            "Not all tags were listed from the tag model, waiting for it");
        return;
    }

    processNotesPendingTagAddition();
}

void EnexImporter::clear()
{
    QNDEBUG("enex", "EnexImporter::clear");

    m_tagNamesByImportedNoteLocalId.clear();
    m_addTagRequestIdByTagNameBimap.clear();
    m_expungedTagLocalIds.clear();

    m_addNotebookRequestId = QUuid();

    m_notesPendingTagAddition.clear();
    m_addNoteRequestIds.clear();

    m_pendingNotebookModelToStart = false;
}

void EnexImporter::onAddTagComplete(qevercloud::Tag tag, QUuid requestId)
{
    const auto it = m_addTagRequestIdByTagNameBimap.right.find(requestId);
    if (it == m_addTagRequestIdByTagNameBimap.right.end()) {
        return;
    }

    QNDEBUG(
        "enex",
        "EnexImporter::onAddTagComplete: request id = " << requestId
                                                        << ", tag: " << tag);

    Q_UNUSED(m_addTagRequestIdByTagNameBimap.right.erase(it))

    if (Q_UNLIKELY(!tag.name())) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't import ENEX: internal error, could not create a "
                       "new tag in the local storage: the added tag has no "
                       "name"));
        QNWARNING("enex", errorDescription);
        Q_EMIT enexImportFailed(errorDescription);
        return;
    }

    QString tagName = tag.name()->toLower();

    for (auto noteIt = m_notesPendingTagAddition.begin();
         noteIt != m_notesPendingTagAddition.end();)
    {
        auto & note = *noteIt;

        const auto tagIt = m_tagNamesByImportedNoteLocalId.find(note.localId());
        if (Q_UNLIKELY(tagIt == m_tagNamesByImportedNoteLocalId.end())) {
            QNWARNING(
                "enex",
                "Detected note within the list of those pending tags addition "
                    << "which doesn't really wait for tags addition");

            addNoteToLocalStorage(note);
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
                "enex",
                "Found tag for note " << note.localId()
                                      << ": name = " << tagName
                                      << ", local id = " << tag.localId());

            if (!note.tagLocalIds().contains(tag.localId())) {
                note.mutableTagLocalIds().append(tag.localId());
            }

            Q_UNUSED(tagNames.erase(tagNameIt))
            break;
        }

        if (!tagNames.isEmpty()) {
            QNTRACE(
                "enex",
                "Still pending " << tagNames.size() << " tag names for note "
                                 << note.localId());

            ++noteIt;
            continue;
        }

        QNDEBUG(
            "enex",
            "Added the last missing tag for note "
                << note.localId()
                << ", can send it to the local storage right away");

        Q_UNUSED(m_tagNamesByImportedNoteLocalId.erase(tagIt))

        addNoteToLocalStorage(note);
        noteIt = m_notesPendingTagAddition.erase(noteIt);
    }
}

void EnexImporter::onAddTagFailed(
    qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId)
{
    const auto it = m_addTagRequestIdByTagNameBimap.right.find(requestId);
    if (it == m_addTagRequestIdByTagNameBimap.right.end()) {
        return;
    }

    QNDEBUG(
        "enex",
        "EnexImporter::onAddTagFailed: request id = "
            << requestId << ", error description = " << errorDescription
            << ", tag: " << tag);

    Q_UNUSED(m_addTagRequestIdByTagNameBimap.right.erase(it))

    ErrorString error(QT_TR_NOOP("Can't import ENEX"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(error);
}

void EnexImporter::onExpungeTagComplete(
    qevercloud::Tag tag, QStringList expungedChildTagLocalIds, QUuid requestId)
{
    QNDEBUG(
        "enex",
        "EnexImporter::onExpungeTagComplete: request id = "
            << requestId << ", tag: " << tag
            << "\nExpunged child tag local ids: "
            << expungedChildTagLocalIds.join(QStringLiteral(", ")));

    if (!isInProgress()) {
        QNDEBUG("enex", "Not in progress right now, won't do anything");
        return;
    }

    QStringList expungedTagLocalIds;
    expungedTagLocalIds.reserve(expungedChildTagLocalIds.size() + 1);
    expungedTagLocalIds = expungedChildTagLocalIds;
    expungedTagLocalIds << tag.localId();

    for (const auto & expungedTagLocalId: qAsConst(expungedTagLocalIds)) {
        Q_UNUSED(m_expungedTagLocalIds.insert(expungedTagLocalId))
    }

    // Just in case check if some of our pending notes have either of these tag
    // local ids, if so, remove them
    for (auto & note: m_notesPendingTagAddition) {
        auto tagLocalIds = note.tagLocalIds();
        if (tagLocalIds.isEmpty()) {
            continue;
        }

        for (const auto & expungedTagLocalId: qAsConst(expungedTagLocalIds)) {
            if (tagLocalIds.contains(expungedTagLocalId)) {
                Q_UNUSED(tagLocalIds.removeAll(expungedTagLocalId))
            }
        }

        note.setTagLocalIds(tagLocalIds);
    }
}

void EnexImporter::onAddNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNDEBUG(
        "enex",
        "EnexImporter::onAddNotebookComplete: request id = "
            << requestId << ", notebook: " << notebook);

    m_addNotebookRequestId = QUuid();
    m_notebookLocalId = notebook.localId();
    start();
}

void EnexImporter::onAddNotebookFailed(
    qevercloud::Notebook notebook, ErrorString errorDescription,
    QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNWARNING(
        "enex",
        "EnexImporter::onAddNotebookFailed: request id = "
            << requestId << ", error description: " << errorDescription
            << "; notebook: " << notebook);

    m_addNotebookRequestId = QUuid();

    ErrorString error(QT_TR_NOOP("Can't import ENEX"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(error);
}

void EnexImporter::onExpungeNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId)
{
    QNDEBUG(
        "enex",
        "EnexImporter::onExpungeNotebookComplete: request id = "
            << requestId << ", notebook: " << notebook);

    if (Q_UNLIKELY(
            !m_notebookLocalId.isEmpty() &&
            (notebook.localId() == m_notebookLocalId)))
    {
        ErrorString error(
            QT_TR_NOOP("Can't complete ENEX import: notebook was "
                       "expunged during the import"));
        QNWARNING("enex", error << ", notebook: " << notebook);
        clear();
        Q_EMIT enexImportFailed(error);
    }
}

void EnexImporter::onAddNoteComplete(qevercloud::Note note, QUuid requestId)
{
    const auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "enex",
        "EnexImporter::onAddNoteComplete: request id = " << requestId
                                                         << ", note: " << note);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    if (!m_addNoteRequestIds.isEmpty()) {
        QNDEBUG(
            "enex",
            "Still pending " << m_addNoteRequestIds.size()
                             << " add note request ids");
        return;
    }

    if (!m_notesPendingTagAddition.isEmpty()) {
        QNDEBUG(
            "enex",
            "There are still " << m_notesPendingTagAddition.size()
                               << " notes pending tag addition");
        return;
    }

    QNDEBUG(
        "enex",
        "There are no pending add note requests and no notes pending tags "
            << "addition => it looks like the import has finished");
    Q_EMIT enexImportedSuccessfully(m_enexFilePath);
}

void EnexImporter::onAddNoteFailed(
    qevercloud::Note note, ErrorString errorDescription, QUuid requestId)
{
    const auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNWARNING(
        "enex",
        "EnexImporter::onAddNoteFailed: request id = "
            << requestId << ", error description = " << errorDescription
            << ", note: " << note);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    ErrorString error(QT_TR_NOOP("Can't import ENEX"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(error);
}

void EnexImporter::onAllTagsListed()
{
    QNDEBUG("enex", "EnexImporter::onAllTagsListed");

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
    QNDEBUG("enex", "EnexImporter::onAllNotebooksListed");

    QObject::disconnect(
        &m_notebookModel, &NotebookModel::notifyAllNotebooksListed, this,
        &EnexImporter::onAllNotebooksListed);

    if (!m_pendingNotebookModelToStart) {
        return;
    }

    m_pendingNotebookModelToStart = false;
    start();
}

void EnexImporter::connectToLocalStorage()
{
    QNDEBUG("enex", "EnexImporter::connectToLocalStorage");

    if (m_connectedToLocalStorage) {
        QNDEBUG("enex", "Already connected to the local storage");
        return;
    }

    QObject::connect(
        this, &EnexImporter::addTag, &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddTagRequest);

    QObject::connect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addTagComplete,
        this, &EnexImporter::onAddTagComplete);

    QObject::connect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addTagFailed,
        this, &EnexImporter::onAddTagFailed);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeTagComplete, this,
        &EnexImporter::onExpungeTagComplete);

    QObject::connect(
        this, &EnexImporter::addNotebook, &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddNotebookRequest);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::addNotebookComplete, this,
        &EnexImporter::onAddNotebookComplete);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::addNotebookFailed, this,
        &EnexImporter::onAddNotebookFailed);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete, this,
        &EnexImporter::onExpungeNotebookComplete);

    QObject::connect(
        this, &EnexImporter::addNote, &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddNoteRequest);

    QObject::connect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addNoteComplete,
        this, &EnexImporter::onAddNoteComplete);

    QObject::connect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addNoteFailed,
        this, &EnexImporter::onAddNoteFailed);

    m_connectedToLocalStorage = true;
}

void EnexImporter::disconnectFromLocalStorage()
{
    QNDEBUG("enex", "EnexImporter::disconnectFromLocalStorage");

    if (!m_connectedToLocalStorage) {
        QNTRACE("enex", "Not connected to local storage at the moment");
        return;
    }

    QObject::disconnect(
        this, &EnexImporter::addTag, &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddTagRequest);

    QObject::disconnect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addTagComplete,
        this, &EnexImporter::onAddTagComplete);

    QObject::disconnect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addTagFailed,
        this, &EnexImporter::onAddTagFailed);

    QObject::disconnect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeTagComplete, this,
        &EnexImporter::onExpungeTagComplete);

    QObject::disconnect(
        this, &EnexImporter::addNotebook, &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddNotebookRequest);

    QObject::disconnect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::addNotebookComplete, this,
        &EnexImporter::onAddNotebookComplete);

    QObject::disconnect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::addNotebookFailed, this,
        &EnexImporter::onAddNotebookFailed);

    QObject::disconnect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete, this,
        &EnexImporter::onExpungeNotebookComplete);

    QObject::disconnect(
        this, &EnexImporter::addNote, &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddNoteRequest);

    QObject::disconnect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addNoteComplete,
        this, &EnexImporter::onAddNoteComplete);

    QObject::disconnect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addNoteFailed,
        this, &EnexImporter::onAddNoteFailed);

    m_connectedToLocalStorage = false;
}

void EnexImporter::processNotesPendingTagAddition()
{
    QNDEBUG("enex", "EnexImporter::processNotesPendingTagAddition");

    for (auto it = m_notesPendingTagAddition.begin();
         it != m_notesPendingTagAddition.end();)
    {
        auto & note = *it;

        auto tagIt = m_tagNamesByImportedNoteLocalId.find(note.localId());
        if (Q_UNLIKELY(tagIt == m_tagNamesByImportedNoteLocalId.end())) {
            QNWARNING(
                "enex",
                "Detected note within the list of those pending tags addition "
                    << "which doesn't really wait for tags addition");
            addNoteToLocalStorage(note);
            it = m_notesPendingTagAddition.erase(it);
            continue;
        }

        auto & tagNames = tagIt.value();
        QStringList nonexistentTagNames;
        nonexistentTagNames.reserve(tagNames.size());

        for (auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end();) {
            const auto & tagName = *tagNameIt;

            const QString tagLocalId = m_tagModel.localIdForItemName(
                tagName,
                /* linked notebook guid = */ {});

            if (tagLocalId.isEmpty()) {
                nonexistentTagNames << tagName;
                QNDEBUG(
                    "enex",
                    "No tag called \""
                        << tagName << "\" exists, it would need to be created");
                ++tagNameIt;
                continue;
            }

            if (m_expungedTagLocalIds.contains(tagLocalId)) {
                nonexistentTagNames << tagName;

                QNDEBUG(
                    "enex",
                    "Tag local id "
                        << tagLocalId
                        << " found within the model has been marked "
                        << "as the one of expunged tag; will need "
                        << "to create a new tag with such name");

                ++tagNameIt;
                continue;
            }

            QNTRACE(
                "enex",
                "Local id for tag name " << tagName << " is " << tagLocalId);

            if (!note.tagLocalIds().contains(tagLocalId)) {
                note.mutableTagLocalIds().append(tagLocalId);
            }

            tagNameIt = tagNames.erase(tagNameIt);
        }

        if (tagNames.isEmpty()) {
            QNTRACE(
                "enex",
                "Found all tags for note with local id " << note.localId());
            Q_UNUSED(m_tagNamesByImportedNoteLocalId.erase(tagIt))
        }

        if (nonexistentTagNames.isEmpty()) {
            QNDEBUG(
                "enex",
                "Found no nonexistent tags, can send this note to "
                    << "the local storage right away");
            addNoteToLocalStorage(note);
            it = m_notesPendingTagAddition.erase(it);
            continue;
        }

        for (const auto & nonexistentTag: qAsConst(nonexistentTagNames)) {
            auto requestIdIt = m_addTagRequestIdByTagNameBimap.left.find(
                nonexistentTag.toLower());

            if (requestIdIt != m_addTagRequestIdByTagNameBimap.left.end()) {
                QNTRACE(
                    "enex",
                    "Nonexistent tag "
                        << nonexistentTag
                        << " is already being added to the local storage");
            }
            else {
                addTagToLocalStorage(nonexistentTag);
            }
        }

        ++it;
    }
}

void EnexImporter::addNoteToLocalStorage(const qevercloud::Note & note)
{
    QNDEBUG("enex", "EnexImporter::addNoteToLocalStorage");

    connectToLocalStorage();

    const QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(requestId));

    QNTRACE(
        "enex",
        "Emitting the request to add note to local storage: "
            << "request id = " << requestId << ", note: " << note);

    Q_EMIT addNote(note, requestId);
}

void EnexImporter::addTagToLocalStorage(const QString & tagName)
{
    QNDEBUG("enex", "EnexImporter::addTagToLocalStorage: " << tagName);

    connectToLocalStorage();

    qevercloud::Tag newTag;
    newTag.setName(tagName);

    const QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addTagRequestIdByTagNameBimap.insert(
        AddTagRequestIdByTagNameBimap::value_type(
            tagName.toLower(), requestId)))

    QNTRACE(
        "enex",
        "Emitting the request to add tag to local storage: "
            << "request id = " << requestId << ", tag: " << newTag);

    Q_EMIT addTag(newTag, requestId);
}

void EnexImporter::addNotebookToLocalStorage(const QString & notebookName)
{
    QNDEBUG(
        "enex",
        "EnexImporter::addNotebookToLocalStorage: notebook name = "
            << notebookName);

    connectToLocalStorage();

    qevercloud::Notebook newNotebook;
    newNotebook.setName(notebookName);

    m_addNotebookRequestId = QUuid::createUuid();

    QNTRACE(
        "enex",
        "Emitting the request to add notebook to local storage: "
            << "request id = " << m_addNotebookRequestId
            << ", notebook: " << newNotebook);

    Q_EMIT addNotebook(newNotebook, m_addNotebookRequestId);
}

} // namespace quentier
