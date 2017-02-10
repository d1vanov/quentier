/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LocalStorageCacheAsyncTester.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/local_storage/DefaultLocalStorageCacheExpiryChecker.h>
#include <quentier/local_storage/LocalStorageCacheManager.h>
#include <quentier/logging/QuentierLogger.h>
#include <QThread>

namespace quentier {
namespace test {

LocalStorageCacheAsyncTester::LocalStorageCacheAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThreadWorker(Q_NULLPTR),
    m_pLocalStorageCacheManager(Q_NULLPTR),
    m_pLocalStorageManagerThread(Q_NULLPTR),
    m_firstNotebook(),
    m_secondNotebook(),
    m_currentNotebook(),
    m_addedNotebooksCount(0),
    m_firstNote(),
    m_secondNote(),
    m_currentNote(),
    m_addedNotesCount(0),
    m_firstTag(),
    m_secondTag(),
    m_currentTag(),
    m_addedTagsCount(0),
    m_firstLinkedNotebook(),
    m_secondLinkedNotebook(),
    m_currentLinkedNotebook(),
    m_addedLinkedNotebooksCount(0),
    m_firstSavedSearch(),
    m_secondSavedSearch(),
    m_currentSavedSearch(),
    m_addedSavedSearchesCount(0)
{}

LocalStorageCacheAsyncTester::~LocalStorageCacheAsyncTester()
{
    // NOTE: shouldn't attempt to delete m_pLocalStorageManagerThread as Qt's parent-child system
    // should take care of that

    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
    }
}

void LocalStorageCacheAsyncTester::onInitTestCase()
{
    QString username = QStringLiteral("LocalStorageCacheAsyncTester");
    qint32 userId = 12;
    bool startFromScratch = true;
    bool overrideLock = false;

    if (m_pLocalStorageManagerThread) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = Q_NULLPTR;
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
        m_pLocalStorageManagerThreadWorker = Q_NULLPTR;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new QThread(this);

    Account account(username, Account::Type::Evernote, userId);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(account, startFromScratch, overrideLock);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();
}

void LocalStorageCacheAsyncTester::onWorkerInitialized()
{
    m_pLocalStorageCacheManager = m_pLocalStorageManagerThreadWorker->localStorageCacheManager();
    if (!m_pLocalStorageCacheManager) {
        QString error = QStringLiteral("Local storage cache is not enabled by default for unknown reason");
        QNWARNING(error);
        emit failure(error);
        return;
    }

    addNotebook();
}

void LocalStorageCacheAsyncTester::onAddNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: found wrong state"); \
        QNWARNING(errorDescription << ": " << m_state); \
        emit failure(errorDescription.nonLocalizedString()); \
    }

