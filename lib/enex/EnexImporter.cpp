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

#include "EnexImporter.h"

#include <lib/model/notebook/NotebookModel.h>
#include <lib/model/tag/TagModel.h>

#include <quentier/enml/ENMLConverter.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

#include <QFile>

namespace quentier {

EnexImporter::EnexImporter(
        const QString & enexFilePath,
        const QString & notebookName,
        LocalStorageManagerAsync & localStorageManagerAsync,
        TagModel & tagModel, NotebookModel & notebookModel,
        QObject * parent) :
    QObject(parent),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_tagModel(tagModel),
    m_notebookModel(notebookModel),
    m_enexFilePath(enexFilePath),
    m_notebookName(notebookName),
    m_notebookLocalUid(),
    m_tagNamesByImportedNoteLocalUid(),
    m_addTagRequestIdByTagNameBimap(),
    m_expungedTagLocalUids(),
    m_addNotebookRequestId(),
    m_notesPendingTagAddition(),
    m_addNoteRequestIds(),
    m_pendingNotebookModelToStart(false),
    m_connectedToLocalStorage(false)
{
    if (!m_tagModel.allTagsListed()) {
        QObject::connect(&m_tagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                         this, QNSLOT(EnexImporter,onAllTagsListed));
    }

    if (!m_notebookModel.allNotebooksListed()) {
        QObject::connect(&m_notebookModel,
                         QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                         this, QNSLOT(EnexImporter,onAllNotebooksListed));
    }
}

bool EnexImporter::isInProgress() const
{
    QNDEBUG("EnexImporter::isInProgress");

    if (!m_addTagRequestIdByTagNameBimap.empty()) {
        QNDEBUG("There are " << m_addTagRequestIdByTagNameBimap.size()
                << " pending requests to add tag");
        return true;
    }

    if (!m_addNoteRequestIds.isEmpty()) {
        QNDEBUG("There are " << m_addNoteRequestIds.size()
                << " pending requests to add note");
        return true;
    }

    if (!m_tagModel.allTagsListed() && !m_notesPendingTagAddition.isEmpty()) {
        QNDEBUG("Not all tags were listed in the tag model yet + "
                << "there are " << m_notesPendingTagAddition.size()
                << " notes pending tag addition");
        return true;
    }

    if (!m_notebookModel.allNotebooksListed() && m_pendingNotebookModelToStart) {
        QNDEBUG("Not all notebooks were listed in the notebook "
                "model yet, pending them");
        return true;
    }

    QNDEBUG("No signs of pending progress detected");
    return false;
}

void EnexImporter::start()
{
    QNDEBUG("EnexImporter::start");

    clear();

    if (Q_UNLIKELY(!m_notebookModel.allNotebooksListed())) {
        QNDEBUG("Not all notebooks were listed in the notebook "
                "model yet, delaying the start");
        m_pendingNotebookModelToStart = true;
        return;
    }

    if (m_notebookLocalUid.isEmpty())
    {
        if (Q_UNLIKELY(m_notebookName.isEmpty()))
        {
            ErrorString errorDescription(QT_TR_NOOP("Can't import ENEX: "
                                                    "the notebook name is empty"));
            QNWARNING(errorDescription);
            Q_EMIT enexImportFailed(errorDescription);
            return;
        }

        ErrorString notebookNameError;
        if (Q_UNLIKELY(!Notebook::validateName(m_notebookName, &notebookNameError)))
        {
            ErrorString errorDescription(QT_TR_NOOP("Can't import ENEX: "
                                                    "the notebook name is invalid"));
            errorDescription.appendBase(notebookNameError.base());
            errorDescription.appendBase(notebookNameError.additionalBases());
            errorDescription.details() = notebookNameError.details();
            QNWARNING(errorDescription);
            Q_EMIT enexImportFailed(errorDescription);
            return;
        }

        QString notebookLocalUid =
            m_notebookModel.localUidForItemName(m_notebookName,
                                                /* linked notebook guid = */
                                                QString());
        if (notebookLocalUid.isEmpty())
        {
            QNDEBUG("Could not find a user's own notebook's local "
                    << "uid for notebook name " << m_notebookName
                    << " within the notebook model; will create such notebook");

            if (m_addNotebookRequestId.isNull()) {
                addNotebookToLocalStorage(m_notebookName);
            }
            else {
                QNDEBUG("Already pending the notebook addition, "
                        << "request id = " << m_addNotebookRequestId);
            }

            return;
        }

        m_notebookLocalUid = notebookLocalUid;
    }

    QFile enexFile(m_enexFilePath);
    bool res = enexFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!res)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't import ENEX: can't open "
                                                "enex file for writing"));
        errorDescription.details() = m_enexFilePath;
        QNWARNING(errorDescription);
        Q_EMIT enexImportFailed(errorDescription);
        return;
    }

    QString enex = QString::fromUtf8(enexFile.readAll());
    enexFile.close();

    QVector<Note> importedNotes;
    ErrorString errorDescription;
    ENMLConverter converter;
    res = converter.importEnex(enex, importedNotes,
                               m_tagNamesByImportedNoteLocalUid,
                               errorDescription);
    if (!res) {
        Q_EMIT enexImportFailed(errorDescription);
        return;
    }

    for(auto it = importedNotes.begin(); it != importedNotes.end(); )
    {
        Note & note = *it;
        note.setNotebookLocalUid(m_notebookLocalUid);

        auto tagIt = m_tagNamesByImportedNoteLocalUid.find(note.localUid());
        if (tagIt == m_tagNamesByImportedNoteLocalUid.end())
        {
            QNTRACE("Imported note doesn't have tag names assigned "
                    << "to it, can add it to local storage right away: "
                    << note);
            addNoteToLocalStorage(note);
            it = importedNotes.erase(it);
            continue;
        }

        QStringList & tagNames = tagIt.value();
        for(auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end(); )
        {
            const QString & tagName = *tagNameIt;
            if (!tagName.isEmpty()) {
                ++tagNameIt;
                continue;
            }

            QNDEBUG("Removing empty tag name from the list of tag names for note "
                    << note.localUid());
            tagNameIt = tagNames.erase(tagNameIt);
        }

        if (Q_UNLIKELY(tagNames.isEmpty()))
        {
            QNDEBUG("No tag names are left for note " << note.localUid()
                    << " after the cleanup of empty tag names, can add the "
                    << "note to local storage right away");

            addNoteToLocalStorage(note);
            it = importedNotes.erase(it);
            continue;
        }

        ++it;
    }

    if (importedNotes.isEmpty()) {
        QNDEBUG("No notes had any tags, waiting for the completion "
                "of all add note requests");
        return;
    }

    m_notesPendingTagAddition = importedNotes;
    QNDEBUG("There are " << m_notesPendingTagAddition.size()
            << " notes which need tags assignment to them");

    if (!m_tagModel.allTagsListed()) {
        QNDEBUG("Not all tags were listed from the tag model, waiting for it");
        return;
    }

    processNotesPendingTagAddition();
}

