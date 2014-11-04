#ifndef __QUTE_NOTE__CORE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

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
    void getNotebookCountRequest();
    void addNotebookRequest(Notebook notebook);
    void updateNotebookRequest(Notebook notebook);
    void findNotebookRequest(Notebook notebook);
    void findDefaultNotebookRequest(Notebook notebook);
    void findLastUsedNotebookRequest(Notebook notebook);
    void findDefaultOrLastUsedNotebookRequest(Notebook notebook);
    void listAllNotebooksRequest();
    void listAllSharedNotebooksRequest();
    void listSharedNotebooksPerNotebookRequest(QString notebookGuid);
    void expungeNotebookRequest(Notebook notebook);

private Q_SLOTS:
    void onWorkerInitialized();
    void onGetNotebookCountCompleted(int count);
    void onGetNotebookCountFailed(QString errorDescription);
    void onAddNotebookCompleted(Notebook notebook);
    void onAddNotebookFailed(Notebook notebook, QString errorDescription);
    void onUpdateNotebookCompleted(Notebook notebook);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription);
    void onFindNotebookCompleted(Notebook notebook);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription);
    void onFindDefaultNotebookCompleted(Notebook notebook);
    void onFindDefaultNotebookFailed(Notebook notebook, QString errorDescription);
    void onFindLastUsedNotebookCompleted(Notebook notebook);
    void onFindLastUsedNotebookFailed(Notebook notebook, QString errorDescription);
    void onFindDefaultOrLastUsedNotebookCompleted(Notebook notebook);
    void onFindDefaultOrLastUsedNotebookFailed(Notebook notebook, QString errorDescription);
    void onListAllNotebooksCompleted(QList<Notebook> notebooks);
    void onListAllNotebooksFailed(QString errorDescription);
    void onListAllSharedNotebooksCompleted(QList<SharedNotebookWrapper> sharedNotebooks);
    void onListAllSharedNotebooksFailed(QString errorDescription);
    void onListSharedNotebooksPerNotebookGuidCompleted(QString notebookGuid, QList<SharedNotebookWrapper> sharedNotebooks);
    void onListSharedNotebooksPerNotebookGuidFailed(QString notebookGuid, QString errorDescription);
    void onExpungeNotebookCompleted(Notebook notebook);
    void onExpungeNotebookFailed(Notebook notebook, QString errorDescription);

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

#endif // __QUTE_NOTE__CORE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