    if (m_state == STATE_SENT_NOTEBOOK_ADD_REQUEST)
    {
        if (m_currentNotebook != notebook) {
            errorDescription.base() = "Internal error in LocalStorageCacheAsyncTester: notebook in onAddNotebookCompleted "
                                      "doesn't match the original notebook";
            QNWARNING(errorDescription << QStringLiteral("; original notebook: ") << m_currentNotebook
                      << QStringLiteral("\nFound notebook: ") << notebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        ++m_addedNotebooksCount;

        if (m_addedNotebooksCount == 1) {
            m_firstNotebook = m_currentNotebook;
        }
        else if (m_addedNotebooksCount == 2) {
            m_secondNotebook = m_currentNotebook;
        }

        if (m_addedNotebooksCount > MAX_NOTEBOOKS_TO_STORE)
        {
            const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(m_firstNotebook.localUid(),
                                                                                   LocalStorageCacheManager::LocalUid);
            if (pNotebook) {
                errorDescription.base() = QStringLiteral("Found notebook which should not have been present in the local storage cache");
                QNWARNING(errorDescription << QStringLiteral(": ") << pNotebook->toString());
                emit failure(errorDescription.nonLocalizedString());
            }
            else {
                updateNotebook();
            }

            return;
        }
        else if (m_addedNotebooksCount > 1)
        {
            const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(m_firstNotebook.localUid(),
                                                                                   LocalStorageCacheManager::LocalUid);
            if (!pNotebook) {
                errorDescription.base() = QStringLiteral("Notebook which should have been present in the local storage cache was not found there");
                QNWARNING(errorDescription << QStringLiteral(", first notebook: ") << m_firstNotebook);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }
        }

        addNotebook();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", notebook: ") << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_NOTEBOOK_UPDATE_REQUEST)
    {
        if (m_secondNotebook != notebook) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: notebook in "
                                                     "onUpdateNotebookCompleted doesn't match the original notebook");
            QNWARNING(errorDescription << QStringLiteral("; original notebook: ") << m_secondNotebook
                      << QStringLiteral("\nFound notebook: ") << notebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(notebook.localUid(),
                                                                               LocalStorageCacheManager::LocalUid);
        if (!pNotebook) {
            errorDescription.base() = QStringLiteral("Updated notebook which should have been present "
                                                     "in the local storage cache was not found there");
            QNWARNING(errorDescription << QStringLiteral(", notebook: ") << notebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
        else if (*pNotebook != notebook) {
            errorDescription.base() = QStringLiteral("Updated notebook does not match the notebook stored in local storage cache");
            QNWARNING(errorDescription << QStringLiteral(", notebook: ") << notebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Ok, updated notebook was cached correctly, moving to testing notes
        addNote();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", notebook: ") << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onAddNoteCompleted(Note note, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_NOTE_ADD_REQUEST)
    {
        if (m_currentNote != note) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: "
                                                     "note in onAddNoteCompleted doesn't match the original note");
            QNWARNING(errorDescription << QStringLiteral("; original note: ") << m_currentNote
                      << QStringLiteral("\nFound note: ") << note);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        if (m_secondNotebook.localUid() != note.notebookLocalUid()) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: "
                                                     "notebook in onAddNoteCompleted doesn't match the original notebook");
            QNWARNING(errorDescription << QStringLiteral("; original notebook: ") << m_secondNotebook
                      << QStringLiteral("\nFound note's local notebook uid: ") << note.notebookLocalUid());
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        ++m_addedNotesCount;

        if (m_addedNotesCount == 1) {
            m_firstNote = m_currentNote;
        }
        else if (m_addedNotesCount == 2) {
            m_secondNote = m_currentNote;
        }

        if (m_addedNotesCount > MAX_NOTES_TO_STORE)
        {
            const Note * pNote = m_pLocalStorageCacheManager->findNote(m_firstNote.localUid(),
                                                                       LocalStorageCacheManager::LocalUid);
            if (pNote) {
                errorDescription.base() = QStringLiteral("Found note which should not have been present in the local storage cache: ");
                errorDescription.details() = pNote->toString();
                QNWARNING(errorDescription);
                emit failure(errorDescription.nonLocalizedString());
            }
            else {
                updateNote();
            }

            return;
        }
        else if (m_addedNotesCount > 1)
        {
            const Note * pNote = m_pLocalStorageCacheManager->findNote(m_firstNote.localUid(),
                                                                       LocalStorageCacheManager::LocalUid);
            if (!pNote) {
                errorDescription.base() = QStringLiteral("Note which should have been present in the local storage cache was not found there");
                QNWARNING(errorDescription << QStringLiteral(", first note: ") << m_firstNote);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }
        }

        addNote();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", note: ") << note);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onUpdateNoteCompleted(Note note, bool updateResources,
                                                         bool updateTags, QUuid requestId)
{
    Q_UNUSED(requestId)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_NOTE_UPDATE_REQUEST)
    {
        if (m_secondNote != note) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: "
                                                     "note in onUpdateNoteCompleted doesn't match the original note");
            QNWARNING(errorDescription << QStringLiteral("; original note: ") << m_secondNote
                      << QStringLiteral("\nFound note: ") << note);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        if (m_secondNotebook.localUid() != note.notebookLocalUid()) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: "
                                                     "note's notebook local uid in onUpdateNoteCompleted doesn't match "
                                                     "the original notebook local uid");
            QNWARNING(errorDescription << QStringLiteral("; original notebook: ") << m_secondNotebook
                      << QStringLiteral("\nUpdated note's notebook local uid: ") << note.notebookLocalUid());
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        const Note * pNote = m_pLocalStorageCacheManager->findNote(note.localUid(),
                                                                   LocalStorageCacheManager::LocalUid);
        if (!pNote) {
            errorDescription.base() = QStringLiteral("Updated note which should have been present in the local storage "
                                                     "cache was not found there");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
        else if (*pNote != note) {
            errorDescription.base() = QStringLiteral("Updated note does not match the note stored in local storage cache");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Ok, updated note was cached correctly, moving to testing tags
        addTag();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                                      ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", note: ") << note);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onAddTagCompleted(Tag tag, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_TAG_ADD_REQUEST)
    {
        if (m_currentTag != tag) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: "
                                                     "tag in onAddTagCompleted doesn't match the original tag");
            QNWARNING(errorDescription << QStringLiteral("; original tag: ") << m_currentTag
                      << QStringLiteral("\nFound tag: ") << tag);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        ++m_addedTagsCount;

        if (m_addedTagsCount == 1) {
            m_firstTag = m_currentTag;
        }
        else if (m_addedTagsCount == 2) {
            m_secondTag = m_currentTag;
        }

        if (m_addedTagsCount > MAX_TAGS_TO_STORE)
        {
            const Tag * pTag = m_pLocalStorageCacheManager->findTag(m_firstTag.localUid(),
                                                                    LocalStorageCacheManager::LocalUid);
            if (pTag) {
                errorDescription.base() = QStringLiteral("Found tag which should not have been present in the local storage cache");
                QNWARNING(errorDescription << QStringLiteral(": ") << *pTag);
                emit failure(errorDescription.nonLocalizedString());
            }
            else {
                updateTag();
            }

            return;
        }
        else if (m_addedTagsCount > 1)
        {
            const Tag * pTag = m_pLocalStorageCacheManager->findTag(m_firstTag.localUid(),
                                                                    LocalStorageCacheManager::LocalUid);
            if (!pTag) {
                errorDescription.base() = QStringLiteral("Tag which should have been present in the local storage cache was not found there");
                QNWARNING(errorDescription << QStringLiteral(", first tag: ") << m_firstTag);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }

            // Check that we can also find the tag by name in the cache
            pTag = m_pLocalStorageCacheManager->findTagByName(m_firstTag.name());
            if (!pTag) {
                errorDescription.base() = QStringLiteral("Tag present in the local storage cache could not be found by tag name");
                QNWARNING(errorDescription << QStringLiteral(", first tag: ") << m_firstTag);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }
        }

        addTag();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", request id = ") << requestId << QStringLiteral(", tag: ") << tag);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_TAG_UPDATE_REQUEST)
    {
        if (m_secondTag != tag) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: "
                                                     "tag in onUpdateTagCompleted doesn't match the original tag");
            QNWARNING(errorDescription << QStringLiteral("; original tag: ") << m_secondTag
                      << QStringLiteral("\nFound tag: ") << tag);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        const Tag * pTag = m_pLocalStorageCacheManager->findTag(tag.localUid(),
                                                                LocalStorageCacheManager::LocalUid);
        if (!pTag) {
            errorDescription.base() = QStringLiteral("Updated tag which should have been present in the local storage cache "
                                                     "was not found there");
            QNWARNING(errorDescription << QStringLiteral(", tag: ") << tag);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
        else if (*pTag != tag) {
            errorDescription.base() = QStringLiteral("Updated tag does not match the tag stored in the local storage cache");
            QNWARNING(errorDescription << QStringLiteral(", tag: ") << tag);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Ok, updated tag was cached correctly, moving to testing linked notebooks
        addLinkedNotebook();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", request id = ") << requestId << QStringLiteral(", tag: ") << tag);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_LINKED_NOTEBOOK_ADD_REQUEST)
    {
        if (m_currentLinkedNotebook != linkedNotebook) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: "
                                                     "linked notebook in onAddLinkedNotebookCompleted doesn't match the original linked notebook");
            QNWARNING(errorDescription << QStringLiteral("; original linked notebook: ") << m_currentLinkedNotebook
                      << QStringLiteral("\nFound linked notebook: ") << linkedNotebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        ++m_addedLinkedNotebooksCount;

        if (m_addedLinkedNotebooksCount == 1) {
            m_firstLinkedNotebook = m_currentLinkedNotebook;
        }
        else if (m_addedLinkedNotebooksCount == 2) {
            m_secondLinkedNotebook = m_currentLinkedNotebook;
        }

        if (m_addedLinkedNotebooksCount > MAX_LINKED_NOTEBOOKS_TO_STORE)
        {
            const LinkedNotebook * pLinkedNotebook = m_pLocalStorageCacheManager->findLinkedNotebook(m_firstLinkedNotebook.guid());
            if (pLinkedNotebook) {
                errorDescription.base() = QStringLiteral("Found linked notebook which should not have been present in the local storage cache");
                QNWARNING(errorDescription << QStringLiteral(": ") << *pLinkedNotebook);
                emit failure(errorDescription.nonLocalizedString());
            }
            else {
                updateLinkedNotebook();
            }

            return;
        }
        else if (m_addedLinkedNotebooksCount > 1)
        {
            const LinkedNotebook * pLinkedNotebook = m_pLocalStorageCacheManager->findLinkedNotebook(m_firstLinkedNotebook.guid());
            if (!pLinkedNotebook) {
                errorDescription.base() = QStringLiteral("Linked notebook which should have been present in the local storage cache was not found there");
                QNWARNING(errorDescription << QStringLiteral(", first linked notebook: ") << m_firstLinkedNotebook);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }
        }

        addLinkedNotebook();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", linked notebook: ") << linkedNotebook);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_LINKED_NOTEBOOK_UPDATE_REQUEST)
    {
        if (m_secondLinkedNotebook != linkedNotebook) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: linked notebook in "
                                                     "onUpdateLinkedNotebookCompleted doesn't match the original linked notebook");
            QNWARNING(errorDescription << QStringLiteral("; original linked notebook: ") << m_secondLinkedNotebook
                      << QStringLiteral("\nFound linked notebook: ") << linkedNotebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        const LinkedNotebook * pLinkedNotebook = m_pLocalStorageCacheManager->findLinkedNotebook(linkedNotebook.guid());
        if (!pLinkedNotebook) {
            errorDescription.base() = QStringLiteral("Updated linked notebook which should have been present "
                                                     "in the local storage cache was not found there");
            QNWARNING(errorDescription << QStringLiteral(", linked notebook: ") << linkedNotebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
        else if (*pLinkedNotebook != linkedNotebook) {
            errorDescription.base() = QStringLiteral("Updated linked notebook does not match the linked notebook stored "
                                                     "in the local storage cache");
            QNWARNING(errorDescription << QStringLiteral(", linked notebook: ") << linkedNotebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Ok, updated linked notebook was cached correctly, moving to testing saved searches
        addSavedSearch();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", linked notebook: ") << linkedNotebook);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onAddSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_SAVED_SEARCH_ADD_REQUEEST)
    {
        if (m_currentSavedSearch != search) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCacheAsyncTester: saved search in "
                                                     "onAddSavedSearchCompleted doesn't match the original saved search");
            QNWARNING(errorDescription << QStringLiteral("; original saved search: ") << m_currentSavedSearch
                      << QStringLiteral("\nFound saved search: ") << search);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        ++m_addedSavedSearchesCount;

        if (m_addedSavedSearchesCount == 1) {
            m_firstSavedSearch = m_currentSavedSearch;
        }
        else if (m_addedSavedSearchesCount == 2) {
            m_secondSavedSearch = m_currentSavedSearch;
        }

        if (m_addedSavedSearchesCount > MAX_SAVED_SEARCHES_TO_STORE)
        {
            const SavedSearch * pSavedSearch = m_pLocalStorageCacheManager->findSavedSearch(m_firstSavedSearch.localUid(),
                                                                                            LocalStorageCacheManager::LocalUid);
            if (pSavedSearch) {
                errorDescription.base() = QStringLiteral("Found saved search which should not have been present in the local storage cache");
                QNWARNING(errorDescription << QStringLiteral(": ") << *pSavedSearch);
                emit failure(errorDescription.nonLocalizedString());
            }
            else {
                updateSavedSearch();
            }

            return;
        }
        else if (m_addedSavedSearchesCount > 1)
        {
            const SavedSearch * pSavedSearch = m_pLocalStorageCacheManager->findSavedSearch(m_firstSavedSearch.localUid(),
                                                                                            LocalStorageCacheManager::LocalUid);
            if (!pSavedSearch) {
                errorDescription.base() = QStringLiteral("Saved search which should have been present in the local storage cache was not found there");
                QNWARNING(errorDescription << QStringLiteral(", first saved search: ") << m_firstSavedSearch);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }
        }

        addSavedSearch();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", saved search: ") << search);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_SAVED_SEARCH_UPDATE_REQUEST)
    {
        if (m_secondSavedSearch != search) {
            errorDescription.base() = QStringLiteral("Internal error in LocalStorageCachesyncTester: saved search in "
                                                     "onUpdateSavedSearchCompleted doesn't match the original saved search");
            QNWARNING(errorDescription << QStringLiteral("; original saved search: ") << m_secondSavedSearch
                      << QStringLiteral("\nFound saved search: ") << search);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        const SavedSearch * pSavedSearch = m_pLocalStorageCacheManager->findSavedSearch(search.localUid(),
                                                                                        LocalStorageCacheManager::LocalUid);
        if (!pSavedSearch) {
            errorDescription.base() = QStringLiteral("Updated saved search which should have been present "
                                                     "in the local storage cache was not found there");
            QNWARNING(errorDescription << QStringLiteral(", saved search: ") << search);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
        else if (*pSavedSearch != search) {
            errorDescription.base() = QStringLiteral("Updated saved search does not match the saved search in the local storage cache");
            QNWARNING(errorDescription << QStringLiteral(", saved search: ") << search);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Ok, updated saved search was cached correctly, can finally return successfully
        emit success();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", saved search: ") << search);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::onFailure(ErrorString errorDescription)
{
    QNWARNING(QStringLiteral("LocalStorageCacheAsyncTester::onFailure: ") << errorDescription);
    emit failure(errorDescription.nonLocalizedString());
}

void LocalStorageCacheAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,failure,ErrorString),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onFailure,ErrorString));

    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,started),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,init));
    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     m_pLocalStorageManagerThread, QNSLOT(LocalStorageManagerThreadWorker,deleteLater));

    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,initialized),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onWorkerInitialized));

    // Request --> slot connections
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,addNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,updateNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,addNoteRequest,Note,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,updateNoteRequest,Note,bool,bool,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,addTagRequest,Tag,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,updateTagRequest,Tag,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,addLinkedNotebookRequest,LinkedNotebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddLinkedNotebookRequest,LinkedNotebook,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,updateLinkedNotebookRequest,LinkedNotebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateLinkedNotebookRequest,LinkedNotebook,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,addSavedSearchRequest,SavedSearch,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(LocalStorageCacheAsyncTester,updateSavedSearchRequest,SavedSearch,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteComplete,Note,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddNoteCompleted,Note,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateNoteCompleted,Note,bool,bool,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateNoteFailed,Note,bool,bool,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddTagCompleted,Tag,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateTagCompleted,Tag,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addLinkedNotebookFailed,LinkedNotebook,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddLinkedNotebookFailed,LinkedNotebook,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateLinkedNotebookFailed,LinkedNotebook,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateLinkedNotebookFailed,LinkedNotebook,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onAddSavedSearchFailed,SavedSearch,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(LocalStorageCacheAsyncTester,onUpdateSavedSearchFailed,SavedSearch,ErrorString,QUuid));
}

