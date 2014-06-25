#include "NoteLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>
#include <QPainter>

namespace qute_note {
namespace test {

NoteLocalStorageManagerAsyncTester::NoteLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_pNotebook(),
    m_pExtraNotebook(),
    m_pInitialNote(),
    m_pFoundNote(),
    m_pModifiedNote(),
    m_initialNotes(),
    m_extraNotes()
{}

NoteLocalStorageManagerAsyncTester::~NoteLocalStorageManagerAsyncTester()
{}

void NoteLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "NoteLocalStorageManagerAsyncTester";
    qint32 userId = 5;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread != nullptr) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch, this);
    createConnections();

    m_pNotebook = QSharedPointer<Notebook>(new Notebook);
    m_pNotebook->setGuid("00000000-0000-0000-c000-000000000047");
    m_pNotebook->setUpdateSequenceNumber(1);
    m_pNotebook->setName("Fake notebook name");
    m_pNotebook->setCreationTimestamp(1);
    m_pNotebook->setModificationTimestamp(1);
    m_pNotebook->setDefaultNotebook(true);
    m_pNotebook->setLastUsed(false);
    m_pNotebook->setPublishingUri("Fake publishing uri");
    m_pNotebook->setPublishingOrder(1);
    m_pNotebook->setPublishingAscending(true);
    m_pNotebook->setPublishingPublicDescription("Fake public description");
    m_pNotebook->setPublished(true);
    m_pNotebook->setStack("Fake notebook stack");
    m_pNotebook->setBusinessNotebookDescription("Fake business notebook description");
    m_pNotebook->setBusinessNotebookPrivilegeLevel(1);
    m_pNotebook->setBusinessNotebookRecommended(true);

    QString errorDescription;
    if (!m_pNotebook->checkParameters(errorDescription)) {
        QNWARNING("Found invalid notebook: " << *m_pNotebook << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_NOTEBOOK_REQUEST;
    emit addNotebookRequest(m_pNotebook);
}

void NoteLocalStorageManagerAsyncTester::onAddNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "NoteLocalStorageManagerAsyncTester::onAddNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        QNWARNING(errorDescription << ": " << m_state); \
        QNTRANSLATE(errorDescription); \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_ADD_NOTEBOOK_REQUEST)
    {
        if (m_pNotebook != notebook) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "notebook in onAddNotebookCompleted slot doesn't match "
                               "the original Notebook";
            QNWARNING(errorDescription);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_pInitialNote = QSharedPointer<Note>(new Note);
        m_pInitialNote->setGuid("00000000-0000-0000-c000-000000000048");
        m_pInitialNote->setUpdateSequenceNumber(1);
        m_pInitialNote->setTitle("Fake note");
        m_pInitialNote->setContent("Fake note content");
        m_pInitialNote->setCreationTimestamp(1);
        m_pInitialNote->setModificationTimestamp(1);
        m_pInitialNote->setNotebookGuid(m_pNotebook->guid());
        m_pInitialNote->setActive(true);

        m_state = STATE_SENT_ADD_REQUEST;
        emit addNoteRequest(m_pInitialNote, m_pNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTEBOOK_REQUEST)
    {
        QSharedPointer<Note> extraNote = QSharedPointer<Note>(new Note);
        extraNote->setGuid("00000000-0000-0000-c000-000000000006");
        extraNote->setUpdateSequenceNumber(6);
        extraNote->setActive(true);
        extraNote->setContent("Fake note content three");
        extraNote->setCreationTimestamp(3);
        extraNote->setModificationTimestamp(3);
        extraNote->setNotebookGuid(m_pExtraNotebook->guid());
        extraNote->setTitle("Fake note title three");

        m_state = STATE_SENT_ADD_EXTRA_NOTE_THREE_REQUEST;
        emit addNoteRequest(extraNote, m_pExtraNotebook);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onAddNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onGetNoteCountCompleted(int count)
{
    QString errorDescription;

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = "GetNoteCount returned result different from the expected one (1): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_pModifiedNote->setLocal(false);
        m_pModifiedNote->setDeleted(true);
        m_pModifiedNote->setDeletionTimestamp(3);
        m_state = STATE_SENT_DELETE_REQUEST;
        emit deleteNoteRequest(m_pModifiedNote);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = "GetNoteCount returned result different from the expected one (0): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        QSharedPointer<Note> extraNote = QSharedPointer<Note>(new Note);
        extraNote->setGuid("00000000-0000-0000-c000-000000000001");
        extraNote->setUpdateSequenceNumber(1);
        extraNote->setActive(true);
        extraNote->setContent("Fake note content one");
        extraNote->setCreationTimestamp(1);
        extraNote->setModificationTimestamp(1);
        extraNote->setNotebookGuid(m_pNotebook->guid());
        extraNote->setTitle("Fake note title one");

        ResourceWrapper resource;
        resource.setGuid("00000000-0000-0000-c000-000000000002");
        resource.setUpdateSequenceNumber(2);
        resource.setNoteGuid(extraNote->guid());
        resource.setDataBody(QByteArray("Fake resource data body"));
        resource.setDataSize(resource.dataBody().size());
        resource.setDataHash("Fake hash      1");
        resource.setMime("text/plain");
        resource.setHeight(20);
        resource.setWidth(20);

        extraNote->addResource(resource);

        m_state = STATE_SENT_ADD_EXTRA_NOTE_ONE_REQUEST;
        emit addNoteRequest(extraNote, m_pNotebook);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onGetNoteCountFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onAddNoteCompleted(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!note.isNull(), "NoteLocalStorageManagerAsyncTester::onAddNoteCompleted slot",
               "Found NULL shared pointer to Note");

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_pInitialNote != note) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "note pointer in onAddNoteCompleted slot doesn't match "
                               "the pointer to the original Note";
            QNWARNING(errorDescription);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_pFoundNote = QSharedPointer<Note>(new Note);
        m_pFoundNote->setLocalGuid(note->localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        bool withResourceBinaryData = true;
        emit findNoteRequest(m_pFoundNote, withResourceBinaryData);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTE_ONE_REQUEST)
    {
        m_initialNotes << *note;

        QSharedPointer<Note> extraNote = QSharedPointer<Note>(new Note);
        extraNote->setGuid("00000000-0000-0000-c000-000000000004");
        extraNote->setUpdateSequenceNumber(4);
        extraNote->setActive(true);
        extraNote->setContent("Fake note content two");
        extraNote->setCreationTimestamp(2);
        extraNote->setModificationTimestamp(2);
        extraNote->setNotebookGuid(m_pNotebook->guid());
        extraNote->setTitle("Fake note title two");

        m_state = STATE_SENT_ADD_EXTRA_NOTE_TWO_REQUEST;
        emit addNoteRequest(extraNote, m_pNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTE_TWO_REQUEST)
    {
        m_initialNotes << *note;

        m_pExtraNotebook = QSharedPointer<Notebook>(new Notebook);
        m_pExtraNotebook->setGuid("00000000-0000-0000-c000-000000000005");
        m_pExtraNotebook->setUpdateSequenceNumber(1);
        m_pExtraNotebook->setName("Fake notebook name two");
        m_pExtraNotebook->setCreationTimestamp(1);
        m_pExtraNotebook->setModificationTimestamp(1);
        m_pExtraNotebook->setDefaultNotebook(false);
        m_pExtraNotebook->setLastUsed(true);

        m_state = STATE_SENT_ADD_EXTRA_NOTEBOOK_REQUEST;
        emit addNotebookRequest(m_pExtraNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTE_THREE_REQUEST)
    {
        m_initialNotes << *note;

        m_state = STATE_SENT_LIST_ALL_NOTES_PER_NOTEBOOK_ONE_REQUEST;
        bool withResourceBinaryData = true;
        emit listAllNotesPerNotebookRequest(m_pNotebook, withResourceBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onAddNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << (note.isNull() ? QString("NULL") : note->ToQString())
              << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onUpdateNoteCompleted(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!note.isNull(), "NoteLocalStorageManagerAsyncTester::onUpdateNoteCompleted slot",
               "Found NULL shared pointer to Note");

    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_pModifiedNote != note) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "note pointer in onUpdateNoteCompleted slot doesn't match "
                               "the pointer to the original updated Note";
            QNWARNING(errorDescription);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        bool withResourceBinaryData = true;
        emit findNoteRequest(m_pFoundNote, withResourceBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onUpdateNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << (note.isNull() ? QString("NULL") : note->ToQString())
              << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onFindNoteCompleted(QSharedPointer<Note> note, bool withResourceBinaryData)
{
    Q_ASSERT_X(!note.isNull(), "NoteLocalStorageManagerAsyncTester::onFindNoteCompleted slot",
               "Found NULL shared pointer to Note");

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_pFoundNote != note) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "note pointer in onFindNoteCompleted slot doesn't match "
                               "the pointer to the original Note";
            QNWARNING(errorDescription);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pInitialNote.isNull());
        if (*m_pFoundNote != *m_pInitialNote) {
            errorDescription = "Added and found notes in local storage don't match";
            QNWARNING(errorDescription << ": Note added to LocalStorageManager: " << *m_pInitialNote
                      << "\nNote found in LocalStorageManager: " << *m_pFoundNote);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        // Ok, found note is good, updating it now
        m_pModifiedNote = QSharedPointer<Note>(new Note(*m_pInitialNote));
        m_pModifiedNote->setUpdateSequenceNumber(m_pInitialNote->updateSequenceNumber() + 1);
        m_pModifiedNote->setContent(m_pInitialNote->content() + "_modified");
        m_pModifiedNote->setTitle(m_pInitialNote->title() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateNoteRequest(m_pModifiedNote, m_pNotebook);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_pFoundNote != note) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "note pointer in onFindNoteCompleted slot doesn't match "
                               "the pointer to the original modified Note";
            QNWARNING(errorDescription);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pModifiedNote.isNull());
        if (*m_pFoundNote != *m_pModifiedNote) {
            errorDescription = "Updated and found notes in local storage don't match";
            QNWARNING(errorDescription << ": Note updated in LocalStorageManager: " << *m_pModifiedNote
                      << "\nNote found in LocalStorageManager: " << *m_pFoundNote);
            QNTRANSLATE(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getNoteCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        Q_ASSERT(!m_pModifiedNote.isNull());
        errorDescription = "Found note which should have been expunged from local storage";
        QNWARNING(errorDescription << ": Note expunged from LocalStorageManager: " << *m_pModifiedNote
                  << "\nNote found in LocalStorageManager: " << *m_pFoundNote);
        QNTRANSLATE(errorDescription);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onFindNoteFailed(QSharedPointer<Note> note, bool withResourceBinaryData, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getNoteCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", Note: " << (note.isNull() ? QString("NULL") : note->ToQString())
              << ", withResourceBinaryData = " << (withResourceBinaryData ? "true" : "false"));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onListAllNotesPerNotebookCompleted(QSharedPointer<Notebook> notebook, bool withResourceBinaryData, QList<Note> notes)
{
    QString errorDescription;

    if (m_state == STATE_SENT_LIST_ALL_NOTES_PER_NOTEBOOK_ONE_REQUEST)
    {
        foreach(const Note & note, notes)
        {
            if (!m_initialNotes.contains(note)) {
                errorDescription = "One of found notes was not found within initial notes";
                QNWARNING(errorDescription << ", unfound note: " << note);
                QNTRANSLATE(errorDescription);
                emit failure(errorDescription);
                return;
            }

            if (note.notebookGuid() != m_pNotebook->guid()) {
                errorDescription = "One of found notes has invalid notebook guid: expected ";
                errorDescription += m_pNotebook->guid();
                errorDescription += ", found: ";
                errorDescription += note.notebookGuid();
                QNWARNING(errorDescription);
                QNTRANSLATE(errorDescription);
                emit failure(errorDescription);
                return;
            }
        }
    }
    else if (m_state == STATE_SENT_LIST_ALL_NOTES_PER_NOTEBOOK_TWO_REQUEST)
    {
        foreach(const Note & note, notes)
        {
            if (!m_initialNotes.contains(note)) {
                errorDescription = "One of found notes was not found within initial notes";
                QNWARNING(errorDescription);
                QNTRANSLATE(errorDescription);
                emit failure(errorDescription);
                return;
            }

            if (note.notebookGuid() != m_pExtraNotebook->guid()) {
                errorDescription = "One of found notes has invalid notebook guid: expected ";
                errorDescription += m_pExtraNotebook->guid();
                errorDescription += ", found: ";
                errorDescription += note.notebookGuid();
                QNWARNING(errorDescription);
                QNTRANSLATE(errorDescription);
                emit failure(errorDescription);
                return;
            }
        }
    }
    HANDLE_WRONG_STATE();

    emit success();
}

void NoteLocalStorageManagerAsyncTester::onListAllNotesPerNotebookFailed(QSharedPointer<Notebook> notebook, bool withResourceBinaryData, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString())
              << ", withResourceBinaryData = " << (withResourceBinaryData ? "true" : "false"));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onDeleteNoteCompleted(QSharedPointer<Note> note)
{
    Q_ASSERT_X(!note.isNull(), "NoteLocalStorageManagerAsyncTester::onDeleteNoteCompleted",
               "Found NULL pointer to Note");

    QString errorDescription;

    if (m_pModifiedNote != note) {
        errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                           "note pointer in onDeleteNoteCompleted slot doesn't match "
                           "the pointer to the original deleted Note";
        QNWARNING(errorDescription);
        QNTRANSLATE(errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_pModifiedNote->setLocal(true);
    m_state = STATE_SENT_EXPUNGE_REQUEST;
    emit expungeNoteRequest(m_pModifiedNote);
}

void NoteLocalStorageManagerAsyncTester::onDeleteNoteFailed(QSharedPointer<Note> note, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << (note.isNull() ? QString("NULL") : note->ToQString()));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onExpungeNoteCompleted(QSharedPointer<Note> note)
{
    Q_ASSERT_X(!note.isNull(), "NoteLocalStorageManagerAsyncTester::onExpungeNoteCompleted slot",
               "Found NULL pointer to Note");

    QString errorDescription;

    if (m_pModifiedNote != note) {
        errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                           "note pointer in onExpungeNoteCompleted slot doesn't match "
                           "the pointer to the original expunged Note";
        QNWARNING(errorDescription);
        QNTRANSLATE(errorDescription);
        emit failure(errorDescription);
        return;
    }

    Q_ASSERT(!m_pFoundNote.isNull());
    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    bool withResourceBinaryData = true;
    emit findNoteRequest(m_pFoundNote, withResourceBinaryData);
}

void NoteLocalStorageManagerAsyncTester::onExpungeNoteFailed(QSharedPointer<Note> note, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << (note.isNull() ? QString("NULL") : note->ToQString()));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(addNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onAddNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(getNoteCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetNoteCountRequest()));
    QObject::connect(this, SIGNAL(addNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onAddNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(updateNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(findNoteRequest(QSharedPointer<Note>,bool)),
                     m_pLocalStorageManagerThread, SLOT(onFindNoteRequest(QSharedPointer<Note>,bool)));
    QObject::connect(this, SIGNAL(listAllNotesPerNotebookRequest(QSharedPointer<Notebook>,bool)),
                     m_pLocalStorageManagerThread, SLOT(onListAllNotesPerNotebookRequest(QSharedPointer<Notebook>,bool)));
    QObject::connect(this, SIGNAL(deleteNoteRequest(QSharedPointer<Note>)), m_pLocalStorageManagerThread,
                     SLOT(onDeleteNoteRequest(QSharedPointer<Note>)));
    QObject::connect(this, SIGNAL(expungeNoteRequest(QSharedPointer<Note>)), m_pLocalStorageManagerThread,
                     SLOT(onExpungeNoteRequest(QSharedPointer<Note>)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onAddNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onAddNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getNoteCountComplete(int)),
                     this, SLOT(onGetNoteCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getNoteCountFailed(QString)),
                     this, SLOT(onGetNoteCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteComplete(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     this, SLOT(onAddNoteCompleted(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)),
                     this, SLOT(onAddNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNoteComplete(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     this, SLOT(onUpdateNoteCompleted(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)),
                     this, SLOT(onUpdateNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNoteComplete(QSharedPointer<Note>,bool)),
                     this, SLOT(onFindNoteCompleted(QSharedPointer<Note>,bool)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNoteFailed(QSharedPointer<Note>,bool,QString)),
                     this, SLOT(onFindNoteFailed(QSharedPointer<Note>,bool,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllNotesPerNotebookComplete(QSharedPointer<Notebook>,bool,QList<Note>)),
                     this, SLOT(onListAllNotesPerNotebookCompleted(QSharedPointer<Notebook>,bool,QList<Note>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllNotesPerNotebookFailed(QSharedPointer<Notebook>,bool,QString)),
                     this, SLOT(onListAllNotesPerNotebookFailed(QSharedPointer<Notebook>,bool,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(deleteNoteComplete(QSharedPointer<Note>)),
                     this, SLOT(onDeleteNoteCompleted(QSharedPointer<Note>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(deleteNoteFailed(QSharedPointer<Note>,QString)),
                     this, SLOT(onDeleteNoteFailed(QSharedPointer<Note>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeNoteComplete(QSharedPointer<Note>)),
                     this, SLOT(onExpungeNoteCompleted(QSharedPointer<Note>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeNoteFailed(QSharedPointer<Note>,QString)),
                     this, SLOT(onExpungeNoteFailed(QSharedPointer<Note>,QString)));
}

} // namespace qute_note
} // namespace test
