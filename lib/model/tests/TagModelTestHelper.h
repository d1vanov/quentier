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

#ifndef QUENTIER_LIB_MODEL_TESTS_TAG_MODEL_TEST_HELPER_H
#define QUENTIER_LIB_MODEL_TESTS_TAG_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>

namespace quentier {

class ITagModelItem;
class TagModel;

class TagModelTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit TagModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent = nullptr);

    ~TagModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onUpdateTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

private:
    [[nodiscard]] bool checkSorting(
        const TagModel & model, const ITagModelItem * pRootItem,
        ErrorString & errorDescription) const;

    void notifyFailureWithStackTrace(ErrorString errorDescription);

    struct LessByName
    {
        [[nodiscard]] bool operator()(
            const ITagModelItem * pLhs,
            const ITagModelItem * pRhs) const noexcept;
    };

    struct GreaterByName
    {
        [[nodiscard]] bool operator()(
            const ITagModelItem * pLhs,
            const ITagModelItem * pRhs) const noexcept;
    };

private:
    LocalStorageManagerAsync * m_pLocalStorageManagerAsync;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TESTS_TAG_MODEL_TEST_HELPER_H
