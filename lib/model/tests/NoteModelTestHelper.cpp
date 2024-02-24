/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include "TestMacros.h"
#include "TestUtils.h"
#include "modeltest.h"

#include <lib/model/note/NoteModel.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/UidGenerator.h>

#include <QEventLoop>
#include <QTimer>

namespace quentier {

NoteModelTestHelper::NoteModelTestHelper(
    local_storage::ILocalStoragePtr localStorage, QObject * parent) :
    QObject{parent},
    m_localStorage{std::move(localStorage)}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "NoteModelTestHelper ctor: local storage is null"}};
    }

    auto * notifier = m_localStorage->notifier();

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notePut, this,
        &NoteModelTestHelper::onNotePut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteUpdated, this,
        [this](
            const qevercloud::Note & note,
            const local_storage::ILocalStorage::UpdateNoteOptions options) {
            Q_UNUSED(options);
            onNoteUpdated(note);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteExpunged, this,
        &NoteModelTestHelper::onNoteExpunged);
}

NoteModelTestHelper::~NoteModelTestHelper() = default;

void NoteModelTestHelper::test()
{
    QNDEBUG("tests::model_test::NoteModel", "NoteModelTestHelper::test");

    ErrorString errorDescription;

    try {
        qevercloud::Notebook firstNotebook;
        firstNotebook.setName(QStringLiteral("First notebook"));
        firstNotebook.setLocalOnly(true);
        firstNotebook.setLocallyModified(false);

        qevercloud::Notebook secondNotebook;
        secondNotebook.setGuid(UidGenerator::Generate());
        secondNotebook.setName(QStringLiteral("Second notebook"));
        secondNotebook.setLocalOnly(false);
        secondNotebook.setLocallyModified(false);

        qevercloud::Notebook thirdNotebook;
        thirdNotebook.setGuid(UidGenerator::Generate());
        thirdNotebook.setName(QStringLiteral("Third notebook"));
        thirdNotebook.setLocalOnly(false);
        thirdNotebook.setLocallyModified(false);

        {
            auto f = threading::whenAll(
                QList<QFuture<void>>{}
                << m_localStorage->putNotebook(firstNotebook)
                << m_localStorage->putNotebook(secondNotebook)
                << m_localStorage->putNotebook(thirdNotebook));

            waitForFuture(f);

            try {
                f.waitForFinished();
            }
            catch (const IQuentierException & e) {
                Q_EMIT failure(e.errorMessage());
                return;
            }
            catch (const QException & e) {
                Q_EMIT failure(ErrorString{e.what()});
                return;
            }
        }

        qevercloud::Tag firstTag;
        firstTag.setName(QStringLiteral("First tag"));
        firstTag.setLocalOnly(true);
        firstTag.setLocallyModified(false);

        qevercloud::Tag secondTag;
        secondTag.setName(QStringLiteral("Second tag"));
        secondTag.setLocalOnly(true);
        secondTag.setLocallyModified(true);

        qevercloud::Tag thirdTag;
        thirdTag.setName(QStringLiteral("Third tag"));
        thirdTag.setGuid(UidGenerator::Generate());
        thirdTag.setLocalOnly(false);
        thirdTag.setLocallyModified(false);

        qevercloud::Tag fourthTag;
        fourthTag.setName(QStringLiteral("Fourth tag"));
        fourthTag.setGuid(UidGenerator::Generate());
        fourthTag.setLocalOnly(false);
        fourthTag.setLocallyModified(true);
        fourthTag.setParentGuid(thirdTag.guid());
        fourthTag.setParentTagLocalId(thirdTag.localId());

        const auto putTag = [this](const qevercloud::Tag & tag) {
            auto f = m_localStorage->putTag(tag);

            waitForFuture(f);

            try {
                f.waitForFinished();
            }
            catch (const IQuentierException & e) {
                Q_EMIT failure(e.errorMessage());
                return false;
            }
            catch (const QException & e) {
                Q_EMIT failure(ErrorString{e.what()});
                return false;
            }

            return true;
        };

        // Will add tags one by one to ensure that any parent tag would be
        // present in the local storage by the time its child tags are added
        {
            const auto tags = QList<qevercloud::Tag>{} << firstTag << secondTag
                                                       << thirdTag << fourthTag;
            for (const auto & tag: std::as_const(tags)) {
                if (!putTag(tag)) {
                    return;
                }
            }
        }

        qevercloud::Note firstNote;
        firstNote.setTitle(QStringLiteral("First note"));

        firstNote.setContent(
            QStringLiteral("<en-note><h1>First note</h1></en-note>"));

        firstNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        firstNote.setUpdated(firstNote.created());
        firstNote.setNotebookLocalId(firstNotebook.localId());
        firstNote.setLocalOnly(true);

        firstNote.setTagLocalIds(
            QStringList{} << firstTag.localId() << secondTag.localId());

        firstNote.setLocallyModified(false);

        qevercloud::Note secondNote;
        secondNote.setTitle(QStringLiteral("Second note"));

        secondNote.setContent(
            QStringLiteral("<en-note><h1>Second note</h1></en-note>"));

        secondNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        secondNote.setUpdated(QDateTime::currentMSecsSinceEpoch());
        secondNote.setNotebookLocalId(firstNotebook.localId());
        secondNote.setLocalOnly(true);
        secondNote.setTagLocalIds(QStringList{} << firstTag.localId());
        secondNote.setLocallyModified(true);

        qevercloud::Note thirdNote;
        thirdNote.setGuid(UidGenerator::Generate());
        thirdNote.setTitle(QStringLiteral("Third note"));

        thirdNote.setContent(
            QStringLiteral("<en-note><h1>Third note</h1></en-note>"));

        thirdNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        thirdNote.setUpdated(thirdNote.created());
        thirdNote.setNotebookLocalId(secondNotebook.localId());
        thirdNote.setNotebookGuid(secondNotebook.guid());
        thirdNote.setLocalOnly(false);
        thirdNote.setTagLocalIds(QStringList{} << thirdTag.localId());
        thirdNote.setTagGuids(
            QList<qevercloud::Guid>{} << thirdTag.guid().value());

        qevercloud::Note fourthNote;
        fourthNote.setGuid(UidGenerator::Generate());
        fourthNote.setTitle(QStringLiteral("Fourth note"));

        fourthNote.setContent(
            QStringLiteral("<en-note><h1>Fourth note</h1></en-note>"));

        fourthNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        fourthNote.setUpdated(fourthNote.created());
        fourthNote.setNotebookLocalId(secondNotebook.localId());
        fourthNote.setNotebookGuid(secondNotebook.guid());
        fourthNote.setLocalOnly(false);
        fourthNote.setLocallyModified(true);

        fourthNote.setTagLocalIds(
            QStringList{} << thirdTag.localId() << fourthTag.localId());

        fourthNote.setTagGuids(
            QList<qevercloud::Guid>{} << thirdTag.guid().value()
                                      << fourthTag.guid().value());

        qevercloud::Note fifthNote;
        fifthNote.setGuid(UidGenerator::Generate());
        fifthNote.setTitle(QStringLiteral("Fifth note"));

        fifthNote.setContent(
            QStringLiteral("<en-note><h1>Fifth note</h1></en-note>"));

        fifthNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        fifthNote.setUpdated(fifthNote.created());
        fifthNote.setNotebookLocalId(secondNotebook.localId());
        fifthNote.setNotebookGuid(secondNotebook.guid());
        fifthNote.setDeleted(QDateTime::currentMSecsSinceEpoch());
        fifthNote.setLocalOnly(false);
        fifthNote.setLocallyModified(true);

        qevercloud::Note sixthNote;
        sixthNote.setGuid(UidGenerator::Generate());
        sixthNote.setTitle(QStringLiteral("Sixth note"));

        sixthNote.setContent(
            QStringLiteral("<en-note><h1>Sixth note</h1></en-note>"));

        sixthNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        sixthNote.setUpdated(sixthNote.created());
        sixthNote.setNotebookLocalId(thirdNotebook.localId());
        sixthNote.setNotebookGuid(thirdNotebook.guid());
        sixthNote.setLocalOnly(false);
        sixthNote.setLocallyModified(false);
        sixthNote.setTagLocalIds(QStringList{} << fourthTag.localId());
        sixthNote.setTagGuids(
            QList<qevercloud::Guid>{} << fourthTag.guid().value());

        const auto putNote = [this](const qevercloud::Note & note) {
            auto f = m_localStorage->putNote(note);

            waitForFuture(f);

            try {
                f.waitForFinished();
            }
            catch (const IQuentierException & e) {
                Q_EMIT failure(e.errorMessage());
                return false;
            }
            catch (const QException & e) {
                Q_EMIT failure(ErrorString{e.what()});
                return false;
            }

            return true;
        };

        {
            const auto notes = QList<qevercloud::Note>{}
                << firstNote << secondNote << thirdNote << fourthNote
                << fifthNote << sixthNote;
            for (const auto & note: std::as_const(notes)) {
                if (!putNote(note)) {
                    return;
                }
            }
        }

        NoteCache noteCache{20};
        NotebookCache notebookCache{3};
        Account account{QStringLiteral("Default name"), Account::Type::Local};

        auto * model = new NoteModel(
            account, m_localStorage, noteCache, notebookCache,
            this, NoteModel::IncludedNotes::All);

        {
            QEventLoop loop;
            QObject::connect(
                model,
                &NoteModel::minimalNotesBatchLoaded,
                &loop,
                &QEventLoop::quit);

            QTimer invokingTimer;
            invokingTimer.setSingleShot(true);
            invokingTimer.singleShot(0, this, [model] { model->start(); });

            loop.exec();
        }

        ModelTest t1{model};
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        auto firstIndex = model->indexForLocalId(firstNote.localId());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for local id");
        }

        firstIndex = model->index(
            firstIndex.row(), static_cast<int>(NoteModel::Column::Dirty));
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for dirty column");
        }

        bool res = model->setData(firstIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change the dirty flag in the note "
                << "model manually which is not intended");
        }

        auto data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the note model while "
                << "expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL(
                "The dirty state appears to have changed after setData in "
                << "note model even though the method returned false");
        }

        // Should be able to make the non-synchronizable (local) item
        // synchronizable (non-local)
        firstIndex = model->index(
            firstIndex.row(),
            static_cast<int>(NoteModel::Column::Synchronizable));

        if (!firstIndex.isValid()) {
            FAIL(
                "Can't get the valid note model index for synchronizable "
                "column");
        }

        res = model->setData(firstIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL(
                "Can't change the synchronizable flag from false "
                << "to true for note model item");
        }

        // Change of synchronizable flag should have led to the change of the
        // item's row so fetching the relevant index from the model.
        firstIndex = model->indexForLocalId(firstNote.localId());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for local id");
        }

        firstIndex = model->index(
            firstIndex.row(),
            static_cast<int>(NoteModel::Column::Synchronizable));

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the note model while "
                << "expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The synchronizable flag appears to have not "
                << "changed after setData in note model even though "
                << "the method returned true");
        }

        // Verify the dirty flag has changed as a result of making the item
        // synchronizable

        // Get the item's index for local id again as its row might have
        // changed due to the automatic update of modification time
        firstIndex = model->indexForLocalId(firstNote.localId());
        if (!firstIndex.isValid()) {
            FAIL(
                "Can't get the valid note model item index for "
                << "local id after making the note synchronizable");
        }

        firstIndex = model->index(
            firstIndex.row(), static_cast<int>(NoteModel::Column::Dirty));
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for dirty column");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the note model while "
                << "expected to get the state of dirty flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The dirty state hasn't changed after making "
                << "the note model item synchronizable while it was "
                << "expected to have changed");
        }

        // Should not be able to make the synchronizable (non-local) item
        // non-synchronizable (local)
        firstIndex = model->index(
            firstIndex.row(),
            static_cast<int>(NoteModel::Column::Synchronizable));

        if (!firstIndex.isValid()) {
            FAIL(
                "Can't get the valid note model item index for "
                << "synchronizable column");
        }

        res = model->setData(firstIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change the synchronizable flag in "
                << "note model from true to false which is not intended");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the note model while "
                << "expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL(
                "The synchronizable state appears to have changed "
                << "after setData in note model even though the method "
                << "returned false");
        }

        // Should be able to change the title
        firstIndex = model->index(
            firstIndex.row(), static_cast<int>(NoteModel::Column::Title));
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid note model item index for title column");
        }

        QString newTitle = QStringLiteral("First note (modified)");
        res = model->setData(firstIndex, newTitle, Qt::EditRole);
        if (!res) {
            FAIL("Can't change the title of note model item");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the note model while "
                << "expected to get the note item's title");
        }

        if (data.toString() != newTitle) {
            FAIL(
                "The title of the note item returned by the model "
                << "does not match the title just set to this item: "
                << "received " << data.toString() << ", expected " << newTitle);
        }

        // Should be able to mark the note as deleted

        // Get the item's index for local id again as its row might have
        // changed due to the automatic update of modification time
        firstIndex = model->indexForLocalId(firstNote.localId());
        if (!firstIndex.isValid()) {
            FAIL(
                "Can't get the valid note model item index for "
                << "local id after changing the note's title");
        }

        firstIndex = model->index(
            firstIndex.row(),
            static_cast<int>(NoteModel::Column::DeletionTimestamp));
        if (!firstIndex.isValid()) {
            FAIL(
                "Can't get the valid note model item index for "
                << "deletion timestamp column");
        }

        qint64 deletionTimestamp = QDateTime::currentMSecsSinceEpoch();
        res = model->setData(firstIndex, deletionTimestamp, Qt::EditRole);
        if (!res) {
            FAIL("Can't set the deletion timestamp onto the note model item");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the note model while "
                << "expected to get the note item's deletion timestamp");
        }

        bool conversionResult = false;
        qint64 itemDeletionTimestamp = data.toLongLong(&conversionResult);
        if (!conversionResult) {
            FAIL(
                "Can't convert the note model item's deletion "
                << "timestamp data to the actual timestamp");
        }

        if (deletionTimestamp != itemDeletionTimestamp) {
            FAIL(
                "The note model item's deletion timestamp doesn't "
                << "match the timestamp originally set");
        }

        // Should be able to remove the deletion timestamp from the note

        // Get the item's index for local id again as its row might have
        // changed due to the automatic update of modification time
        firstIndex = model->indexForLocalId(firstNote.localId());
        if (!firstIndex.isValid()) {
            FAIL(
                "Can't get the valid note model item index for "
                << "local id after marking the note as deleted one");
        }

        deletionTimestamp = 0;
        res = model->setData(firstIndex, deletionTimestamp, Qt::EditRole);
        if (!res) {
            FAIL("Can't set zero deletion timestamp onto the note model item");
        }

        data = model->data(firstIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the note model while "
                << "expected to get the note item's deletion timestamp");
        }

        conversionResult = false;
        itemDeletionTimestamp = data.toLongLong(&conversionResult);
        if (!conversionResult) {
            FAIL(
                "Can't convert the note model item's deletion "
                << "timestamp data to the actual timestamp");
        }

        if (deletionTimestamp != itemDeletionTimestamp) {
            FAIL(
                "The note model item's deletion timestamp doesn't "
                << "match the expected value of 0");
        }

        // Should not be able to remove the row corresponding to a note with
        // non-empty guid
        auto thirdIndex = model->indexForLocalId(thirdNote.localId());
        if (!thirdIndex.isValid()) {
            FAIL("Can't get the valid note model item index for local id");
        }

        res = model->removeRow(thirdIndex.row(), QModelIndex());
        if (res) {
            FAIL(
                "Was able to remove the row corresponding to "
                << "a note with non-empty guid which is not intended");
        }

        auto thirdIndexAfterFailedRemoval =
            model->indexForLocalId(thirdNote.localId());

        if (!thirdIndexAfterFailedRemoval.isValid()) {
            FAIL(
                "Can't get the valid note model item index after "
                << "the failed row removal attempt");
        }

        if (thirdIndexAfterFailedRemoval.row() != thirdIndex.row()) {
            FAIL(
                "Note model returned item index with a different "
                << "row after the failed row removal attempt");
        }

        // Check sorting
        QVector<NoteModel::Column> columns;
        columns.reserve(model->columnCount(QModelIndex()));

        columns << NoteModel::Column::CreationTimestamp
                << NoteModel::Column::ModificationTimestamp
                << NoteModel::Column::DeletionTimestamp
                << NoteModel::Column::Title << NoteModel::Column::PreviewText
                << NoteModel::Column::NotebookName << NoteModel::Column::Size
                << NoteModel::Column::Synchronizable
                << NoteModel::Column::Dirty;

        int numColumns = columns.size();
        for (int i = 0; i < numColumns; ++i) {
            // Test the ascending case
            model->sort(static_cast<int>(columns[i]), Qt::AscendingOrder);
            {
                QEventLoop loop;
                QObject::connect(
                    model,
                    &NoteModel::minimalNotesBatchLoaded,
                    &loop,
                    &QEventLoop::quit);
                loop.exec();
            }

            checkSorting(*model);

            // Test the descending case
            model->sort(static_cast<int>(columns[i]), Qt::DescendingOrder);
            {
                QEventLoop loop;
                QObject::connect(
                    model,
                    &NoteModel::minimalNotesBatchLoaded,
                    &loop,
                    &QEventLoop::quit);
                loop.exec();
            }

            checkSorting(*model);
        }

        m_model = model;
        m_firstNotebook = firstNotebook;
        m_noteToExpungeLocalId = secondNote.localId();

        // Should be able to add new note model item and get asynchonous
        // acknowledgement from the local storage about that
        m_expectingNewNoteFromLocalStorage = true;
        ErrorString errorDescription;

        auto newItemIndex =
            model->createNoteItem(firstNotebook.localId(), errorDescription);

        if (!newItemIndex.isValid()) {
            FAIL(
                "Failed to create new note model item: "
                << errorDescription.nonLocalizedString());
        }

        {
            QEventLoop loop;
            QObject::connect(
                this, &NoteModelTestHelper::success, &loop, &QEventLoop::quit);
            QObject::connect(
                this, &NoteModelTestHelper::failure, &loop,
                [&](const ErrorString &) { loop.quit(); });
            loop.exec();
        }

        model->stop(IStartable::StopMode::Forced);
        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void NoteModelTestHelper::onNotePut(const qevercloud::Note & note)
{
    if (!m_expectingNewNoteFromLocalStorage) {
        return;
    }

    QNDEBUG(
        "tests::model_test::NoteModel",
        "NoteModelTestHelper::onNotePut: " << note);

    m_expectingNewNoteFromLocalStorage = false;

    ErrorString errorDescription;

    try {
        const auto * item = m_model->itemForLocalId(note.localId());
        if (Q_UNLIKELY(!item)) {
            FAIL(
                "Can't find just added note's item in the note "
                "model by local id");
        }

        // Should be able to update the note model item and get the asynchronous
        // acknowledgement from the local storage about that
        m_expectingNoteUpdateFromLocalStorage = true;
        auto itemIndex = m_model->indexForLocalId(note.localId());
        if (!itemIndex.isValid()) {
            FAIL(
                "Can't find the valid model index for the note "
                << "item just added to the model");
        }

        itemIndex = m_model->index(
            itemIndex.row(), static_cast<int>(NoteModel::Column::Title));
        if (!itemIndex.isValid()) {
            FAIL(
                "Can't find the valid model index for the note "
                << "item's title column");
        }

        QString title = QStringLiteral("Modified title");
        bool res = m_model->setData(itemIndex, title, Qt::EditRole);
        if (!res) {
            FAIL("Can't update the note item model's title");
        }

        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void NoteModelTestHelper::onNoteUpdated(const qevercloud::Note & note)
{
    if (m_expectingNoteUpdateFromLocalStorage) {
        QNDEBUG(
            "tests::model_test::NoteModel",
            "NoteModelTestHelper::onUpdateNoteComplete: note = " << note);

        m_expectingNoteUpdateFromLocalStorage = false;

        ErrorString errorDescription;

        try {
            const auto * item = m_model->itemForLocalId(note.localId());
            if (Q_UNLIKELY(!item)) {
                FAIL(
                    "Can't find the updated note's item in the note "
                    << "model by local id");
            }

            auto itemIndex = m_model->indexForLocalId(note.localId());
            if (!itemIndex.isValid()) {
                FAIL(
                    "Can't find the valid model index for the note "
                    << "item just updated");
            }

            QString title = item->title();
            if (title != QStringLiteral("Modified title")) {
                FAIL(
                    "It appears the note model item's title hasn't "
                    << "really changed as it was expected");
            }

            itemIndex = m_model->index(
                itemIndex.row(),
                static_cast<int>(NoteModel::Column::DeletionTimestamp));

            if (!itemIndex.isValid()) {
                FAIL(
                    "Can't find the valid model index for "
                    << "the note item's deletion timestamp column");
            }

            // Should be able to set the deletion timestamp to the note model
            // item and receive the asynchronous acknowledge from the local
            // storage
            m_expectingNoteDeletionFromLocalStorage = true;
            qint64 deletionTimestamp = QDateTime::currentMSecsSinceEpoch();

            bool res =
                m_model->setData(itemIndex, deletionTimestamp, Qt::EditRole);

            if (!res) {
                FAIL(
                    "Can't set the deletion timestamp onto the note model "
                    << "item");
            }

            return;
        }
        CATCH_EXCEPTION()

        Q_EMIT failure(errorDescription);
    }
    else if (m_expectingNoteDeletionFromLocalStorage) {
        QNDEBUG(
            "tests::model_test::NoteModel",
            "NoteModelTestHelper::onUpdateNoteComplete: note = " << note);

        m_expectingNoteDeletionFromLocalStorage = false;

        ErrorString errorDescription;

        try {
            const auto * item = m_model->itemForLocalId(note.localId());
            if (Q_UNLIKELY(!item)) {
                FAIL(
                    "Can't find the deleted note's item in "
                    << "the note model by local id");
            }

            if (item->deletionTimestamp() == 0) {
                FAIL(
                    "The note model item's deletion timestamp is "
                    << "unexpectedly zero");
            }

            // Should be able to remove the row with a non-synchronizable
            // (local) note and get the asynchronous acknowledgement from
            // the local storage
            auto itemIndex = m_model->indexForLocalId(m_noteToExpungeLocalId);
            if (!itemIndex.isValid()) {
                FAIL(
                    "Can't get the valid note model item index "
                    << "for local id");
            }

            m_expectingNoteExpungeFromLocalStorage = true;
            bool res = m_model->removeRow(itemIndex.row(), QModelIndex());
            if (!res) {
                FAIL(
                    "Can't remove the row with a non-synchronizable "
                    << "note item from the model");
            }

            return;
        }
        CATCH_EXCEPTION()

        Q_EMIT failure(errorDescription);
    }
}

void NoteModelTestHelper::onNoteExpunged(const QString & noteLocalId)
{
    if (!m_expectingNoteExpungeFromLocalStorage) {
        return;
    }

    QNDEBUG(
        "tests::model_test::NoteModel",
        "NoteModelTestHelper::onExpungeNoteComplete: " << noteLocalId);

    m_expectingNoteExpungeFromLocalStorage = false;

    ErrorString errorDescription;

    try {
        if (noteLocalId != m_noteToExpungeLocalId) {
            FAIL("Unexpected note got expunged from local storage");
        }

        auto itemIndex = m_model->indexForLocalId(m_noteToExpungeLocalId);
        if (itemIndex.isValid()) {
            FAIL(
                "Was able to get the valid model index for the removed note "
                << "item by local id which is not intended");
        }

        const auto * item = m_model->itemForLocalId(m_noteToExpungeLocalId);
        if (item) {
            FAIL(
                "Was able to get the non-null pointer to the note model item "
                << "while the corresponding note was expunged from the local "
                << "storage");
        }

        Q_EMIT success();
        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void NoteModelTestHelper::checkSorting(const NoteModel & model)
{
    const int numRows = model.rowCount(QModelIndex());

    QList<NoteModelItem> items;
    items.reserve(numRows);
    for (int i = 0; i < numRows; ++i) {
        const auto * item = model.itemAtRow(i);
        if (Q_UNLIKELY(!item)) {
            FAIL("Unexpected null pointer to the note model item");
        }

        items << *item;
    }

    const bool ascending = (model.sortOrder() == Qt::AscendingOrder);
    switch (model.sortingColumn()) {
    case NoteModel::Column::CreationTimestamp:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessByCreationTimestamp());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterByCreationTimestamp());
        }
        break;
    }
    case NoteModel::Column::ModificationTimestamp:
    {
        if (ascending) {
            std::sort(
                items.begin(), items.end(), LessByModificationTimestamp());
        }
        else {
            std::sort(
                items.begin(), items.end(), GreaterByModificationTimestamp());
        }
        break;
    }
    case NoteModel::Column::DeletionTimestamp:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessByDeletionTimestamp());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterByDeletionTimestamp());
        }
        break;
    }
    case NoteModel::Column::Title:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessByTitle());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterByTitle());
        }
        break;
    }
    case NoteModel::Column::PreviewText:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessByPreviewText());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterByPreviewText());
        }
        break;
    }
    case NoteModel::Column::NotebookName:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessByNotebookName());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterByNotebookName());
        }
        break;
    }
    case NoteModel::Column::Size:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessBySize());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterBySize());
        }
        break;
    }
    case NoteModel::Column::Synchronizable:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessBySynchronizable());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterBySynchronizable());
        }
        break;
    }
    default:
        break;
    }

    for (int i = 0; i < numRows; ++i) {
        const auto * item = model.itemAtRow(i);
        if (Q_UNLIKELY(!item)) {
            FAIL("Unexpected null pointer to the note model item");
        }

        if (item->localId() != items[i].localId()) {
            FAIL("Found mismatched note model items when checking the sorting");
        }
    }
}