void EnexImporter::clear()
{
    QNDEBUG("EnexImporter::clear");

    m_tagNamesByImportedNoteLocalUid.clear();
    m_addTagRequestIdByTagNameBimap.clear();
    m_expungedTagLocalUids.clear();

    m_addNotebookRequestId = QUuid();

    m_notesPendingTagAddition.clear();
    m_addNoteRequestIds.clear();

    m_pendingNotebookModelToStart = false;
}

void EnexImporter::onAddTagComplete(Tag tag, QUuid requestId)
{
    auto it = m_addTagRequestIdByTagNameBimap.right.find(requestId);
    if (it == m_addTagRequestIdByTagNameBimap.right.end()) {
        return;
    }

    QNDEBUG("EnexImporter::onAddTagComplete: request id = "
            << requestId << ", tag: " << tag);

    Q_UNUSED(m_addTagRequestIdByTagNameBimap.right.erase(it))

    if (Q_UNLIKELY(!tag.hasName()))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't import ENEX: internal error, could not create a "
                       "new tag in the local storage: the added tag has no name"));
        QNWARNING(errorDescription);
        Q_EMIT enexImportFailed(errorDescription);
        return;
    }

    QString tagName = tag.name().toLower();

    for(auto noteIt = m_notesPendingTagAddition.begin();
        noteIt != m_notesPendingTagAddition.end(); )
    {
        Note & note = *noteIt;

        auto tagIt = m_tagNamesByImportedNoteLocalUid.find(note.localUid());
        if (Q_UNLIKELY(tagIt == m_tagNamesByImportedNoteLocalUid.end()))
        {
            QNWARNING("Detected note within the list of those "
                      "pending tags addition which doesn't "
                      "really wait for tags addition");
            addNoteToLocalStorage(note);
            noteIt = m_notesPendingTagAddition.erase(noteIt);
            continue;
        }

        QStringList & tagNames = tagIt.value();
        for(auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end(); )
        {
            const QString & currentTagName = *tagNameIt;
            if (currentTagName.toLower() != tagName) {
                ++tagNameIt;
                continue;
            }

            QNDEBUG("Found tag for note " << note.localUid()
                    << ": name = " << tagName
                    << ", local uid = " << tag.localUid());

            if (!note.hasTagLocalUids() ||
                !note.tagLocalUids().contains(tag.localUid()))
            {
                note.addTagLocalUid(tag.localUid());
            }

            Q_UNUSED(tagNames.erase(tagNameIt))
            break;
        }

        if (!tagNames.isEmpty())
        {
            QNTRACE("Still pending " << tagNames.size()
                    << " tag names for note " << note.localUid());
            ++noteIt;
            continue;
        }

        QNDEBUG("Added the last missing tag for note " << note.localUid()
                << ", can send it to the local storage right away");

        Q_UNUSED(m_tagNamesByImportedNoteLocalUid.erase(tagIt))

        addNoteToLocalStorage(note);
        noteIt = m_notesPendingTagAddition.erase(noteIt);
    }
}

