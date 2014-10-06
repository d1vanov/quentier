#include "LocalStorageCacheAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <client/local_storage/DefaultLocalStorageCacheExpiryChecker.h>
#include <client/local_storage/LocalStorageCacheManager.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

LocalStorageCacheAsyncTester::LocalStorageCacheAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_pLocalStorageCacheManager(nullptr),
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
}

void LocalStorageCacheAsyncTester::onInitTestCase()
{
    QString username = "LocalStorageCacheAsyncTester";
    qint32 userId = 12;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread != nullptr) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch, this);

    createConnections();

    m_pLocalStorageCacheManager = m_pLocalStorageManagerThread->localStorageCacheManager();
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

void LocalStorageCacheAsyncTester::onAddNoteCompleted(Note note, Notebook notebook)
{
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

void LocalStorageCacheAsyncTester::onAddNoteFailed(Note note, Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", note: " << note << "\nnotebook: " << notebook);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onUpdateNoteCompleted(Note note, Notebook notebook)
{
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

void LocalStorageCacheAsyncTester::onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
}

void LocalStorageCacheAsyncTester::onAddTagCompleted(Tag tag)
{
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
        }

        addTag();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddTagFailed(Tag tag, QString errorDescription)
{
    QNWARNING(errorDescription << ", tag: " << tag);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onUpdateTagCompleted(Tag tag)
{
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

void LocalStorageCacheAsyncTester::onUpdateTagFailed(Tag tag, QString errorDescription)
{
    QNWARNING(errorDescription << ", tag: " << tag);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook)
{
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

void LocalStorageCacheAsyncTester::onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", linked notebook: " << linkedNotebook);
    emit failure(errorDescription);
}

void LocalStorageCacheAsyncTester::onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook)
{
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
        // TODO: switch to testing saved searches
        emit success();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", linked notebook: " << linkedNotebook);
}

void LocalStorageCacheAsyncTester::onAddSavedSearchCompleted(SavedSearch search)
{
    // TODO: implement
    Q_UNUSED(search)
}

void LocalStorageCacheAsyncTester::onAddSavedSearchFailed(SavedSearch search, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(errorDescription)
}

void LocalStorageCacheAsyncTester::onUpdateSavedSearchCompleted(SavedSearch search)
{
    // TODO: implement
    Q_UNUSED(search)
}

void LocalStorageCacheAsyncTester::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(errorDescription)
}

void LocalStorageCacheAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(addNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(updateNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(addNoteRequest(Note,Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(updateNoteRequest(Note,Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(addTagRequest(Tag)),
                     m_pLocalStorageManagerThread, SLOT(onAddTagRequest(Tag)));
    QObject::connect(this, SIGNAL(updateTagRequest(Tag)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateTagRequest(Tag)));
    QObject::connect(this, SIGNAL(addLinkedNotebookRequest(LinkedNotebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(updateLinkedNotebookRequest(LinkedNotebook)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(addSavedSearchRequest(SavedSearch)),
                     m_pLocalStorageManagerThread, SLOT(onAddSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(updateSavedSearchRequest(SavedSearch)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateSavedSearchRequest(SavedSearch)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookComplete(Notebook)),
                     this, SLOT(onAddNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookFailed(Notebook,QString)),
                     this, SLOT(onAddNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNotebookComplete(Notebook)),
                     this, SLOT(onUpdateNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNotebookFailed(Notebook,QString)),
                     this, SLOT(onUpdateNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteComplete(Note,Notebook)),
                     this, SLOT(onAddNoteCompleted(Note,Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteFailed(Note,Notebook,QString)),
                     this, SLOT(onAddNoteFailed(Note,Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNoteComplete(Note,Notebook)),
                     this, SLOT(onUpdateNoteCompleted(Note,Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNoteFailed(Note,Notebook,QString)),
                     this, SLOT(onUpdateNoteFailed(Note,Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addTagComplete(Tag)),
                     this, SLOT(onAddTagCompleted(Tag)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addTagFailed(Tag,QString)),
                     this, SLOT(onAddTagFailed(Tag,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateTagComplete(Tag)),
                     this, SLOT(onUpdateTagCompleted(Tag)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateTagFailed(Tag,QString)),
                     this, SLOT(onUpdateTagFailed(Tag,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addLinkedNotebookComplete(LinkedNotebook)),
                     this, SLOT(onAddLinkedNotebookCompleted(LinkedNotebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addLinkedNotebookFailed(LinkedNotebook,QString)),
                     this, SLOT(onAddLinkedNotebookFailed(LinkedNotebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateLinkedNotebookComplete(LinkedNotebook)),
                     this, SLOT(onUpdateLinkedNotebookCompleted(LinkedNotebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateLinkedNotebookFailed(LinkedNotebook,QString)),
                     this, SLOT(onUpdateLinkedNotebookFailed(LinkedNotebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addSavedSearchComplete(SavedSearch)),
                     this, SLOT(onAddSavedSearchCompleted(SavedSearch)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addSavedSearchFailed(SavedSearch,QString)),
                     this, SLOT(onAddSavedSearchFailed(SavedSearch,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateSavedSearchComplete(SavedSearch)),
                     this, SLOT(onUpdateSavedSearchCompleted(SavedSearch)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateSavedSearchFailed(SavedSearch,QString)),
                     this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString)));
}

void LocalStorageCacheAsyncTester::addNotebook()
{
    m_currentNotebook = Notebook();

    m_currentNotebook.setUpdateSequenceNumber(m_addedNotebooksCount + 1);
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
    m_currentNote.setUpdateSequenceNumber(m_addedNotesCount + 1);
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
    m_currentTag.setUpdateSequenceNumber(m_addedTagsCount + 1);
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

} // namespace test
} // namespace qute_note
