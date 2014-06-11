#include "LinkedNotebookLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

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
    // NOTE: shouldn't attempt to delete m_pLocalStorageManagerThread as Qt's parent-child system
    // should take care of that
}

void LinkedNotebookLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "LinkedNotebookLocalStorageManagerAsyncTester";
    qint32 userId = 1;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread != nullptr) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch, this);

    m_pInitialLinkedNotebook = QSharedPointer<LinkedNotebook>(new LinkedNotebook);
    m_pInitialLinkedNotebook->setGuid("00000000-0000-0000-c000-000000000001");
    m_pInitialLinkedNotebook->setUpdateSequenceNumber(1);
    m_pInitialLinkedNotebook->setShareName("Fake linked notebook share name");
    m_pInitialLinkedNotebook->setUsername("Fake linked notebook username");
    m_pInitialLinkedNotebook->setShardId("Fake linked notebook shard id");
    m_pInitialLinkedNotebook->setShareKey("Fake linked notebook share key");
    m_pInitialLinkedNotebook->setUri("Fake linked notebook uri");
    m_pInitialLinkedNotebook->setNoteStoreUrl("Fake linked notebook note store url");
    m_pInitialLinkedNotebook->setWebApiUrlPrefix("Fake linked notebook web api url prefix");
    m_pInitialLinkedNotebook->setStack("Fake linked notebook stack");
    m_pInitialLinkedNotebook->setBusinessId(1);

    QString errorDescription;
    if (!m_pInitialLinkedNotebook->checkParameters(errorDescription)) {
        QNWARNING("Found invalid LinkedNotebook: " << *m_pInitialLinkedNotebook << ", error: "
                  << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addLinkedNotebookRequest(m_pInitialLinkedNotebook);
}

void LinkedNotebookLocalStorageManagerAsyncTester::onGetLinkedNotebookCountCompleted(int count)
{
#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = QObject::tr("Internal error in LinkedNotebookLocalStorageManagerAsyncTester: " \
                                       "found wrong state"); \
        emit failure(errorDescription); \
        return; \
    }

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
    Q_ASSERT_X(!notebook.isNull(), "LinkedNotebookLocalStorageManagerAsyncTester::onAddLinkedNotebookCompleted slot",
               "Found NULL shared pointer to LinkedNotebook");

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_pInitialLinkedNotebook != notebook) {
            errorDescription = "Internal error in LinkedNotebookLocalStorageManagerAsyncTester: "
                               "notebook in addLinkedNotebookCompleted slot doesn't match the original LinkedNotebook";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_pFoundLinkedNotebook = QSharedPointer<LinkedNotebook>(new LinkedNotebook);
        m_pFoundLinkedNotebook->setLocalGuid(notebook->localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findLinkedNotebookRequest(m_pFoundLinkedNotebook);
    }
    HANDLE_WRONG_STATE();
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

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace qute_note