void LocalStorageCacheAsyncTester::addNotebook()
{
    m_currentNotebook = Notebook();

    m_currentNotebook.setUpdateSequenceNumber(static_cast<qint32>(m_addedNotebooksCount + 1));
    m_currentNotebook.setName(QStringLiteral("Fake notebook #") + QString::number(m_addedNotebooksCount + 1));
    m_currentNotebook.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    m_currentNotebook.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    m_currentNotebook.setDefaultNotebook((m_addedNotebooksCount == 0) ? true : false);
    m_currentNotebook.setLastUsed(false);

    m_state = STATE_SENT_NOTEBOOK_ADD_REQUEST;
    emit addNotebookRequest(m_currentNotebook);
}

void LocalStorageCacheAsyncTester::updateNotebook()
{
    m_secondNotebook.setUpdateSequenceNumber(m_secondNotebook.updateSequenceNumber() + 1);
    m_secondNotebook.setName(m_secondNotebook.name() + QStringLiteral("_modified"));
    m_secondNotebook.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    m_state = STATE_SENT_NOTEBOOK_UPDATE_REQUEST;
    emit updateNotebookRequest(m_secondNotebook);
}

void LocalStorageCacheAsyncTester::addNote()
{
    m_currentNote = Note();
    m_currentNote.setUpdateSequenceNumber(static_cast<qint32>(m_addedNotesCount + 1));
    m_currentNote.setTitle(QStringLiteral("Fake note #") + QString::number(m_addedNotesCount + 1));
    m_currentNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    m_currentNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    m_currentNote.setActive(true);
    m_currentNote.setContent(QStringLiteral("<en-note><h1>Hello, world</h1></en-note>"));
    m_currentNote.setNotebookLocalUid(m_secondNotebook.localUid());

    m_state = STATE_SENT_NOTE_ADD_REQUEST;
    emit addNoteRequest(m_currentNote);
}

