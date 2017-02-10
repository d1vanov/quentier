/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteLocalStorageManagerAsyncTester.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QThread>

namespace quentier {
namespace test {

NoteLocalStorageManagerAsyncTester::NoteLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThreadWorker(Q_NULLPTR),
    m_pLocalStorageManagerThread(Q_NULLPTR),
    m_notebook(),
    m_extraNotebook(),
    m_initialNote(),
    m_foundNote(),
    m_modifiedNote(),
    m_initialNotes(),
    m_extraNotes()
{}

NoteLocalStorageManagerAsyncTester::~NoteLocalStorageManagerAsyncTester()
{
    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
    }
}

void NoteLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = QStringLiteral("NoteLocalStorageManagerAsyncTester");
    qint32 userId = 5;
    bool startFromScratch = true;
    bool overrideLock = false;

    if (m_pLocalStorageManagerThread) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = Q_NULLPTR;
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
        m_pLocalStorageManagerThreadWorker = Q_NULLPTR;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new QThread(this);
    Account account(username, Account::Type::Evernote, userId);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(account, startFromScratch, overrideLock);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();

}

void NoteLocalStorageManagerAsyncTester::onWorkerInitialized()
{
    m_notebook.clear();
    m_notebook.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000047"));
    m_notebook.setUpdateSequenceNumber(1);
    m_notebook.setName(QStringLiteral("Fake notebook name"));
    m_notebook.setCreationTimestamp(1);
    m_notebook.setModificationTimestamp(1);
    m_notebook.setDefaultNotebook(true);
    m_notebook.setLastUsed(false);
    m_notebook.setPublishingUri(QStringLiteral("Fake publishing uri"));
    m_notebook.setPublishingOrder(1);
    m_notebook.setPublishingAscending(true);
    m_notebook.setPublishingPublicDescription(QStringLiteral("Fake public description"));
    m_notebook.setPublished(true);
    m_notebook.setStack(QStringLiteral("Fake notebook stack"));
    m_notebook.setBusinessNotebookDescription(QStringLiteral("Fake business notebook description"));
    m_notebook.setBusinessNotebookPrivilegeLevel(1);
    m_notebook.setBusinessNotebookRecommended(true);

    ErrorString errorDescription;
    if (!m_notebook.checkParameters(errorDescription)) {
        QNWARNING(QStringLiteral("Found invalid notebook: ") << m_notebook << QStringLiteral(", error: ") << errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    m_state = STATE_SENT_ADD_NOTEBOOK_REQUEST;
    emit addNotebookRequest(m_notebook);
}

void NoteLocalStorageManagerAsyncTester::onAddNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription.base() = QStringLiteral("Internal error in NoteLocalStorageManagerAsyncTester: found wrong state"); \
        QNWARNING(errorDescription << ": " << m_state); \
        emit failure(errorDescription.nonLocalizedString()); \
    }

    if (m_state == STATE_SENT_ADD_NOTEBOOK_REQUEST)
    {
        if (m_notebook != notebook) {
            errorDescription.base() = QStringLiteral("Internal error in NoteLocalStorageManagerAsyncTester: "
                                                     "notebook in onAddNotebookCompleted slot doesn't match "
                                                     "the original Notebook");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_initialNote.clear();
        m_initialNote.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000048"));
        m_initialNote.setUpdateSequenceNumber(1);
        m_initialNote.setTitle(QStringLiteral("Fake note"));
        m_initialNote.setContent(QStringLiteral("<en-note><h1>Hello, world</h1></en-note>"));
        m_initialNote.setCreationTimestamp(1);
        m_initialNote.setModificationTimestamp(1);
        m_initialNote.setNotebookGuid(m_notebook.guid());
        m_initialNote.setNotebookLocalUid(m_notebook.localUid());
        m_initialNote.setActive(true);

        m_state = STATE_SENT_ADD_REQUEST;
        emit addNoteRequest(m_initialNote);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTEBOOK_REQUEST)
    {
        Note extraNote;
        extraNote.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000006"));
        extraNote.setUpdateSequenceNumber(6);
        extraNote.setActive(true);
        extraNote.setContent(QStringLiteral("<en-note><h1>Hello, world 3</h1></en-note>"));
        extraNote.setCreationTimestamp(3);
        extraNote.setModificationTimestamp(3);
        extraNote.setNotebookGuid(m_extraNotebook.guid());
        extraNote.setNotebookLocalUid(m_extraNotebook.localUid());
        extraNote.setTitle(QStringLiteral("Fake note title three"));

        m_state = STATE_SENT_ADD_EXTRA_NOTE_THREE_REQUEST;
        emit addNoteRequest(extraNote);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onAddNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", Notebook: ") << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void NoteLocalStorageManagerAsyncTester::onNoteCountCompleted(int count, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription.base() = QStringLiteral("GetNoteCount returned result different from the expected one (1): ");
            errorDescription.details() = QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_modifiedNote.setLocal(false);
        m_modifiedNote.setActive(false);
        m_modifiedNote.setDeletionTimestamp(3);
        m_state = STATE_SENT_DELETE_REQUEST;
        emit updateNoteRequest(m_modifiedNote, /* update resources = */ false, /* update tags = */ false);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription.base() = QStringLiteral("GetNoteCount returned result different from the expected one (0): ");
            errorDescription.details() = QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        Note extraNote;
        extraNote.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000001"));
        extraNote.setUpdateSequenceNumber(1);
        extraNote.setActive(true);
        extraNote.setContent(QStringLiteral("<en-note><h1>Hello, world 1</h1></en-note>"));
        extraNote.setCreationTimestamp(1);
        extraNote.setModificationTimestamp(1);
        extraNote.setNotebookGuid(m_notebook.guid());
        extraNote.setNotebookLocalUid(m_notebook.localUid());
        extraNote.setTitle(QStringLiteral("Fake note title one"));

        Resource resource;
        resource.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000002"));
        resource.setUpdateSequenceNumber(2);
        resource.setNoteGuid(extraNote.guid());
        resource.setDataBody(QByteArray("Fake resource data body"));
        resource.setDataSize(resource.dataBody().size());
        resource.setDataHash(QByteArray("Fake hash      1"));
        resource.setMime(QStringLiteral("text/plain"));
        resource.setHeight(20);
        resource.setWidth(20);

        extraNote.addResource(resource);

        Resource resource2;
        resource2.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000009"));
        resource2.setUpdateSequenceNumber(3);
        resource2.setNoteGuid(extraNote.guid());
        resource2.setDataBody(QByteArray("Fake resource data body"));
        resource2.setDataSize(resource.dataBody().size());
        resource2.setDataHash(QByteArray("Fake hash      9"));
        resource2.setMime(QStringLiteral("text/plain"));
        resource2.setHeight(30);
        resource2.setWidth(30);

        extraNote.addResource(resource2);

        qevercloud::NoteAttributes & noteAttributes = extraNote.noteAttributes();
        noteAttributes.altitude = 20.0;
        noteAttributes.latitude = 10.0;
        noteAttributes.longitude = 30.0;
        noteAttributes.author = QStringLiteral("NoteLocalStorageManagerAsyncTester");
        noteAttributes.lastEditedBy = QStringLiteral("Same as author");
        noteAttributes.placeName = QStringLiteral("Testing hall");
        noteAttributes.sourceApplication = QStringLiteral("tester");

        m_state = STATE_SENT_ADD_EXTRA_NOTE_ONE_REQUEST;
        emit addNoteRequest(extraNote);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onNoteCountFailed(ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId);
    emit failure(errorDescription.nonLocalizedString());
}

void NoteLocalStorageManagerAsyncTester::onAddNoteCompleted(Note note, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialNote != note) {
            errorDescription.base() = QStringLiteral("Internal error in NoteLocalStorageManagerAsyncTester: "
                                                     "note in onAddNoteCompleted slot doesn't match the original Note");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_foundNote = Note();
        m_foundNote.setLocalUid(note.localUid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        bool withResourceBinaryData = true;
        emit findNoteRequest(m_foundNote, withResourceBinaryData);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTE_ONE_REQUEST)
    {
        m_initialNotes << note;

        Note extraNote;
        extraNote.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000004"));
        extraNote.setUpdateSequenceNumber(4);
        extraNote.setActive(true);
        extraNote.setContent(QStringLiteral("<en-note><h1>Hello, world 2</h1></en-note>"));
        extraNote.setCreationTimestamp(2);
        extraNote.setModificationTimestamp(2);
        extraNote.setNotebookGuid(m_notebook.guid());
        extraNote.setNotebookLocalUid(m_notebook.localUid());
        extraNote.setTitle(QStringLiteral("Fake note title two"));

        m_state = STATE_SENT_ADD_EXTRA_NOTE_TWO_REQUEST;
        emit addNoteRequest(extraNote);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTE_TWO_REQUEST)
    {
        m_initialNotes << note;

        m_extraNotebook.clear();
        m_extraNotebook.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000005"));
        m_extraNotebook.setUpdateSequenceNumber(1);
        m_extraNotebook.setName(QStringLiteral("Fake notebook name two"));
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

        m_state = STATE_SENT_LIST_NOTES_PER_NOTEBOOK_ONE_REQUEST;
        bool withResourceBinaryData = true;
        LocalStorageManager::ListObjectsOptions flag = LocalStorageManager::ListAll;
        size_t limit = 0, offset = 0;
        LocalStorageManager::ListNotesOrder::type order = LocalStorageManager::ListNotesOrder::NoOrder;
        LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;
        emit listNotesPerNotebookRequest(m_notebook, withResourceBinaryData, flag,
                                         limit, offset, order, orderDirection);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", note: ") << note);
    emit failure(errorDescription.nonLocalizedString());
}

void NoteLocalStorageManagerAsyncTester::onUpdateNoteCompleted(Note note, bool updateResources,
                                                               bool updateTags, QUuid requestId)
{
    Q_UNUSED(requestId)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedNote != note) {
            errorDescription.base() = QStringLiteral("Internal error in NoteLocalStorageManagerAsyncTester: "
                                                     "note in onUpdateNoteCompleted slot doesn't match "
                                                     "the original updated Note");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        bool withResourceBinaryData = true;
        emit findNoteRequest(m_foundNote, withResourceBinaryData);
    }
    else if (m_state == STATE_SENT_DELETE_REQUEST)
    {
        ErrorString errorDescription;

        if (m_modifiedNote != note) {
            errorDescription.base() = QStringLiteral("Internal error in NoteLocalStorageManagerAsyncTester: "
                                                     "note in onUpdateNoteCompleted slot after the deletion update doesn't match "
                                                     "the original deleted Note");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_modifiedNote.setLocal(true);
        m_state = STATE_SENT_EXPUNGE_REQUEST;
        emit expungeNoteRequest(m_modifiedNote);
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onUpdateNoteFailed(Note note, bool updateResources,
                                                            bool updateTags, ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", note: ") << note);
    emit failure(errorDescription.nonLocalizedString());
}

void NoteLocalStorageManagerAsyncTester::onFindNoteCompleted(Note note, bool withResourceBinaryData, QUuid requestId)
{
    Q_UNUSED(requestId)
    Q_UNUSED(withResourceBinaryData)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_initialNote != note) {
            errorDescription.base() = QStringLiteral("Internal error in NoteLocalStorageManagerAsyncTester: "
                                                     "note in onFindNoteCompleted slot doesn't match the original Note");
            QNWARNING(errorDescription << QStringLiteral("; original note: ") << m_initialNote
                      << QStringLiteral("\nFound note: ") << note);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Ok, found note is good, updating it now
        m_modifiedNote = m_initialNote;
        m_modifiedNote.setUpdateSequenceNumber(m_initialNote.updateSequenceNumber() + 1);
        m_modifiedNote.setTitle(m_initialNote.title() + QStringLiteral("_modified"));

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateNoteRequest(m_modifiedNote, /* update resources = */ true, /* update tags = */ true);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_modifiedNote != note) {
            errorDescription.base() = QStringLiteral("Internal error in NoteLocalStorageManagerAsyncTester: "
                                                     "not in onFindNoteCompleted slot doesn't match "
                                                     "the original modified Note");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_modifiedNote = note;

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getNoteCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription.base() = QStringLiteral("Found note which should have been expunged from local storage");
        QNWARNING(errorDescription << QStringLiteral(": Note expunged from LocalStorageManager: ") << m_modifiedNote
                  << QStringLiteral("\nNote found in LocalStorageManager: ") << m_foundNote);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }
    HANDLE_WRONG_STATE();
}

