/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteModelTestHelper.h"
#include "../../models/NoteModel.h"
#include "modeltest.h"
#include "TestMacros.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/exception/IQuentierException.h>

namespace quentier {

NoteModelTestHelper::NoteModelTestHelper(LocalStorageManagerAsync * pLocalStorageManagerAsync,
                                         QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerAsync(pLocalStorageManagerAsync),
    m_model(Q_NULLPTR),
    m_firstNotebook(),
    m_noteToExpungeLocalUid(),
    m_expectingNewNoteFromLocalStorage(false),
    m_expectingNoteUpdateFromLocalStorage(false),
    m_expectingNoteDeletionFromLocalStorage(false),
    m_expectingNoteExpungeFromLocalStorage(false)
{
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onAddNoteComplete,Note,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onAddNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onUpdateNoteComplete,Note,LocalStorageManager::UpdateNoteOptions,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteFailed,Note,LocalStorageManager::UpdateNoteOptions,ErrorString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onUpdateNoteFailed,Note,LocalStorageManager::UpdateNoteOptions,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,Note,bool,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onFindNoteFailed,Note,bool,bool,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listNotesFailed,
                                                         LocalStorageManager::ListObjectsOptions,bool,bool,
                                                         size_t,size_t,LocalStorageManager::ListNotesOrder::type,
                                                         LocalStorageManager::OrderDirection::type,QString,ErrorString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onListNotesFailed,LocalStorageManager::ListObjectsOptions,bool,bool,
                                  size_t,size_t,LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,
                                  QString,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onExpungeNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onAddNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onUpdateNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(NoteModelTestHelper,onAddTagFailed,Tag,ErrorString,QUuid));
}

void NoteModelTestHelper::launchTest()
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::launchTest"));

    ErrorString errorDescription;

