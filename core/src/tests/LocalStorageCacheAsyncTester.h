#ifndef __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_CACHE_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_CACHE_ASYNC_TESTER_H

#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/SavedSearch.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(LocalStorageCacheManager)

namespace test {

class LocalStorageCacheAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit LocalStorageCacheAsyncTester(QObject * parent = nullptr);
    virtual ~LocalStorageCacheAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals
    void addNotebookRequest(Notebook notebook);
    void updateNotebookRequest(Notebook notebook);

    void addNoteRequest(Note note, Notebook notebook);
    void updateNoteRequest(Note note, Notebook notebook);

    void addTagRequest(Tag tag, QUuid requestId = QUuid());
    void updateTagRequest(Tag tag, QUuid requestId = QUuid());

    void addLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId = QUuid());
    void updateLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId = QUuid());

    void addSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());
    void updateSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();

    void onAddNotebookCompleted(Notebook notebook);
    void onAddNotebookFailed(Notebook notebook, QString errorDescription);

    void onUpdateNotebookCompleted(Notebook notebook);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription);

    void onAddNoteCompleted(Note note, Notebook notebook);
    void onAddNoteFailed(Note note, Notebook notebook, QString errorDescription);

    void onUpdateNoteCompleted(Note note, Notebook notebook);
    void onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription);

    void onAddTagCompleted(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId);

    void onUpdateTagCompleted(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId);

    void onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId);
    void onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId);

    void onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId);
    void onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId);

    void onAddSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);

    void onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);

private:
    void createConnections();

    void addNotebook();
    void updateNotebook();

    void addNote();
    void updateNote();

    void addTag();
    void updateTag();

    void addLinkedNotebook();
    void updateLinkedNotebook();

    void addSavedSearch();
    void updateSavedSearch();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_NOTEBOOK_ADD_REQUEST,
        STATE_SENT_NOTEBOOK_UPDATE_REQUEST,
        STATE_SENT_NOTE_ADD_REQUEST,
        STATE_SENT_NOTE_UPDATE_REQUEST,
        STATE_SENT_TAG_ADD_REQUEST,
        STATE_SENT_TAG_UPDATE_REQUEST,
        STATE_SENT_LINKED_NOTEBOOK_ADD_REQUEST,
        STATE_SENT_LINKED_NOTEBOOK_UPDATE_REQUEST,
        STATE_SENT_SAVED_SEARCH_ADD_REQUEEST,
        STATE_SENT_SAVED_SEARCH_UPDATE_REQUEST
    };

    State   m_state;
    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
    const LocalStorageCacheManager *    m_pLocalStorageCacheManager;
    QThread *                           m_pLocalStorageManagerThread;

    Notebook    m_firstNotebook;
    Notebook    m_secondNotebook;
    Notebook    m_currentNotebook;
    size_t      m_addedNotebooksCount;

    Note        m_firstNote;
    Note        m_secondNote;
    Note        m_currentNote;
    size_t      m_addedNotesCount;

    Tag         m_firstTag;
    Tag         m_secondTag;
    Tag         m_currentTag;
    size_t      m_addedTagsCount;

    LinkedNotebook      m_firstLinkedNotebook;
    LinkedNotebook      m_secondLinkedNotebook;
    LinkedNotebook      m_currentLinkedNotebook;
    size_t              m_addedLinkedNotebooksCount;

    SavedSearch         m_firstSavedSearch;
    SavedSearch         m_secondSavedSearch;
    SavedSearch         m_currentSavedSearch;
    size_t              m_addedSavedSearchesCount;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_CACHE_ASYNC_TESTER_H
