#ifndef __LIB_QUTE_NOTE__TESTS__NOTE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __LIB_QUTE_NOTE__TESTS__NOTE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <qute_note/local_storage/LocalStorageManager.h>
#include <qute_note/types/Notebook.h>
#include <qute_note/types/Note.h>
#include <QUuid>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class NoteLocalStorageManagerAsyncTester : public QObject
{
    Q_OBJECT
public:
    explicit NoteLocalStorageManagerAsyncTester(QObject * parent = nullptr);
    ~NoteLocalStorageManagerAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals
    void addNotebookRequest(Notebook notebook, QUuid requestId = QUuid());
    void getNoteCountRequest(QUuid requestId = QUuid());
    void addNoteRequest(Note note, Notebook notebook, QUuid requestId = QUuid());
    void updateNoteRequest(Note note, Notebook notebook, QUuid requestId = QUuid());
    void findNoteRequest(Note note, bool withResourceBinaryData, QUuid requestId = QUuid());
    void listAllNotesPerNotebookRequest(Notebook notebook, bool withResourceBinaryData,
                                        LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QUuid requestId = QUuid());
    void deleteNoteRequest(Note note, QUuid requestId = QUuid());
    void expungeNoteRequest(Note note, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();
    void onAddNotebookCompleted(Notebook notebook, QUuid requestId);
    void onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onGetNoteCountCompleted(int count, QUuid requestId);
    void onGetNoteCountFailed(QString errorDescription, QUuid requestId);
    void onAddNoteCompleted(Note note, Notebook notebook, QUuid requestId);
    void onAddNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNoteCompleted(Note note, Notebook notebook, QUuid requestId);
    void onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindNoteCompleted(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId);
    void onListAllNotesPerNotebookCompleted(Notebook notebook, bool withResourceBinaryData,
                                            LocalStorageManager::ListObjectsOptions flag,
                                            size_t limit, size_t offset,
                                            LocalStorageManager::ListNotesOrder::type order,
                                            LocalStorageManager::OrderDirection::type orderDirection,
                                            QList<Note> notes, QUuid requestId);
    void onListAllNotesPerNotebookFailed(Notebook notebook, bool withResourceBinaryData,
                                         LocalStorageManager::ListObjectsOptions flag,
                                         size_t limit, size_t offset,
                                         LocalStorageManager::ListNotesOrder::type order,
                                         LocalStorageManager::OrderDirection::type orderDirection,
                                         QString errorDescription, QUuid requestId);
    void onDeleteNoteCompleted(Note note, QUuid requestId);
    void onDeleteNoteFailed(Note note, QString errorDescription, QUuid requestId);
    void onExpungeNoteCompleted(Note note, QUuid requestId);
    void onExpungeNoteFailed(Note note, QString errorDescription, QUuid requestId);

private:
    void createConnections();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_ADD_NOTEBOOK_REQUEST,
        STATE_SENT_ADD_REQUEST,
        STATE_SENT_FIND_AFTER_ADD_REQUEST,
        STATE_SENT_UPDATE_REQUEST,
        STATE_SENT_FIND_AFTER_UPDATE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST,
        STATE_SENT_DELETE_REQUEST,
        STATE_SENT_EXPUNGE_REQUEST,
        STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_ADD_EXTRA_NOTEBOOK_REQUEST,
        STATE_SENT_ADD_EXTRA_NOTE_ONE_REQUEST,
        STATE_SENT_ADD_EXTRA_NOTE_TWO_REQUEST,
        STATE_SENT_ADD_EXTRA_NOTE_THREE_REQUEST,
        STATE_SENT_LIST_ALL_NOTES_PER_NOTEBOOK_ONE_REQUEST,
        STATE_SENT_LIST_ALL_NOTES_PER_NOTEBOOK_TWO_REQUEST
    };

    State m_state;

    LocalStorageManagerThreadWorker * m_pLocalStorageManagerThreadWorker;
    QThread *                   m_pLocalStorageManagerThread;

    Notebook                    m_notebook;
    Notebook                    m_extraNotebook;
    Note                        m_initialNote;
    Note                        m_foundNote;
    Note                        m_modifiedNote;
    QList<Note>                 m_initialNotes;
    QList<Note>                 m_extraNotes;
};

} // namespace qute_note
} // namespace test

#endif // __LIB_QUTE_NOTE__TESTS__NOTE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
