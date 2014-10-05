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
                               "notebook in onAddNotebookCompleted doesn't match the original Notebook";
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
                errorDescription = "Found notebook which should not have been present in the local storage cache: " +
                                   pNotebook->ToQString();
                QNWARNING(errorDescription);
                emit failure(errorDescription);
            }
            else {
                updateNotebook();
                return;
            }

            return;
        }
        else if (m_addedNotebooksCount > 1)
        {
            const Notebook * pNotebook = m_pLocalStorageCacheManager->findNotebook(m_firstNotebook.localGuid(),
                                                                                   LocalStorageCacheManager::LocalGuid);
            if (!pNotebook) {
                QString error = "Notebook which should have been present in the local storage cache was not found there";
                QNWARNING(error << ", first notebook: " << m_firstNotebook);
                emit failure(error);
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
                               "notebook in onUpdateNotebookCompleted doesn't match the original Notebook";
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
        // TODO: switch to testing notes
        emit success();
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
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(notebook)
}

void LocalStorageCacheAsyncTester::onAddNoteFailed(Note note, Notebook notebook, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
}

void LocalStorageCacheAsyncTester::onUpdateNoteCompleted(Note note, Notebook notebook)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(notebook)
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
    // TODO: implement
    Q_UNUSED(tag)
}

void LocalStorageCacheAsyncTester::onAddTagFailed(Tag tag, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(errorDescription)
}

void LocalStorageCacheAsyncTester::onUpdateTagCompleted(Tag tag)
{
    // TODO: implement
    Q_UNUSED(tag)
}

void LocalStorageCacheAsyncTester::onUpdateTagFailed(Tag tag, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(errorDescription)
}

void LocalStorageCacheAsyncTester::onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook)
{
    // TODO: implement
    Q_UNUSED(linkedNotebook)
}

void LocalStorageCacheAsyncTester::onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(linkedNotebook)
    Q_UNUSED(errorDescription)
}

void LocalStorageCacheAsyncTester::onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook)
{
    // TODO: implement
    Q_UNUSED(linkedNotebook)
}

void LocalStorageCacheAsyncTester::onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(linkedNotebook)
    Q_UNUSED(errorDescription)
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
    QObject::connect(this, SIGNAL(addLinkedNotebook(LinkedNotebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(updateLinkedNotebook(LinkedNotebook)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(addSavedSearch(SavedSearch)),
                     m_pLocalStorageManagerThread, SLOT(onAddSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(updateSavedSearch(SavedSearch)),
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
    // TODO: implement
}

}
} // namespace qute_note