void LocalStorageCacheAsyncTester::updateNote()
{
    m_secondNote.setUpdateSequenceNumber(m_secondNote.updateSequenceNumber() + 1);
    m_secondNote.setTitle(m_secondNote.title() + QStringLiteral("_modified"));
    m_secondNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    m_state = STATE_SENT_NOTE_UPDATE_REQUEST;
    emit updateNoteRequest(m_secondNote, /* update resources = */ true, /* update tags = */ true);
}

void LocalStorageCacheAsyncTester::addTag()
{
    m_currentTag = Tag();
    m_currentTag.setUpdateSequenceNumber(static_cast<qint32>(m_addedTagsCount + 1));
    m_currentTag.setName("Fake tag #" + QString::number(m_addedTagsCount + 1));

    m_state = STATE_SENT_TAG_ADD_REQUEST;
    emit addTagRequest(m_currentTag);
}

void LocalStorageCacheAsyncTester::updateTag()
{
    m_secondTag.setUpdateSequenceNumber(m_secondTag.updateSequenceNumber() + 1);
    m_secondTag.setName(m_secondTag.name() + QStringLiteral("_modified"));

    m_state = STATE_SENT_TAG_UPDATE_REQUEST;
    emit updateTagRequest(m_secondTag);
}

void LocalStorageCacheAsyncTester::addLinkedNotebook()
{
    m_currentLinkedNotebook = LinkedNotebook();

    QString guid = QStringLiteral("00000000-0000-0000-c000-0000000000");
    if (m_addedLinkedNotebooksCount < 9) {
        guid += "0";
    }
    guid += QString::number(m_addedLinkedNotebooksCount + 1);

    m_currentLinkedNotebook.setGuid(guid);
    m_currentLinkedNotebook.setShareName(QStringLiteral("Fake linked notebook share name"));

    m_state = STATE_SENT_LINKED_NOTEBOOK_ADD_REQUEST;
    emit addLinkedNotebookRequest(m_currentLinkedNotebook);
}

