#ifndef __LIB_QUTE_NOTE__TESTS__LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __LIB_QUTE_NOTE__TESTS__LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <qute_note/local_storage/LocalStorageManager.h>
#include <qute_note/types/LinkedNotebook.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class LinkedNotebookLocalStorageManagerAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit LinkedNotebookLocalStorageManagerAsyncTester(QObject * parent = nullptr);
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
    void onGetLinkedNotebookCountFailed(QString errorDescription, QUuid requestId);
    void onAddLinkedNotebookCompleted(LinkedNotebook notebook, QUuid requestId);
    void onAddLinkedNotebookFailed(LinkedNotebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateLinkedNotebookCompleted(LinkedNotebook notebook, QUuid requestId);
    void onUpdateLinkedNotebookFailed(LinkedNotebook notebook, QString errorDescription, QUuid requestId);
    void onFindLinkedNotebookCompleted(LinkedNotebook notebook, QUuid requestId);
    void onFindLinkedNotebookFailed(LinkedNotebook notebook, QString errorDescription, QUuid requestId);
    void onListAllLinkedNotebooksCompleted(size_t limit, size_t offset,
                                           LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                           LocalStorageManager::OrderDirection::type orderDirection,
                                           QList<LinkedNotebook> linkedNotebooks,
                                           QUuid requestId);
    void onListAllLinkedNotebooksFailed(size_t limit, size_t offset,
                                        LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QString errorDescription, QUuid requestId);
    void onExpungeLinkedNotebookCompleted(LinkedNotebook notebook, QUuid requestId);
    void onExpungeLinkedNotebookFailed(LinkedNotebook notebook, QString errorDescription, QUuid requestId);

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
} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TESTS__LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
