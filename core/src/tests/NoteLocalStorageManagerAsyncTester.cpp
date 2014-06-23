#include "NoteLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

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
        errorDescription = QObject::tr(qPrintable(errorDescription)); \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_ADD_NOTEBOOK_REQUEST)
    {
        if (m_pNotebook != notebook) {
            errorDescription = "Internal error in NoteLocalStorageManagerAsyncTester: "
                               "notebook in onAddNotebookCompleted slot doesn't match "
                               "the original Notebook";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
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

        m_state = STATE_SENT_ADD_REQUEST;
        emit addNoteRequest(m_pInitialNote, m_pNotebook);
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
    Q_UNUSED(count)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onGetNoteCountFailed(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onAddNoteCompleted(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(note)
    Q_UNUSED(notebook)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onAddNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(note)
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onUpdateNoteCompleted(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(note)
    Q_UNUSED(notebook)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onUpdateNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(note)
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onFindNoteCompleted(QSharedPointer<Note> note, bool withResourceBinaryData)
{
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onFindNoteFailed(QSharedPointer<Note> note, bool withResourceBinaryData, QString errorDescription)
{
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onListAllNotesPerNotebookCompleted(QSharedPointer<Notebook> notebook, bool withResourceBinaryData, QList<Note> notes)
{
    Q_UNUSED(notebook)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(notes)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onListAllNotesPerNotebookFailed(QSharedPointer<Notebook> notebook, bool withResourceBinaryData, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onDeleteNoteCompleted(QSharedPointer<Note> note)
{
    Q_UNUSED(note)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onDeleteNoteFailed(QSharedPointer<Note> note, QString errorDescription)
{
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onExpungeNoteCompleted(QSharedPointer<Note> note)
{
    Q_UNUSED(note)
    // TODO: implement
}

void NoteLocalStorageManagerAsyncTester::onExpungeNoteFailed(QSharedPointer<Note> note, QString errorDescription)
{
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    // TODO: implement
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
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNoteComplete(QSharedPointer<Note>)),
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
