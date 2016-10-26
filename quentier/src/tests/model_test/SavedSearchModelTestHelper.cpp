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

#include "SavedSearchModelTestHelper.h"
#include "../../models/SavedSearchModel.h"
#include "modeltest.h"
#include "Macros.h"
#include <quentier/utility/SysInfo.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/exception/IQuentierException.h>

namespace quentier {

SavedSearchModelTestHelper::SavedSearchModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                                       QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerThreadWorker(pLocalStorageManagerThreadWorker)
{
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onAddSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onUpdateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onFindSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,
                                                                LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onListSavedSearchesFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                                  QNLocalizedString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(SavedSearchModelTestHelper,onExpungeSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));
}

void SavedSearchModelTestHelper::test()
{
    QNDEBUG(QStringLiteral("SavedSearchModelTestHelper::test"));

    try {
        SavedSearch first;
        first.setName(QStringLiteral("First search"));
        first.setQuery(QStringLiteral("First search query"));
        first.setLocal(true);
        first.setDirty(true);

        SavedSearch second;
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

        // NOTE: exploiting the direct connection used in the current test environment:
        // after the following lines the local storage would be filled with the test objects
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(first, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(second, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(third, QUuid());
        m_pLocalStorageManagerThreadWorker->onAddSavedSearchRequest(fourth, QUuid());

        SavedSearchCache cache(20);
        Account account(QStringLiteral("Default user"), Account::Type::Local);

        SavedSearchModel * model = new SavedSearchModel(account, *m_pLocalStorageManagerThreadWorker, cache, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index for local uid"));
        }

        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Dirty, QModelIndex());
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index for dirty column"));
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL(QStringLiteral("Was able to change the dirty flag in saved search model manually which is not intended"));
        }

        QVariant data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the saved search model while expected to get the state of dirty flag"));
        }

        if (data.toBool()) {
            FAIL(QStringLiteral("The dirty state appears to have changed after setData in saved search model even though the method returned false"));
        }

        // Should be able to make the non-synchronizable (local) item synchronizable (non-local)
        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Synchronizable, QModelIndex());
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index for synchronizable column"));
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't change the synchronizable flag from false to true for saved search model"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the saved search model while expected to get the state of synchronizable flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The synchronizable state appears to have not changed after setData in saved search model even though the method returned true"));
        }

        // Verify the dirty flag has changed as a result of makind the item synchronizable
        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Dirty, QModelIndex());
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index for dirty column"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the saved search model while expected to get the state of dirty flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The dirty state hasn't changed after making the saved search model item synchronizable while it was expected to have changed"));
        }

        // Should not be able to make the synchronizable (non-local) item non-synchronizable (local)
        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Synchronizable, QModelIndex());
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index for synchronizable column"));
        }

        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL(QStringLiteral("Was able to change the synchronizable flag in saved search model from true to false which is not intended"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the saved search model while expected to get the state of synchronizable flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The synchronizable state appears to have changed after setData in saved search model even though the method returned false"));
        }

        // Should be able to change name and query columns
        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Name);
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index for name column"));
        }

        QString newName = QStringLiteral("second (name modified)");
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't change the name of the saved search model item"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the saved search model while expected to get the name of the saved search item"));
        }

        if (data.toString() != newName) {
            FAIL(QStringLiteral("The name of the saved search item returned by the model does not match the name just set to this item: received ")
                 << data.toString() << QStringLiteral(", expected ") << newName);
        }

        secondIndex = model->index(secondIndex.row(), SavedSearchModel::Columns::Query, QModelIndex());
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index for query column"));
        }

        QString newQuery = QStringLiteral("second query (modified)");
        res = model->setData(secondIndex, QVariant(newQuery), Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't change the query of the saved search model item"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the saved search model while expected to get the query of the saved search item"));
        }

        if (data.toString() != newQuery) {
            FAIL(QStringLiteral("The query of the saved search item returned by the model does not match the query just set to this item: received ")
                 << data.toString() << QStringLiteral(", expected ") << newQuery);
        }

        // Should not be able to remove the row with a synchronizable (non-local) saved search
        res = model->removeRow(secondIndex.row(), QModelIndex());
        if (res) {
            FAIL(QStringLiteral("Was able to remove the row with a synchronizable saved search which is not intended"));
        }

        QModelIndex secondIndexAfterFailedRemoval = model->indexForLocalUid(second.localUid());
        if (!secondIndexAfterFailedRemoval.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index after the failed row removal attempt"));
        }

        if (secondIndexAfterFailedRemoval.row() != secondIndex.row()) {
            FAIL(QStringLiteral("Saved search model returned item index with a different row after the failed row removal attempt"));
        }

        // Should be able to remove the row with a non-synchronizable (local) saved search
        QModelIndex firstIndex = model->indexForLocalUid(first.localUid());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid saved search item model index for local uid"));
        }

        res = model->removeRow(firstIndex.row(), QModelIndex());
        if (!res) {
            FAIL(QStringLiteral("Can't remove the row with a non-synchronizable saved search item from the model"));
        }

        QModelIndex firstIndexAfterRemoval = model->indexForLocalUid(first.localUid());
        if (firstIndexAfterRemoval.isValid()) {
            FAIL(QStringLiteral("Was able to get the valid model index for the removed saved search item by local uid which is not intended"));
        }

        // Check the sorting for saved search items: by default should sort by name in ascending order
        res = checkSorting(*model);
        if (!res) {
            FAIL(QStringLiteral("Sorting check failed for the saved search model for ascending order"));
        }

        // Change the sort order and check the sorting again
        model->sort(SavedSearchModel::Columns::Name, Qt::DescendingOrder);

        res = checkSorting(*model);
        if (!res) {
            FAIL(QStringLiteral("Sorting check failed for the saved search model for descending order"));
        }

        emit success();
        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void SavedSearchModelTestHelper::onAddSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("SavedSearchModelTestHelper::onAddSavedSearchFailed: search = ") << search
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

void SavedSearchModelTestHelper::onUpdateSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("SavedSearchModelTestHelper::onUpdateSavedSearchFailed: search = ") << search
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

void SavedSearchModelTestHelper::onFindSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("SavedSearchModelTestHelper::onFindSavedSearchFailed: search = ") << search
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

void SavedSearchModelTestHelper::onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                           size_t limit, size_t offset,
                                                           LocalStorageManager::ListSavedSearchesOrder::type order,
                                                           LocalStorageManager::OrderDirection::type orderDirection,
                                                           QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("SavedSearchModelTestHelper::onListSavedSearchesFailed: flag = ") << flag << QStringLiteral(", limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ")
            << requestId);

    emit failure();
}

void SavedSearchModelTestHelper::onExpungeSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("SavedSearchModelTestHelper::onExpungeSavedSearchFailed: search = ") << search
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

bool SavedSearchModelTestHelper::checkSorting(const SavedSearchModel & model) const
{
    int numRows = model.rowCount(QModelIndex());

    QStringList names;
    names.reserve(numRows);
    for(int i = 0; i < numRows; ++i) {
        QModelIndex index = model.index(i, SavedSearchModel::Columns::Name, QModelIndex());
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

bool SavedSearchModelTestHelper::LessByName::operator()(const QString & lhs, const QString & rhs) const
{
    return (lhs.toUpper().localeAwareCompare(rhs.toUpper()) <= 0);
}

bool SavedSearchModelTestHelper::GreaterByName::operator()(const QString & lhs, const QString & rhs) const
{
    return (lhs.toUpper().localeAwareCompare(rhs.toUpper()) > 0);
}

} // namespace quentier
