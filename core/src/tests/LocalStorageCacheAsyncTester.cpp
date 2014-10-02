#include "LocalStorageCacheAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

LocalStorageCacheAsyncTester::LocalStorageCacheAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
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

        // TODO: if m_addedNotebooksCount is strictly larger than the max number of notebooks
        // stored in the local storage cache, need to ensure that the first notebook is no longer in the cache

        // else do the following:
        addNotebook();
    }
    HANDLE_WRONG_STATE()
}

void LocalStorageCacheAsyncTester::onAddNotebookFailed(Notebook notebook, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
}

void LocalStorageCacheAsyncTester::onUpdateNotebookCompleted(Notebook notebook)
{
    // TODO: implement
    Q_UNUSED(notebook)
}

void LocalStorageCacheAsyncTester::onUpdateNotebookFailed(Notebook notebook, QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
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
    // TODO: implement
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

}
} // namespace qute_note
