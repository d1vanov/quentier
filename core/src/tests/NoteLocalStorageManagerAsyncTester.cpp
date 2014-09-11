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
    m_notebook(),
    m_extraNotebook(),
    m_initialNote(),
    m_foundNote(),
    m_modifiedNote(),
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

    m_notebook.clear();
    m_notebook.setGuid("00000000-0000-0000-c000-000000000047");
    m_notebook.setUpdateSequenceNumber(1);
    m_notebook.setName("Fake notebook name");
    m_notebook.setCreationTimestamp(1);
    m_notebook.setModificationTimestamp(1);
    m_notebook.setDefaultNotebook(true);
    m_notebook.setLastUsed(false);
    m_notebook.setPublishingUri("Fake publishing uri");
    m_notebook.setPublishingOrder(1);
    m_notebook.setPublishingAscending(true);
    m_notebook.setPublishingPublicDescription("Fake public description");
    m_notebook.setPublished(true);
    m_notebook.setStack("Fake notebook stack");
    m_notebook.setBusinessNotebookDescription("Fake business notebook description");
    m_notebook.setBusinessNotebookPrivilegeLevel(1);
    m_notebook.setBusinessNotebookRecommended(true);

    QString errorDescription;
    if (!m_notebook.checkParameters(errorDescription)) {
        QNWARNING("Found invalid notebook: " << m_notebook << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_NOTEBOOK_REQUEST;
    emit addNotebookRequest(m_notebook);
}

void NoteLocalStorageManagerAsyncTester::onAddNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        QNWARNING(errorDescription << ": " << m_state); \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_ADD_NOTEBOOK_REQUEST)
    {
        if (m_notebook != notebook) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "notebook in onAddNotebookCompleted slot doesn't match "
                               "the original Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_initialNote.clear();
        m_initialNote.setGuid("00000000-0000-0000-c000-000000000048");
        m_initialNote.setUpdateSequenceNumber(1);
        m_initialNote.setTitle("Fake note");
        m_initialNote.setContent("<en-note><h1>Hello, world</h1></en-note>");
        m_initialNote.setCreationTimestamp(1);
        m_initialNote.setModificationTimestamp(1);
        m_initialNote.setNotebookGuid(m_notebook.guid());
        m_initialNote.setActive(true);

        m_state = STATE_SENT_ADD_REQUEST;
        emit addNoteRequest(m_initialNote, m_notebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTEBOOK_REQUEST)
    {
        Note extraNote;
        extraNote.setGuid("00000000-0000-0000-c000-000000000006");
        extraNote.setUpdateSequenceNumber(6);
        extraNote.setActive(true);
        extraNote.setContent("<en-note><h1>Hello, world 3</h1></en-note>");
        extraNote.setCreationTimestamp(3);
        extraNote.setModificationTimestamp(3);
        extraNote.setNotebookGuid(m_extraNotebook.guid());
        extraNote.setTitle("Fake note title three");

        m_state = STATE_SENT_ADD_EXTRA_NOTE_THREE_REQUEST;
        emit addNoteRequest(extraNote, m_extraNotebook);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onAddNotebookFailed(Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << notebook);
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
            emit failure(errorDescription);
            return;
        }

        m_modifiedNote.setLocal(false);
        m_modifiedNote.setActive(false);
        m_modifiedNote.setDeletionTimestamp(3);
        m_state = STATE_SENT_DELETE_REQUEST;
        emit deleteNoteRequest(m_modifiedNote);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = "GetNoteCount returned result different from the expected one (0): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Note extraNote;
        extraNote.setGuid("00000000-0000-0000-c000-000000000001");
        extraNote.setUpdateSequenceNumber(1);
        extraNote.setActive(true);
        extraNote.setContent("<en-note><h1>Hello, world 1</h1></en-note>");
        extraNote.setCreationTimestamp(1);
        extraNote.setModificationTimestamp(1);
        extraNote.setNotebookGuid(m_notebook.guid());
        extraNote.setTitle("Fake note title one");

        ResourceWrapper resource;
        resource.setGuid("00000000-0000-0000-c000-000000000002");
        resource.setUpdateSequenceNumber(2);
        resource.setNoteGuid(extraNote.guid());
        resource.setDataBody(QByteArray("Fake resource data body"));
        resource.setDataSize(resource.dataBody().size());
        resource.setDataHash("Fake hash      1");
        resource.setMime("text/plain");
        resource.setHeight(20);
        resource.setWidth(20);

        extraNote.addResource(resource);

        ResourceWrapper resource2;
        resource2.setGuid("00000000-0000-0000-c000-000000000009");
        resource2.setUpdateSequenceNumber(3);
        resource2.setNoteGuid(extraNote.guid());
        resource2.setDataBody(QByteArray("Fake resource data body"));
        resource2.setDataSize(resource.dataBody().size());
        resource2.setDataHash("Fake hash      9");
        resource2.setMime("text/plain");
        resource2.setHeight(30);
        resource2.setWidth(30);

        extraNote.addResource(resource2);

        qevercloud::NoteAttributes & noteAttributes = extraNote.noteAttributes();
        noteAttributes.altitude = 20.0;
        noteAttributes.latitude = 10.0;
        noteAttributes.longitude = 30.0;
        noteAttributes.author = "NoteLocalStorageManagerAsyncTester";
        noteAttributes.lastEditedBy = "Same as author";
        noteAttributes.placeName = "Testing hall";
        noteAttributes.sourceApplication = "tester";

        m_state = STATE_SENT_ADD_EXTRA_NOTE_ONE_REQUEST;
        emit addNoteRequest(extraNote, m_notebook);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onGetNoteCountFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onAddNoteCompleted(Note note, Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialNote != note) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "note in onAddNoteCompleted slot doesn't match "
                               "the original Note";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_foundNote = Note();
        m_foundNote.setLocalGuid(note.localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        bool withResourceBinaryData = true;
        emit findNoteRequest(m_foundNote, withResourceBinaryData);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTE_ONE_REQUEST)
    {
        m_initialNotes << note;

        Note extraNote;
        extraNote.setGuid("00000000-0000-0000-c000-000000000004");
        extraNote.setUpdateSequenceNumber(4);
        extraNote.setActive(true);
        extraNote.setContent("<en-note><h1>Hello, world 2</h1></en-note>");
        extraNote.setCreationTimestamp(2);
        extraNote.setModificationTimestamp(2);
        extraNote.setNotebookGuid(m_notebook.guid());
        extraNote.setTitle("Fake note title two");

        m_state = STATE_SENT_ADD_EXTRA_NOTE_TWO_REQUEST;
        emit addNoteRequest(extraNote, m_notebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTE_TWO_REQUEST)
    {
        m_initialNotes << note;

        m_extraNotebook.clear();
        m_extraNotebook.setGuid("00000000-0000-0000-c000-000000000005");
        m_extraNotebook.setUpdateSequenceNumber(1);
        m_extraNotebook.setName("Fake notebook name two");
        m_extraNotebook.setCreationTimestamp(1);
        m_extraNotebook.setModificationTimestamp(1);
        m_extraNotebook.setDefaultNotebook(false);
        m_extraNotebook.setLastUsed(true);

        m_state = STATE_SENT_ADD_EXTRA_NOTEBOOK_REQUEST;
        emit addNotebookRequest(m_extraNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTE_THREE_REQUEST)
    {
        m_initialNotes << note;

        m_state = STATE_SENT_LIST_ALL_NOTES_PER_NOTEBOOK_ONE_REQUEST;
        bool withResourceBinaryData = true;
        emit listAllNotesPerNotebookRequest(m_notebook, withResourceBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onAddNoteFailed(Note note, Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << note << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onUpdateNoteCompleted(Note note, Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedNote != note) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "note in onUpdateNoteCompleted slot doesn't match "
                               "the original updated Note";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        bool withResourceBinaryData = true;
        emit findNoteRequest(m_foundNote, withResourceBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << note << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onFindNoteCompleted(Note note, bool withResourceBinaryData)
{
    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_initialNote != note) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "note in onFindNoteCompleted slot doesn't match "
                               "the original Note";
            QNWARNING(errorDescription << "; original note: " << m_initialNote
                      << "\nFound note: " << note);
            emit failure(errorDescription);
            return;
        }

        m_foundNote = note;

        // Ok, found note is good, updating it now
        m_modifiedNote = m_initialNote;
        m_modifiedNote.setUpdateSequenceNumber(m_initialNote.updateSequenceNumber() + 1);
        m_modifiedNote.setTitle(m_initialNote.title() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateNoteRequest(m_modifiedNote, m_notebook);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_modifiedNote != note) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "not in onFindNoteCompleted slot doesn't match "
                               "the original modified Note";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_modifiedNote = note;
        m_foundNote = note;

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getNoteCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription = "Found note which should have been expunged from local storage";
        QNWARNING(errorDescription << ": Note expunged from LocalStorageManager: " << m_modifiedNote
                  << "\nNote found in LocalStorageManager: " << m_foundNote);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getNoteCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", Note: " << note << ", withResourceBinaryData = "
              << (withResourceBinaryData ? "true" : "false"));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onListAllNotesPerNotebookCompleted(Notebook notebook, bool withResourceBinaryData, QList<Note> notes)
{
    QString errorDescription;

    if (m_state == STATE_SENT_LIST_ALL_NOTES_PER_NOTEBOOK_ONE_REQUEST)
    {
        foreach(const Note & note, notes)
        {
            if (!m_initialNotes.contains(note)) {
                errorDescription = "One of found notes was not found within initial notes";
                QNWARNING(errorDescription << ", unfound note: " << note);
                emit failure(errorDescription);
                return;
            }

            if (note.notebookGuid() != m_notebook.guid()) {
                errorDescription = "One of found notes has invalid notebook guid: expected ";
                errorDescription += m_notebook.guid();
                errorDescription += ", found: ";
                errorDescription += note.notebookGuid();
                QNWARNING(errorDescription);
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
                emit failure(errorDescription);
                return;
            }

            if (note.notebookGuid() != m_extraNotebook.guid()) {
                errorDescription = "One of found notes has invalid notebook guid: expected ";
                errorDescription += m_extraNotebook.guid();
                errorDescription += ", found: ";
                errorDescription += note.notebookGuid();
                QNWARNING(errorDescription);
                emit failure(errorDescription);
                return;
            }
        }
    }
    HANDLE_WRONG_STATE();

    emit success();
}

void NoteLocalStorageManagerAsyncTester::onListAllNotesPerNotebookFailed(Notebook notebook, bool withResourceBinaryData, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << notebook
              << ", withResourceBinaryData = " << (withResourceBinaryData ? "true" : "false"));
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onDeleteNoteCompleted(Note note)
{
    QString errorDescription;

    if (m_modifiedNote != note) {
        errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                           "note in onDeleteNoteCompleted slot doesn't match "
                           "the original deleted Note";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_modifiedNote.setLocal(true);
    m_state = STATE_SENT_EXPUNGE_REQUEST;
    emit expungeNoteRequest(m_modifiedNote);
}

void NoteLocalStorageManagerAsyncTester::onDeleteNoteFailed(Note note, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << note);
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::onExpungeNoteCompleted(Note note)
{
    QString errorDescription;

    if (m_modifiedNote != note) {
        errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                           "note in onExpungeNoteCompleted slot doesn't match "
                           "the original expunged Note";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    bool withResourceBinaryData = true;
    emit findNoteRequest(m_foundNote, withResourceBinaryData);
}

void NoteLocalStorageManagerAsyncTester::onExpungeNoteFailed(Note note, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << note);
    emit failure(errorDescription);
}

void NoteLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(addNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(getNoteCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetNoteCountRequest()));
    QObject::connect(this, SIGNAL(addNoteRequest(Note,Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(updateNoteRequest(Note,Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(findNoteRequest(Note,bool)),
                     m_pLocalStorageManagerThread, SLOT(onFindNoteRequest(Note,bool)));
    QObject::connect(this, SIGNAL(listAllNotesPerNotebookRequest(Notebook,bool)),
                     m_pLocalStorageManagerThread, SLOT(onListAllNotesPerNotebookRequest(Notebook,bool)));
    QObject::connect(this, SIGNAL(deleteNoteRequest(Note)), m_pLocalStorageManagerThread,
                     SLOT(onDeleteNoteRequest(Note)));
    QObject::connect(this, SIGNAL(expungeNoteRequest(Note)), m_pLocalStorageManagerThread,
                     SLOT(onExpungeNoteRequest(Note)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookComplete(Notebook)),
                     this, SLOT(onAddNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookFailed(Notebook,QString)),
                     this, SLOT(onAddNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getNoteCountComplete(int)),
                     this, SLOT(onGetNoteCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getNoteCountFailed(QString)),
                     this, SLOT(onGetNoteCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteComplete(Note,Notebook)),
                     this, SLOT(onAddNoteCompleted(Note,Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteFailed(Note,Notebook,QString)),
                     this, SLOT(onAddNoteFailed(Note,Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNoteComplete(Note,Notebook)),
                     this, SLOT(onUpdateNoteCompleted(Note,Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNoteFailed(Note,Notebook,QString)),
                     this, SLOT(onUpdateNoteFailed(Note,Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNoteComplete(Note,bool)),
                     this, SLOT(onFindNoteCompleted(Note,bool)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNoteFailed(Note,bool,QString)),
                     this, SLOT(onFindNoteFailed(Note,bool,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllNotesPerNotebookComplete(Notebook,bool,QList<Note>)),
                     this, SLOT(onListAllNotesPerNotebookCompleted(Notebook,bool,QList<Note>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllNotesPerNotebookFailed(Notebook,bool,QString)),
                     this, SLOT(onListAllNotesPerNotebookFailed(Notebook,bool,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(deleteNoteComplete(Note)),
                     this, SLOT(onDeleteNoteCompleted(Note)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(deleteNoteFailed(Note,QString)),
                     this, SLOT(onDeleteNoteFailed(Note,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeNoteComplete(Note)),
                     this, SLOT(onExpungeNoteCompleted(Note)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeNoteFailed(Note,QString)),
                     this, SLOT(onExpungeNoteFailed(Note,QString)));
}

} // namespace qute_note
} // namespace test
