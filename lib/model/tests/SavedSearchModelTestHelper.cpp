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

#include "SavedSearchModelTestHelper.h"

#include "modeltest.h"
#include "TestMacros.h"

#include <lib/model/SavedSearchModel.h>

#include <quentier/exception/IQuentierException.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/SysInfo.h>

namespace quentier {

SavedSearchModelTestHelper::SavedSearchModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerAsync(pLocalStorageManagerAsync)
{
    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::addSavedSearchFailed,
        this,
        &SavedSearchModelTestHelper::onAddSavedSearchFailed);

    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::updateSavedSearchFailed,
        this,
        &SavedSearchModelTestHelper::onUpdateSavedSearchFailed);

    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::findSavedSearchFailed,
        this,
        &SavedSearchModelTestHelper::onFindSavedSearchFailed);

    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::listSavedSearchesFailed,
        this,
        &SavedSearchModelTestHelper::onListSavedSearchesFailed);

    QObject::connect(
        pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::expungeSavedSearchFailed,
        this,
        &SavedSearchModelTestHelper::onExpungeSavedSearchFailed);
}

SavedSearchModelTestHelper::~SavedSearchModelTestHelper() = default;

void SavedSearchModelTestHelper::test()
{
    QNDEBUG(
        "tests:model_test:saved_search",
        "SavedSearchModelTestHelper::test");

    ErrorString errorDescription;

    try {
        SavedSearch first;
        first.setName(QStringLiteral("First search"));
        first.setQuery(QStringLiteral("First search query"));
        first.setLocal(true);
        first.setDirty(true);

        SavedSearch second;
        second.setGuid(UidGenerator::Generate());
        second.setName(QStringLiteral("Second search"));
        second.setQuery(QStringLiteral("Second search query"));
        second.setLocal(true);
        second.setDirty(false);

        SavedSearch third;
        third.setName(QStringLiteral("Third search"));
        third.setQuery(QStringLiteral("Third search query"));
        third.setLocal(false);
        third.setDirty(true);

        SavedSearch fourth;
        fourth.setName(QStringLiteral("Fourth search"));
        fourth.setQuery(QStringLiteral("Fourth search query"));
        fourth.setLocal(false);
        fourth.setDirty(false);

        // NOTE: exploiting the direct connection used in the current test
        // environment: after the following lines the local storage would be
        // filled with the test objects
        m_pLocalStorageManagerAsync->onAddSavedSearchRequest(first, QUuid());
        m_pLocalStorageManagerAsync->onAddSavedSearchRequest(second, QUuid());
        m_pLocalStorageManagerAsync->onAddSavedSearchRequest(third, QUuid());
        m_pLocalStorageManagerAsync->onAddSavedSearchRequest(fourth, QUuid());

        SavedSearchCache cache(20);
        Account account(QStringLiteral("Default user"), Account::Type::Local);

        auto * model = new SavedSearchModel(
            account,
            *m_pLocalStorageManagerAsync,
            cache,
            this);

        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        auto secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid saved search item model "
                << "index for local uid");
        }

        secondIndex = model->index(
            secondIndex.row(),
            SavedSearchModel::Columns::Dirty);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid saved search item model "
                << "index for dirty column");
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the dirty flag in saved "
                << "search model manually which is not intended");
        }

        auto data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the saved search model "
                << "while expected to get the state of dirty flag");
        }

        if (data.toBool()) {
            FAIL("The dirty state appears to have changed after "
                << "setData in saved search model even though "
                << "the method returned false");
        }

        // Should only be able to make the non-synchronizable (local) item
        // synchronizable (non-local) with non-local account
        // 1) Trying with local account
        secondIndex = model->index(
            secondIndex.row(),
            SavedSearchModel::Columns::Synchronizable);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid saved search item model "
                << "index for synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the synchronizable flag "
                << "from false to true for saved search model item "
                << "with local account");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the saved search model "
                << "while expected to get the state of synchronizable flag");
        }

        if (data.toBool()) {
            FAIL("Even though setData returned false on attempt "
                << "to make the saved search item synchronizable "
                << "with the local account, the actual data within "
                << "the model appears to have changed");
        }

        // 2) Trying the non-local account
        account = Account(
            QStringLiteral("Evernote user"),
            Account::Type::Evernote,
            qevercloud::UserID(1));

        model->updateAccount(account);

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL("Wasn't able to change the synchronizable flag "
                << "from false to true for saved search model item "
                << "even with the account of Evernote type");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the saved search model "
                << "while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have not "
                << "changed after setData in saved search model "
                << "even though the method returned true");
        }

        // Verify the dirty flag has changed as a result of making the item
        // synchronizable
        secondIndex = model->index(
            secondIndex.row(),
            SavedSearchModel::Columns::Dirty);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid saved search item model "
                << "index for dirty column");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the saved search "
                << "model while expected to get the state of dirty flag");
        }

        if (!data.toBool()) {
            FAIL("The dirty state hasn't changed after making "
                << "the saved search model item synchronizable "
                << "while it was expected to have changed");
        }

        // Should not be able to make the synchronizable (non-local) item
        // non-synchronizable (local)
        secondIndex = model->index(
            secondIndex.row(),
            SavedSearchModel::Columns::Synchronizable);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid saved search item model "
                << "index for synchronizable column");
        }

        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL("Was able to change the synchronizable flag in "
                << "saved search model from true to false which "
                << "is not intended");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the saved search model "
                << "while expected to get the state of synchronizable flag");
        }

        if (!data.toBool()) {
            FAIL("The synchronizable state appears to have changed "
                << "after setData in saved search model even though "
                << "the method returned false");
        }

        // Should be able to change name and query columns
        secondIndex = model->index(
            secondIndex.row(),
            SavedSearchModel::Columns::Name);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid saved search item model "
                << "index for name column");
        }

        QString newName = QStringLiteral("second (name modified)");
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            FAIL("Can't change the name of the saved search model item");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the saved search model "
                << "while expected to get the name of the saved search item");
        }

        if (data.toString() != newName) {
            FAIL("The name of the saved search item returned by "
                << "the model does not match the name just set to "
                << "this item: received " << data.toString()
                << ", expected " << newName);
        }

        secondIndex = model->index(
            secondIndex.row(),
            SavedSearchModel::Columns::Query);

        if (!secondIndex.isValid()) {
            FAIL("Can't get the valid saved search item model "
                << "index for query column");
        }

        QString newQuery = QStringLiteral("second query (modified)");
        res = model->setData(secondIndex, QVariant(newQuery), Qt::EditRole);
        if (!res) {
            FAIL("Can't change the query of the saved search "
                << "model item");
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL("Null data was returned by the saved search "
                << "model while expected to get the query of "
                << "the saved search item");
        }

        if (data.toString() != newQuery) {
            FAIL("The query of the saved search item returned "
                << "by the model does not match the query just set "
                << "to this item: received " << data.toString()
                << ", expected " << newQuery);
        }

        // Should not be able to remove the row with a saved search with
        // non-empty guid
        res = model->removeRow(secondIndex.row(), QModelIndex());
        if (res) {
            FAIL("Was able to remove the row with a saved search "
                << "with non-empty guid which is not intended");
        }

        auto secondIndexAfterFailedRemoval = model->indexForLocalUid(
            second.localUid());

        if (!secondIndexAfterFailedRemoval.isValid()) {
            FAIL("Can't get the valid saved search item model "
                << "index after the failed row removal attempt");
        }

        if (secondIndexAfterFailedRemoval.row() != secondIndex.row()) {
            FAIL("Saved search model returned item index with a different row "
                << "after the failed row removal attempt");
        }

        // Should be able to remove the row with a (local) saved search having
        // empty guid
        auto firstIndex = model->indexForLocalUid(first.localUid());
        if (!firstIndex.isValid()) {
            FAIL("Can't get the valid saved search item model index for local "
                << "uid");
        }

        res = model->removeRow(firstIndex.row(), QModelIndex());
        if (!res) {
            FAIL("Can't remove the row with a saved search item with empty "
                << "guid from the model");
        }

        auto firstIndexAfterRemoval = model->indexForLocalUid(first.localUid());
        if (firstIndexAfterRemoval.isValid()) {
            FAIL("Was able to get the valid model index for "
                << "the removed saved search item by local uid "
                << "which is not intended");
        }

        // Set the account to local again to test accounting for saved search
        // name reservation in create/remove/create cycles
        account = Account(QStringLiteral("Local user"), Account::Type::Local);
        model->updateAccount(account);

        // Should not be able to create the saved search with existing name
        ErrorString errorDescription;

        auto fifthSavedSearchIndex = model->createSavedSearch(
            third.name(),
            QStringLiteral("My search"),
            errorDescription);

        if (fifthSavedSearchIndex.isValid()) {
            FAIL("Was able to create saved search with the same "
                << "name as the already existing one");
        }

        // The error description should say something about the inability to
        // create the saved search
        if (errorDescription.isEmpty()) {
            FAIL("The error description about the inability to "
                << "create the saved search due to name collision is empty");
        }

        // Should be able to create the saved search with a new name
        QString fifthSavedSearchName = QStringLiteral("Fifth");
        errorDescription.clear();

        fifthSavedSearchIndex = model->createSavedSearch(
            fifthSavedSearchName,
            QStringLiteral("My search"),
            errorDescription);

        if (!fifthSavedSearchIndex.isValid()) {
            FAIL("Wasn't able to create a saved search with "
                << "the name not present within the saved search model");
        }

        // Should no longer be able to create the saved search with the same
        // name as the just added one
        auto sixthSavedSearchIndex = model->createSavedSearch(
            fifthSavedSearchName,
            QStringLiteral("My search"),
            errorDescription);

        if (sixthSavedSearchIndex.isValid()) {
            FAIL("Was able to create a saved search with "
                << "the same name as just created one");
        }

        // The error description should say something about the inability to
        // create the saved search
        if (errorDescription.isEmpty()) {
            FAIL("The error description about the inability to "
                << "create the saved search due to name collision is empty");
        }

        // Should be able to remove the just added local saved search
        res = model->removeRow(
            fifthSavedSearchIndex.row(),
            fifthSavedSearchIndex.parent());

        if (!res) {
            FAIL("Wasn't able to remove the saved search without "
                << "guid just added to the saved search model");
        }

        // Should again be able to create the saved search with the same name
        errorDescription.clear();

        fifthSavedSearchIndex = model->createSavedSearch(
            fifthSavedSearchName,
            QStringLiteral("My search"),
            errorDescription);

        if (!fifthSavedSearchIndex.isValid()) {
            FAIL("Wasn't able to create the saved search with "
                << "the same name as the just removed one");
        }

        // Check the sorting for saved search items: by default should sort
        // by name in ascending order
        res = checkSorting(*model);
        if (!res) {
            FAIL("Sorting check failed for the saved search model "
                << "for ascending order");
        }

        // Change the sort order and check the sorting again
        model->sort(SavedSearchModel::Columns::Name, Qt::DescendingOrder);

        res = checkSorting(*model);
        if (!res) {
            FAIL("Sorting check failed for the saved search model "
                << "for descending order");
        }

        Q_EMIT success();
        return;
    }
    CATCH_EXCEPTION()

    Q_EMIT failure(errorDescription);
}

