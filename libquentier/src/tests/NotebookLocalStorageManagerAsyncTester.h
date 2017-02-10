/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_TESTS_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define LIB_QUENTIER_TESTS_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/SharedNotebook.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class NotebookLocalStorageManagerAsyncTester : public QObject
{
    Q_OBJECT
public:
    explicit NotebookLocalStorageManagerAsyncTester(QObject * parent = Q_NULLPTR);
    ~NotebookLocalStorageManagerAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals:
    void getNotebookCountRequest(QUuid requestId = QUuid());
    void addNotebookRequest(Notebook notebook, QUuid requestId = QUuid());
    void updateNotebookRequest(Notebook notebook, QUuid requestId = QUuid());
    void findNotebookRequest(Notebook notebook, QUuid requestId = QUuid());
    void findDefaultNotebookRequest(Notebook notebook, QUuid requestId = QUuid());
    void findLastUsedNotebookRequest(Notebook notebook, QUuid requestId = QUuid());
    void findDefaultOrLastUsedNotebookRequest(Notebook notebook, QUuid requestId = QUuid());
    void listAllNotebooksRequest(size_t limit, size_t offset,
                                 LocalStorageManager::ListNotebooksOrder::type order,
                                 LocalStorageManager::OrderDirection::type orderDirection,
                                 QString linkedNotebookGuid, QUuid requestId = QUuid());
    void listAllSharedNotebooksRequest(QUuid requestId = QUuid());
    void listSharedNotebooksPerNotebookRequest(QString notebookGuid, QUuid requestId = QUuid());
    void expungeNotebookRequest(Notebook notebook, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();
    void onGetNotebookCountCompleted(int count, QUuid requestId);
    void onGetNotebookCountFailed(ErrorString errorDescription, QUuid requestId);
    void onAddNotebookCompleted(Notebook notebook, QUuid requestId);
    void onAddNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onUpdateNotebookCompleted(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onFindNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onFindDefaultNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindDefaultNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onFindLastUsedNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindLastUsedNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onFindDefaultOrLastUsedNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindDefaultOrLastUsedNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onListAllNotebooksCompleted(size_t limit, size_t offset,
                                     LocalStorageManager::ListNotebooksOrder::type order,
                                     LocalStorageManager::OrderDirection::type orderDirection,
                                     QString linkedNotebookGuid, QList<Notebook> notebooks, QUuid requestId);
    void onListAllNotebooksFailed(size_t limit, size_t offset,
                                  LocalStorageManager::ListNotebooksOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId);
    void onListAllSharedNotebooksCompleted(QList<SharedNotebook> sharedNotebooks, QUuid requestId);
    void onListAllSharedNotebooksFailed(ErrorString errorDescription, QUuid requestId);
    void onListSharedNotebooksPerNotebookGuidCompleted(QString notebookGuid, QList<SharedNotebook> sharedNotebooks, QUuid requestId);
    void onListSharedNotebooksPerNotebookGuidFailed(QString notebookGuid, ErrorString errorDescription, QUuid requestId);
    void onExpungeNotebookCompleted(Notebook notebook, QUuid requestId);
    void onExpungeNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onFailure(ErrorString errorDescription);

private:
    void createConnections();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_ADD_REQUEST,
        STATE_SENT_FIND_AFTER_ADD_REQUEST,
        STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST,
        STATE_SENT_UPDATE_REQUEST,
        STATE_SENT_FIND_AFTER_UPDATE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST,
        STATE_SENT_DELETE_REQUEST,
        STATE_SENT_EXPUNGE_REQUEST,
        STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_ADD_EXTRA_NOTEBOOK_ONE_REQUEST,
        STATE_SENT_ADD_EXTRA_NOTEBOOK_TWO_REQUEST,
        STATE_SENT_LIST_NOTEBOOKS_REQUEST,
        STATE_SENT_LIST_ALL_SHARED_NOTEBOOKS_REQUEST,
        STATE_SENT_LIST_SHARED_NOTEBOOKS_PER_NOTEBOOK_REQUEST,
        STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_ADD,
        STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD,
        STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD,
        STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_UPDATE,
        STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_UPDATE,
        STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_UPDATE
    };

    State m_state;

    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
    QThread *                   m_pLocalStorageManagerThread;

    qint32                      m_userId;

    Notebook                    m_initialNotebook;
    Notebook                    m_foundNotebook;
    Notebook                    m_modifiedNotebook;
    QList<Notebook>             m_initialNotebooks;
    QList<SharedNotebook>       m_allInitialSharedNotebooks;
    QList<SharedNotebook>       m_initialSharedNotebooksPerNotebook;
};

} // namespace quentier
} // namespace test

#endif // LIB_QUENTIER_TESTS_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
