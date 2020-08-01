/*
 * Copyright 2016-2020 Dmitry Ivanov
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
#include "modeltest.h"
#include "TestMacros.h"

#include <lib/model/FavoritesModel.h>
#include <lib/model/NoteModel.h>

#include <quentier/exception/IQuentierException.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/SysInfo.h>

namespace quentier {

FavoritesModelTestHelper::FavoritesModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerAsync(pLocalStorageManagerAsync)
{
    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteComplete,
        this,
        &FavoritesModelTestHelper::onUpdateNoteComplete);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteFailed,
        this,
        &FavoritesModelTestHelper::onUpdateNoteFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteFailed,
        this,
        &FavoritesModelTestHelper::onFindNoteFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::listNotesFailed,
        this,
        &FavoritesModelTestHelper::onListNotesFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookComplete,
        this,
        &FavoritesModelTestHelper::onUpdateNotebookComplete);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookFailed,
        this,
        &FavoritesModelTestHelper::onUpdateNotebookFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookFailed,
        this,
        &FavoritesModelTestHelper::onFindNotebookFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::listNotebooksFailed,
        this,
        &FavoritesModelTestHelper::onListNotebooksFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateTagComplete,
        this,
        &FavoritesModelTestHelper::onUpdateTagComplete);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateTagFailed,
        this,
        &FavoritesModelTestHelper::onUpdateTagFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::findTagFailed,
        this,
        &FavoritesModelTestHelper::onFindTagFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::listTagsFailed,
        this,
        &FavoritesModelTestHelper::onListTagsFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateSavedSearchComplete,
        this,
        &FavoritesModelTestHelper::onUpdateSavedSearchComplete);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateSavedSearchFailed,
        this,
        &FavoritesModelTestHelper::onUpdateSavedSearchFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::findSavedSearchFailed,
        this,
        &FavoritesModelTestHelper::onFindSavedSearchFailed);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::listSavedSearchesFailed,
        this,
        &FavoritesModelTestHelper::onListSavedSearchesFailed);
}

FavoritesModelTestHelper::~FavoritesModelTestHelper() = default;

void FavoritesModelTestHelper::test()
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::test");

    ErrorString errorDescription;

    try
    {
        m_firstNotebook.setName(QStringLiteral("First notebook"));
        m_firstNotebook.setLocal(true);
        m_firstNotebook.setDirty(false);

        m_secondNotebook.setName(QStringLiteral("Second notebook"));
        m_secondNotebook.setLocal(false);
        m_secondNotebook.setGuid(UidGenerator::Generate());
        m_secondNotebook.setDirty(false);
        m_secondNotebook.setFavorited(true);

        m_thirdNotebook.setName(QStringLiteral("Third notebook"));
        m_thirdNotebook.setLocal(true);
        m_thirdNotebook.setDirty(true);
        m_thirdNotebook.setFavorited(true);

        m_pLocalStorageManagerAsync->onAddNotebookRequest(
            m_firstNotebook,
            QUuid());

        m_pLocalStorageManagerAsync->onAddNotebookRequest(
            m_secondNotebook,
            QUuid());

        m_pLocalStorageManagerAsync->onAddNotebookRequest(
            m_thirdNotebook,
            QUuid());

        m_firstSavedSearch.setName(QStringLiteral("First saved search"));
        m_firstSavedSearch.setLocal(false);
        m_firstSavedSearch.setGuid(UidGenerator::Generate());
        m_firstSavedSearch.setDirty(false);
        m_firstSavedSearch.setFavorited(true);

        m_secondSavedSearch.setName(QStringLiteral("Second saved search"));
        m_secondSavedSearch.setLocal(true);
        m_secondSavedSearch.setDirty(true);

        m_thirdSavedSearch.setName(QStringLiteral("Third saved search"));
        m_thirdSavedSearch.setLocal(true);
        m_thirdSavedSearch.setDirty(false);
        m_thirdSavedSearch.setFavorited(true);

        m_fourthSavedSearch.setName(QStringLiteral("Fourth saved search"));
        m_fourthSavedSearch.setLocal(false);
        m_fourthSavedSearch.setGuid(UidGenerator::Generate());
        m_fourthSavedSearch.setDirty(true);
        m_fourthSavedSearch.setFavorited(true);

        m_pLocalStorageManagerAsync->onAddSavedSearchRequest(
            m_firstSavedSearch, QUuid());

        m_pLocalStorageManagerAsync->onAddSavedSearchRequest(
            m_secondSavedSearch, QUuid());

        m_pLocalStorageManagerAsync->onAddSavedSearchRequest(
            m_thirdSavedSearch, QUuid());

        m_pLocalStorageManagerAsync->onAddSavedSearchRequest(
            m_fourthSavedSearch, QUuid());

        m_firstTag.setName(QStringLiteral("First tag"));
        m_firstTag.setLocal(true);
        m_firstTag.setDirty(false);

        m_secondTag.setName(QStringLiteral("Second tag"));
        m_secondTag.setLocal(false);
        m_secondTag.setGuid(UidGenerator::Generate());
        m_secondTag.setDirty(true);

        m_thirdTag.setName(QStringLiteral("Third tag"));
        m_thirdTag.setLocal(false);
        m_thirdTag.setGuid(UidGenerator::Generate());
        m_thirdTag.setDirty(false);
        m_thirdTag.setParentLocalUid(m_secondTag.localUid());
        m_thirdTag.setParentGuid(m_secondTag.guid());
        m_thirdTag.setFavorited(true);

        m_fourthTag.setName(QStringLiteral("Fourth tag"));
        m_fourthTag.setLocal(true);
        m_fourthTag.setDirty(true);
        m_fourthTag.setFavorited(true);

        m_pLocalStorageManagerAsync->onAddTagRequest(m_firstTag, QUuid());
        m_pLocalStorageManagerAsync->onAddTagRequest(m_secondTag, QUuid());
        m_pLocalStorageManagerAsync->onAddTagRequest(m_thirdTag, QUuid());
        m_pLocalStorageManagerAsync->onAddTagRequest(m_fourthTag, QUuid());

        m_firstNote.setTitle(QStringLiteral("First note"));

        m_firstNote.setContent(
            QStringLiteral("<en-note><h1>First note</h1></en-note>"));

        m_firstNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_firstNote.setModificationTimestamp(m_firstNote.creationTimestamp());
        m_firstNote.setNotebookLocalUid(m_firstNotebook.localUid());
        m_firstNote.setLocal(true);

        m_firstNote.setTagLocalUids(
            QStringList() << m_firstTag.localUid() << m_secondTag.localUid());

        m_firstNote.setDirty(false);
        m_firstNote.setFavorited(true);

        m_secondNote.setTitle(QStringLiteral("Second note"));

        m_secondNote.setContent(
            QStringLiteral("<en-note><h1>Second note</h1></en-note>"));

        m_secondNote.setCreationTimestamp(
            QDateTime::currentMSecsSinceEpoch());

        m_secondNote.setModificationTimestamp(
            QDateTime::currentMSecsSinceEpoch());

        m_secondNote.setNotebookLocalUid(m_firstNotebook.localUid());
        m_secondNote.setLocal(true);

        m_secondNote.setTagLocalUids(
            QStringList() << m_firstTag.localUid() << m_fourthTag.localUid());

        m_secondNote.setDirty(true);
        m_secondNote.setFavorited(true);

        m_thirdNote.setGuid(UidGenerator::Generate());
        m_thirdNote.setTitle(QStringLiteral("Third note"));

        m_thirdNote.setContent(
            QStringLiteral("<en-note><h1>Third note</h1></en-note>"));

        m_thirdNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_thirdNote.setModificationTimestamp(m_thirdNote.creationTimestamp());
        m_thirdNote.setNotebookLocalUid(m_secondNotebook.localUid());
        m_thirdNote.setNotebookGuid(m_secondNotebook.guid());
        m_thirdNote.setLocal(false);
        m_thirdNote.setTagLocalUids(QStringList() << m_thirdTag.localUid());
        m_thirdNote.setTagGuids(QStringList() << m_thirdTag.guid());
        m_thirdNote.setFavorited(true);

        m_fourthNote.setGuid(UidGenerator::Generate());
        m_fourthNote.setTitle(QStringLiteral("Fourth note"));

        m_fourthNote.setContent(
            QStringLiteral("<en-note><h1>Fourth note</h1></en-note>"));

        m_fourthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_fourthNote.setModificationTimestamp(m_fourthNote.creationTimestamp());
        m_fourthNote.setNotebookLocalUid(m_secondNotebook.localUid());
        m_fourthNote.setNotebookGuid(m_secondNotebook.guid());
        m_fourthNote.setLocal(false);
        m_fourthNote.setDirty(true);

        m_fourthNote.setTagLocalUids(
            QStringList() << m_secondTag.localUid() << m_thirdTag.localUid());

        m_fourthNote.setTagGuids(
            QStringList() << m_secondTag.guid() << m_thirdTag.guid());

        m_fifthNote.setGuid(UidGenerator::Generate());
        m_fifthNote.setTitle(QStringLiteral("Fifth note"));

        m_fifthNote.setContent(
            QStringLiteral("<en-note><h1>Fifth note</h1></en-note>"));

        m_fifthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_fifthNote.setModificationTimestamp(m_fifthNote.creationTimestamp());
        m_fifthNote.setNotebookLocalUid(m_secondNotebook.localUid());
        m_fifthNote.setNotebookGuid(m_secondNotebook.guid());
        m_fifthNote.setDeletionTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_fifthNote.setLocal(false);
        m_fifthNote.setDirty(true);

        m_sixthNote.setTitle(QStringLiteral("Sixth note"));

        m_sixthNote.setContent(
            QStringLiteral("<en-note><h1>Sixth note</h1></en-note>"));

        m_sixthNote.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
        m_sixthNote.setModificationTimestamp(m_sixthNote.creationTimestamp());
        m_sixthNote.setNotebookLocalUid(m_thirdNotebook.localUid());
        m_sixthNote.setLocal(true);
        m_sixthNote.setDirty(true);
        m_sixthNote.setTagLocalUids(QStringList() << m_fourthTag.localUid());
        m_sixthNote.setFavorited(true);

        m_pLocalStorageManagerAsync->onAddNoteRequest(m_firstNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(m_secondNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(m_thirdNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(m_fourthNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(m_fifthNote, QUuid());
        m_pLocalStorageManagerAsync->onAddNoteRequest(m_sixthNote, QUuid());

        NoteCache noteCache(10);
        NotebookCache notebookCache(3);
        TagCache tagCache(5);
        SavedSearchCache savedSearchCache(5);

        Account account(QStringLiteral("Default user"), Account::Type::Local);

        auto * model = new FavoritesModel(
            account,
            *m_pLocalStorageManagerAsync,
            noteCache,
            notebookCache,
            tagCache,
            savedSearchCache,
            this);

        ModelTest t1(model);
        Q_UNUSED(t1)

        // The favorites model should have items corresponding to favorited
        // notebooks, notes, tags and saved searches and shouldn't have items
        // corresponding to non-favorited notebooks, notes, tags and saved
        // searches
        auto firstNotebookIndex = model->indexForLocalUid(
            m_firstNotebook.localUid());

        if (firstNotebookIndex.isValid()) {
            FAIL("The favorites model unexpectedly contains "
                << "the item corresponding to non-favorited notebook");
        }

        auto secondSavedSearchIndex = model->indexForLocalUid(
            m_secondSavedSearch.localUid());

        if (secondSavedSearchIndex.isValid()) {
            FAIL("The favorites model unexpectedly contains "
                << "the item corresponding to non-favorited saved search");
        }

        auto firstTagIndex = model->indexForLocalUid(m_firstTag.localUid());
        if (firstTagIndex.isValid()) {
            FAIL("The favorites model unexpectedly contains "
                << "the item corresponding to non-favorited tag");
        }

        auto fourthNoteIndex = model->indexForLocalUid(m_fourthNote.localUid());
        if (fourthNoteIndex.isValid()) {
            FAIL("The favorites model unexpectedly contains "
                << "the item corresponding to non-favorited note");
        }

        // The favorites model should have items corresponding to favorited
        // notebooks, notes, tags and saved searches
        auto secondNotebookIndex = model->indexForLocalUid(
            m_secondNotebook.localUid());

        if (!secondNotebookIndex.isValid()) {
            FAIL("The favorites model unexpectedly doesn't contain "
                << "the item corresponding to favorited notebook");
        }

        auto firstSavedSearchIndex = model->indexForLocalUid(
            m_firstSavedSearch.localUid());

        if (!firstSavedSearchIndex.isValid()) {
            FAIL("The favorites model unexpectedly doesn't contain "
                << "the item corresponding to favorited saved search");
        }

        auto thirdTagIndex = model->indexForLocalUid(m_thirdTag.localUid());
        if (!thirdTagIndex.isValid()) {
            FAIL("The favorites model unexpectedly doesn't contain "
                << "the item corresponding to favorited tag");
        }

        auto firstNoteIndex = model->indexForLocalUid(m_firstNote.localUid());
        if (!firstNoteIndex.isValid()) {
            FAIL("The favorites model unexpectedly doesn't contain "
                << "the item corresponding to favorited note");
        }

        // Shouldn't be able to change the type of the item manyally
        secondNotebookIndex = model->index(
            secondNotebookIndex.row(),
            FavoritesModel::Columns::Type);

        if (!secondNotebookIndex.isValid()) {
            FAIL("Can't get the valid favorites model index for type column");
        }

        bool res = model->setData(
            secondNotebookIndex,
            QVariant(FavoritesModelItem::Type::Note),
            Qt::EditRole);

        if (res) {
            FAIL("Was able to change the type of the favorites "
                << "model item which is not intended");
        }

        auto data = model->data(secondNotebookIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the favorites model "
                << "while expected to get the type of the item");
        }

        bool conversionResult = false;
        qint32 type = data.toInt(&conversionResult);
        if (!conversionResult) {
            FAIL("Can't convert the favorites model item's type to int");
        }

        if (static_cast<FavoritesModelItem::Type::type>(type) !=
            FavoritesModelItem::Type::Notebook)
        {
            FAIL("The favorites model item's type should be notebook "
                << "but it is not so after the attempt to change "
                << "the item's type manually");
        }

        // Should not be able to change the number of affected notes manually
        secondNotebookIndex = model->index(
            secondNotebookIndex.row(),
            FavoritesModel::Columns::NumNotesTargeted);

        if (!secondNotebookIndex.isValid()) {
            FAIL("Can't get the valid favorites model index for "
                << "num notes targeted column");
        }

        res = model->setData(secondNotebookIndex, QVariant(9999), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the num targeted notes for "
                << "the favorites model item which is not intended");
        }

        data = model->data(secondNotebookIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the favorites model while expected "
                << "to get the number of notes targeted by the item");
        }

        conversionResult = false;
        qint32 numNotesTargeted = data.toInt(&conversionResult);
        if (!conversionResult) {
            FAIL("Can't convert the favorites model item's num "
                << "targeted notes to int");
        }

        if (numNotesTargeted == 9999) {
            FAIL("The num notes targeted column appears to have "
                << "changed after setData in favorites model even "
                << "though the method returned false");
        }

        // Should be able to change the display name of the item
        secondNotebookIndex = model->index(
            secondNotebookIndex.row(),
            FavoritesModel::Columns::DisplayName);

        if (!secondNotebookIndex.isValid()) {
            FAIL("Can't get the valid favorites model index for "
                << "display name column");
        }

        res = model->setData(
            secondNotebookIndex,
            m_secondNotebook.name() + QStringLiteral("_modified"),
            Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        data = model->data(secondNotebookIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the favorites model "
                << "while expected to get the display name of the item");
        }

        if (data.toString() !=
            (m_secondNotebook.name() + QStringLiteral("_modified")))
        {
            FAIL("The name of the item appears to have not changed "
                << "after setData in favorites model even though "
                << "the method returned true");
        }

        // Favoriting some previously non-favorited item should make it appear
        // in the favorites model
        m_firstNotebook.setFavorited(true);

        m_pLocalStorageManagerAsync->onUpdateNotebookRequest(
            m_firstNotebook,
            QUuid());

        firstNotebookIndex = model->indexForLocalUid(
            m_firstNotebook.localUid());

        if (!firstNotebookIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to just favorited notebook");
        }

        m_secondSavedSearch.setFavorited(true);

        m_pLocalStorageManagerAsync->onUpdateSavedSearchRequest(
            m_secondSavedSearch,
            QUuid());

        secondSavedSearchIndex = model->indexForLocalUid(
            m_secondSavedSearch.localUid());

        if (!secondSavedSearchIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to just favorited saved search");
        }

        m_firstTag.setFavorited(true);
        m_pLocalStorageManagerAsync->onUpdateTagRequest(m_firstTag, QUuid());

        firstTagIndex = model->indexForLocalUid(m_firstTag.localUid());
        if (!firstTagIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to just favorited tag");
        }

        m_fourthNote.setFavorited(true);

        m_pLocalStorageManagerAsync->onUpdateNoteRequest(
            m_fourthNote,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            LocalStorageManager::UpdateNoteOptions(),
#else
            LocalStorageManager::UpdateNoteOptions(0),
#endif
            QUuid());

        fourthNoteIndex = model->indexForLocalUid(m_fourthNote.localUid());
        if (!fourthNoteIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to just favorited note");
        }

        // After expunging the parent tag its child tags should should not be
        // present within the model
        m_pLocalStorageManagerAsync->onExpungeTagRequest(m_secondTag, QUuid());

        thirdTagIndex = model->indexForLocalUid(m_thirdTag.localUid());
        if (thirdTagIndex.isValid()) {
            FAIL("Got valid model index for the tag item which "
                << "should have been removed from the favorites model "
                << "as its parent tag was expunged");
        }

        // After the tag's promotion expunging the tag previously being the parent
        // should not trigger the removal of its ex-child
        m_pLocalStorageManagerAsync->onAddTagRequest(m_secondTag, QUuid());
        m_pLocalStorageManagerAsync->onAddTagRequest(m_thirdTag, QUuid());

        m_thirdTag.setParentGuid(QString());
        m_thirdTag.setParentLocalUid(QString());

        m_pLocalStorageManagerAsync->onUpdateTagRequest(m_thirdTag, QUuid());

        thirdTagIndex = model->indexForLocalUid(m_thirdTag.localUid());
        if (!thirdTagIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to just added and updated tag");
        }

        m_pLocalStorageManagerAsync->onExpungeTagRequest(m_secondTag, QUuid());

        thirdTagIndex = model->indexForLocalUid(m_thirdTag.localUid());
        if (!thirdTagIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to a tag which has "
                << "previously been a child of now expunged tag");
        }

        // Unfavoriting the previously favorited item should make it disappear
        // from the favorites model
        auto thirdNotebookIndex = model->indexForLocalUid(
            m_thirdNotebook.localUid());

        if (!thirdNotebookIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites model item "
                << "corresponding to the favorited notebook");
        }

        m_thirdNotebook.setFavorited(false);

        m_pLocalStorageManagerAsync->onUpdateNotebookRequest(
            m_thirdNotebook,
            QUuid());

        thirdNotebookIndex = model->indexForLocalUid(
            m_thirdNotebook.localUid());

        if (thirdNotebookIndex.isValid()) {
            FAIL("Was able to get the valid model index for the favorites "
                << "model item corresponding to the notebook which has just "
                << "been unfavorited");
        }

        auto thirdSavedSearchIndex = model->indexForLocalUid(
            m_thirdSavedSearch.localUid());

        if (!thirdSavedSearchIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited saved search");
        }

        m_thirdSavedSearch.setFavorited(false);

        m_pLocalStorageManagerAsync->onUpdateSavedSearchRequest(
            m_thirdSavedSearch,
            QUuid());

        thirdSavedSearchIndex = model->indexForLocalUid(
            m_thirdSavedSearch.localUid());

        if (thirdSavedSearchIndex.isValid()) {
            FAIL("Was able to get the valid model index for "
                << "the favorites model item corresponding to "
                << "the saved search which has just been unfavorited");
        }

        thirdTagIndex = model->indexForLocalUid(m_thirdTag.localUid());
        if (!thirdTagIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited tag");
        }

        m_thirdTag.setFavorited(false);
        m_pLocalStorageManagerAsync->onUpdateTagRequest(m_thirdTag, QUuid());

        thirdTagIndex = model->indexForLocalUid(m_thirdTag.localUid());
        if (thirdTagIndex.isValid()) {
            FAIL("Was able to get the valid model index for "
                << "the favorites model item corresponding to "
                << "the tag which has just been unfavorited");
        }

        auto thirdNoteIndex = model->indexForLocalUid(
            m_thirdNote.localUid());

        if (!thirdNoteIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited note");
        }

        m_thirdNote.setFavorited(false);

        m_pLocalStorageManagerAsync->onUpdateNoteRequest(
            m_thirdNote,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            LocalStorageManager::UpdateNoteOptions(),
#else
            LocalStorageManager::UpdateNoteOptions(0),
#endif
            QUuid());

        thirdNoteIndex = model->indexForLocalUid(m_thirdNote.localUid());
        if (thirdNoteIndex.isValid()) {
            FAIL("Was able to get the valid model index for "
                << "the favorites model item corresponding to "
                << "the note which has just been unfavorited");
        }

        // The removal of items from the favorites model should cause
        // the updates of corresponding data elements in the local storage

        // First restore the favorited status of some items unfavorited just
        // above
        m_thirdNotebook.setFavorited(true);

        m_pLocalStorageManagerAsync->onUpdateNotebookRequest(
            m_thirdNotebook,
            QUuid());

        m_thirdSavedSearch.setFavorited(true);

        m_pLocalStorageManagerAsync->onUpdateSavedSearchRequest(
            m_thirdSavedSearch,
            QUuid());

        m_thirdTag.setFavorited(true);
        m_pLocalStorageManagerAsync->onUpdateTagRequest(m_thirdTag, QUuid());

        m_thirdNote.setFavorited(true);

        m_pLocalStorageManagerAsync->onUpdateNoteRequest(
            m_thirdNote,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            LocalStorageManager::UpdateNoteOptions(),
#else
            LocalStorageManager::UpdateNoteOptions(0),
#endif
            QUuid());

        thirdNotebookIndex = model->indexForLocalUid(
            m_thirdNotebook.localUid());

        if (!thirdNotebookIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited notebook");
        }

        m_expectingNotebookUnfavoriteFromLocalStorage = true;
        res = model->removeRow(thirdNotebookIndex.row(), QModelIndex());
        if (!res) {
            FAIL("Can't remove row from the favorites model "
                << "corresponding to the favorited notebook");
        }

        if (m_thirdNotebook.isFavorited()) {
            FAIL("The notebook which should have been unfavorited "
                << "after the removal of corresponding item "
                << "from the favorites model is still favorited");
        }

        thirdSavedSearchIndex = model->indexForLocalUid(
            m_thirdSavedSearch.localUid());

        if (!thirdSavedSearchIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited saved search");
        }

        m_expectingSavedSearchUnfavoriteFromLocalStorage = true;
        res = model->removeRow(thirdSavedSearchIndex.row(), QModelIndex());
        if (!res) {
            FAIL("Can't remove row from the favorites model "
                << "corresponding to the favorited saved search");
        }

        if (m_thirdSavedSearch.isFavorited()) {
            FAIL("The saved search which should have been unfavorited "
                << "after the removal of corresponding item "
                << "from the favorites model is still favorited");
        }

        thirdTagIndex = model->indexForLocalUid(m_thirdTag.localUid());
        if (!thirdTagIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited tag");
        }

        m_expectingTagUnfavoriteFromLocalStorage = true;
        res = model->removeRow(thirdTagIndex.row(), QModelIndex());
        if (!res) {
            FAIL("Can't remove row from the favorites model "
                << "corresponding to the favorited tag");
        }

        if (m_thirdTag.isFavorited()) {
            FAIL("The tag which should have been unfavorited after "
                << "the removal of corresponding item from "
                << "the favorites model is still favorited");
        }

        thirdNoteIndex = model->indexForLocalUid(m_thirdNote.localUid());
        if (!thirdNoteIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited note");
        }

        m_expectingNoteUnfavoriteFromLocalStorage = true;
        res = model->removeRow(thirdNoteIndex.row(), QModelIndex());
        if (!res) {
            FAIL("Can't remove row from the favorites model "
                << "corresponding to the favorited note");
        }

        if (m_thirdNote.isFavorited()) {
            FAIL("The note which should have been unfavorited after "
                << "the removal of corresponding item from "
                << "the favorites model is still favorited");
        }

        // Check sorting
        QVector<FavoritesModel::Columns::type> columns;
        columns.reserve(model->columnCount());

        columns << FavoritesModel::Columns::Type
                << FavoritesModel::Columns::DisplayName
                << FavoritesModel::Columns::NumNotesTargeted;

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

        // Ensure the modifications of favorites model items propagate properly
        // to the local storage
        m_expectingTagUpdateFromLocalStorage = true;
        m_expectingNotebookUpdateFromLocalStorage = true;
        m_expectingSavedSearchUpdateFromLocalStorage = true;
        m_expectingNoteUpdateFromLocalStorage = true;

        secondNotebookIndex = model->indexForLocalUid(
            m_secondNotebook.localUid());

        if (!secondNotebookIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited notebook");
        }

        secondNotebookIndex = model->index(
            secondNotebookIndex.row(),
            FavoritesModel::Columns::DisplayName);

        if (!secondNotebookIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the display name column");
        }

        res = model->setData(
            secondNotebookIndex,
            QStringLiteral("Manual notebook name"),
            Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        firstSavedSearchIndex = model->indexForLocalUid(
            m_firstSavedSearch.localUid());

        if (!firstSavedSearchIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited saved search");
        }

        firstSavedSearchIndex = model->index(
            firstSavedSearchIndex.row(),
            FavoritesModel::Columns::DisplayName);

        if (!firstSavedSearchIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the display name column");
        }

        res = model->setData(
            firstSavedSearchIndex,
            QStringLiteral("Manual saved search name"),
            Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        auto fourthTagIndex = model->indexForLocalUid(m_fourthTag.localUid());
        if (!fourthTagIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited tag");
        }

        fourthTagIndex = model->index(
            fourthTagIndex.row(),
            FavoritesModel::Columns::DisplayName);

        if (!fourthTagIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the display name column");
        }

        res = model->setData(
            fourthTagIndex,
            QStringLiteral("Manual tag name"),
            Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        firstNoteIndex = model->indexForLocalUid(m_firstNote.localUid());
        if (!firstNoteIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the favorited note");
        }

        firstNoteIndex = model->index(
            firstNoteIndex.row(),
            FavoritesModel::Columns::DisplayName);

        if (!firstNoteIndex.isValid()) {
            FAIL("Can't get the valid model index for the favorites "
                << "model item corresponding to the display name column");
        }

        res = model->setData(
            firstNoteIndex,
            QStringLiteral("Manual note name"),
            Qt::EditRole);

        if (!res) {
            FAIL("Can't change the display name of the favorites model item");
        }

        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void FavoritesModelTestHelper::onUpdateNoteComplete(
    Note note, LocalStorageManager::UpdateNoteOptions options, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateNoteComplete: note = "
            << note << "\nUpdate resource metadata = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                ? "true"
                : "false")
            << ", update resource binary data = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceBinaryData)
                ? "true"
                : "false")
            << ", update tags = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                ? "true"
                : "false")
            << ", request id = " << requestId);

    if (m_expectingNoteUpdateFromLocalStorage)
    {
        if (note.title() != QStringLiteral("Manual note name"))
        {
            QString error = QStringLiteral(
                "The title of the note updated in the local storage doesn't "
                "match the expected one");

            notifyFailureWithStackTrace(ErrorString(error));
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
    else if (m_expectingNoteUnfavoriteFromLocalStorage)
    {
        if (note.isFavorited())
        {
            QString error = QStringLiteral(
                "The note which should have been unfavorited in the local "
                "storage is still favorited");

            notifyFailureWithStackTrace(ErrorString(error));
            return;
        }

        m_thirdNote = note;
        m_expectingNoteUnfavoriteFromLocalStorage = false;
    }
}

void FavoritesModelTestHelper::onUpdateNoteFailed(
    Note note, LocalStorageManager::UpdateNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateNoteFailed: note = "
            << note << "\nUpdate resource metadata = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                ? "true"
                : "false")
            << ", update resource binary data = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceBinaryData)
                ? "true"
                : "false")
            << ", update tags = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                ? "true"
                : "false")
            << ", error description: " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onFindNoteFailed(
    Note note, LocalStorageManager::GetNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onFindNoteFailed: note = "
            << note << "\nWith resource metadata = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
            << ", with resource binary data = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onListNotesFailed(
    LocalStorageManager::ListObjectsOptions flag,
    LocalStorageManager::GetNoteOptions options,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onListNotesFailed: flag = "
            << flag << ", with resource metadata = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
            << ", with resource binary data = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
            << ", limit = " << limit
            << ", offset = " << offset
            << ", order = " << order
            << ", direction = " << orderDirection
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onUpdateNotebookComplete(
    Notebook notebook, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateNotebookComplete: "
            << "notebook = " << notebook
            << ", request id = " << requestId);

    if (m_expectingNotebookUpdateFromLocalStorage)
    {
        if (notebook.name() != QStringLiteral("Manual notebook name"))
        {
            QString error = QStringLiteral(
                "The name of the notebook updated in the local storage doesn't "
                "match the expected one");

            notifyFailureWithStackTrace(ErrorString(error));
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
    else if (m_expectingNotebookUnfavoriteFromLocalStorage)
    {
        if (notebook.isFavorited())
        {
            QString error = QStringLiteral(
                "The notebook which should have been unfavorited in the local "
                "storage is still favorited");

            notifyFailureWithStackTrace(ErrorString(error));
            return;
        }

        m_thirdNotebook = notebook;
        m_expectingNotebookUnfavoriteFromLocalStorage = false;
    }
}

void FavoritesModelTestHelper::onUpdateNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateNotebookFailed: "
            << "notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onFindNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onFindNotebookFailed: notebook = "
            << notebook << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onListNotebooksFailed(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onListNotebooksFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull()
                ? QStringLiteral("<null>")
                : linkedNotebookGuid)
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateTagComplete: tag = "
            << tag << ", request id = " << requestId);

    if (m_expectingTagUpdateFromLocalStorage)
    {
        if (tag.name() != QStringLiteral("Manual tag name"))
        {
            QString error = QStringLiteral(
                "The name of the tag updated in the local storage doesn't "
                "match the expected one");

            notifyFailureWithStackTrace(ErrorString(error));
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
    else if (m_expectingTagUnfavoriteFromLocalStorage)
    {
        if (tag.isFavorited())
        {
            QString error = QStringLiteral(
                "The tag which should have been unfavorited in the local "
                "storage is still favorited");

            notifyFailureWithStackTrace(ErrorString(error));
            return;
        }

        m_thirdTag = tag;
        m_expectingTagUnfavoriteFromLocalStorage = false;
    }
}

void FavoritesModelTestHelper::onUpdateTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateTagFailed: tag = "
            << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onFindTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onFindTagFailed: tag = "
            << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onListTagsFailed(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListTagsOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid,
    ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onListTagsFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid: is null = "
            << (linkedNotebookGuid.isNull()
                ? "true"
                : "false")
            << ", is empty = "
            << (linkedNotebookGuid.isEmpty()
                ? "true"
                : "false")
            << ", value = " << linkedNotebookGuid
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onUpdateSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateSavedSearchComplete: "
            << "search = " << search << ", request id = " << requestId);

    if (m_expectingSavedSearchUpdateFromLocalStorage)
    {
        if (search.name() != QStringLiteral("Manual saved search name"))
        {
            QString error = QStringLiteral(
                "The name of the saved search updated in the local storage "
                "doesn't match the expected one");

            notifyFailureWithStackTrace(ErrorString(error));
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
    else if (m_expectingSavedSearchUnfavoriteFromLocalStorage)
    {
        if (search.isFavorited())
        {
            QString error = QStringLiteral(
                "The saved search which should have been unfavorited in "
                "the local storage is still favorited");

            notifyFailureWithStackTrace(ErrorString(error));
            return;
        }

        m_thirdSavedSearch = search;
        m_expectingSavedSearchUnfavoriteFromLocalStorage = false;
    }
}

void FavoritesModelTestHelper::onUpdateSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onUpdateSavedSearchFailed: "
            << "search = " << search << ", error description = "
            << errorDescription << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onFindSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onFindSavedSearchFailed: search = "
            << search << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::onListSavedSearchesFailed(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListSavedSearchesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::onListSavedSearchesFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void FavoritesModelTestHelper::checkSorting(const FavoritesModel & model)
{
    QNDEBUG(
        "tests:model_test:favorites",
        "FavoritesModelTestHelper::checkSorting");

    int numRows = model.rowCount();

    QVector<FavoritesModelItem> items;
    items.reserve(numRows);
    for(int i = 0; i < numRows; ++i)
    {
        const auto * pItem = model.itemAtRow(i);
        if (Q_UNLIKELY(!pItem)) {
            FAIL("Unexpected null pointer to the favorites model item");
        }

        items << *pItem;
    }

    bool ascending = (model.sortOrder() == Qt::AscendingOrder);
    switch(model.sortingColumn())
    {
    case FavoritesModel::Columns::Type:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByType());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByType());
            }
            break;
        }
    case FavoritesModel::Columns::DisplayName:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByDisplayName());
            }
            else {
                std::sort(items.begin(), items.end(), GreaterByDisplayName());
            }
            break;
        }
    case FavoritesModel::Columns::NumNotesTargeted:
        {
            if (ascending) {
                std::sort(items.begin(), items.end(), LessByNumNotesTargeted());
            }
            else {
                std::sort(
                    items.begin(),
                    items.end(),
                    GreaterByNumNotesTargeted());
            }
            break;
        }
    }

    for(int i = 0; i < numRows; ++i)
    {
        const auto * pItem = model.itemAtRow(i);
        if (Q_UNLIKELY(!pItem)) {
            FAIL("Unexpected null pointer to the favorites model item");
        }

        if (pItem->localUid() != items[i].localUid()) {
            FAIL("Found mismatched favorites model items when checking "
                << "the sorting");
        }
    }
}

void FavoritesModelTestHelper::notifyFailureWithStackTrace(
    ErrorString errorDescription)
{
    SysInfo sysInfo;

    errorDescription.details() +=
        QStringLiteral("\nStack trace: ") + sysInfo.stackTrace();

    Q_EMIT failure(errorDescription);
}

bool FavoritesModelTestHelper::LessByType::operator()(
    const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const
{
    return lhs.type() < rhs.type();
}

bool FavoritesModelTestHelper::GreaterByType::operator()(
    const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const
{
    return lhs.type() > rhs.type();
}

bool FavoritesModelTestHelper::LessByDisplayName::operator()(
    const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const
{
    return lhs.displayName().localeAwareCompare(rhs.displayName()) < 0;
}

bool FavoritesModelTestHelper::GreaterByDisplayName::operator()(
    const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const
{
    return lhs.displayName().localeAwareCompare(rhs.displayName()) > 0;
}

bool FavoritesModelTestHelper::LessByNumNotesTargeted::operator()(
    const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const
{
    return lhs.numNotesTargeted() < rhs.numNotesTargeted();
}

bool FavoritesModelTestHelper::GreaterByNumNotesTargeted::operator()(
    const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const
{
    return lhs.numNotesTargeted() > rhs.numNotesTargeted();
}

} // namespace quentier
