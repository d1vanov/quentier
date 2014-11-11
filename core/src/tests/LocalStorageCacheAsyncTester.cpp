#include "LocalStorageCacheAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <client/local_storage/DefaultLocalStorageCacheExpiryChecker.h>
#include <client/local_storage/LocalStorageCacheManager.h>
#include <logging/QuteNoteLogger.h>
#include <QThread>

namespace qute_note {
namespace test {

LocalStorageCacheAsyncTester::LocalStorageCacheAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThreadWorker(nullptr),
    m_pLocalStorageCacheManager(nullptr),
    m_pLocalStorageManagerThread(nullptr),
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
    QString username = "LocalStorageCacheAsyncTester";
    qint32 userId = 12;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
        m_pLocalStorageManagerThreadWorker = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new QThread(this);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(username, userId, startFromScratch);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();
}

void LocalStorageCacheAsyncTester::onWorkerInitialized()
{
    m_pLocalStorageCacheManager = m_pLocalStorageManagerThreadWorker->localStorageCacheManager();
    if (!m_pLocalStorageCacheManager) {
        QString error = "Local storage cache is not enabled by default for unknown reason";
        QNWARNING(error);
        emit failure(error);
        return;
    }

    addNotebook();
}

void LocalStorageCacheAsyncTester::onAddNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in LocalStorageCacheAsyncTester: " \
                           "found wrong state"; \
        QNWARNING(errorDescription << ": " << m_state); \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_NOTEBOOK_ADD_REQUEST)
    {
        if (m_currentNotebook != notebook) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "notebook in onAddNotebookCompleted doesn't match the original notebook";
            QNWARNING(errorDescription << "; original notebook: " << m_currentNotebook
                      << "\nFound notebook: " << notebook);
            emit failure(errorDescription);
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
            const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(m_firstNotebook.localGuid(),
                                                                                   LocalStorageCacheManager::LocalGuid);
            if (pNotebook) {
                errorDescription = "Found notebook which should not have been present in the local storage cache";
                QNWARNING(errorDescription << ": " << pNotebook->ToQString());
                emit failure(errorDescription);
            }
            else {
                updateNotebook();
            }

            return;
        }
        else if (m_addedNotebooksCount > 1)
        {
            const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(m_firstNotebook.localGuid(),
                                                                                   LocalStorageCacheManager::LocalGuid);
            if (!pNotebook) {
                errorDescription = "Notebook which should have been present in the local storage cache was not found there";
                QNWARNING(errorDescription << ", first notebook: " << m_firstNotebook);
                emit failure(errorDescription);
                return;
            }
        }

