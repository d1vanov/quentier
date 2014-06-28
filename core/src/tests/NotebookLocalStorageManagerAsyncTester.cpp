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
    m_pInitialNotebook->setLastUsed(false);
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
        errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        QNWARNING(errorDescription << ": " << m_state); \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = "GetNotebookCount returned result different from the expected one (1): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_UPDATE;
        emit findDefaultNotebookRequest(m_pFoundNotebook);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = "GetNotebookCount returned result different from the expected one (0): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        QSharedPointer<Notebook> extraNotebook = QSharedPointer<Notebook>(new Notebook);
        extraNotebook->setGuid("00000000-0000-0000-c000-000000000001");
        extraNotebook->setUpdateSequenceNumber(1);
        extraNotebook->setName("Fake extra notebook one");
        extraNotebook->setCreationTimestamp(1);
        extraNotebook->setModificationTimestamp(1);
        extraNotebook->setDefaultNotebook(true);
        extraNotebook->setLastUsed(false);
        extraNotebook->setPublishingUri("Fake publishing uri one");
        extraNotebook->setPublishingOrder(1);
        extraNotebook->setPublishingAscending(true);
        extraNotebook->setPublishingPublicDescription("Fake public description one");
        extraNotebook->setStack("Fake notebook stack one");
        extraNotebook->setBusinessNotebookDescription("Fake business notebook description one");
        extraNotebook->setBusinessNotebookPrivilegeLevel(1);
        extraNotebook->setBusinessNotebookRecommended(true);

        SharedNotebookWrapper sharedNotebookOne;
        sharedNotebookOne.setId(1);
        sharedNotebookOne.setUserId(4);
        sharedNotebookOne.setNotebookGuid(extraNotebook->guid());
        sharedNotebookOne.setEmail("Fake shared notebook email one");
        sharedNotebookOne.setCreationTimestamp(1);
        sharedNotebookOne.setModificationTimestamp(1);
        sharedNotebookOne.setShareKey("Fake shared notebook share key one");
        sharedNotebookOne.setUsername("Fake shared notebook username one");
        sharedNotebookOne.setPrivilegeLevel(1);
        sharedNotebookOne.setAllowPreview(true);
        sharedNotebookOne.setReminderNotifyEmail(true);
        sharedNotebookOne.setReminderNotifyApp(false);

        extraNotebook->addSharedNotebook(sharedNotebookOne);

        SharedNotebookWrapper sharedNotebookTwo;
        sharedNotebookTwo.setId(2);
        sharedNotebookTwo.setUserId(4);
        sharedNotebookTwo.setNotebookGuid(extraNotebook->guid());
        sharedNotebookTwo.setEmail("Fake shared notebook email two");
        sharedNotebookTwo.setCreationTimestamp(1);
        sharedNotebookTwo.setModificationTimestamp(1);
        sharedNotebookTwo.setShareKey("Fake shared notebook share key two");
        sharedNotebookTwo.setUsername("Fake shared notebook username two");
        sharedNotebookTwo.setPrivilegeLevel(1);
        sharedNotebookTwo.setAllowPreview(false);
        sharedNotebookTwo.setReminderNotifyEmail(false);
        sharedNotebookTwo.setReminderNotifyApp(true);

        extraNotebook->addSharedNotebook(sharedNotebookTwo);

        m_allInitialSharedNotebooks << sharedNotebookOne;
        m_allInitialSharedNotebooks << sharedNotebookTwo;

        m_initialSharedNotebooksPerNotebook << sharedNotebookOne;
        m_initialSharedNotebooksPerNotebook << sharedNotebookTwo;

        m_state = STATE_SENT_ADD_EXTRA_NOTEBOOK_ONE_REQUEST;
        emit addNotebookRequest(extraNotebook);
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
            emit failure(errorDescription);
            return;
        }

        m_pFoundNotebook = QSharedPointer<Notebook>(new Notebook);
        m_pFoundNotebook->setLocalGuid(notebook->localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findNotebookRequest(m_pFoundNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTEBOOK_ONE_REQUEST)
    {
        m_initialNotebooks << *notebook;

        QSharedPointer<Notebook> extraNotebook = QSharedPointer<Notebook>(new Notebook);
        extraNotebook->setGuid("00000000-0000-0000-c000-000000000002");
        extraNotebook->setUpdateSequenceNumber(2);
        extraNotebook->setName("Fake extra notebook two");
        extraNotebook->setCreationTimestamp(2);
        extraNotebook->setModificationTimestamp(2);
        extraNotebook->setDefaultNotebook(false);
        extraNotebook->setLastUsed(true);
        extraNotebook->setPublishingUri("Fake publishing uri two");
        extraNotebook->setPublishingOrder(1);
        extraNotebook->setPublishingAscending(false);
        extraNotebook->setPublishingPublicDescription("Fake public description two");
        extraNotebook->setStack("Fake notebook stack two");
        extraNotebook->setBusinessNotebookDescription("Fake business notebook description two");
        extraNotebook->setBusinessNotebookPrivilegeLevel(1);
        extraNotebook->setBusinessNotebookRecommended(false);

        SharedNotebookWrapper sharedNotebook;
        sharedNotebook.setId(3);
        sharedNotebook.setUserId(4);
        sharedNotebook.setNotebookGuid(extraNotebook->guid());
        sharedNotebook.setEmail("Fake shared notebook email three");
        sharedNotebook.setCreationTimestamp(2);
        sharedNotebook.setModificationTimestamp(2);
        sharedNotebook.setShareKey("Fake shared notebook share key three");
        sharedNotebook.setUsername("Fake shared notebook username three");
        sharedNotebook.setPrivilegeLevel(1);
        sharedNotebook.setAllowPreview(true);
        sharedNotebook.setReminderNotifyEmail(true);
        sharedNotebook.setReminderNotifyApp(false);

        m_allInitialSharedNotebooks << sharedNotebook;

        extraNotebook->addSharedNotebook(sharedNotebook);

        m_state = STATE_SENT_ADD_EXTRA_NOTEBOOK_TWO_REQUEST;
        emit addNotebookRequest(extraNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTEBOOK_TWO_REQUEST)
    {
        m_initialNotebooks << *notebook;

        m_state = STATE_SENT_LIST_NOTEBOOKS_REQUEST;
        emit listAllNotebooksRequest();
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
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pInitialNotebook.isNull());
        if (*m_pFoundNotebook != *m_pInitialNotebook) {
            errorDescription = "Added and found notebooks in local storage don't match";
            QNWARNING(errorDescription << ": Notebook added to LocalStorageManager: " << *m_pInitialNotebook
                      << "\nNotebook found in LocalStorageManager: " << *m_pFoundNotebook);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_ADD;
        emit findDefaultNotebookRequest(m_pFoundNotebook);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_pFoundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindNotebookCompleted slot doesn't match "
                               "the pointer to the original modified Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pModifiedNotebook.isNull());
        if (*m_pFoundNotebook != *m_pModifiedNotebook) {
            errorDescription = "Updated and found notebooks in local storage don't match";
            QNWARNING(errorDescription);
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
    Q_ASSERT_X(!notebook.isNull(), "NotebookLocalStorageManagerAsyncTester::onFindDefaultNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_ADD)
    {
        if (m_pFoundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindDefaultNotebookCompleted slot doesn't match "
                               "the pointer to the original added Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pInitialNotebook.isNull());
        if (*m_pFoundNotebook != *m_pInitialNotebook) {
            errorDescription = "Added and found notebooks in local storage don't match";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD;
        emit findLastUsedNotebookRequest(m_pFoundNotebook);
    }
    else if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_UPDATE)
    {
        errorDescription = "Error: found default notebook which should not have been in local storage";
        QNWARNING(errorDescription << ": Notebook found in LocalStorageManager: " << *notebook);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_UPDATE) {
        m_state = STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_UPDATE;
        emit findLastUsedNotebookRequest(m_pFoundNotebook);
        return;
    }

    QNWARNING(errorDescription << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_UPDATE)
    {
        if (m_pFoundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindLastUsedNotebookCompleted slot doesn't match "
                               "the pointer to the original modified Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pModifiedNotebook.isNull());
        if (*m_pFoundNotebook != *m_pModifiedNotebook) {
            errorDescription = "Updated and found notebooks in local storage don't match";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_UPDATE;
        emit findDefaultOrLastUsedNotebookRequest(m_pFoundNotebook);
    }
    else if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD)
    {
        errorDescription = "Error: found last used notebook which should not have been in LocalStorageManager";
        QNWARNING(errorDescription << ": Notebook found in LocalStorageManager: " << *notebook);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD) {
        m_state = STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD;
        emit findDefaultOrLastUsedNotebookRequest(m_pFoundNotebook);
        return;
    }

    QNWARNING(errorDescription << ": Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

    if ( (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD) ||
         (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_UPDATE) )
    {
        if (m_pFoundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                    "notebook pointer in onFindDefaultOrLastUsedNotebookCompleted slot doesn't match "
                    "the pointer to the original Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        if (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD)
        {
            Q_ASSERT(!m_pInitialNotebook.isNull());
            if (*m_pFoundNotebook != *m_pInitialNotebook) {
                errorDescription = "Added and found notebooks in local storage don't match";
                QNWARNING(errorDescription);
                emit failure(errorDescription);
                return;
            }

            // Ok, found notebook is good, modifying it now
            m_pModifiedNotebook = QSharedPointer<Notebook>(new Notebook(*m_pInitialNotebook));
            m_pModifiedNotebook->setUpdateSequenceNumber(m_pInitialNotebook->updateSequenceNumber() + 1);
            m_pModifiedNotebook->setName(m_pInitialNotebook->name() + "_modified");
            m_pModifiedNotebook->setDefaultNotebook(false);
            m_pModifiedNotebook->setLastUsed(true);
            m_pModifiedNotebook->setModificationTimestamp(m_pInitialNotebook->modificationTimestamp() + 1);
            m_pModifiedNotebook->setPublishingUri(m_pInitialNotebook->publishingUri() + "_modified");
            m_pModifiedNotebook->setPublishingAscending(!m_pInitialNotebook->isPublishingAscending());
            m_pModifiedNotebook->setPublishingPublicDescription(m_pInitialNotebook->publishingPublicDescription() + "_modified");
            m_pModifiedNotebook->setStack(m_pInitialNotebook->stack() + "_modified");
            m_pModifiedNotebook->setBusinessNotebookDescription(m_pInitialNotebook->businessNotebookDescription() + "_modified");

            m_state = STATE_SENT_UPDATE_REQUEST;
            emit updateNotebookRequest(m_pModifiedNotebook);
        }
        else
        {
            Q_ASSERT(!m_pModifiedNotebook.isNull());
            if (*m_pFoundNotebook != *m_pModifiedNotebook) {
                errorDescription = "Updated and found notebooks in local storage don't match";
                QNWARNING(errorDescription);
                emit failure(errorDescription);
                return;
            }

            m_state = STATE_SENT_EXPUNGE_REQUEST;
            emit expungeNotebookRequest(m_pModifiedNotebook);
        }
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ": Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onListAllNotebooksCompleted(QList<Notebook> notebooks)
{
    QString errorDescription;

    if (m_initialNotebooks.size() != notebooks.size()) {
        errorDescription = "Sizes of listed and reference notebooks don't match";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    foreach(const Notebook & notebook, m_initialNotebooks) {
        if (!notebooks.contains(notebook)) {
            QString errorDescription = "One of initial notebooks is not found within listed notebooks";
            QNWARNING(errorDescription << ", notebook which was not found: " << notebook);
            emit failure(errorDescription);
            return;
        }
    }

    m_state = STATE_SENT_LIST_ALL_SHARED_NOTEBOOKS_REQUEST;
    emit listAllSharedNotebooksRequest();
}

void NotebookLocalStorageManagerAsyncTester::onListAllNotebooksFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onListAllSharedNotebooksCompleted(QList<SharedNotebookWrapper> sharedNotebooks)
{
    QString errorDescription;

    if (m_allInitialSharedNotebooks.size() != sharedNotebooks.size()) {
        errorDescription = "Sizes of listed and reference shared notebooks don't match";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    foreach(const SharedNotebookWrapper & sharedNotebook, m_allInitialSharedNotebooks) {
        if (!sharedNotebooks.contains(sharedNotebook)) {
            errorDescription = "One of initial shared notebooks is not found within listed shared notebooks";
            QNWARNING(errorDescription << ", shared notebook which was not found: " << sharedNotebook);
            emit failure(errorDescription);
            return;
        }
    }

    m_state = STATE_SENT_LIST_SHARED_NOTEBOOKS_PER_NOTEBOOK_REQUEST;
    emit listSharedNotebooksPerNotebookRequest("00000000-0000-0000-c000-000000000001");
}

void NotebookLocalStorageManagerAsyncTester::onListAllSharedNotebooksFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onListSharedNotebooksPerNotebookGuidCompleted(QString notebookGuid,
                                                                                           QList<SharedNotebookWrapper> sharedNotebooks)
{
    QString errorDescription;

    if (m_initialSharedNotebooksPerNotebook.size() != sharedNotebooks.size()) {
        errorDescription = "Sizes of listed and reference shared notebooks don't match";
        QNWARNING(errorDescription << ", notebook guid = " << notebookGuid);
        emit failure(errorDescription);
        return;
    }

    foreach(const SharedNotebookWrapper & sharedNotebook, m_initialSharedNotebooksPerNotebook) {
        if (!sharedNotebooks.contains(sharedNotebook)) {
            errorDescription = "One of initial shared notebooks is not found within listed shared notebooks";
            QNWARNING(errorDescription << ", shared notebook which was not found: " << sharedNotebook
                      << ", notebook guid = " << notebookGuid);
            emit failure(errorDescription);
            return;
        }
    }

    emit success();
}

void NotebookLocalStorageManagerAsyncTester::onListSharedNotebooksPerNotebookGuidFailed(QString notebookGuid,
                                                                                        QString errorDescription)
{
    QNWARNING(errorDescription << ", notebook guid = " << notebookGuid);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onExpungeNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "NotebookLocalStorageManagerAsyncTester::onExpungeNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

    if (m_pModifiedNotebook != notebook) {
        errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                "notebook pointer in onExpungeNotebookCompleted slot doesn't match "
                "the pointer to the original expunged Notebook";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    Q_ASSERT(!m_pFoundNotebook.isNull());
    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findNotebookRequest(m_pFoundNotebook);
}

void NotebookLocalStorageManagerAsyncTester::onExpungeNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(getNotebookCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetNotebookCountRequest()));
    QObject::connect(this, SIGNAL(addNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onAddNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(updateNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(findNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onFindNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(findDefaultNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onFindDefaultNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(findLastUsedNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onFindLastUsedNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(findDefaultOrLastUsedNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onFindDefaultOrLastUsedNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(listAllNotebooksRequest()), m_pLocalStorageManagerThread,
                     SLOT(onListAllNotebooksRequest()));
    QObject::connect(this, SIGNAL(listAllSharedNotebooksRequest()), m_pLocalStorageManagerThread,
                     SLOT(onListAllSharedNotebooksRequest()));
    QObject::connect(this, SIGNAL(listSharedNotebooksPerNotebookRequest(QString)),
                     m_pLocalStorageManagerThread, SLOT(onListSharedNotebooksPerNotebookGuidRequest(QString)));
    QObject::connect(this, SIGNAL(expungeNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeNotebookRequest(QSharedPointer<Notebook>)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getNotebookCountComplete(int)),
                     this, SLOT(onGetNotebookCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getNotebookCountFailed(QString)),
                     this, SLOT(onGetNotebookCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onAddNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onAddNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onUpdateNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onUpdateNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onFindNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onFindNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findDefaultNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onFindDefaultNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findDefaultNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onFindDefaultNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findLastUsedNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onFindLastUsedNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findLastUsedNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onFindLastUsedNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findDefaultOrLastUsedNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onFindDefaultOrLastUsedNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findDefaultOrLastUsedNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onFindDefaultOrLastUsedNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllNotebooksComplete(QList<Notebook>)),
                     this, SLOT(onListAllNotebooksCompleted(QList<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllNotebooksFailed(QString)),
                     this, SLOT(onListAllNotebooksFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllSharedNotebooksComplete(QList<SharedNotebookWrapper>)),
                     this, SLOT(onListAllSharedNotebooksCompleted(QList<SharedNotebookWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllSharedNotebooksFailed(QString)),
                     this, SLOT(onListAllSharedNotebooksFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listSharedNotebooksPerNotebookGuidComplete(QString,QList<SharedNotebookWrapper>)),
                     this, SLOT(onListSharedNotebooksPerNotebookGuidCompleted(QString,QList<SharedNotebookWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listSharedNotebooksPerNotebookGuidFailed(QString,QString)),
                     this, SLOT(onListSharedNotebooksPerNotebookGuidFailed(QString,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onExpungeNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onExpungeNotebookFailed(QSharedPointer<Notebook>,QString)));
}

#undef HANDLE_WRONG_STATE

} // namespace qute_note
} // namespace test
