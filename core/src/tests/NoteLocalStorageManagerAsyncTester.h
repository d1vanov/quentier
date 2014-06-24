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
    void addNotebookRequest(QSharedPointer<Notebook> notebook);
    void getNoteCountRequest();
    void addNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void updateNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void findNoteRequest(QSharedPointer<Note> note, bool withResourceBinaryData);
    void listAllNotesPerNotebookRequest(QSharedPointer<Notebook> notebook,
                                        bool withResourceBinaryData);
    void deleteNoteRequest(QSharedPointer<Note> note);
    void expungeNoteRequest(QSharedPointer<Note> note);

private Q_SLOTS:
    void onAddNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onAddNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void onGetNoteCountCompleted(int count);
    void onGetNoteCountFailed(QString errorDescription);
    void onAddNoteCompleted(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void onAddNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription);
    void onUpdateNoteCompleted(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void onUpdateNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription);
    void onFindNoteCompleted(QSharedPointer<Note> note, bool withResourceBinaryData);
    void onFindNoteFailed(QSharedPointer<Note> note, bool withResourceBinaryData, QString errorDescription);
    void onListAllNotesPerNotebookCompleted(QSharedPointer<Notebook> notebook, bool withResourceBinaryData, QList<Note> notes);
    void onListAllNotesPerNotebookFailed(QSharedPointer<Notebook> notebook, bool withResourceBinaryData, QString errorDescription);
    void onDeleteNoteCompleted(QSharedPointer<Note> note);
    void onDeleteNoteFailed(QSharedPointer<Note> note, QString errorDescription);
    void onExpungeNoteCompleted(QSharedPointer<Note> note);
    void onExpungeNoteFailed(QSharedPointer<Note> note, QString errorDescription);

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
    QSharedPointer<Notebook>    m_pNotebook;
    QSharedPointer<Notebook>    m_pExtraNotebook;
    QSharedPointer<Note>        m_pInitialNote;
    QSharedPointer<Note>        m_pFoundNote;
    QSharedPointer<Note>        m_pModifiedNote;
    QList<Note>                 m_initialNotes;
    QList<Note>                 m_extraNotes;
};

} // namespace qute_note
} // namespace test

#endif // __QUTE_NOTE__CORE__TESTS__NOTE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