void SavedSearchModelTestHelper::onAddSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:saved_search",
        "SavedSearchModelTestHelper::onAddSavedSearchFailed: "
            << "search = " << search << ", error description = "
            << errorDescription << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void SavedSearchModelTestHelper::onUpdateSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:saved_search",
        "SavedSearchModelTestHelper::onUpdateSavedSearchFailed: search = "
            << search << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void SavedSearchModelTestHelper::onFindSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:saved_search",
        "SavedSearchModelTestHelper::onFindSavedSearchFailed: search = "
            << search << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void SavedSearchModelTestHelper::onListSavedSearchesFailed(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListSavedSearchesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:saved_search",
        "SavedSearchModelTestHelper::onListSavedSearchesFailed: "
            << "flag = " << flag << ", limit = "
            << limit << ", offset = " << offset
            << ", order = " << order
            << ", direction = " << orderDirection
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

void SavedSearchModelTestHelper::onExpungeSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(
        "tests:model_test:saved_search",
        "SavedSearchModelTestHelper::onExpungeSavedSearchFailed: "
            << "search = " << search
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    notifyFailureWithStackTrace(errorDescription);
}

bool SavedSearchModelTestHelper::checkSorting(
    const SavedSearchModel & model) const
{
    int numRows = model.rowCount(QModelIndex());

    QStringList names;
    names.reserve(numRows);
    for(int i = 0; i < numRows; ++i) {
        auto index = model.index(i, SavedSearchModel::Columns::Name);
        QString name = model.data(index, Qt::EditRole).toString();
        names << name;
    }

    QStringList sortedNames = names;
    if (model.sortOrder() == Qt::AscendingOrder) {
        std::sort(sortedNames.begin(), sortedNames.end(), LessByName());
    }
    else {
        std::sort(sortedNames.begin(), sortedNames.end(), GreaterByName());
    }

    return (names == sortedNames);
}

void SavedSearchModelTestHelper::notifyFailureWithStackTrace(
    ErrorString errorDescription)
{
    SysInfo sysInfo;

    errorDescription.details() += QStringLiteral("\nStack trace: ") +
        sysInfo.stackTrace();

    Q_EMIT failure(errorDescription);
}

bool SavedSearchModelTestHelper::LessByName::operator()(
    const QString & lhs, const QString & rhs) const
{
    return (lhs.toUpper().localeAwareCompare(rhs.toUpper()) <= 0);
}

bool SavedSearchModelTestHelper::GreaterByName::operator()(
    const QString & lhs, const QString & rhs) const
{
    return (lhs.toUpper().localeAwareCompare(rhs.toUpper()) > 0);
}

} // namespace quentier
