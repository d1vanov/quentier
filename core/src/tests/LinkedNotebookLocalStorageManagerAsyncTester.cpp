#include "LinkedNotebookLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>

namespace qute_note {
namespace test {

LinkedNotebookLocalStorageManagerAsyncTester::LinkedNotebookLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_pInitialLinkedNotebook(),
    m_pFoundLinkedNotebook(),
    m_pModifiedLinkedNotebook(),
    m_initialLinkedNotebooks()
{}

LinkedNotebookLocalStorageManagerAsyncTester::~LinkedNotebookLocalStorageManagerAsyncTester()
{
    if (m_pLocalStorageManagerThread != nullptr) {
        m_pLocalStorageManagerThread->exit();
        delete m_pLocalStorageManagerThread;
    }
}

void LinkedNotebookLocalStorageManagerAsyncTester::onInitTestCase()
{
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onGetLinkedNotebookCountCompleted(int count)
{
    Q_UNUSED(count)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onGetLinkedNotebookCountFailed(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onAddLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onAddLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook,
                                                                             QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onUpdateLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onUpdateLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onFindLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onFindLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onListAllLinkedNotebooksCompleted(QList<LinkedNotebook> linkedNotebooks)
{
    Q_UNUSED(linkedNotebooks)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onLIstAllLinkedNotebooksFailed(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onExpungeLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook)
{
    Q_UNUSED(notebook)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::onExpungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void LinkedNotebookLocalStorageManagerAsyncTester::createConnections()
{
    // TODO: implement
}

} // namespace test
} // namespace qute_note