void NoteLocalStorageManagerAsyncTester::onFindNoteFailed(Note note, bool withResourceBinaryData, ErrorString errorDescription, QUuid requestId)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getNoteCountRequest();
        return;
    }

    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", note: ") << note
              << QStringLiteral(", withResourceBinaryData = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")));
    emit failure(errorDescription.nonLocalizedString());
}

void NoteLocalStorageManagerAsyncTester::onListNotesPerNotebookCompleted(Notebook notebook, bool withResourceBinaryData,
                                                                         LocalStorageManager::ListObjectsOptions flag,
                                                                         size_t limit, size_t offset,
                                                                         LocalStorageManager::ListNotesOrder::type order,
                                                                         LocalStorageManager::OrderDirection::type orderDirection,
                                                                         QList<Note> notes, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_LIST_NOTES_PER_NOTEBOOK_ONE_REQUEST)
    {
        foreach(const Note & note, notes)
        {
            if (!m_initialNotes.contains(note)) {
                errorDescription.base() = QStringLiteral("One of found notes was not found within initial notes");
                QNWARNING(errorDescription << QStringLiteral(", unfound note: ") << note);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }

            if (note.notebookGuid() != m_notebook.guid()) {
                errorDescription.base() = QStringLiteral("One of found notes has invalid notebook guid");
                errorDescription.details() = QStringLiteral("expected ");
                errorDescription.details() += m_notebook.guid();
                errorDescription.details() += QStringLiteral(", found: ");
                errorDescription.details() += note.notebookGuid();
                QNWARNING(errorDescription);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }
        }
    }
    else if (m_state == STATE_SENT_LIST_NOTES_PER_NOTEBOOK_TWO_REQUEST)
    {
        foreach(const Note & note, notes)
        {
            if (!m_initialNotes.contains(note)) {
                errorDescription.base() = QStringLiteral("One of found notes was not found within initial notes");
                QNWARNING(errorDescription);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }

            if (note.notebookGuid() != m_extraNotebook.guid()) {
                errorDescription.base() = QStringLiteral("One of found notes has invalid notebook guid");
                errorDescription.details() = QStringLiteral("expected ");
                errorDescription.details() += m_extraNotebook.guid();
                errorDescription.details() += QStringLiteral(", found: ");
                errorDescription.details() += note.notebookGuid();
                QNWARNING(errorDescription);
                emit failure(errorDescription.nonLocalizedString());
                return;
            }
        }
    }
    HANDLE_WRONG_STATE();

    emit success();
}

