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

#ifndef LIB_QUENTIER_TESTS_LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define LIB_QUENTIER_TESTS_LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/LinkedNotebook.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class LinkedNotebookLocalStorageManagerAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit LinkedNotebookLocalStorageManagerAsyncTester(QObject * parent = Q_NULLPTR);
    ~LinkedNotebookLocalStorageManagerAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals:
    void getLinkedNotebookCountRequest(QUuid requestId = QUuid());
    void addLinkedNotebookRequest(LinkedNotebook notebook, QUuid requestId = QUuid());
    void updateLinkedNotebookRequest(LinkedNotebook notebook, QUuid requestId = QUuid());
    void findLinkedNotebookRequest(LinkedNotebook notebook, QUuid requestId = QUuid());
    void listAllLinkedNotebooksRequest(size_t limit, size_t offset,
                                       LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                       LocalStorageManager::OrderDirection::type orderDirection,
                                       QUuid requestId = QUuid());
    void expungeLinkedNotebookRequest(LinkedNotebook notebook, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();
    void onGetLinkedNotebookCountCompleted(int count, QUuid requestId);
    void onGetLinkedNotebookCountFailed(QNLocalizedString errorDescription, QUuid requestId);
    void onAddLinkedNotebookCompleted(LinkedNotebook notebook, QUuid requestId);
    void onAddLinkedNotebookFailed(LinkedNotebook notebook, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateLinkedNotebookCompleted(LinkedNotebook notebook, QUuid requestId);
    void onUpdateLinkedNotebookFailed(LinkedNotebook notebook, QNLocalizedString errorDescription, QUuid requestId);
    void onFindLinkedNotebookCompleted(LinkedNotebook notebook, QUuid requestId);
    void onFindLinkedNotebookFailed(LinkedNotebook notebook, QNLocalizedString errorDescription, QUuid requestId);
    void onListAllLinkedNotebooksCompleted(size_t limit, size_t offset,
                                           LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                           LocalStorageManager::OrderDirection::type orderDirection,
                                           QList<LinkedNotebook> linkedNotebooks,
                                           QUuid requestId);
    void onListAllLinkedNotebooksFailed(size_t limit, size_t offset,
                                        LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeLinkedNotebookCompleted(LinkedNotebook notebook, QUuid requestId);
    void onExpungeLinkedNotebookFailed(LinkedNotebook notebook, QNLocalizedString errorDescription, QUuid requestId);

    void onFailure(QNLocalizedString errorDescription);

private:
    void createConnections();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_ADD_REQUEST,
        STATE_SENT_FIND_AFTER_ADD_REQUEST,
        STATE_SENT_UPDATE_REQUEST,
        STATE_SENT_FIND_AFTER_UPDATE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST,
        STATE_SENT_EXPUNGE_REQUEST,
        STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_ADD_EXTRA_LINKED_NOTEBOOK_ONE_REQUEST,
        STATE_SENT_ADD_EXTRA_LINKED_NOTEBOOK_TWO_REQUEST,
        STATE_SENT_LIST_LINKED_NOTEBOOKS_REQUEST
    };

    State   m_state;

    LocalStorageManagerThreadWorker     * m_pLocalStorageManagerThreadWorker;
    QThread *               m_pLocalStorageManagerThread;

    LinkedNotebook          m_initialLinkedNotebook;
    LinkedNotebook          m_foundLinkedNotebook;
    LinkedNotebook          m_modifiedLinkedNotebook;
    QList<LinkedNotebook>   m_initialLinkedNotebooks;
};

} // namespace test
} // namespace quentier

#endif // LIB_QUENTIER_TESTS_LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
