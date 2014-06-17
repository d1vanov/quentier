#ifndef __QUTE_NOTE__CORE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThread)

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
    void addNotebookRequest(QSharedPointer<Notebook> notebook);
    void updateNotebookRequest(QSharedPointer<Notebook> notebook);
    void findNotebookRequest(QSharedPointer<Notebook> notebook);
    void findDefaultNotebookRequest(QSharedPointer<Notebook> notebook);
    void findLastUsedNotebookRequest(QSharedPointer<Notebook> notebook);
    void findDefaultOrLastUsedNotebookRequest(QSharedPointer<Notebook> notebook);
    void listAllNotebooksRequest();
    void listAllSharedNotebooksRequest();
    void listSharedNotebooksPerNotebookRequest();
    void expungeNotebookRequest(QSharedPointer<Notebook> notebook);

private Q_SLOTS:
    void onGetNotebookCountCompleted(int count);
    void onGetNotebookCountFailed(QString errorDescription);
    void onAddNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onAddNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void onUpdateNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onUpdateNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void onFindNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onFindNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void onFindDefaultNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onFindDefaultNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void onFindLastUsedNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onFindLastUsedNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void onFindDefaultOrLastUsedNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onFindDefaultOrLastUsedNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void onListAllNotebooksCompleted(QList<Notebook> notebooks);
    void onListAllNotebooksFailed(QString errorDescription);
    void onListAllSharedNotebooksCompleted(QList<SharedNotebookWrapper> sharedNotebooks);
    void onListAllSharedNotebooksFailed(QString errorDescription);
    void onListSharedNotebooksPerNotebookGuidCompleted(QList<SharedNotebookWrapper> sharedNotebooks);
    void onListSharedNotebooksPerNotebookGuidFailed(QString errorDescription);
    void onExpungeNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onExpungeNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);

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
        STATE_SENT_FIND_DEFAULT_NOTEBOOK,
        STATE_SENT_FIND_LAST_USED_NOTEBOOK,
        STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK
    };

    State m_state;
    LocalStorageManagerThread * m_pLocalStorageManagerThread;
    QSharedPointer<Notebook>    m_pInitialNotebook;
    QSharedPointer<Notebook>    m_pFoundNotebook;
    QSharedPointer<Notebook>    m_pModifiedNotebook;
    QList<Notebook>             m_initialNotebooks;
    QList<SharedNotebookWrapper>  m_allInitialSharedNotebooks;
    QList<SharedNotebookWrapper>  m_initialSharedNotebooksPerNotebook;
};

} // namespace qute_note
} // namespace test

#endif // __QUTE_NOTE__CORE__TESTS__NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
