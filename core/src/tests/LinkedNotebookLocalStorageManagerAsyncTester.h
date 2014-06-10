#ifndef __QUTE_NOTE__CORE__TESTS__LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/LinkedNotebook.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThread)

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
    void getLinkedNotebookCountRequest();
    void addLinkedNotebookRequest(QSharedPointer<LinkedNotebook> notebook);
    void updateLinkedNotebookRequest(QSharedPointer<LinkedNotebook> notebook);
    void findLinkedNotebookRequest(QSharedPointer<LinkedNotebook> notebook);
    void listAllLinkedNotebooksRequest();
    void expungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook> notebook);

private Q_SLOTS:
    void onGetLinkedNotebookCountCompleted(int count);
    void onGetLinkedNotebookCountFailed(QString errorDescription);
    void onAddLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook);
    void onAddLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription);
    void onUpdateLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook);
    void onUpdateLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription);
    void onFindLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook);
    void onFindLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription);
    void onListAllLinkedNotebooksCompleted(QList<LinkedNotebook> linkedNotebooks);
    void onLIstAllLinkedNotebooksFailed(QString errorDescription);
    void onExpungeLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook);
    void onExpungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription);

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
    LocalStorageManagerThread     * m_pLocalStorageManagerThread;
    QSharedPointer<LinkedNotebook>  m_pInitialLinkedNotebook;
    QSharedPointer<LinkedNotebook>  m_pFoundLinkedNotebook;
    QSharedPointer<LinkedNotebook>  m_pModifiedLinkedNotebook;
    QList<LinkedNotebook>   m_initialLinkedNotebooks;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__LINKED_NOTEBOOK_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