void LocalStorageCacheAsyncTester::updateLinkedNotebook()
{
    m_secondLinkedNotebook.setShareName(m_secondLinkedNotebook.shareName() + QStringLiteral("_modified"));

    m_state = STATE_SENT_LINKED_NOTEBOOK_UPDATE_REQUEST;
    emit updateLinkedNotebookRequest(m_secondLinkedNotebook);
}

void LocalStorageCacheAsyncTester::addSavedSearch()
{
    m_currentSavedSearch = SavedSearch();

    m_currentSavedSearch.setName(QStringLiteral("Saved search #") + QString::number(m_addedSavedSearchesCount + 1));
    m_currentSavedSearch.setQuery(QStringLiteral("Fake saved search query #") + QString::number(m_addedSavedSearchesCount + 1));
    m_currentSavedSearch.setUpdateSequenceNumber(static_cast<qint32>(m_addedSavedSearchesCount + 1));
    m_currentSavedSearch.setQueryFormat(1);
    m_currentSavedSearch.setIncludeAccount(true);

    m_state = STATE_SENT_SAVED_SEARCH_ADD_REQUEEST;
    emit addSavedSearchRequest(m_currentSavedSearch);

}

void LocalStorageCacheAsyncTester::updateSavedSearch()
{
    m_secondSavedSearch.setName(m_secondSavedSearch.name() + QStringLiteral("_modified"));

    m_state = STATE_SENT_SAVED_SEARCH_UPDATE_REQUEST;
    emit updateSavedSearchRequest(m_secondSavedSearch);
}

} // namespace test
} // namespace quentier
