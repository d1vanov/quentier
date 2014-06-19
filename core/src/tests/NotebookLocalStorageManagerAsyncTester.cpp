#include "NotebookLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

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
    QString username = "NotebookLocalStorageManagerAsyncTester";
    qint32 userId = 4;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread != nullptr) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch);
    createConnections();

    m_pInitialNotebook = QSharedPointer<Notebook>(new Notebook);
    m_pInitialNotebook->setGuid("00000000-0000-0000-c000-000000000047");
    m_pInitialNotebook->setUpdateSequenceNumber(1);
    m_pInitialNotebook->setName("Fake notebook name");
    m_pInitialNotebook->setCreationTimestamp(1);
    m_pInitialNotebook->setModificationTimestamp(1);
    m_pInitialNotebook->setDefaultNotebook(true);
    m_pInitialNotebook->setLastUsed(true);
    m_pInitialNotebook->setPublishingUri("Fake publishing uri");
    m_pInitialNotebook->setPublishingOrder(1);
    m_pInitialNotebook->setPublishingAscending(true);
    m_pInitialNotebook->setPublishingPublicDescription("Fake public description");
    m_pInitialNotebook->setPublished(true);
    m_pInitialNotebook->setStack("Fake notebook stack");
    m_pInitialNotebook->setBusinessNotebookDescription("Fake business notebook description");
    m_pInitialNotebook->setBusinessNotebookPrivilegeLevel(1);
    m_pInitialNotebook->setBusinessNotebookRecommended(true);

    SharedNotebookWrapper sharedNotebook;
    sharedNotebook.setId(1);
    sharedNotebook.setUserId(userId);
    sharedNotebook.setNotebookGuid(m_pInitialNotebook->guid());
    sharedNotebook.setEmail("Fake shared notebook email");
    sharedNotebook.setCreationTimestamp(1);
    sharedNotebook.setModificationTimestamp(1);
    sharedNotebook.setShareKey("Fake shared notebook share key");
    sharedNotebook.setUsername("Fake shared notebook username");
    sharedNotebook.setPrivilegeLevel(1);
    sharedNotebook.setAllowPreview(true);
    sharedNotebook.setReminderNotifyEmail(true);
    sharedNotebook.setReminderNotifyApp(false);

    m_pInitialNotebook->addSharedNotebook(sharedNotebook);

    QString errorDescription;
    if (!m_pInitialNotebook->checkParameters(errorDescription)) {
        QNWARNING("Found invalid notebook: " << *m_pInitialNotebook << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addNotebookRequest(m_pInitialNotebook);
}

void NotebookLocalStorageManagerAsyncTester::onGetNotebookCountCompleted(int count)
{
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = QObject::tr("Internal error in NotebookLocalStorageManagerAsyncTester: " \
                                       "found wrong state"); \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = QObject::tr("GetNotebookCount returned result different from the expected one (1): ");
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        m_pModifiedNotebook->setLocal(false);
        m_state = STATE_SENT_EXPUNGE_REQUEST;
        emit expungeNotebookRequest(m_pModifiedNotebook);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = QObject::tr("GetNotebookCount returned result different from the expected one (0): ");
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        emit success();
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onGetNotebookCountFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onAddNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "NotebookLocalStorageManagerAsyncTester::onAddNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_pInitialNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook in onAddNotebookCompleted doesn't match the original Notebook";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_pFoundNotebook = QSharedPointer<Notebook>(new Notebook);
        m_pFoundNotebook->setLocalGuid(notebook->localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findNotebookRequest(m_pFoundNotebook);
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onAddNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onUpdateNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "NotebookLocalStorageManagerAsyncTester::onUpdateNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_pModifiedNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onUpdateNotebookCompleted slot doesn't match "
                               "the pointer to the original modified Notebook";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findNotebookRequest(m_pFoundNotebook);
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onUpdateNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onFindNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "NotebookLocalStorageManagerAsyncTester::onFindNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_pFoundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindNotebookCompleted slot doesn't match "
                               "the pointer to the original Notebook";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pInitialNotebook.isNull());
        if (*m_pFoundNotebook != *m_pInitialNotebook) {
            errorDescription = "Added and found notebooks in local storage don't match";
            QNWARNING(errorDescription << ": Notebook added to LocalStorageManager: " << *m_pInitialNotebook
                      << "\nNotebook found in LocalStorageManager: " << *m_pFoundNotebook);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        // Ok, found notebook is good, modifying it now
        m_pModifiedNotebook = QSharedPointer<Notebook>(new Notebook(*m_pInitialNotebook));
        m_pModifiedNotebook->setUpdateSequenceNumber(m_pInitialNotebook->updateSequenceNumber() + 1);
        m_pModifiedNotebook->setName(m_pInitialNotebook->name() + "_modified");
        m_pModifiedNotebook->setDefaultNotebook(true);
        m_pModifiedNotebook->setLastUsed(false);
        m_pModifiedNotebook->setModificationTimestamp(m_pInitialNotebook->modificationTimestamp() + 1);
        m_pModifiedNotebook->setPublishingUri(m_pInitialNotebook->publishingUri() + "_modified");
        m_pModifiedNotebook->setPublishingAscending(!m_pInitialNotebook->isPublishingAscending());
        m_pModifiedNotebook->setPublishingPublicDescription(m_pInitialNotebook->publishingPublicDescription() + "_modified");
        m_pModifiedNotebook->setStack(m_pInitialNotebook->stack() + "_modified");
        m_pModifiedNotebook->setBusinessNotebookDescription(m_pInitialNotebook->businessNotebookDescription() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateNotebookRequest(m_pModifiedNotebook);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_pFoundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindNotebookCompleted slot doesn't match "
                               "the pointer to the oirginal modified Notebook";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pModifiedNotebook.isNull());
        if (*m_pFoundNotebook != *m_pModifiedNotebook) {
            errorDescription = "Updated and found notebooks in local storage don't match";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getNotebookCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        Q_ASSERT(!m_pModifiedNotebook.isNull());
        errorDescription = "Error: found notebook which should have been expunged from local storage";
        QNWARNING(errorDescription << ": Notebook expunged from LocalStorageManager: " << *m_pModifiedNotebook
                  << "\nNotebook found in LocalStorageManager: " << *m_pFoundNotebook);
        errorDescription = QObject::tr(qPrintable(errorDescription));
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getNotebookCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
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