bool NoteModelTestHelper::LessByCreationTimestamp::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.creationTimestamp() < rhs.creationTimestamp();
}

bool NoteModelTestHelper::GreaterByCreationTimestamp::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.creationTimestamp() > rhs.creationTimestamp();
}

bool NoteModelTestHelper::LessByModificationTimestamp::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.modificationTimestamp() < rhs.modificationTimestamp();
}

bool NoteModelTestHelper::GreaterByModificationTimestamp::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.modificationTimestamp() > rhs.modificationTimestamp();
}

bool NoteModelTestHelper::LessByDeletionTimestamp::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.deletionTimestamp() < rhs.deletionTimestamp();
}

bool NoteModelTestHelper::GreaterByDeletionTimestamp::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.deletionTimestamp() > rhs.deletionTimestamp();
}

bool NoteModelTestHelper::LessByTitle::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.title().localeAwareCompare(rhs.title()) < 0;
}

bool NoteModelTestHelper::GreaterByTitle::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.title().localeAwareCompare(rhs.title()) > 0;
}

bool NoteModelTestHelper::LessByPreviewText::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.previewText().localeAwareCompare(rhs.previewText()) < 0;
}

bool NoteModelTestHelper::GreaterByPreviewText::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.previewText().localeAwareCompare(rhs.previewText()) > 0;
}

bool NoteModelTestHelper::LessByNotebookName::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.notebookName().localeAwareCompare(rhs.notebookName()) < 0;
}

bool NoteModelTestHelper::GreaterByNotebookName::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.notebookName().localeAwareCompare(rhs.notebookName()) > 0;
}

bool NoteModelTestHelper::LessBySize::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.sizeInBytes() < rhs.sizeInBytes();
}

bool NoteModelTestHelper::GreaterBySize::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.sizeInBytes() > rhs.sizeInBytes();
}

bool NoteModelTestHelper::LessBySynchronizable::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return !lhs.isSynchronizable() && rhs.isSynchronizable();
}

bool NoteModelTestHelper::GreaterBySynchronizable::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.isSynchronizable() && !rhs.isSynchronizable();
}

bool NoteModelTestHelper::LessByDirty::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return !lhs.isDirty() && rhs.isDirty();
}

bool NoteModelTestHelper::GreaterByDirty::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const noexcept
{
    return lhs.isDirty() && !rhs.isDirty();
}

} // namespace quentier