void EnexImporter::onAddTagFailed(Tag tag, ErrorString errorDescription,
                                  QUuid requestId)
{
    auto it = m_addTagRequestIdByTagNameBimap.right.find(requestId);
    if (it == m_addTagRequestIdByTagNameBimap.right.end()) {
        return;
    }

    QNDEBUG("EnexImporter::onAddTagFailed: request id = "
            << requestId << ", error description = "
            << errorDescription << ", tag: " << tag);

    Q_UNUSED(m_addTagRequestIdByTagNameBimap.right.erase(it))

    ErrorString error(QT_TR_NOOP("Can't import ENEX"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(error);
}

void EnexImporter::onExpungeTagComplete(
    Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNDEBUG("EnexImporter::onExpungeTagComplete: request id = "
            << requestId << ", tag: " << tag
            << "\nExpunged child tag local uids: "
            << expungedChildTagLocalUids.join(QStringLiteral(", ")));

    if (!isInProgress()) {
        QNDEBUG("Not in progress right now, won't do anything");
        return;
    }

    QStringList expungedTagLocalUids;
    expungedTagLocalUids.reserve(expungedChildTagLocalUids.size() + 1);
    expungedTagLocalUids = expungedChildTagLocalUids;
    expungedTagLocalUids << tag.localUid();

    for(auto it = expungedTagLocalUids.constBegin(),
        end = expungedTagLocalUids.constEnd(); it != end; ++it)
    {
        Q_UNUSED(m_expungedTagLocalUids.insert(*it))
    }

    // Just in case check if some of our pending notes have either of these tag
    // local uids, if so, remove them
    for(auto it = m_notesPendingTagAddition.begin(),
        end = m_notesPendingTagAddition.end(); it != end; ++it)
    {
        Note & note = *it;

        if (!note.hasTagLocalUids()) {
            continue;
        }

        QStringList tagLocalUids = note.tagLocalUids();
        for(auto tagIt = expungedTagLocalUids.constBegin(),
            tagEnd = expungedTagLocalUids.constEnd(); tagIt != tagEnd; ++tagIt)
        {
            if (tagLocalUids.contains(*tagIt)) {
                Q_UNUSED(tagLocalUids.removeAll(*tagIt))
            }
        }

        note.setTagLocalUids(tagLocalUids);
    }
}

void EnexImporter::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNDEBUG("EnexImporter::onAddNotebookComplete: request id = "
            << requestId << ", notebook: " << notebook);

    m_addNotebookRequestId = QUuid();
    m_notebookLocalUid = notebook.localUid();
    start();
}

void EnexImporter::onAddNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNWARNING("EnexImporter::onAddNotebookFailed: request id = "
              << requestId << ", error description: "
              << errorDescription << "; notebook: " << notebook);

    m_addNotebookRequestId = QUuid();

    ErrorString error(QT_TR_NOOP("Can't import ENEX"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(error);
}

void EnexImporter::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("EnexImporter::onExpungeNotebookComplete: request id = "
            << requestId << ", notebook: " << notebook);

    if (Q_UNLIKELY(!m_notebookLocalUid.isEmpty() &&
                   (notebook.localUid() == m_notebookLocalUid)))
    {
        ErrorString error(QT_TR_NOOP("Can't complete ENEX import: notebook was "
                                     "expunged during the import"));
        QNWARNING(error << ", notebook: " << notebook);
        clear();
        Q_EMIT enexImportFailed(error);
    }
}

void EnexImporter::onAddNoteComplete(Note note, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("EnexImporter::onAddNoteComplete: request id = "
            << requestId << ", note: " << note);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    if (!m_addNoteRequestIds.isEmpty()) {
        QNDEBUG("Still pending " << m_addNoteRequestIds.size()
                << " add note request ids");
        return;
    }

    if (!m_notesPendingTagAddition.isEmpty()) {
        QNDEBUG("There are still " << m_notesPendingTagAddition.size()
                << " notes pending tag addition");
        return;
    }

    QNDEBUG("There are no pending add note requests and no notes "
            "pending tags addition => it looks like the import "
            "has finished");
    Q_EMIT enexImportedSuccessfully(m_enexFilePath);
}

void EnexImporter::onAddNoteFailed(
    Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNWARNING("EnexImporter::onAddNoteFailed: request id = "
              << requestId << ", error description = "
              << errorDescription << ", note: " << note);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    ErrorString error(QT_TR_NOOP("Can't import ENEX"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT enexImportFailed(error);
}

void EnexImporter::onAllTagsListed()
{
    QNDEBUG("EnexImporter::onAllTagsListed");

    QObject::disconnect(&m_tagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                        this, QNSLOT(EnexImporter,onAllTagsListed));

    if (!isInProgress()) {
        return;
    }

    processNotesPendingTagAddition();
}

void EnexImporter::onAllNotebooksListed()
{
    QNDEBUG("EnexImporter::onAllNotebooksListed");

    QObject::disconnect(&m_notebookModel,
                        QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                        this,
                        QNSLOT(EnexImporter,onAllNotebooksListed));

    if (!m_pendingNotebookModelToStart) {
        return;
    }

    m_pendingNotebookModelToStart = false;
    start();
}

void EnexImporter::connectToLocalStorage()
{
    QNDEBUG("EnexImporter::connectToLocalStorage");

    if (m_connectedToLocalStorage) {
        QNDEBUG("Already connected to the local storage");
        return;
    }

    QObject::connect(this,
                     QNSIGNAL(EnexImporter,addTag,Tag,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onAddTagRequest,Tag,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addTagComplete,Tag,QUuid),
                     this,
                     QNSLOT(EnexImporter,onAddTagComplete,Tag,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addTagFailed,
                              Tag,ErrorString,QUuid),
                     this,
                     QNSLOT(EnexImporter,onAddTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,
                              Tag,QStringList,QUuid),
                     this,
                     QNSLOT(EnexImporter,onExpungeTagComplete,Tag,QStringList,QUuid));

    QObject::connect(this,
                     QNSIGNAL(EnexImporter,addNotebook,Notebook,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onAddNotebookRequest,
                            Notebook,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(EnexImporter,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(EnexImporter,onAddNotebookFailed,
                            Notebook,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(EnexImporter,onExpungeNotebookComplete,
                            Notebook,QUuid));

    QObject::connect(this,
                     QNSIGNAL(EnexImporter,addNote,Note,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,
                            Note,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,
                              Note,QUuid),
                     this,
                     QNSLOT(EnexImporter,onAddNoteComplete,Note,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,
                              Note,ErrorString,QUuid),
                     this,
                     QNSLOT(EnexImporter,onAddNoteFailed,
                            Note,ErrorString,QUuid));

    m_connectedToLocalStorage = true;
}

void EnexImporter::disconnectFromLocalStorage()
{
    QNDEBUG("EnexImporter::disconnectFromLocalStorage");

    if (!m_connectedToLocalStorage) {
        QNTRACE("Not connected to local storage at the moment");
        return;
    }

    QObject::disconnect(this,
                        QNSIGNAL(EnexImporter,addTag,Tag,QUuid),
                        &m_localStorageManagerAsync,
                        QNSLOT(LocalStorageManagerAsync,onAddTagRequest,
                               Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,
                                 addTagComplete,Tag,QUuid),
                        this,
                        QNSLOT(EnexImporter,onAddTagComplete,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,addTagFailed,
                                 Tag,ErrorString,QUuid),
                        this,
                        QNSLOT(EnexImporter,onAddTagFailed,
                               Tag,ErrorString,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,
                                 Tag,QStringList,QUuid),
                        this,
                        QNSLOT(EnexImporter,onExpungeTagComplete,
                               Tag,QStringList,QUuid));

    QObject::disconnect(this,
                        QNSIGNAL(EnexImporter,addNotebook,Notebook,QUuid),
                        &m_localStorageManagerAsync,
                        QNSLOT(LocalStorageManagerAsync,onAddNotebookRequest,
                               Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,addNotebookComplete,
                                 Notebook,QUuid),
                        this,
                        QNSLOT(EnexImporter,onAddNotebookComplete,
                               Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,addNotebookFailed,
                                 Notebook,ErrorString,QUuid),
                        this,
                        QNSLOT(EnexImporter,onAddNotebookFailed,
                               Notebook,ErrorString,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,
                                 Notebook,QUuid),
                        this,
                        QNSLOT(EnexImporter,onExpungeNotebookComplete,
                               Notebook,QUuid));

    QObject::disconnect(this,
                        QNSIGNAL(EnexImporter,addNote,Note,QUuid),
                        &m_localStorageManagerAsync,
                        QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,
                               Note,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,
                                 Note,QUuid),
                        this,
                        QNSLOT(EnexImporter,onAddNoteComplete,Note,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync,
                        QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,
                                 Note,ErrorString,QUuid),
                        this,
                        QNSLOT(EnexImporter,onAddNoteFailed,
                               Note,ErrorString,QUuid));

    m_connectedToLocalStorage = false;
}

void EnexImporter::processNotesPendingTagAddition()
{
    QNDEBUG("EnexImporter::processNotesPendingTagAddition");

    for(auto it = m_notesPendingTagAddition.begin();
        it != m_notesPendingTagAddition.end(); )
    {
        Note & note = *it;

        auto tagIt = m_tagNamesByImportedNoteLocalUid.find(note.localUid());
        if (Q_UNLIKELY(tagIt == m_tagNamesByImportedNoteLocalUid.end()))
        {
            QNWARNING("Detected note within the list of those "
                      "pending tags addition which doesn't "
                      "really wait for tags addition");
            addNoteToLocalStorage(note);
            it = m_notesPendingTagAddition.erase(it);
            continue;
        }

        QStringList & tagNames = tagIt.value();
        QStringList nonexistentTagNames;
        nonexistentTagNames.reserve(tagNames.size());

        for(auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end(); )
        {
            const QString & tagName = *tagNameIt;

            QString tagLocalUid =
                m_tagModel.localUidForItemName(tagName,
                                               /* linked notebook guid = */
                                               QString());
            if (tagLocalUid.isEmpty())
            {
                nonexistentTagNames << tagName;
                QNDEBUG("No tag called \"" << tagName
                        << "\" exists, it would need to be created");
                ++tagNameIt;
                continue;
            }

            if (m_expungedTagLocalUids.contains(tagLocalUid))
            {
                nonexistentTagNames << tagName;
                QNDEBUG("Tag local uid " << tagLocalUid
                        << " found within the model has been marked "
                        << "as the one of expunged tag; will need "
                        << "to create a new tag with such name");
                ++tagNameIt;
                continue;
            }

            QNTRACE("Local uid for tag name " << tagName
                    << " is " << tagLocalUid);

            if (!note.hasTagLocalUids() ||
                !note.tagLocalUids().contains(tagLocalUid))
            {
                note.addTagLocalUid(tagLocalUid);
            }

            tagNameIt = tagNames.erase(tagNameIt);
        }

        if (tagNames.isEmpty()) {
            QNTRACE("Found all tags for note with local uid "
                    << note.localUid());
            Q_UNUSED(m_tagNamesByImportedNoteLocalUid.erase(tagIt))
        }

        if (nonexistentTagNames.isEmpty())
        {
            QNDEBUG("Found no nonexistent tags, can send this "
                    "note to local storage right away");
            addNoteToLocalStorage(note);
            it = m_notesPendingTagAddition.erase(it);
            continue;
        }

        for(auto nonexistentTagIt = nonexistentTagNames.constBegin(),
            nonexistentTagEnd = nonexistentTagNames.constEnd();
            nonexistentTagIt != nonexistentTagEnd; ++nonexistentTagIt)
        {
            const QString & nonexistentTag = *nonexistentTagIt;

            auto requestIdIt = m_addTagRequestIdByTagNameBimap.left.find(
                nonexistentTag.toLower());
            if (requestIdIt != m_addTagRequestIdByTagNameBimap.left.end())
            {
                QNTRACE("Nonexistent tag " << nonexistentTag
                        << " is already being added to the local storage");
            }
            else
            {
                addTagToLocalStorage(nonexistentTag);
            }
        }

        ++it;
    }
}

void EnexImporter::addNoteToLocalStorage(const Note & note)
{
    QNDEBUG("EnexImporter::addNoteToLocalStorage");

    connectToLocalStorage();

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(requestId));
    QNTRACE("Emitting the request to add note to local storage: "
            << "request id = " << requestId
            << ", note: " << note);
    Q_EMIT addNote(note, requestId);
}

void EnexImporter::addTagToLocalStorage(const QString & tagName)
{
    QNDEBUG("EnexImporter::addTagToLocalStorage: " << tagName);

    connectToLocalStorage();

    Tag newTag;
    newTag.setName(tagName);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addTagRequestIdByTagNameBimap.insert(
            AddTagRequestIdByTagNameBimap::value_type(tagName.toLower(),
                                                      requestId)))
    QNTRACE("Emitting the request to add tag to local storage: "
            << "request id = " << requestId << ", tag: " << newTag);
    Q_EMIT addTag(newTag, requestId);
}

void EnexImporter::addNotebookToLocalStorage(const QString & notebookName)
{
    QNDEBUG("EnexImporter::addNotebookToLocalStorage: notebook "
            << "name = " << notebookName);

    connectToLocalStorage();

    Notebook newNotebook;
    newNotebook.setName(notebookName);

    m_addNotebookRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to add notebook to local storage: "
            << "request id = " << m_addNotebookRequestId
            << ", notebook: " << newNotebook);
    Q_EMIT addNotebook(newNotebook, m_addNotebookRequestId);
}

} // namespace quentier
