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
    createConnections();

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
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in LinkedNotebookLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        emit failure(errorDescription); \
        return; \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = "GetLinkedNotebookCount returned result different from the expected one (1): ";
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_EXPUNGE_REQUEST;
        emit expungeLinkedNotebookRequest(m_pModifiedLinkedNotebook);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = "GetLinkedNotebookCount returned result different from the expected one (0): ";
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        QSharedPointer<LinkedNotebook> extraLinkedNotebook = QSharedPointer<LinkedNotebook>(new LinkedNotebook);
        extraLinkedNotebook->setGuid("00000000-0000-0000-c000-000000000001");
        extraLinkedNotebook->setUpdateSequenceNumber(1);
        extraLinkedNotebook->setUsername("Extra LinkedNotebook");
        extraLinkedNotebook->setShareName("Fake extra linked notebook share name");
        extraLinkedNotebook->setShareKey("Fake extra linked notebook share key");
        extraLinkedNotebook->setShardId("Fake extra linked notebook shard id");
        extraLinkedNotebook->setStack("Fake extra linked notebook stack");
        extraLinkedNotebook->setNoteStoreUrl("Fake extra linked notebook note store url");
        extraLinkedNotebook->setWebApiUrlPrefix("Fake extra linked notebook web api url prefix");
        extraLinkedNotebook->setUri("Fake extra linked notebook uri");

        m_state = STATE_SENT_ADD_EXTRA_LINKED_NOTEBOOK_ONE_REQUEST;
        emit addLinkedNotebookRequest(extraLinkedNotebook);
    }
    HANDLE_WRONG_STATE();
}

void LinkedNotebookLocalStorageManagerAsyncTester::onGetLinkedNotebookCountFailed(QString errorDescription)
{
    emit failure(errorDescription);
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
            emit failure(errorDescription);
            return;
        }

        m_pFoundLinkedNotebook = QSharedPointer<LinkedNotebook>(new LinkedNotebook);
        m_pFoundLinkedNotebook->setLocalGuid(notebook->localGuid());
        m_pFoundLinkedNotebook->setGuid(notebook->guid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findLinkedNotebookRequest(m_pFoundLinkedNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_LINKED_NOTEBOOK_ONE_REQUEST)
    {
        m_initialLinkedNotebooks << *notebook;

        QSharedPointer<LinkedNotebook> extraLinkedNotebook = QSharedPointer<LinkedNotebook>(new LinkedNotebook);
        extraLinkedNotebook->setGuid("00000000-0000-0000-c000-000000000002");
        extraLinkedNotebook->setUpdateSequenceNumber(2);
        extraLinkedNotebook->setUsername("Fake linked notebook username two");
        extraLinkedNotebook->setShareName("Fake extra linked notebook share name two");
        extraLinkedNotebook->setShareKey("Fake extra linked notebook share key two");
        extraLinkedNotebook->setShardId("Fake extra linked notebook shard id two");
        extraLinkedNotebook->setStack("Fake extra linked notebook stack two");
        extraLinkedNotebook->setNoteStoreUrl("Fake extra linked notebook note store url two");
        extraLinkedNotebook->setWebApiUrlPrefix("Fake extra linked notebook web api url prefix two");
        extraLinkedNotebook->setUri("Fake extra linked notebook uri two");

        m_state = STATE_SENT_ADD_EXTRA_LINKED_NOTEBOOK_TWO_REQUEST;
        emit addLinkedNotebookRequest(extraLinkedNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_LINKED_NOTEBOOK_TWO_REQUEST)
    {
        m_initialLinkedNotebooks << *notebook;

        m_state = STATE_SENT_LIST_LINKED_NOTEBOOKS_REQUEST;
        emit listAllLinkedNotebooksRequest();
    }
    HANDLE_WRONG_STATE();
}

void LinkedNotebookLocalStorageManagerAsyncTester::onAddLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook,
                                                                             QString errorDescription)
{
    Q_UNUSED(notebook)
    emit failure(errorDescription);
}

void LinkedNotebookLocalStorageManagerAsyncTester::onUpdateLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "LinkedNotebookLocalStorageManagerAsyncTester::onUpdateLinkedNotebookCompleted slot",
               "Found NULL shared pointer to LinkedNotebook");

    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_pModifiedLinkedNotebook != notebook) {
            errorDescription = "Internal error in LinkedNotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onUpdateLinkedNotebookCompleted slot doesn't match "
                               "the pointer to the original modified LinkedNotebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findLinkedNotebookRequest(m_pFoundLinkedNotebook);
    }
}

void LinkedNotebookLocalStorageManagerAsyncTester::onUpdateLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription)
{
    Q_UNUSED(notebook)
    emit failure(errorDescription);
}

