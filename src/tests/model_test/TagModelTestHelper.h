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

#ifndef QUENTIER_TESTS_MODEL_TEST_TAG_MODEL_TEST_HELPER_H
#define QUENTIER_TESTS_MODEL_TEST_TAG_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(TagModelItem)

class TagModelTestHelper: public QObject
{
    Q_OBJECT
public:
    explicit TagModelTestHelper(LocalStorageManagerAsync * pLocalStorageManagerAsync,
                                QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void failure();
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onUpdateTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                          size_t limit, size_t offset,
                          LocalStorageManager::ListTagsOrder::type order,
                          LocalStorageManager::OrderDirection::type orderDirection,
                          QString linkedNotebookGuid,
                          ErrorString errorDescription, QUuid requestId);
    void onExpungeTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

private:
    bool checkSorting(const TagModel & model, const TagModelItem * rootItem) const;

    struct LessByName
    {
        bool operator()(const TagModelItem * lhs, const TagModelItem * rhs) const;
    };

    struct GreaterByName
    {
        bool operator()(const TagModelItem * lhs, const TagModelItem * rhs) const;
    };

private:
    LocalStorageManagerAsync *   m_pLocalStorageManagerAsync;
};

} // namespace quentier

#endif // QUENTIER_TESTS_MODEL_TEST_TAG_MODEL_TEST_HELPER_H
