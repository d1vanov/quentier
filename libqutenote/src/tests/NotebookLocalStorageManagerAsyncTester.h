#ifndef __LIB_QUTE_NOTE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __LIB_QUTE_NOTE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <qute_note/local_storage/LocalStorageManager.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class NotebookLocalStorageManagerAsyncTester : public QObject
{
    Q_OBJECT
public:
    explicit NotebookLocalStorageManagerAsyncTester(QObject * parent = nullptr);
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
    void onGetNotebookCountFailed(QString errorDescription, QUuid requestId);
    void onAddNotebookCompleted(Notebook notebook, QUuid requestId);
    void onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNotebookCompleted(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindDefaultNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindDefaultNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindLastUsedNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindLastUsedNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindDefaultOrLastUsedNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindDefaultOrLastUsedNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onListAllNotebooksCompleted(size_t limit, size_t offset,
                                     LocalStorageManager::ListNotebooksOrder::type order,
                                     LocalStorageManager::OrderDirection::type orderDirection,
                                     QString linkedNotebookGuid, QList<Notebook> notebooks, QUuid requestId);
    void onListAllNotebooksFailed(size_t limit, size_t offset,
                                  LocalStorageManager::ListNotebooksOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QString linkedNotebookGuid, QString errorDescription, QUuid requestId);
    void onListAllSharedNotebooksCompleted(QList<SharedNotebookWrapper> sharedNotebooks, QUuid requestId);
    void onListAllSharedNotebooksFailed(QString errorDescription, QUuid requestId);
    void onListSharedNotebooksPerNotebookGuidCompleted(QString notebookGuid, QList<SharedNotebookWrapper> sharedNotebooks,
                                                       QUuid requestId);
    void onListSharedNotebooksPerNotebookGuidFailed(QString notebookGuid, QString errorDescription, QUuid requestId);
    void onExpungeNotebookCompleted(Notebook notebook, QUuid requestId);
    void onExpungeNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);

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
    QList<SharedNotebookWrapper>  m_allInitialSharedNotebooks;
    QList<SharedNotebookWrapper>  m_initialSharedNotebooksPerNotebook;
};

} // namespace qute_note
} // namespace test

#endif // __LIB_QUTE_NOTE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
