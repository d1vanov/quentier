#ifndef __QUTE_NOTE__CORE__TESTS__NOTE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__NOTE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/ResourceWrapper.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThread)

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
    void addNotebookRequest(Notebook notebook);
    void getNoteCountRequest();
    void addNoteRequest(Note note, Notebook notebook);
    void updateNoteRequest(Note note, Notebook notebook);
    void findNoteRequest(Note note, bool withResourceBinaryData);
    void listAllNotesPerNotebookRequest(Notebook notebook,
                                        bool withResourceBinaryData);
    void deleteNoteRequest(Note note);
    void expungeNoteRequest(Note note);

private Q_SLOTS:
    void onAddNotebookCompleted(Notebook notebook);
    void onAddNotebookFailed(Notebook notebook, QString errorDescription);
    void onGetNoteCountCompleted(int count);
    void onGetNoteCountFailed(QString errorDescription);
    void onAddNoteCompleted(Note note, Notebook notebook);
    void onAddNoteFailed(Note note, Notebook notebook, QString errorDescription);
    void onUpdateNoteCompleted(Note note, Notebook notebook);
    void onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription);
    void onFindNoteCompleted(Note note, bool withResourceBinaryData);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription);
    void onListAllNotesPerNotebookCompleted(Notebook notebook, bool withResourceBinaryData, QList<Note> notes);
    void onListAllNotesPerNotebookFailed(Notebook notebook, bool withResourceBinaryData, QString errorDescription);
    void onDeleteNoteCompleted(Note note);
    void onDeleteNoteFailed(Note note, QString errorDescription);
    void onExpungeNoteCompleted(Note note);
    void onExpungeNoteFailed(Note note, QString errorDescription);

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
    LocalStorageManagerThread * m_pLocalStorageManagerThread;
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

#endif // __QUTE_NOTE__CORE__TESTS__NOTE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
