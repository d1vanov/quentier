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

#include "FavoritesModelTestHelper.h"

#include "TestMacros.h"
#include "TestUtils.h"
#include "modeltest.h"

#include <lib/model/favorites/FavoritesModel.h>
#include <lib/model/note/NoteModel.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/UidGenerator.h>

#include <QEventLoop>

#include <utility>

namespace quentier {

FavoritesModelTestHelper::FavoritesModelTestHelper(
    local_storage::ILocalStoragePtr localStorage, QObject * parent) :
    QObject{parent},
    m_localStorage{std::move(localStorage)}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "FavoritesModelTestHelper ctor: local storage is null"}};
    }

    auto * notifier = m_localStorage->notifier();

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteUpdated, this,
        [this](
            const qevercloud::Note & note,
            const local_storage::ILocalStorage::UpdateNoteOptions options) {
            Q_UNUSED(options);
            onNoteUpdated(note);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookPut, this,
        &FavoritesModelTestHelper::onNotebookPut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagPut, this,
        &FavoritesModelTestHelper::onTagPut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::savedSearchPut, this,
        &FavoritesModelTestHelper::onSavedSearchPut);
}

FavoritesModelTestHelper::~FavoritesModelTestHelper() = default;

void FavoritesModelTestHelper::test()
{
    QNDEBUG("tests:model_test:favorites", "FavoritesModelTestHelper::test");

    ErrorString errorDescription;

    const auto processFuture = [this](QFuture<void> & f) {
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

    const auto putNotebook = [&, this](const qevercloud::Notebook & notebook) {
        auto f = m_localStorage->putNotebook(notebook);
        return processFuture(f);
    };

    const auto putTag = [&, this](const qevercloud::Tag & tag) {
        auto f = m_localStorage->putTag(tag);
        return processFuture(f);
    };

    const auto putSavedSearch =
        [&, this](const qevercloud::SavedSearch & savedSearch) {
            auto f = m_localStorage->putSavedSearch(savedSearch);
            return processFuture(f);
        };

    const auto putNote = [&, this](const qevercloud::Note & note) {
        auto f = m_localStorage->putNote(note);
        return processFuture(f);
    };

    const auto updateNote = [&, this](const qevercloud::Note & note) {
        auto f = m_localStorage->updateNote(
            note, local_storage::ILocalStorage::UpdateNoteOptions{});
        return processFuture(f);
    };

    try {
        m_firstNotebook.setName(QStringLiteral("First notebook"));
        m_firstNotebook.setLocalOnly(true);
        m_firstNotebook.setLocallyModified(false);

        m_secondNotebook.setName(QStringLiteral("Second notebook"));
        m_secondNotebook.setLocalOnly(false);
        m_secondNotebook.setGuid(UidGenerator::Generate());
        m_secondNotebook.setLocallyModified(false);
        m_secondNotebook.setLocallyFavorited(true);

        m_thirdNotebook.setName(QStringLiteral("Third notebook"));
        m_thirdNotebook.setLocalOnly(true);
        m_thirdNotebook.setLocallyFavorited(true);
        m_thirdNotebook.setLocallyFavorited(true);

        {
            for (const auto & notebook: std::as_const(
                     QList<qevercloud::Notebook>{}
                     << m_firstNotebook << m_secondNotebook << m_thirdNotebook))
            {
                if (!putNotebook(notebook)) {
                    return;
                }
            }
        }

        m_firstSavedSearch.setName(QStringLiteral("First saved search"));
        m_firstSavedSearch.setLocalOnly(false);
        m_firstSavedSearch.setGuid(UidGenerator::Generate());
        m_firstSavedSearch.setLocallyModified(false);
        m_firstSavedSearch.setLocallyFavorited(true);

        m_secondSavedSearch.setName(QStringLiteral("Second saved search"));
        m_secondSavedSearch.setLocalOnly(true);
        m_secondSavedSearch.setLocallyModified(true);

        m_thirdSavedSearch.setName(QStringLiteral("Third saved search"));
        m_thirdSavedSearch.setLocalOnly(true);
        m_thirdSavedSearch.setLocallyModified(false);
        m_thirdSavedSearch.setLocallyFavorited(true);

        m_fourthSavedSearch.setName(QStringLiteral("Fourth saved search"));
        m_fourthSavedSearch.setLocalOnly(false);
        m_fourthSavedSearch.setGuid(UidGenerator::Generate());
        m_fourthSavedSearch.setLocallyModified(true);
        m_fourthSavedSearch.setLocallyFavorited(true);

        for (const auto & savedSearch: std::as_const(
                 QList<qevercloud::SavedSearch>{}
                 << m_firstSavedSearch << m_secondSavedSearch
                 << m_thirdSavedSearch << m_fourthSavedSearch))
        {
            if (!putSavedSearch(savedSearch)) {
                return;
            }
        }

        m_firstTag.setName(QStringLiteral("First tag"));
        m_firstTag.setLocalOnly(true);
        m_firstTag.setLocallyModified(false);

        m_secondTag.setName(QStringLiteral("Second tag"));
        m_secondTag.setLocalOnly(false);
        m_secondTag.setGuid(UidGenerator::Generate());
        m_secondTag.setLocallyModified(true);

        m_thirdTag.setName(QStringLiteral("Third tag"));
        m_thirdTag.setLocalOnly(false);
        m_thirdTag.setGuid(UidGenerator::Generate());
        m_thirdTag.setLocallyModified(false);
        m_thirdTag.setParentTagLocalId(m_secondTag.localId());
        m_thirdTag.setParentGuid(m_secondTag.guid());
        m_thirdTag.setLocallyFavorited(true);

        m_fourthTag.setName(QStringLiteral("Fourth tag"));
        m_fourthTag.setLocalOnly(true);
        m_fourthTag.setLocallyModified(true);
        m_fourthTag.setLocallyFavorited(true);

        for (const auto & tag: std::as_const(
                 QList<qevercloud::Tag>{} << m_firstTag << m_secondTag
                                          << m_thirdTag << m_fourthTag))
        {
            if (!putTag(tag)) {
                return;
            }
        }

        m_firstNote.setTitle(QStringLiteral("First note"));

        m_firstNote.setContent(
            QStringLiteral("<en-note><h1>First note</h1></en-note>"));

        m_firstNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        m_firstNote.setUpdated(m_firstNote.created());
        m_firstNote.setNotebookLocalId(m_firstNotebook.localId());
        m_firstNote.setLocalOnly(true);

        m_firstNote.setTagLocalIds(
            QStringList{} << m_firstTag.localId() << m_secondTag.localId());

        m_firstNote.setLocallyModified(false);
        m_firstNote.setLocallyFavorited(true);

        m_secondNote.setTitle(QStringLiteral("Second note"));

        m_secondNote.setContent(
            QStringLiteral("<en-note><h1>Second note</h1></en-note>"));

        m_secondNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        m_secondNote.setUpdated(QDateTime::currentMSecsSinceEpoch());

        m_secondNote.setNotebookLocalId(m_firstNotebook.localId());
        m_secondNote.setLocalOnly(true);

        m_secondNote.setTagLocalIds(
            QStringList{} << m_firstTag.localId() << m_fourthTag.localId());

        m_secondNote.setLocallyModified(true);
        m_secondNote.setLocallyFavorited(true);

        m_thirdNote.setGuid(UidGenerator::Generate());
        m_thirdNote.setTitle(QStringLiteral("Third note"));

        m_thirdNote.setContent(
            QStringLiteral("<en-note><h1>Third note</h1></en-note>"));

        m_thirdNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        m_thirdNote.setUpdated(m_thirdNote.created());
        m_thirdNote.setNotebookLocalId(m_secondNotebook.localId());
        m_thirdNote.setNotebookGuid(m_secondNotebook.guid());
        m_thirdNote.setLocalOnly(false);
        m_thirdNote.setTagLocalIds(QStringList{} << m_thirdTag.localId());
        m_thirdNote.setTagGuids(
            QList<qevercloud::Guid>{} << m_thirdTag.guid().value());
        m_thirdNote.setLocallyFavorited(true);

        m_fourthNote.setGuid(UidGenerator::Generate());
        m_fourthNote.setTitle(QStringLiteral("Fourth note"));

        m_fourthNote.setContent(
            QStringLiteral("<en-note><h1>Fourth note</h1></en-note>"));

        m_fourthNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        m_fourthNote.setUpdated(m_fourthNote.created());
        m_fourthNote.setNotebookLocalId(m_secondNotebook.localId());
        m_fourthNote.setNotebookGuid(m_secondNotebook.guid());
        m_fourthNote.setLocalOnly(false);
        m_fourthNote.setLocallyModified(true);

        m_fourthNote.setTagLocalIds(
            QStringList{} << m_secondTag.localId() << m_thirdTag.localId());

        m_fourthNote.setTagGuids(
            QList<qevercloud::Guid>{} << m_secondTag.guid().value()
                                      << m_thirdTag.guid().value());

        m_fifthNote.setGuid(UidGenerator::Generate());
        m_fifthNote.setTitle(QStringLiteral("Fifth note"));

        m_fifthNote.setContent(
            QStringLiteral("<en-note><h1>Fifth note</h1></en-note>"));

        m_fifthNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        m_fifthNote.setUpdated(m_fifthNote.created());
        m_fifthNote.setNotebookLocalId(m_secondNotebook.localId());
        m_fifthNote.setNotebookGuid(m_secondNotebook.guid());
        m_fifthNote.setDeleted(QDateTime::currentMSecsSinceEpoch());
        m_fifthNote.setLocalOnly(false);
        m_fifthNote.setLocallyModified(true);

        m_sixthNote.setTitle(QStringLiteral("Sixth note"));

        m_sixthNote.setContent(
            QStringLiteral("<en-note><h1>Sixth note</h1></en-note>"));

        m_sixthNote.setCreated(QDateTime::currentMSecsSinceEpoch());
        m_sixthNote.setUpdated(m_sixthNote.created());
        m_sixthNote.setNotebookLocalId(m_thirdNotebook.localId());
        m_sixthNote.setLocalOnly(true);
        m_sixthNote.setLocallyModified(true);
        m_sixthNote.setTagLocalIds(QStringList{} << m_fourthTag.localId());
        m_sixthNote.setLocallyFavorited(true);

        for (const auto & note: std::as_const(
                 QList<qevercloud::Note>{} << m_firstNote << m_secondNote
                                           << m_thirdNote << m_fourthNote
                                           << m_fifthNote << m_sixthNote))
        {
            if (!putNote(note)) {
                return;
            }
        }

        NoteCache noteCache{10};
        NotebookCache notebookCache{3};
        TagCache tagCache{5};
        SavedSearchCache savedSearchCache{5};

        Account account{QStringLiteral("Default user"), Account::Type::Local};

        auto * model = new FavoritesModel(
            account, m_localStorage, noteCache, notebookCache, tagCache,
            savedSearchCache, this);

        if (!model->allItemsListed()) {
            QEventLoop loop;
            QObject::connect(
                model,
                &FavoritesModel::notifyAllItemsListed,
                &loop,
                &QEventLoop::quit);
            loop.exec();
        }

        ModelTest t1{model};
        Q_UNUSED(t1)

        // The favorites model should have items corresponding to favorited
        // notebooks, notes, tags and saved searches and shouldn't have items
        // corresponding to non-favorited notebooks, notes, tags and saved
        // searches
        auto firstNotebookIndex =
            model->indexForLocalId(m_firstNotebook.localId());

        if (firstNotebookIndex.isValid()) {
            FAIL(
                "The favorites model unexpectedly contains "
                << "the item corresponding to non-favorited notebook");
        }

        auto secondSavedSearchIndex =
            model->indexForLocalId(m_secondSavedSearch.localId());

        if (secondSavedSearchIndex.isValid()) {
            FAIL(
                "The favorites model unexpectedly contains "
                << "the item corresponding to non-favorited saved search");
        }

        auto firstTagIndex = model->indexForLocalId(m_firstTag.localId());
        if (firstTagIndex.isValid()) {
            FAIL(
                "The favorites model unexpectedly contains "
                << "the item corresponding to non-favorited tag");
        }

        auto fourthNoteIndex = model->indexForLocalId(m_fourthNote.localId());
        if (fourthNoteIndex.isValid()) {
            FAIL(
                "The favorites model unexpectedly contains "
                << "the item corresponding to non-favorited note");
        }

        // The favorites model should have items corresponding to favorited
        // notebooks, notes, tags and saved searches
        auto secondNotebookIndex =
            model->indexForLocalId(m_secondNotebook.localId());

        if (!secondNotebookIndex.isValid()) {
            FAIL(
                "The favorites model unexpectedly doesn't contain "
                << "the item corresponding to favorited notebook");
        }

        auto firstSavedSearchIndex =
            model->indexForLocalId(m_firstSavedSearch.localId());

        if (!firstSavedSearchIndex.isValid()) {
            FAIL(
                "The favorites model unexpectedly doesn't contain "
                << "the item corresponding to favorited saved search");
        }

        auto thirdTagIndex = model->indexForLocalId(m_thirdTag.localId());
        if (!thirdTagIndex.isValid()) {
            FAIL(
                "The favorites model unexpectedly doesn't contain "
                << "the item corresponding to favorited tag");
        }

        auto firstNoteIndex = model->indexForLocalId(m_firstNote.localId());
        if (!firstNoteIndex.isValid()) {
            FAIL(
                "The favorites model unexpectedly doesn't contain "
                << "the item corresponding to favorited note");
        }

        // Shouldn't be able to change the type of the item manyally
        secondNotebookIndex = model->index(
            secondNotebookIndex.row(),
            static_cast<int>(FavoritesModel::Column::Type));

        if (!secondNotebookIndex.isValid()) {
            FAIL("Can't get the valid favorites model index for type column");
        }

        bool res = model->setData(
            secondNotebookIndex,
            QVariant(static_cast<int>(FavoritesModelItem::Type::Note)),
            Qt::EditRole);

        if (res) {
            FAIL(
                "Was able to change the type of the favorites "
                << "model item which is not intended");
        }

        auto data = model->data(secondNotebookIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the favorites model "
                << "while expected to get the type of the item");
        }

        bool conversionResult = false;
        qint32 type = data.toInt(&conversionResult);
        if (!conversionResult) {
            FAIL("Can't convert the favorites model item's type to int");
        }

        if (static_cast<FavoritesModelItem::Type>(type) !=
            FavoritesModelItem::Type::Notebook)
        {
            FAIL(
                "The favorites model item's type should be notebook "
                << "but it is not so after the attempt to change "
                << "the item's type manually");
        }

        // Should not be able to change the number of affected notes manually
        secondNotebookIndex = model->index(
            secondNotebookIndex.row(),
            static_cast<int>(FavoritesModel::Column::NoteCount));

        if (!secondNotebookIndex.isValid()) {
            FAIL(
                "Can't get the valid favorites model index for "
                << "num notes targeted column");
        }

        res = model->setData(secondNotebookIndex, QVariant(9999), Qt::EditRole);
        if (res) {
            FAIL(
                "Was able to change the num targeted notes for "
                << "the favorites model item which is not intended");
        }

        data = model->data(secondNotebookIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the favorites model while expected "
                << "to get the number of notes targeted by the item");
        }

        conversionResult = false;
        qint32 numNotesTargeted = data.toInt(&conversionResult);
        if (!conversionResult) {
            FAIL(
                "Can't convert the favorites model item's num "
                << "targeted notes to int");
        }

        if (numNotesTargeted == 9999) {
            FAIL(
                "The num notes targeted column appears to have "
                << "changed after setData in favorites model even "
                << "though the method returned false");
        }

        // Should be able to change the display name of the item
        secondNotebookIndex = model->index(
            secondNotebookIndex.row(),
            static_cast<int>(FavoritesModel::Column::DisplayName));

        if (!secondNotebookIndex.isValid()) {
            FAIL(
                "Can't get the valid favorites model index for "
                << "display name column");
        }

        res = model->setData(
            secondNotebookIndex,
            m_secondNotebook.name().value() + QStringLiteral("_modified"),
            Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        data = model->data(secondNotebookIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(
                "Null data was returned by the favorites model "
                << "while expected to get the display name of the item");
        }

        if (data.toString() !=
            (m_secondNotebook.name().value() + QStringLiteral("_modified")))
        {
            FAIL(
                "The name of the item appears to have not changed "
                << "after setData in favorites model even though "
                << "the method returned true");
        }

        // Favoriting some previously non-favorited item should make it appear
        // in the favorites model
        m_firstNotebook.setLocallyFavorited(true);

        if (!putNotebook(m_firstNotebook)) {
            return;
        }

        firstNotebookIndex = model->indexForLocalId(m_firstNotebook.localId());

        if (!firstNotebookIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to just favorited notebook");
        }

        m_secondSavedSearch.setLocallyFavorited(true);

        if (!putSavedSearch(m_secondSavedSearch)) {
            return;
        }

        secondSavedSearchIndex =
            model->indexForLocalId(m_secondSavedSearch.localId());

        if (!secondSavedSearchIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to just favorited saved search");
        }

        m_firstTag.setLocallyFavorited(true);
        if (!putTag(m_firstTag)) {
            return;
        }

        firstTagIndex = model->indexForLocalId(m_firstTag.localId());
        if (!firstTagIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to just favorited tag");
        }

        m_fourthNote.setLocallyFavorited(true);

        if (!putNote(m_fourthNote)) {
            return;
        }

        fourthNoteIndex = model->indexForLocalId(m_fourthNote.localId());
        if (!fourthNoteIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to just favorited note");
        }

        // After expunging the parent tag its child tags should should not be
        // present within the model
        {
            auto f = m_localStorage->expungeTagByLocalId(m_secondTag.localId());
            if (!processFuture(f)) {
                return;
            }
        }

        thirdTagIndex = model->indexForLocalId(m_thirdTag.localId());
        if (thirdTagIndex.isValid()) {
            FAIL(
                "Got valid model index for the tag item which "
                << "should have been removed from the favorites model "
                << "as its parent tag was expunged");
        }

        // After the tag's promotion expunging the tag previously being the
        // parent should not trigger the removal of its ex-child
        if (!putTag(m_secondTag)) {
            return;
        }

        m_thirdTag.setParentGuid(std::nullopt);
        m_thirdTag.setParentTagLocalId(QString{});

        if (!putTag(m_thirdTag)) {
            return;
        }

        thirdTagIndex = model->indexForLocalId(m_thirdTag.localId());
        if (!thirdTagIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to just added and updated tag");
        }

        {
            auto f = m_localStorage->expungeTagByLocalId(m_secondTag.localId());
            if (!processFuture(f)) {
                return;
            }
        }

        thirdTagIndex = model->indexForLocalId(m_thirdTag.localId());
        if (!thirdTagIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to a tag which has "
                << "previously been a child of now expunged tag");
        }

        // Unfavoriting the previously favorited item should make it disappear
        // from the favorites model
        auto thirdNotebookIndex =
            model->indexForLocalId(m_thirdNotebook.localId());

        if (!thirdNotebookIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites model item "
                << "corresponding to the favorited notebook");
        }

        m_thirdNotebook.setLocallyFavorited(false);

        if (!putNotebook(m_thirdNotebook)) {
            return;
        }

        thirdNotebookIndex = model->indexForLocalId(m_thirdNotebook.localId());

        if (thirdNotebookIndex.isValid()) {
            FAIL(
                "Was able to get the valid model index for the favorites "
                << "model item corresponding to the notebook which has just "
                << "been unfavorited");
        }

        auto thirdSavedSearchIndex =
            model->indexForLocalId(m_thirdSavedSearch.localId());

        if (!thirdSavedSearchIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited saved search");
        }

        m_thirdSavedSearch.setLocallyFavorited(false);

        if (!putSavedSearch(m_thirdSavedSearch)) {
            return;
        }

        thirdSavedSearchIndex =
            model->indexForLocalId(m_thirdSavedSearch.localId());

        if (thirdSavedSearchIndex.isValid()) {
            FAIL(
                "Was able to get the valid model index for "
                << "the favorites model item corresponding to "
                << "the saved search which has just been unfavorited");
        }

        thirdTagIndex = model->indexForLocalId(m_thirdTag.localId());
        if (!thirdTagIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited tag");
        }

        m_thirdTag.setLocallyFavorited(false);
        if (!putTag(m_thirdTag)) {
            return;
        }

        thirdTagIndex = model->indexForLocalId(m_thirdTag.localId());
        if (thirdTagIndex.isValid()) {
            FAIL(
                "Was able to get the valid model index for "
                << "the favorites model item corresponding to "
                << "the tag which has just been unfavorited");
        }

        auto thirdNoteIndex = model->indexForLocalId(m_thirdNote.localId());

        if (!thirdNoteIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited note");
        }

        m_thirdNote.setLocallyFavorited(false);

        if (!updateNote(m_thirdNote)) {
            return;
        }

        thirdNoteIndex = model->indexForLocalId(m_thirdNote.localId());
        if (thirdNoteIndex.isValid()) {
            FAIL(
                "Was able to get the valid model index for "
                << "the favorites model item corresponding to "
                << "the note which has just been unfavorited");
        }

        // The removal of items from the favorites model should cause
        // the updates of corresponding data elements in the local storage

        // First restore the favorited status of some items unfavorited just
        // above
        m_thirdNotebook.setLocallyFavorited(true);
        if (!putNotebook(m_thirdNotebook)) {
            return;
        }

        m_thirdSavedSearch.setLocallyFavorited(true);
        if (!putSavedSearch(m_thirdSavedSearch)) {
            return;
        }

        m_thirdTag.setLocallyFavorited(true);
        if (!putTag(m_thirdTag)) {
            return;
        }

        m_thirdNote.setLocallyFavorited(true);
        if (!updateNote(m_thirdNote)) {
            return;
        }

        thirdNotebookIndex = model->indexForLocalId(m_thirdNotebook.localId());

        if (!thirdNotebookIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited notebook");
        }

        m_expectingNotebookUnfavoriteFromLocalStorage = true;
        res = model->removeRow(thirdNotebookIndex.row(), QModelIndex());
        if (!res) {
            FAIL(
                "Can't remove row from the favorites model "
                << "corresponding to the favorited notebook");
        }

        if (m_thirdNotebook.isLocallyFavorited()) {
            FAIL(
                "The notebook which should have been unfavorited "
                << "after the removal of corresponding item "
                << "from the favorites model is still favorited");
        }

        thirdSavedSearchIndex =
            model->indexForLocalId(m_thirdSavedSearch.localId());

        if (!thirdSavedSearchIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited saved search");
        }

        m_expectingSavedSearchUnfavoriteFromLocalStorage = true;
        res = model->removeRow(thirdSavedSearchIndex.row(), QModelIndex());
        if (!res) {
            FAIL(
                "Can't remove row from the favorites model "
                << "corresponding to the favorited saved search");
        }

        if (m_thirdSavedSearch.isLocallyFavorited()) {
            FAIL(
                "The saved search which should have been unfavorited "
                << "after the removal of corresponding item "
                << "from the favorites model is still favorited");
        }

        thirdTagIndex = model->indexForLocalId(m_thirdTag.localId());
        if (!thirdTagIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited tag");
        }

        m_expectingTagUnfavoriteFromLocalStorage = true;
        res = model->removeRow(thirdTagIndex.row(), QModelIndex());
        if (!res) {
            FAIL(
                "Can't remove row from the favorites model "
                << "corresponding to the favorited tag");
        }

        if (m_thirdTag.isLocallyFavorited()) {
            FAIL(
                "The tag which should have been unfavorited after "
                << "the removal of corresponding item from "
                << "the favorites model is still favorited");
        }

        thirdNoteIndex = model->indexForLocalId(m_thirdNote.localId());
        if (!thirdNoteIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited note");
        }

        m_expectingNoteUnfavoriteFromLocalStorage = true;
        res = model->removeRow(thirdNoteIndex.row(), QModelIndex());
        if (!res) {
            FAIL(
                "Can't remove row from the favorites model "
                << "corresponding to the favorited note");
        }

        if (m_thirdNote.isLocallyFavorited()) {
            FAIL(
                "The note which should have been unfavorited after "
                << "the removal of corresponding item from "
                << "the favorites model is still favorited");
        }

        // Check sorting
        QList<FavoritesModel::Column> columns;
        columns.reserve(model->columnCount());

        columns << FavoritesModel::Column::Type
                << FavoritesModel::Column::DisplayName
                << FavoritesModel::Column::NoteCount;

        int numColumns = columns.size();
        for (int i = 0; i < numColumns; ++i) {
            // Test the ascending case
            model->sort(static_cast<int>(columns[i]), Qt::AscendingOrder);
            checkSorting(*model);

            // Test the descending case
            model->sort(static_cast<int>(columns[i]), Qt::DescendingOrder);
            checkSorting(*model);
        }

        // Ensure the modifications of favorites model items propagate properly
        // to the local storage
        m_expectingTagUpdateFromLocalStorage = true;
        m_expectingNotebookUpdateFromLocalStorage = true;
        m_expectingSavedSearchUpdateFromLocalStorage = true;
        m_expectingNoteUpdateFromLocalStorage = true;

        secondNotebookIndex =
            model->indexForLocalId(m_secondNotebook.localId());

        if (!secondNotebookIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited notebook");
        }

        secondNotebookIndex = model->index(
            secondNotebookIndex.row(),
            static_cast<int>(FavoritesModel::Column::DisplayName));

        if (!secondNotebookIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the display name column");
        }

        res = model->setData(
            secondNotebookIndex, QStringLiteral("Manual notebook name"),
            Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        firstSavedSearchIndex =
            model->indexForLocalId(m_firstSavedSearch.localId());

        if (!firstSavedSearchIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited saved search");
        }

        firstSavedSearchIndex = model->index(
            firstSavedSearchIndex.row(),
            static_cast<int>(FavoritesModel::Column::DisplayName));

        if (!firstSavedSearchIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the display name column");
        }

        res = model->setData(
            firstSavedSearchIndex, QStringLiteral("Manual saved search name"),
            Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        auto fourthTagIndex = model->indexForLocalId(m_fourthTag.localId());
        if (!fourthTagIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited tag");
        }

        fourthTagIndex = model->index(
            fourthTagIndex.row(),
            static_cast<int>(FavoritesModel::Column::DisplayName));

        if (!fourthTagIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the display name column");
        }

        res = model->setData(
            fourthTagIndex, QStringLiteral("Manual tag name"), Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        firstNoteIndex = model->indexForLocalId(m_firstNote.localId());
        if (!firstNoteIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited note");
        }

        firstNoteIndex = model->index(
            firstNoteIndex.row(),
            static_cast<int>(FavoritesModel::Column::DisplayName));

        if (!firstNoteIndex.isValid()) {
            FAIL(
                "Can't get the valid model index for the favorites "
                << "model item corresponding to the display name column");
        }

        res = model->setData(
            firstNoteIndex, QStringLiteral("Manual note name"), Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void FavoritesModelTestHelper::onNoteUpdated(const qevercloud::Note & note)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onNoteUpdated: note = " << note);

    if (m_expectingNoteUpdateFromLocalStorage) {
        if (note.title() != QStringLiteral("Manual note name")) {
            QString error = QStringLiteral(
                "The title of the note updated in the local storage doesn't "
                "match the expected one");

            Q_EMIT failure(ErrorString{std::move(error)});
            return;
        }

        m_expectingNoteUpdateFromLocalStorage = false;

        if (!m_expectingNotebookUpdateFromLocalStorage &&
            !m_expectingTagUpdateFromLocalStorage &&
            !m_expectingSavedSearchUpdateFromLocalStorage)
        {
            Q_EMIT success();
        }
    }
    else if (m_expectingNoteUnfavoriteFromLocalStorage) {
        if (note.isLocallyFavorited()) {
            QString error = QStringLiteral(
                "The note which should have been unfavorited in the local "
                "storage is still favorited");

            Q_EMIT failure(ErrorString{std::move(error)});
            return;
        }

        m_thirdNote = note;
        m_expectingNoteUnfavoriteFromLocalStorage = false;
    }
}

void FavoritesModelTestHelper::onNotebookPut(
    const qevercloud::Notebook & notebook)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateNotebookComplete: notebook = "
            << notebook);

    if (m_expectingNotebookUpdateFromLocalStorage) {
        if (notebook.name() != QStringLiteral("Manual notebook name")) {
            QString error = QStringLiteral(
                "The name of the notebook updated in the local storage doesn't "
                "match the expected one");

            Q_EMIT failure(ErrorString{std::move(error)});
            return;
        }

        m_expectingNotebookUpdateFromLocalStorage = false;

        if (!m_expectingNoteUpdateFromLocalStorage &&
            !m_expectingTagUpdateFromLocalStorage &&
            !m_expectingSavedSearchUpdateFromLocalStorage)
        {
            Q_EMIT success();
        }
    }
    else if (m_expectingNotebookUnfavoriteFromLocalStorage) {
        if (notebook.isLocallyFavorited()) {
            QString error = QStringLiteral(
                "The notebook which should have been unfavorited in the local "
                "storage is still favorited");

            Q_EMIT failure(ErrorString{std::move(error)});
            return;
        }

        m_thirdNotebook = notebook;
        m_expectingNotebookUnfavoriteFromLocalStorage = false;
    }
}

void FavoritesModelTestHelper::onTagPut(const qevercloud::Tag & tag)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateTagComplete: tag = " << tag);

    if (m_expectingTagUpdateFromLocalStorage) {
        if (tag.name() != QStringLiteral("Manual tag name")) {
            QString error = QStringLiteral(
                "The name of the tag updated in the local storage doesn't "
                "match the expected one");

            Q_EMIT failure(ErrorString{std::move(error)});
            return;
        }

        m_expectingTagUpdateFromLocalStorage = false;

        if (!m_expectingNoteUpdateFromLocalStorage &&
            !m_expectingSavedSearchUpdateFromLocalStorage &&
            !m_expectingNotebookUpdateFromLocalStorage)
        {
            Q_EMIT success();
        }
    }
    else if (m_expectingTagUnfavoriteFromLocalStorage) {
        if (tag.isLocallyFavorited()) {
            QString error = QStringLiteral(
                "The tag which should have been unfavorited in the local "
                "storage is still favorited");

            Q_EMIT failure(ErrorString{std::move(error)});
            return;
        }

        m_thirdTag = tag;
        m_expectingTagUnfavoriteFromLocalStorage = false;
    }
}