    try {
        Notebook firstNotebook;
        firstNotebook.setName(QStringLiteral("First notebook"));
        firstNotebook.setLocal(true);
        firstNotebook.setDirty(false);

        Notebook secondNotebook;
        secondNotebook.setGuid(UidGenerator::Generate());
        secondNotebook.setName(QStringLiteral("Second notebook"));
        secondNotebook.setLocal(false);
        secondNotebook.setDirty(false);

        Notebook thirdNotebook;
        thirdNotebook.setGuid(UidGenerator::Generate());
        thirdNotebook.setName(QStringLiteral("Third notebook"));
        thirdNotebook.setLocal(false);
        thirdNotebook.setDirty(false);

        m_pLocalStorageManagerAsync->onAddNotebookRequest(firstNotebook, QUuid());
        m_pLocalStorageManagerAsync->onAddNotebookRequest(secondNotebook, QUuid());
        m_pLocalStorageManagerAsync->onAddNotebookRequest(thirdNotebook, QUuid());

        Tag firstTag;
        firstTag.setName(QStringLiteral("First tag"));
        firstTag.setLocal(true);
        firstTag.setDirty(false);

        Tag secondTag;
        secondTag.setName(QStringLiteral("Second tag"));
        secondTag.setLocal(true);
        secondTag.setDirty(true);

        Tag thirdTag;
        thirdTag.setName(QStringLiteral("Third tag"));
        thirdTag.setGuid(UidGenerator::Generate());
        thirdTag.setLocal(false);
        thirdTag.setDirty(false);

        Tag fourthTag;
        fourthTag.setName(QStringLiteral("Fourth tag"));
        fourthTag.setGuid(UidGenerator::Generate());
        fourthTag.setLocal(false);
        fourthTag.setDirty(true);
        fourthTag.setParentGuid(thirdTag.guid());
        fourthTag.setParentLocalUid(thirdTag.localUid());

        m_pLocalStorageManagerAsync->onAddTagRequest(firstTag, QUuid());
        m_pLocalStorageManagerAsync->onAddTagRequest(secondTag, QUuid());
        m_pLocalStorageManagerAsync->onAddTagRequest(thirdTag, QUuid());
        m_pLocalStorageManagerAsync->onAddTagRequest(fourthTag, QUuid());

        Note firstNote;
        firstNote.setTitle(QStringLiteral("First note"));
        firstNote.setContent(QStringLiteral("<en-note><h1>First note</h1></en-note>"));
        firstNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        firstNote.setModificationTimestamp(firstNote.creationTimestamp());
        firstNote.setNotebookLocalUid(firstNotebook.localUid());
        firstNote.setLocal(true);
        firstNote.setTagLocalUids(QStringList() << firstTag.localUid() << secondTag.localUid());
        firstNote.setDirty(false);

        Note secondNote;
        secondNote.setTitle(QStringLiteral("Second note"));
        secondNote.setContent(QStringLiteral("<en-note><h1>Second note</h1></en-note>"));
        secondNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        secondNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
        secondNote.setNotebookLocalUid(firstNotebook.localUid());
        secondNote.setLocal(true);
        secondNote.setTagLocalUids(QStringList() << firstTag.localUid());
        secondNote.setDirty(true);

        Note thirdNote;
        thirdNote.setGuid(UidGenerator::Generate());
        thirdNote.setTitle(QStringLiteral("Third note"));
        thirdNote.setContent(QStringLiteral("<en-note><h1>Third note</h1></en-note>"));
        thirdNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        thirdNote.setModificationTimestamp(thirdNote.creationTimestamp());
        thirdNote.setNotebookLocalUid(secondNotebook.localUid());
        thirdNote.setNotebookGuid(secondNotebook.guid());
        thirdNote.setLocal(false);
        thirdNote.setTagLocalUids(QStringList() << thirdTag.localUid());
        thirdNote.setTagGuids(QStringList() << thirdTag.guid());

        Note fourthNote;
        fourthNote.setGuid(UidGenerator::Generate());
        fourthNote.setTitle(QStringLiteral("Fourth note"));
        fourthNote.setContent(QStringLiteral("<en-note><h1>Fourth note</h1></en-note>"));
        fourthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        fourthNote.setModificationTimestamp(fourthNote.creationTimestamp());
        fourthNote.setNotebookLocalUid(secondNotebook.localUid());
        fourthNote.setNotebookGuid(secondNotebook.guid());
        fourthNote.setLocal(false);
        fourthNote.setDirty(true);
        fourthNote.setTagLocalUids(QStringList() << thirdTag.localUid() << fourthTag.localUid());
        fourthNote.setTagGuids(QStringList() << thirdTag.guid() << fourthTag.guid());

        Note fifthNote;
        fifthNote.setGuid(UidGenerator::Generate());
        fifthNote.setTitle(QStringLiteral("Fifth note"));
        fifthNote.setContent(QStringLiteral("<en-note><h1>Fifth note</h1></en-note>"));
        fifthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        fifthNote.setModificationTimestamp(fifthNote.creationTimestamp());
        fifthNote.setNotebookLocalUid(secondNotebook.localUid());
        fifthNote.setNotebookGuid(secondNotebook.guid());
        fifthNote.setDeletionTimestamp(QDateTime::currentMSecsSinceEpoch());
        fifthNote.setLocal(false);
        fifthNote.setDirty(true);

        Note sixthNote;
        sixthNote.setGuid(UidGenerator::Generate());
        sixthNote.setTitle(QStringLiteral("Sixth note"));
        sixthNote.setContent(QStringLiteral("<en-note><h1>Sixth note</h1></en-note>"));
        sixthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        sixthNote.setModificationTimestamp(sixthNote.creationTimestamp());
        sixthNote.setNotebookLocalUid(thirdNotebook.localUid());
        sixthNote.setNotebookGuid(thirdNotebook.guid());
        sixthNote.setLocal(false);
        sixthNote.setDirty(false);
        sixthNote.setTagLocalUids(QStringList() << fourthTag.localUid());
        sixthNote.setTagGuids(QStringList() << fourthTag.guid());

        m_pLocalStorageManagerAsync->onAddNoteRequest(firstNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(secondNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(thirdNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(fourthNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(fifthNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(sixthNote, QUuid());

        NoteCache noteCache(20);
        NotebookCache notebookCache(3);
        Account account(QStringLiteral("Default name"), Account::Type::Local);

        NoteModel * model = new NoteModel(account, *m_pLocalStorageManagerAsync,
                                          noteCache, notebookCache, this, NoteModel::IncludedNotes::All);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex firstIndex = model->indexForLocalUid(firstNote.localUid());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for local uid"));
        }

        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Dirty, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for dirty column"));
        }

        bool res = model->setData(firstIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL(QStringLiteral("Was able to change the dirty flag in the note model manually which is not intended"));
        }

        QVariant data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the note model while expected to get the state of dirty flag"));
        }

        if (data.toBool()) {
            FAIL(QStringLiteral("The dirty state appears to have changed after setData in note model even though the method returned false"));
        }

        // Should be able to make the non-synchronizable (local) item synchronizable (non-local)
        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Synchronizable, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model index for synchronizable column"));
        }

        res = model->setData(firstIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't change the synchronizable flag from false to true for note model item"));
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the note model while expected to get the state of synchronizable flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The synchronizable flag appears to have not changed after setData in note model even though the method returned true"));
        }

        // Verify the dirty flag has changed as a result of making the item synchronizable

        // Get the item's index for local uid again as its row might have changed
        // due to the automatic update of modification time
        firstIndex = model->indexForLocalUid(firstNote.localUid());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for local uid after making the note synchronizable"));
        }

        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Dirty, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for dirty column"));
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the note model while expected to get the state of dirty flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The dirty state hasn't changed after making the note model item synchronizable while it was expected to have changed"));
        }

