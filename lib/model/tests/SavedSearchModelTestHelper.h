/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_TESTS_SAVED_SEARCH_MODEL_TEST_HELPER_H
#define QUENTIER_LIB_MODEL_TESTS_SAVED_SEARCH_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>

namespace quentier {

class SavedSearchModel;

class SavedSearchModelTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit SavedSearchModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent = nullptr);

    ~SavedSearchModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddSavedSearchFailed(
        qevercloud::SavedSearch search, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateSavedSearchFailed(
        qevercloud::SavedSearch search, ErrorString errorDescription,
        QUuid requestId);

    void onFindSavedSearchFailed(
        qevercloud::SavedSearch search, ErrorString errorDescription,
        QUuid requestId);

    void onListSavedSearchesFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onExpungeSavedSearchFailed(
        qevercloud::SavedSearch search, ErrorString errorDescription,
        QUuid requestId);

private:
    [[nodiscard]] bool checkSorting(const SavedSearchModel & model) const;
    void notifyFailureWithStackTrace(ErrorString errorDescription);

    struct LessByName
    {
        [[nodiscard]] bool operator()(
            const QString & lhs, const QString & rhs) const noexcept;
    };

    struct GreaterByName
    {
        [[nodiscard]] bool operator()(
            const QString & lhs, const QString & rhs) const noexcept;
    };

private:
    LocalStorageManagerAsync * m_pLocalStorageManagerAsync;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TESTS_SAVED_SEARCH_MODEL_TEST_HELPER_H