void LinkedNotebookLocalStorageManagerAsyncTester::onFindLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "LinkedNotebookLocalStorageManagerAsyncTester::onFindLinkedNotebookCompleted slot",
               "Found NULL shared pointer to LinkedNotebook");

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_pFoundLinkedNotebook != notebook) {
            errorDescription = "Internal error in LinkedNotebookLocalStorageAsyncTester: "
                               "notebook pointer in onFindLinkedNotebookCompleted slot doesn't match "
                               "the pointer to the original LinkedNotebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pInitialLinkedNotebook.isNull());
        if (*m_pFoundLinkedNotebook != *m_pInitialLinkedNotebook) {
            errorDescription = "Added and found linked notebooks in local storage don't match";
            QNWARNING(errorDescription << ": LinkedNotebook added to LocalStorageManager: " << *m_pInitialLinkedNotebook
                      << "\nLinkedNotebook found in LocalStorageManager: " << *m_pFoundLinkedNotebook);
            emit failure(errorDescription);
            return;
        }

        // Ok, found linked notebook is good, updating it now
        m_pModifiedLinkedNotebook = QSharedPointer<LinkedNotebook>(new LinkedNotebook(*m_pInitialLinkedNotebook));
        m_pModifiedLinkedNotebook->setUpdateSequenceNumber(m_pInitialLinkedNotebook->updateSequenceNumber() + 1);
        m_pModifiedLinkedNotebook->setUsername(m_pInitialLinkedNotebook->username() + "_modified");
        m_pModifiedLinkedNotebook->setStack(m_pInitialLinkedNotebook->stack() + "_modified");
        m_pModifiedLinkedNotebook->setShareName(m_pInitialLinkedNotebook->shareName() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateLinkedNotebookRequest(m_pModifiedLinkedNotebook);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_pFoundLinkedNotebook != notebook) {
            errorDescription = "Internal error in LinkedNotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindLinkedNotebookCompleted slot doesn't match "
                               "the pointer to the original modified LinkedNotebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pModifiedLinkedNotebook.isNull());
        if (*m_pFoundLinkedNotebook != *m_pModifiedLinkedNotebook) {
            errorDescription = "Updated and found linked notebooks in local storage don't match";
            QNWARNING(errorDescription << ": LinkedNotebook updated in LocalStorageManager: "
                      << *m_pModifiedLinkedNotebook << "\nLinkedNotebook found in LocalStorageManager: "
                      << *m_pFoundLinkedNotebook);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getLinkedNotebookCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        Q_ASSERT(!m_pModifiedLinkedNotebook.isNull());
        errorDescription = "Error: found linked notebook which should have been expunged from local storage";
        QNWARNING(errorDescription << ": LinkedNotebook expunged from LocalStorageManager: " << *m_pModifiedLinkedNotebook
                  << "\nLinkedNotebook found in LocalStorageManager: " << *m_pFoundLinkedNotebook);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void LinkedNotebookLocalStorageManagerAsyncTester::onFindLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getLinkedNotebookCountRequest();
        return;
    }

    Q_UNUSED(notebook)
    emit failure(errorDescription);
}

void LinkedNotebookLocalStorageManagerAsyncTester::onListAllLinkedNotebooksCompleted(QList<LinkedNotebook> linkedNotebooks)
{
    int numInitialLinkedNotebooks = m_initialLinkedNotebooks.size();
    int numFoundLinkedNotebooks = linkedNotebooks.size();

    QString errorDescription;

    if (numInitialLinkedNotebooks != numFoundLinkedNotebooks) {
        errorDescription = "Error: number of found linked notebooks does not correspond "
                           "to the number of original added linked notebooks";
        emit failure(errorDescription);
        return;
    }

    foreach(const LinkedNotebook & notebook, m_initialLinkedNotebooks)
    {
        if (!linkedNotebooks.contains(notebook)) {
            errorDescription = "Error: one of initial linked notebooks was not found "
                               "within found linked notebooks";
            emit failure(errorDescription);
            return;
        }
    }

    emit success();
}

void LinkedNotebookLocalStorageManagerAsyncTester::onLIstAllLinkedNotebooksFailed(QString errorDescription)
{
    emit failure(errorDescription);
}

void LinkedNotebookLocalStorageManagerAsyncTester::onExpungeLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "LinkedNotebookLocalStorageManagerAsyncTester::onExpungeLinkedNotebookCompleted slot",
               "Found NULL shared pointer to LinkedNotebook");

    QString errorDescription;

    if (m_pModifiedLinkedNotebook != notebook) {
        errorDescription = "Internal error in LinkedNotebookLocalStorageManagerAsyncTester: "
                           "linked notebook pointer in onExpungeLinkedNotebookCompleted slot doesn't match "
                           "the pointer to the original expunged LinkedNotebook";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    Q_ASSERT(!m_pFoundLinkedNotebook.isNull());
    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findLinkedNotebookRequest(m_pFoundLinkedNotebook);
}

void LinkedNotebookLocalStorageManagerAsyncTester::onExpungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook> notebook,
                                                                                 QString errorDescription)
{
    Q_UNUSED(notebook)
    emit failure(errorDescription);
}

void LinkedNotebookLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(getLinkedNotebookCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetLinkedNotebookCountRequest()));
    QObject::connect(this, SIGNAL(addLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)),
                     m_pLocalStorageManagerThread, SLOT(onAddLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)));
    QObject::connect(this, SIGNAL(updateLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)));
    QObject::connect(this, SIGNAL(findLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)),
                     m_pLocalStorageManagerThread, SLOT(onFindLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)));
    QObject::connect(this, SIGNAL(listAllLinkedNotebooksRequest()), m_pLocalStorageManagerThread,
                     SLOT(onListAllLinkedNotebooksRequest()));
    QObject::connect(this, SIGNAL(expungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getLinkedNotebookCountComplete(int)),
                     this, SLOT(onGetLinkedNotebookCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getLinkedNotebookCountFailed(QString)),
                     this, SLOT(onGetLinkedNotebookCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SLOT(onAddLinkedNotebookCompleted(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SLOT(onAddLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SLOT(onUpdateLinkedNotebookCompleted(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SLOT(onUpdateLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SLOT(onFindLinkedNotebookCompleted(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SLOT(onFindLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)),
                     this, SLOT(onListAllLinkedNotebooksCompleted(QList<LinkedNotebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllLinkedNotebooksFailed(QString)),
                     this, SLOT(onLIstAllLinkedNotebooksFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SLOT(onExpungeLinkedNotebookCompleted(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SLOT(onExpungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace qute_note