        addNotebook();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddNotebookFailed(Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", notebook: " << notebook);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onUpdateNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_NOTEBOOK_UPDATE_REQUEST)
    {
        if (m_secondNotebook != notebook) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "notebook in onUpdateNotebookCompleted doesn't match the original notebook";
            QNWARNING(errorDescription << "; original notebook: " << m_secondNotebook
                      << "\nFound notebook: " << notebook);
            emit failure(errorDescription);
            return;
        }

        const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(notebook.localGuid(),
                                                                               LocalStorageCacheManager::LocalGuid);
        if (!pNotebook) {
            errorDescription = "Updated notebook which should have been present in the local storage cache "
                               "was not found there";
            QNWARNING(errorDescription << ", notebook: " << notebook);
            emit failure(errorDescription);
            return;
        }
        else if (*pNotebook != notebook) {
            errorDescription = "Updated notebook does not match the notebook stored in local storage cache";
            QNWARNING(errorDescription << ", notebook: " << notebook);
            emit failure(errorDescription);
            return;
        }

        // Ok, updated notebook was cached correctly, moving to testing notes
        addNote();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateNotebookFailed(Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", notebook: " << notebook);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onAddNoteCompleted(Note note, Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_NOTE_ADD_REQUEST)
    {
        if (m_currentNote != note) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "note in onAddNoteCompleted doesn't match the original note";
            QNWARNING(errorDescription << "; original note: " << m_currentNote
                      << "\nFound note: " << note);
            emit failure(errorDescription);
            return;
        }

        if (m_secondNotebook != notebook) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "notebook in onAddNoteCompleted doesn't match the original notebook";
            QNWARNING(errorDescription << "; original notebook: " << m_secondNotebook
                      << "\nFound notebook: " << notebook);
            emit failure(errorDescription);
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
            const Note * pNote = m_pLocalStorageCacheManager->findNote(m_firstNote.localGuid(),
                                                                       LocalStorageCacheManager::LocalGuid);
            if (pNote) {
                errorDescription = "Found note which should not have been present in the local storage cache: " +
                                   pNote->ToQString();
                QNWARNING(errorDescription);
                emit failure(errorDescription);
            }
            else {
                updateNote();
            }

            return;
        }
        else if (m_addedNotesCount > 1)
        {
            const Note * pNote = m_pLocalStorageCacheManager->findNote(m_firstNote.localGuid(),
                                                                       LocalStorageCacheManager::LocalGuid);
            if (!pNote) {
                errorDescription = "Note which should have been present in the local storage cache was not found there";
                QNWARNING(errorDescription << ", first note: " << m_firstNote);
                emit failure(errorDescription);
                return;
            }
        }

        addNote();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", note: " << note << "\nnotebook: " << notebook);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onUpdateNoteCompleted(Note note, Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_NOTE_UPDATE_REQUEST)
    {
        if (m_secondNote != note) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "note in onUpdateNoteCompleted doesn't match the original note";
            QNWARNING(errorDescription << "; original note: " << m_secondNote
                      << "\nFound note: " << note);
            emit failure(errorDescription);
            return;
        }

        if (m_secondNotebook != notebook) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "notebook in onUpdateNoteCompleted doesn't match the original notebook";
            QNWARNING(errorDescription << "; original notebook: " << m_secondNotebook
                      << "\nFound notebook: " << notebook);
            emit failure(errorDescription);
            return;
        }

        const Note * pNote = m_pLocalStorageCacheManager->findNote(note.localGuid(),
                                                                   LocalStorageCacheManager::LocalGuid);
        if (!pNote) {
            errorDescription = "Updated note which should have been present in the local storage cache was not found there";
            QNWARNING(errorDescription << ", note: " << note);
            emit failure(errorDescription);
            return;
        }
        else if (*pNote != note) {
            errorDescription = "Updated note does not match the note stored in local storage cache";
            QNWARNING(errorDescription << ", note: " << note);
            emit failure(errorDescription);
            return;
        }

        // Ok, updated note was cached correctly, moving to testing tags
        addTag();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", note: " << note << "\nnotebook: " << notebook);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onAddTagCompleted(Tag tag, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_TAG_ADD_REQUEST)
    {
        if (m_currentTag != tag) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "tag in onAddTagCompleted doesn't match the original tag";
            QNWARNING(errorDescription << "; original tag: " << m_currentTag
                      << "\nFound tag: " << tag);
            emit failure(errorDescription);
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
            const Tag * pTag = m_pLocalStorageCacheManager->findTag(m_firstTag.localGuid(),
                                                                    LocalStorageCacheManager::LocalGuid);
            if (pTag) {
                errorDescription = "Found tag which should not have been present in the local storage cache";
                QNWARNING(errorDescription << ": " << *pTag);
                emit failure(errorDescription);
            }
            else {
                updateTag();
            }

            return;
        }
        else if (m_addedTagsCount > 1)
        {
            const Tag * pTag = m_pLocalStorageCacheManager->findTag(m_firstTag.localGuid(),
                                                                    LocalStorageCacheManager::LocalGuid);
            if (!pTag) {
                errorDescription = "Tag which should have been present in the local storage cache was not found there";
                QNWARNING(errorDescription << ", first tag: " << m_firstTag);
                emit failure(errorDescription);
                return;
            }

            // Check that we can also find the tag by name in the cache
            pTag = m_pLocalStorageCacheManager->findTagByName(m_firstTag.name());
            if (!pTag) {
                errorDescription = "Tag present in the local storage cache could not be found by tag name";
                QNWARNING(errorDescription << ", first tag: " << m_firstTag);
                emit failure(errorDescription);
                return;
            }
        }

        addTag();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", request id = " << requestId << ", tag: " << tag);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_TAG_UPDATE_REQUEST)
    {
        if (m_secondTag != tag) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "tag in onUpdateTagCompleted doesn't match the original tag";
            QNWARNING(errorDescription << "; original tag: " << m_secondTag
                      << "\nFound tag: " << tag);
            emit failure(errorDescription);
            return;
        }

        const Tag * pTag = m_pLocalStorageCacheManager->findTag(tag.localGuid(),
                                                                LocalStorageCacheManager::LocalGuid);
        if (!pTag) {
            errorDescription = "Updated tag which should have been present in the local storage cache "
                               "was not found there";
            QNWARNING(errorDescription << ", tag: " << tag);
            emit failure(errorDescription);
            return;
        }
        else if (*pTag != tag) {
            errorDescription = "Updated tag does not match the tag stored in the local storage cache";
            QNWARNING(errorDescription << ", tag: " << tag);
            emit failure(errorDescription);
            return;
        }

        // Ok, updated tag was cached correctly, moving to testing linked notebooks
        addLinkedNotebook();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", request id = " << requestId << ", tag: " << tag);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_LINKED_NOTEBOOK_ADD_REQUEST)
    {
        if (m_currentLinkedNotebook != linkedNotebook) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "linked notebook in onAddLinkedNotebookCompleted doesn't match the original linked notebook";
            QNWARNING(errorDescription << "; original linked notebook: " << m_currentLinkedNotebook
                      << "\nFound linked notebook: " << linkedNotebook);
            emit failure(errorDescription);
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
                errorDescription = "Found linked notebook which should not have been present in the local storage cache";
                QNWARNING(errorDescription << ": " << *pLinkedNotebook);
                emit failure(errorDescription);
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
                errorDescription = "Linked notebook which should have been present in the local storage cache was not found there";
                QNWARNING(errorDescription << ", first linked notebook: " << m_firstLinkedNotebook);
                emit failure(errorDescription);
                return;
            }
        }

        addLinkedNotebook();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", linked notebook: " << linkedNotebook);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_LINKED_NOTEBOOK_UPDATE_REQUEST)
    {
        if (m_secondLinkedNotebook != linkedNotebook) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "linked notebook in onUpdateLinkedNotebookCompleted doesn't match the original linked notebook";
            QNWARNING(errorDescription << "; original linked notebook: " << m_secondLinkedNotebook
                      << "\nFound linked notebook: " << linkedNotebook);
            emit failure(errorDescription);
            return;
        }

        const LinkedNotebook * pLinkedNotebook = m_pLocalStorageCacheManager->findLinkedNotebook(linkedNotebook.guid());
        if (!pLinkedNotebook) {
            errorDescription = "Updated linked notebook which should have been present in the local storage cache "
                               "was not found there";
            QNWARNING(errorDescription << ", linked notebook: " << linkedNotebook);
            emit failure(errorDescription);
            return;
        }
        else if (*pLinkedNotebook != linkedNotebook) {
            errorDescription = "Updated linked notebook does not match the linked notebook stored in the local storage cache";
            QNWARNING(errorDescription << ", linked notebook: " << linkedNotebook);
            emit failure(errorDescription);
            return;
        }

        // Ok, updated linked notebook was cached correctly, moving to testing saved searches
        addSavedSearch();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", linked notebook: " << linkedNotebook);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onAddSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_SAVED_SEARCH_ADD_REQUEEST)
    {
        if (m_currentSavedSearch != search) {
            errorDescription = "Internal error in LocalStorageCacheAsyncTester: "
                               "saved search in onAddSavedSearchCompleted doesn't match "
                               "the original saved search";
            QNWARNING(errorDescription << "; original saved search: " << m_currentSavedSearch
                      << "\nFound saved search: " << search);
            emit failure(errorDescription);
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
            const SavedSearch * pSavedSearch = m_pLocalStorageCacheManager->findSavedSearch(m_firstSavedSearch.localGuid(),
                                                                                            LocalStorageCacheManager::LocalGuid);
            if (pSavedSearch) {
                errorDescription = "Found saved search which should not have been present in the local storage cache";
                QNWARNING(errorDescription << ": " << *pSavedSearch);
                emit failure(errorDescription);
            }
            else {
                updateSavedSearch();
            }

            return;
        }
        else if (m_addedSavedSearchesCount > 1)
        {
            const SavedSearch * pSavedSearch = m_pLocalStorageCacheManager->findSavedSearch(m_firstSavedSearch.localGuid(),
                                                                                            LocalStorageCacheManager::LocalGuid);
            if (!pSavedSearch) {
                errorDescription = "Saved search which should have been present in the local storage cache was not found there";
                QNWARNING(errorDescription << ", first saved search: " << m_firstSavedSearch);
                emit failure(errorDescription);
                return;
            }
        }

        addSavedSearch();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", saved search: " << search);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_SAVED_SEARCH_UPDATE_REQUEST)
    {
        if (m_secondSavedSearch != search) {
            errorDescription = "Internal error in LocalStorageCachesyncTester: "
                               "saved search in onUpdateSavedSearchCompleted doesn't match the original saved search";
            QNWARNING(errorDescription << "; original saved search: " << m_secondSavedSearch
                      << "\nFound saved search: " << search);
            emit failure(errorDescription);
            return;
        }

        const SavedSearch * pSavedSearch = m_pLocalStorageCacheManager->findSavedSearch(search.localGuid(),
                                                                                        LocalStorageCacheManager::LocalGuid);
        if (!pSavedSearch) {
            errorDescription = "Updated saved search which should have been present in the local storage cache "
                               "was not found there";
            QNWARNING(errorDescription << ", saved search: " << search);
            emit failure(errorDescription);
            return;
        }
        else if (*pSavedSearch != search) {
            errorDescription = "Updated saved search does not match the saved search in the local storage cache";
            QNWARNING(errorDescription << ", saved search: " << search);
            emit failure(errorDescription);
            return;
        }

        // Ok, updated saved search was cached correctly, can finally return successfully
        emit success();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", saved search: " << search);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(failure(QString)), this, SIGNAL(failure(QString)));

    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(started()), m_pLocalStorageManagerThreadWorker, SLOT(init()));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(finished()), m_pLocalStorageManagerThread, SLOT(deleteLater()));

    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(initialized()), this, SLOT(onWorkerInitialized()));

    // Request --> slot connections
    QObject::connect(this, SIGNAL(addNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(updateNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(addNoteRequest(Note,Notebook,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,Notebook,QUuid)));
    QObject::connect(this, SIGNAL(updateNoteRequest(Note,Notebook,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook,QUuid)));
    QObject::connect(this, SIGNAL(addTagRequest(Tag,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(updateTagRequest(Tag,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(addLinkedNotebookRequest(LinkedNotebook,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(updateLinkedNotebookRequest(LinkedNotebook,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(addSavedSearchRequest(SavedSearch,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(updateSavedSearchRequest(SavedSearch,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch,QUuid)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addNotebookComplete(Notebook)),
                     this, SLOT(onAddNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addNotebookFailed(Notebook,QString)),
                     this, SLOT(onAddNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateNotebookComplete(Notebook)),
                     this, SLOT(onUpdateNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateNotebookFailed(Notebook,QString)),
                     this, SLOT(onUpdateNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addNoteComplete(Note,Notebook,QUuid)),
                     this, SLOT(onAddNoteCompleted(Note,Notebook,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addNoteFailed(Note,Notebook,QString,QUuid)),
                     this, SLOT(onAddNoteFailed(Note,Notebook,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateNoteComplete(Note,Notebook,QUuid)),
                     this, SLOT(onUpdateNoteCompleted(Note,Notebook,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateNoteFailed(Note,Notebook,QString,QUuid)),
                     this, SLOT(onUpdateNoteFailed(Note,Notebook,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addTagComplete(Tag,QUuid)),
                     this, SLOT(onAddTagCompleted(Tag,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addTagFailed(Tag,QString,QUuid)),
                     this, SLOT(onAddTagFailed(Tag,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag,QUuid)),
                     this, SLOT(onUpdateTagCompleted(Tag,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString,QUuid)),
                     this, SLOT(onUpdateTagFailed(Tag,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addLinkedNotebookComplete(LinkedNotebook,QUuid)),
                     this, SLOT(onAddLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addLinkedNotebookFailed(LinkedNotebook,QString,QUuid)),
                     this, SLOT(onAddLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookComplete(LinkedNotebook,QUuid)),
                     this, SLOT(onUpdateLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)),
                     this, SLOT(onUpdateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addSavedSearchComplete(SavedSearch,QUuid)),
                     this, SLOT(onAddSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addSavedSearchFailed(SavedSearch,QString,QUuid)),
                     this, SLOT(onAddSavedSearchFailed(SavedSearch,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateSavedSearchComplete(SavedSearch,QUuid)),
                     this, SLOT(onUpdateSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString,QUuid)),
                     this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString,QUuid)));
}

void LocalStorageCacheAsyncTester::addNotebook()
{
    m_currentNotebook = Notebook();

    m_currentNotebook.setUpdateSequenceNumber(static_cast<qint32>(m_addedNotebooksCount + 1));
    m_currentNotebook.setName("Fake notebook #" + QString::number(m_addedNotebooksCount + 1));
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
    m_secondNotebook.setName(m_secondNotebook.name() + "_modified");
    m_secondNotebook.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    m_state = STATE_SENT_NOTEBOOK_UPDATE_REQUEST;
    emit updateNotebookRequest(m_secondNotebook);
}

void LocalStorageCacheAsyncTester::addNote()
{
    m_currentNote = Note();
    m_currentNote.setUpdateSequenceNumber(static_cast<qint32>(m_addedNotesCount + 1));
    m_currentNote.setTitle("Fake note #" + QString::number(m_addedNotesCount + 1));
    m_currentNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    m_currentNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    m_currentNote.setActive(true);
    m_currentNote.setContent("<en-note><h1>Hello, world</h1></en-note>");

    m_state = STATE_SENT_NOTE_ADD_REQUEST;
    emit addNoteRequest(m_currentNote, m_secondNotebook);
}

void LocalStorageCacheAsyncTester::updateNote()
{
    m_secondNote.setUpdateSequenceNumber(m_secondNote.updateSequenceNumber() + 1);
    m_secondNote.setTitle(m_secondNote.title() + "_modified");
    m_secondNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    m_state = STATE_SENT_NOTE_UPDATE_REQUEST;
    emit updateNoteRequest(m_secondNote, m_secondNotebook);
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
    m_secondTag.setName(m_secondTag.name() + "_modified");

    m_state = STATE_SENT_TAG_UPDATE_REQUEST;
    emit updateTagRequest(m_secondTag);
}

void LocalStorageCacheAsyncTester::addLinkedNotebook()
{
    m_currentLinkedNotebook = LinkedNotebook();

    QString guid = "00000000-0000-0000-c000-0000000000";
    if (m_addedLinkedNotebooksCount < 9) {
        guid += "0";
    }
    guid += QString::number(m_addedLinkedNotebooksCount + 1);

    m_currentLinkedNotebook.setGuid(guid);
    m_currentLinkedNotebook.setShareName("Fake linked notebook share name");

    m_state = STATE_SENT_LINKED_NOTEBOOK_ADD_REQUEST;
    emit addLinkedNotebookRequest(m_currentLinkedNotebook);
}

void LocalStorageCacheAsyncTester::updateLinkedNotebook()
{
    m_secondLinkedNotebook.setShareName(m_secondLinkedNotebook.shareName() + "_modified");

    m_state = STATE_SENT_LINKED_NOTEBOOK_UPDATE_REQUEST;
    emit updateLinkedNotebookRequest(m_secondLinkedNotebook);
}

void LocalStorageCacheAsyncTester::addSavedSearch()
{
    m_currentSavedSearch = SavedSearch();

    m_currentSavedSearch.setName("Saved search #" + QString::number(m_addedSavedSearchesCount + 1));
    m_currentSavedSearch.setQuery("Fake saved search query #" + QString::number(m_addedSavedSearchesCount + 1));
    m_currentSavedSearch.setUpdateSequenceNumber(static_cast<qint32>(m_addedSavedSearchesCount + 1));
    m_currentSavedSearch.setQueryFormat(1);
    m_currentSavedSearch.setIncludeAccount(true);

    m_state = STATE_SENT_SAVED_SEARCH_ADD_REQUEEST;
    emit addSavedSearchRequest(m_currentSavedSearch);

}

void LocalStorageCacheAsyncTester::updateSavedSearch()
{
    m_secondSavedSearch.setName(m_secondSavedSearch.name() + "_modified");

    m_state = STATE_SENT_SAVED_SEARCH_UPDATE_REQUEST;
    emit updateSavedSearchRequest(m_secondSavedSearch);
}

} // namespace test
} // namespace qute_note