        // Should not be able to make the synchronizable (non-local) item non-synchronizable (local)
        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Synchronizable, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for synchronizable column"));
        }

        res = model->setData(firstIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL(QStringLiteral("Was able to change the synchronizable flag in note model from true to false which is not intended"));
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the note model while expected to get the state of synchronizable flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The synchronizable state appears to have changed after setData in note model even though the method returned false"));
        }

        // Should be able to change the title
        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::Title, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for title column"));
        }

        QString newTitle = QStringLiteral("First note (modified)");
        res = model->setData(firstIndex, newTitle, Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't change the title of note model item"));
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the note model while expected to get the note item's title"));
        }

        if (data.toString() != newTitle) {
            FAIL(QStringLiteral("The title of the note item returned by the model does not match the title just set to this item: received ")
                 << data.toString() << QStringLiteral(", expected ") << newTitle);
        }

        // Should be able to mark the note as deleted

        // Get the item's index for local uid again as its row might have changed
        // due to the automatic update of modification time
        firstIndex = model->indexForLocalUid(firstNote.localUid());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for local uid after changing the note's title"));
        }

        firstIndex = model->index(firstIndex.row(), NoteModel::Columns::DeletionTimestamp, QModelIndex());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for deletion timestamp column"));
        }

        qint64 deletionTimestamp = QDateTime::currentMSecsSinceEpoch();
        res = model->setData(firstIndex, deletionTimestamp, Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't set the deletion timestamp onto the note model item"));
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the note model while expected to get the note item's deletion timestamp"));
        }

        bool conversionResult = false;
        qint64 itemDeletionTimestamp = data.toLongLong(&conversionResult);
        if (!conversionResult) {
            FAIL(QStringLiteral("Can't convert the note model item's deletion timestamp data to the actual timestamp"));
        }

        if (deletionTimestamp != itemDeletionTimestamp) {
            FAIL(QStringLiteral("The note model item's deletion timestamp doesn't match the timestamp originally set"));
        }

        // Should be able to remove the deletion timestamp from the note

        // Get the item's index for local uid again as its row might have changed
        // due to the automatic update of modification time
        firstIndex = model->indexForLocalUid(firstNote.localUid());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for local uid after marking the note as deleted one"));
        }

        deletionTimestamp = 0;
        res = model->setData(firstIndex, deletionTimestamp, Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't set zero deletion timestamp onto the note model item"));
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the note model while expected to get the note item's deletion timestamp"));
        }

        conversionResult = false;
        itemDeletionTimestamp = data.toLongLong(&conversionResult);
        if (!conversionResult) {
            FAIL(QStringLiteral("Can't convert the note model item's deletion timestamp data to the actual timestamp"));
        }

        if (deletionTimestamp != itemDeletionTimestamp) {
            FAIL(QStringLiteral("The note model item's deletion timestamp doesn't match the expected value of 0"));
        }

        // Should not be able to remove the row corresponding to a note with non-empty guid
        QModelIndex thirdIndex = model->indexForLocalUid(thirdNote.localUid());
        if (!thirdIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index for local uid"));
        }

        res = model->removeRow(thirdIndex.row(), QModelIndex());
        if (res) {
            FAIL(QStringLiteral("Was able to remove the row corresponding to a note with non-empty guid which is not intended"));
        }

        QModelIndex thirdIndexAfterFailedRemoval = model->indexForLocalUid(thirdNote.localUid());
        if (!thirdIndexAfterFailedRemoval.isValid()) {
            FAIL(QStringLiteral("Can't get the valid note model item index after the failed row removal attempt"));
        }

        if (thirdIndexAfterFailedRemoval.row() != thirdIndex.row()) {
            FAIL(QStringLiteral("Note model returned item index with a different row after the failed row removal attempt"));
        }

        // Check sorting
        QVector<NoteModel::Columns::type> columns;
        columns.reserve(model->columnCount(QModelIndex()));
        columns << NoteModel::Columns::CreationTimestamp
                << NoteModel::Columns::ModificationTimestamp
                << NoteModel::Columns::DeletionTimestamp
                << NoteModel::Columns::Title
                << NoteModel::Columns::PreviewText
                << NoteModel::Columns::NotebookName
                << NoteModel::Columns::Size
                << NoteModel::Columns::Synchronizable
                << NoteModel::Columns::Dirty;

        int numColumns = columns.size();
        for(int i = 0; i < numColumns; ++i)
        {
            // Test the ascending case
            model->sort(columns[i], Qt::AscendingOrder);
            checkSorting(*model);

            // Test the descending case
            model->sort(columns[i], Qt::DescendingOrder);
            checkSorting(*model);
        }

        m_model = model;
        m_firstNotebook = firstNotebook;
        m_noteToExpungeLocalUid = secondNote.localUid();

        // Should be able to add the new note model item and get the asynchonous acknowledgement from the local storage about that
        m_expectingNewNoteFromLocalStorage = true;
        Q_UNUSED(model->createNoteItem(firstNotebook.localUid()))

        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void NoteModelTestHelper::onAddNoteComplete(Note note, QUuid requestId)
{
    if (!m_expectingNewNoteFromLocalStorage) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteModelTestHelper::onAddNoteComplete: note = ") << note);

    Q_UNUSED(requestId)
    m_expectingNewNoteFromLocalStorage = false;

    ErrorString errorDescription;

    try
    {
        const NoteModelItem * item = m_model->itemForLocalUid(note.localUid());
        if (Q_UNLIKELY(!item)) {
            FAIL(QStringLiteral("Can't find just added note's item in the note model by local uid"));
        }

        // Should be able to update the note model item and get the asynchronous acknowledgement from the local storage about that
        m_expectingNoteUpdateFromLocalStorage = true;
        QModelIndex itemIndex = m_model->indexForLocalUid(note.localUid());
        if (!itemIndex.isValid()) {
            FAIL(QStringLiteral("Can't find the valid model index for the note item just added to the model"));
        }

        itemIndex = m_model->index(itemIndex.row(), NoteModel::Columns::Title, QModelIndex());
        if (!itemIndex.isValid()) {
            FAIL(QStringLiteral("Can't find the valid model index for the note item's title column"));
        }

        QString title = QStringLiteral("Modified title");
        bool res = m_model->setData(itemIndex, title, Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't update the note item model's title"));
        }

        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void NoteModelTestHelper::onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::onAddNoteFailed: note = ") << note << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NoteModelTestHelper::onUpdateNoteComplete(Note note, LocalStorageManager::UpdateNoteOptions options, QUuid requestId)
{
    Q_UNUSED(options)
    Q_UNUSED(requestId)

    if (m_expectingNoteUpdateFromLocalStorage)
    {
        QNDEBUG(QStringLiteral("NoteModelTestHelper::onUpdateNoteComplete: note = ") << note);

        m_expectingNoteUpdateFromLocalStorage = false;

        ErrorString errorDescription;

        try
        {
            const NoteModelItem * item = m_model->itemForLocalUid(note.localUid());
            if (Q_UNLIKELY(!item)) {
                FAIL(QStringLiteral("Can't find the updated note's item in the note model by local uid"));
            }

            QModelIndex itemIndex = m_model->indexForLocalUid(note.localUid());
            if (!itemIndex.isValid()) {
                FAIL(QStringLiteral("Can't find the valid model index for the note item just updated"));
            }

            QString title = item->title();
            if (title != QStringLiteral("Modified title")) {
                FAIL(QStringLiteral("It appears the note model item's title hasn't really changed as it was expected"));
            }

            itemIndex = m_model->index(itemIndex.row(), NoteModel::Columns::DeletionTimestamp, QModelIndex());
            if (!itemIndex.isValid()) {
                FAIL(QStringLiteral("Can't find the valid model index for the note item's deletion timestamp column"));
            }

            // Should be able to set the deletion timestamp to the note model item and receive the asynchronous acknowledge from the local storage
            m_expectingNoteDeletionFromLocalStorage = true;
            qint64 deletionTimestamp = QDateTime::currentMSecsSinceEpoch();
            bool res = m_model->setData(itemIndex, deletionTimestamp, Qt::EditRole);
            if (!res) {
                FAIL(QStringLiteral("Can't set the deletion timestamp onto the note model item"));
            }

            return;
        }
        CATCH_EXCEPTION()

        Q_EMIT failure(errorDescription);
    }
    else if (m_expectingNoteDeletionFromLocalStorage)
    {
        QNDEBUG(QStringLiteral("NoteModelTestHelper::onUpdateNoteComplete: note = ") << note);

        Q_UNUSED(requestId)
        m_expectingNoteDeletionFromLocalStorage = false;

        ErrorString errorDescription;

        try
        {
            const NoteModelItem * item = m_model->itemForLocalUid(note.localUid());
            if (Q_UNLIKELY(!item)) {
                FAIL(QStringLiteral("Can't find the deleted note's item in the note model by local uid"));
            }

            if (item->deletionTimestamp() == 0) {
                FAIL(QStringLiteral("The note model item's deletion timestamp is unexpectedly zero"));
            }

            // Should be able to remove the row with a non-synchronizable (local) note and get
            // the asynchronous acknowledgement from the local storage
            QModelIndex itemIndex = m_model->indexForLocalUid(m_noteToExpungeLocalUid);
            if (!itemIndex.isValid()) {
                FAIL(QStringLiteral("Can't get the valid note model item index for local uid"));
            }

            m_expectingNoteExpungeFromLocalStorage = true;
            bool res = m_model->removeRow(itemIndex.row(), QModelIndex());
            if (!res) {
                FAIL(QStringLiteral("Can't remove the row with a non-synchronizable note item from the model"));
            }

            return;
        }
        CATCH_EXCEPTION()

        Q_EMIT failure(errorDescription);
    }
}

void NoteModelTestHelper::onUpdateNoteFailed(Note note, LocalStorageManager::UpdateNoteOptions options,
                                             ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::onUpdateNoteFailed: note = ") << note << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(options)
    notifyFailureWithStackTrace(errorDescription);
}

void NoteModelTestHelper::onFindNoteFailed(Note note, bool withResourceMetadata, bool withResourceBinaryData,
                                           ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::onFindNoteFailed: note = ") << note << QStringLiteral("\nWith resource metadata = ")
            << (withResourceMetadata ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", with resource binary data = ")
            << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", error description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NoteModelTestHelper::onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceMetadata,
                                            bool withResourceBinaryData, size_t limit, size_t offset,
                                            LocalStorageManager::ListNotesOrder::type order,
                                            LocalStorageManager::OrderDirection::type orderDirection,
                                            QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::onListNotesFailed: flag = ") << flag << QStringLiteral(", with resource metadata = ")
            << (withResourceMetadata ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", with resource binary data = ")
            << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NoteModelTestHelper::onExpungeNoteComplete(Note note, QUuid requestId)
{
    if (!m_expectingNoteExpungeFromLocalStorage) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteModelTestHelper::onExpungeNoteComplete: note = ") << note);

    Q_UNUSED(requestId)
    m_expectingNoteExpungeFromLocalStorage = false;

    ErrorString errorDescription;

    try
    {
        QModelIndex itemIndex = m_model->indexForLocalUid(m_noteToExpungeLocalUid);
        if (itemIndex.isValid()) {
            FAIL(QStringLiteral("Was able to get the valid model index for the removed note item by local uid which is not intended"));
        }

        const NoteModelItem * item = m_model->itemForLocalUid(m_noteToExpungeLocalUid);
        if (item) {
            FAIL(QStringLiteral("Was able to get the non-null pointer to the note model item while the corresponding note was expunged from local storage"));
        }

        Q_EMIT success();
        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void NoteModelTestHelper::onExpungeNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::onExpungeNoteFailed: note = ") << note << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NoteModelTestHelper::onAddNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::onAddNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NoteModelTestHelper::onUpdateNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::onUpdateNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NoteModelTestHelper::onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteModelTestHelper::onAddTagFailed: tag = ") << tag << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void NoteModelTestHelper::checkSorting(const NoteModel & model)
{
    int numRows = model.rowCount(QModelIndex());

    QVector<NoteModelItem> items;
    items.reserve(numRows);
    for(int i = 0; i < numRows; ++i)
    {
        const NoteModelItem * item = model.itemAtRow(i);
        if (Q_UNLIKELY(!item)) {
            FAIL(QStringLiteral("Unexpected null pointer to the note model item"));
        }

        items << *item;
    }

    bool ascending = (model.sortOrder() == Qt::AscendingOrder);
    switch(model.sortingColumn())
    {
    case NoteModel::Columns::CreationTimestamp:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByCreationTimestamp());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByCreationTimestamp());
            }
            break;
        }
    case NoteModel::Columns::ModificationTimestamp:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByModificationTimestamp());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByModificationTimestamp());
            }
            break;
        }
    case NoteModel::Columns::DeletionTimestamp:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByDeletionTimestamp());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByDeletionTimestamp());
            }
            break;
        }
    case NoteModel::Columns::Title:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByTitle());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByTitle());
            }
            break;
        }
    case NoteModel::Columns::PreviewText:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByPreviewText());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByPreviewText());
            }
            break;
        }
    case NoteModel::Columns::NotebookName:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByNotebookName());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByNotebookName());
            }
            break;
        }
    case NoteModel::Columns::Size:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessBySize());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterBySize());
            }
            break;
        }
    case NoteModel::Columns::Synchronizable:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessBySynchronizable());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterBySynchronizable());
            }
            break;
        }
    case NoteModel::Columns::ThumbnailImage:
    case NoteModel::Columns::TagNameList:
        break;
    }

    for(int i = 0; i < numRows; ++i)
    {
        const NoteModelItem * item = model.itemAtRow(i);
        if (Q_UNLIKELY(!item)) {
            FAIL(QStringLiteral("Unexpected null pointer to the note model item"));
        }

        if (item->localUid() != items[i].localUid()) {
            FAIL(QStringLiteral("Found mismatched note model items when checking the sorting"));
        }
    }
}