void NoteLocalStorageManagerAsyncTester::onListNotesPerNotebookFailed(Notebook notebook, bool withResourceBinaryData,
                                                                      LocalStorageManager::ListObjectsOptions flag,
                                                                      size_t limit, size_t offset,
                                                                      LocalStorageManager::ListNotesOrder::type order,
                                                                      LocalStorageManager::OrderDirection::type orderDirection,
                                                                      ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)

    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", notebook: ") << notebook
              << QStringLiteral(", withResourceBinaryData = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")));
    emit failure(errorDescription.nonLocalizedString());
}

void NoteLocalStorageManagerAsyncTester::onExpungeNoteCompleted(Note note, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_modifiedNote != note) {
        errorDescription.base() = QStringLiteral("Internal error in NoteLocalStorageManagerAsyncTester: "
                                                 "note in onExpungeNoteCompleted slot doesn't match "
                                                 "the original expunged Note");
        QNWARNING(errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    bool withResourceBinaryData = true;
    emit findNoteRequest(m_foundNote, withResourceBinaryData);
}

void NoteLocalStorageManagerAsyncTester::onExpungeNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", note: ") << note);
    emit failure(errorDescription.nonLocalizedString());
}

void NoteLocalStorageManagerAsyncTester::onFailure(ErrorString errorDescription)
{
    QNWARNING(QStringLiteral("NoteLocalStorageManagerAsyncTester::onFailure: ") << errorDescription);
    emit failure(errorDescription.nonLocalizedString());
}

void NoteLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,failure,ErrorString),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onFailure,ErrorString));

    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,started),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,init));
    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));

    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,initialized),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onWorkerInitialized));

    // Request --> slot connections
    QObject::connect(this, QNSIGNAL(NoteLocalStorageManagerAsyncTester,addNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NoteLocalStorageManagerAsyncTester,getNoteCountRequest,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onNoteCountRequest,QUuid));
    QObject::connect(this, QNSIGNAL(NoteLocalStorageManagerAsyncTester,addNoteRequest,Note,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteLocalStorageManagerAsyncTester,updateNoteRequest,Note,bool,bool,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteLocalStorageManagerAsyncTester,findNoteRequest,Note,bool,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteLocalStorageManagerAsyncTester,listNotesPerNotebookRequest,Notebook,bool,
                                    LocalStorageManager::ListObjectsOptions,size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                                    LocalStorageManager::OrderDirection::type,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListNotesPerNotebookRequest,
                                                                Notebook,bool,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this, QNSIGNAL(NoteLocalStorageManagerAsyncTester,expungeNoteRequest,Note,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNoteRequest,Note,QUuid));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onAddNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onAddNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,noteCountComplete,int,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onNoteCountCompleted,int,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,noteCountFailed,ErrorString,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onNoteCountFailed,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onAddNoteCompleted,Note,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onAddNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onUpdateNoteCompleted,Note,bool,bool,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onUpdateNoteFailed,Note,bool,bool,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onFindNoteCompleted,Note,bool,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onFindNoteFailed,Note,bool,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listNotesPerNotebookComplete,Notebook,bool,
                              LocalStorageManager::ListObjectsOptions,size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QList<Note>,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onListNotesPerNotebookCompleted,Notebook,bool,
                                  LocalStorageManager::ListObjectsOptions,size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                                  LocalStorageManager::OrderDirection::type,QList<Note>,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listNotesPerNotebookFailed,Notebook,bool,
                              LocalStorageManager::ListObjectsOptions,size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,ErrorString,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onListNotesPerNotebookFailed,Notebook,bool,
                                  LocalStorageManager::ListObjectsOptions,size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                                  LocalStorageManager::OrderDirection::type,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onExpungeNoteCompleted,Note,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(NoteLocalStorageManagerAsyncTester,onExpungeNoteFailed,Note,ErrorString,QUuid));
}

} // namespace quentier
} // namespace test
