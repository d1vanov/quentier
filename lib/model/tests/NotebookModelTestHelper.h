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

#ifndef QUENTIER_LIB_MODEL_TESTS_NOTEBOOK_MODEL_TEST_HELPER_H
#define QUENTIER_LIB_MODEL_TESTS_NOTEBOOK_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(INotebookModelItem)
QT_FORWARD_DECLARE_CLASS(NotebookModel)

class NotebookModelTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit NotebookModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent = nullptr);

    virtual ~NotebookModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onUpdateNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onFindNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

private:
    bool checkSorting(
        const NotebookModel & model,
        const INotebookModelItem * pModelItem) const;

    void notifyFailureWithStackTrace(ErrorString errorDescription);

    struct LessByName
    {
        bool operator()(
            const INotebookModelItem * pLhs,
            const INotebookModelItem * pRhs) const;
    };

    struct GreaterByName
    {
        bool operator()(
            const INotebookModelItem * pLhs,
            const INotebookModelItem * pRhs) const;
    };

private:
    LocalStorageManagerAsync * m_pLocalStorageManagerAsync;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TESTS_NOTEBOOK_MODEL_TEST_HELPER_H