void FavoritesModelTestHelper::onSavedSearchPut(
    const qevercloud::SavedSearch & search)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateSavedSearchComplete: search = "
            << search);

    if (m_expectingSavedSearchUpdateFromLocalStorage) {
        if (search.name() != QStringLiteral("Manual saved search name")) {
            QString error = QStringLiteral(
                "The name of the saved search updated in the local storage "
                "doesn't match the expected one");

            Q_EMIT failure(ErrorString{std::move(error)});
            return;
        }

        m_expectingSavedSearchUpdateFromLocalStorage = false;

        if (!m_expectingNoteUpdateFromLocalStorage &&
            !m_expectingNotebookUpdateFromLocalStorage &&
            !m_expectingTagUpdateFromLocalStorage)
        {
            Q_EMIT success();
        }
    }
    else if (m_expectingSavedSearchUnfavoriteFromLocalStorage) {
        if (search.isLocallyFavorited()) {
            QString error = QStringLiteral(
                "The saved search which should have been unfavorited in "
                "the local storage is still favorited");

            Q_EMIT failure(ErrorString{std::move(error)});
            return;
        }

        m_thirdSavedSearch = search;
        m_expectingSavedSearchUnfavoriteFromLocalStorage = false;
    }
}

void FavoritesModelTestHelper::checkSorting(const FavoritesModel & model)
{
    QNDEBUG(
        "tests:model_test:favorites", "FavoritesModelTestHelper::checkSorting");

    const int numRows = model.rowCount();

    QList<FavoritesModelItem> items;
    items.reserve(numRows);
    for (int i = 0; i < numRows; ++i) {
        const auto * item = model.itemAtRow(i);
        if (Q_UNLIKELY(!item)) {
            FAIL("Unexpected null pointer to the favorites model item");
        }

        items << *item;
    }

    const bool ascending = (model.sortOrder() == Qt::AscendingOrder);
    switch (static_cast<FavoritesModel::Column>(model.sortingColumn())) {
    case FavoritesModel::Column::Type:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessByType());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterByType());
        }
        break;
    }
    case FavoritesModel::Column::DisplayName:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessByDisplayName());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterByDisplayName());
        }
        break;
    }
    case FavoritesModel::Column::NoteCount:
    {
        if (ascending) {
            std::sort(items.begin(), items.end(), LessByNoteCount());
        }
        else {
            std::sort(items.begin(), items.end(), GreaterByNoteCount());
        }
        break;
    }
    }

    for (int i = 0; i < numRows; ++i) {
        const auto * item = model.itemAtRow(i);
        if (Q_UNLIKELY(!item)) {
            FAIL("Unexpected null pointer to the favorites model item");
        }

        if (item->localId() != items[i].localId()) {
            FAIL(
                "Found mismatched favorites model items when checking "
                << "the sorting");
        }
    }
}