void NoteModelTestHelper::notifyFailureWithStackTrace(ErrorString errorDescription)
{
    SysInfo sysInfo;
    errorDescription.details() += QStringLiteral("\nStack trace: ") + sysInfo.stackTrace();
    Q_EMIT failure(errorDescription);
}

bool NoteModelTestHelper::LessByCreationTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.creationTimestamp() < rhs.creationTimestamp();
}

bool NoteModelTestHelper::GreaterByCreationTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.creationTimestamp() > rhs.creationTimestamp();
}

bool NoteModelTestHelper::LessByModificationTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.modificationTimestamp() < rhs.modificationTimestamp();
}

bool NoteModelTestHelper::GreaterByModificationTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.modificationTimestamp() > rhs.modificationTimestamp();
}

bool NoteModelTestHelper::LessByDeletionTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.deletionTimestamp() < rhs.deletionTimestamp();
}

bool NoteModelTestHelper::GreaterByDeletionTimestamp::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.deletionTimestamp() > rhs.deletionTimestamp();
}

bool NoteModelTestHelper::LessByTitle::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.title().localeAwareCompare(rhs.title()) < 0;
}

bool NoteModelTestHelper::GreaterByTitle::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.title().localeAwareCompare(rhs.title()) > 0;
}

bool NoteModelTestHelper::LessByPreviewText::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.previewText().localeAwareCompare(rhs.previewText()) < 0;
}

bool NoteModelTestHelper::GreaterByPreviewText::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.previewText().localeAwareCompare(rhs.previewText()) > 0;
}

bool NoteModelTestHelper::LessByNotebookName::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.notebookName().localeAwareCompare(rhs.notebookName()) < 0;
}

bool NoteModelTestHelper::GreaterByNotebookName::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.notebookName().localeAwareCompare(rhs.notebookName()) > 0;
}

bool NoteModelTestHelper::LessBySize::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.sizeInBytes() < rhs.sizeInBytes();
}

bool NoteModelTestHelper::GreaterBySize::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.sizeInBytes() > rhs.sizeInBytes();
}

bool NoteModelTestHelper::LessBySynchronizable::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return !lhs.isSynchronizable() && rhs.isSynchronizable();
}

bool NoteModelTestHelper::GreaterBySynchronizable::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.isSynchronizable() && !rhs.isSynchronizable();
}

bool NoteModelTestHelper::LessByDirty::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return !lhs.isDirty() && rhs.isDirty();
}

bool NoteModelTestHelper::GreaterByDirty::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    return lhs.isDirty() && !rhs.isDirty();
}

} // namespace quentier
