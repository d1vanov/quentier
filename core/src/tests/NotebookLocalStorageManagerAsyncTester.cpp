#include "NotebookLocalStorageManagerAsyncTester.h"

namespace qute_note {
namespace test {

NotebookLocalStorageManagerAsyncTester::NotebookLocalStorageManagerAsyncTester(QObject *parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_pInitialNotebook(),
    m_pFoundNotebook(),
    m_pModifiedNotebook(),
    m_initialNotebooks(),
    m_allInitialSharedNotebooks(),
    m_initialSharedNotebooksPerNotebook()
{}

NotebookLocalStorageManagerAsyncTester::~NotebookLocalStorageManagerAsyncTester()
{}

void NotebookLocalStorageManagerAsyncTester::onInitTestCase()
{
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onGetNotebookCountCompleted(int count)
{
    Q_UNUSED(count)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onGetNotebookCountFailed(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onAddNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onAddNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onUpdateNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onUpdateNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onFindNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onFindNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onListAllNotebooksCompleted(QList<Notebook> notebooks)
{
    Q_UNUSED(notebooks)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onListAllNotebooksFailed(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onListAllSharedNotebooksCompleted(QList<SharedNotebookWrapper> sharedNotebooks)
{
    Q_UNUSED(sharedNotebooks)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onListAllSharedNotebooksFailed(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onListSharedNotebooksPerNotebookGuidCompleted(QList<SharedNotebookWrapper> sharedNotebooks)
{
    Q_UNUSED(sharedNotebooks)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onListSharedNotebooksPerNotebookGuidFailed(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onExpungeNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::onExpungeNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void NotebookLocalStorageManagerAsyncTester::createConnections()
{
    // TODO: implement
}

} // namespace qute_note
} // namespace test