bool FavoritesModelTestHelper::LessByType::operator()(
    const FavoritesModelItem & lhs,
    const FavoritesModelItem & rhs) const noexcept
{
    return lhs.type() < rhs.type();
}

bool FavoritesModelTestHelper::GreaterByType::operator()(
    const FavoritesModelItem & lhs,
    const FavoritesModelItem & rhs) const noexcept
{
    return lhs.type() > rhs.type();
}

bool FavoritesModelTestHelper::LessByDisplayName::operator()(
    const FavoritesModelItem & lhs,
    const FavoritesModelItem & rhs) const noexcept
{
    return lhs.displayName().localeAwareCompare(rhs.displayName()) < 0;
}

bool FavoritesModelTestHelper::GreaterByDisplayName::operator()(
    const FavoritesModelItem & lhs,
    const FavoritesModelItem & rhs) const noexcept
{
    return lhs.displayName().localeAwareCompare(rhs.displayName()) > 0;
}

bool FavoritesModelTestHelper::LessByNoteCount::operator()(
    const FavoritesModelItem & lhs,
    const FavoritesModelItem & rhs) const noexcept
{
    return lhs.noteCount() < rhs.noteCount();
}

bool FavoritesModelTestHelper::GreaterByNoteCount::operator()(
    const FavoritesModelItem & lhs,
    const FavoritesModelItem & rhs) const noexcept
{
    return lhs.noteCount() > rhs.noteCount();
}

} // namespace quentier
