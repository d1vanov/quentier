#include "NotebookLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

NotebookLocalStorageManagerAsyncTester::NotebookLocalStorageManagerAsyncTester(QObject *parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_initialNotebook(),
    m_foundNotebook(),
    m_modifiedNotebook(),
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

    m_initialNotebook.clear();
    m_initialNotebook.setGuid("00000000-0000-0000-c000-000000000047");
    m_initialNotebook.setUpdateSequenceNumber(1);
    m_initialNotebook.setName("Fake notebook name");
    m_initialNotebook.setCreationTimestamp(1);
    m_initialNotebook.setModificationTimestamp(1);
    m_initialNotebook.setDefaultNotebook(true);
    m_initialNotebook.setLastUsed(false);
    m_initialNotebook.setPublishingUri("Fake publishing uri");
    m_initialNotebook.setPublishingOrder(1);
    m_initialNotebook.setPublishingAscending(true);
    m_initialNotebook.setPublishingPublicDescription("Fake public description");
    m_initialNotebook.setPublished(true);
    m_initialNotebook.setStack("Fake notebook stack");
    m_initialNotebook.setBusinessNotebookDescription("Fake business notebook description");
    m_initialNotebook.setBusinessNotebookPrivilegeLevel(1);
    m_initialNotebook.setBusinessNotebookRecommended(true);

    SharedNotebookWrapper sharedNotebook;
    sharedNotebook.setId(1);
    sharedNotebook.setUserId(userId);
    sharedNotebook.setNotebookGuid(m_initialNotebook.guid());
    sharedNotebook.setEmail("Fake shared notebook email");
    sharedNotebook.setCreationTimestamp(1);
    sharedNotebook.setModificationTimestamp(1);
    sharedNotebook.setShareKey("Fake shared notebook share key");
    sharedNotebook.setUsername("Fake shared notebook username");
    sharedNotebook.setPrivilegeLevel(1);
    sharedNotebook.setAllowPreview(true);
    sharedNotebook.setReminderNotifyEmail(true);
    sharedNotebook.setReminderNotifyApp(false);

    m_initialNotebook.addSharedNotebook(sharedNotebook);

    QString errorDescription;
    if (!m_initialNotebook.checkParameters(errorDescription)) {
        QNWARNING("Found invalid notebook: " << m_initialNotebook << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addNotebookRequest(m_initialNotebook);
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
        emit findDefaultNotebookRequest(m_foundNotebook);
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

        Notebook extraNotebook;
        extraNotebook.setGuid("00000000-0000-0000-c000-000000000001");
        extraNotebook.setUpdateSequenceNumber(1);
        extraNotebook.setName("Fake extra notebook one");
        extraNotebook.setCreationTimestamp(1);
        extraNotebook.setModificationTimestamp(1);
        extraNotebook.setDefaultNotebook(true);
        extraNotebook.setLastUsed(false);
        extraNotebook.setPublishingUri("Fake publishing uri one");
        extraNotebook.setPublishingOrder(1);
        extraNotebook.setPublishingAscending(true);
        extraNotebook.setPublishingPublicDescription("Fake public description one");
        extraNotebook.setStack("Fake notebook stack one");
        extraNotebook.setBusinessNotebookDescription("Fake business notebook description one");
        extraNotebook.setBusinessNotebookPrivilegeLevel(1);
        extraNotebook.setBusinessNotebookRecommended(true);

        SharedNotebookWrapper sharedNotebookOne;
        sharedNotebookOne.setId(1);
        sharedNotebookOne.setUserId(4);
        sharedNotebookOne.setNotebookGuid(extraNotebook.guid());
        sharedNotebookOne.setEmail("Fake shared notebook email one");
        sharedNotebookOne.setCreationTimestamp(1);
        sharedNotebookOne.setModificationTimestamp(1);
        sharedNotebookOne.setShareKey("Fake shared notebook share key one");
        sharedNotebookOne.setUsername("Fake shared notebook username one");
        sharedNotebookOne.setPrivilegeLevel(1);
        sharedNotebookOne.setAllowPreview(true);
        sharedNotebookOne.setReminderNotifyEmail(true);
        sharedNotebookOne.setReminderNotifyApp(false);

        extraNotebook.addSharedNotebook(sharedNotebookOne);

        SharedNotebookWrapper sharedNotebookTwo;
        sharedNotebookTwo.setId(2);
        sharedNotebookTwo.setUserId(4);
        sharedNotebookTwo.setNotebookGuid(extraNotebook.guid());
        sharedNotebookTwo.setEmail("Fake shared notebook email two");
        sharedNotebookTwo.setCreationTimestamp(1);
        sharedNotebookTwo.setModificationTimestamp(1);
        sharedNotebookTwo.setShareKey("Fake shared notebook share key two");
        sharedNotebookTwo.setUsername("Fake shared notebook username two");
        sharedNotebookTwo.setPrivilegeLevel(1);
        sharedNotebookTwo.setAllowPreview(false);
        sharedNotebookTwo.setReminderNotifyEmail(false);
        sharedNotebookTwo.setReminderNotifyApp(true);

        extraNotebook.addSharedNotebook(sharedNotebookTwo);

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

void NotebookLocalStorageManagerAsyncTester::onAddNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook in onAddNotebookCompleted doesn't match the original Notebook";
            QNWARNING(errorDescription << "; original notebook: " << m_initialNotebook
                      << "\nFound notebook: " << notebook);
            emit failure(errorDescription);
            return;
        }

        m_foundNotebook = Notebook();
        m_foundNotebook.setLocalGuid(notebook.localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findNotebookRequest(m_foundNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTEBOOK_ONE_REQUEST)
    {
        m_initialNotebooks << notebook;

        Notebook extraNotebook;
        extraNotebook.setGuid("00000000-0000-0000-c000-000000000002");
        extraNotebook.setUpdateSequenceNumber(2);
        extraNotebook.setName("Fake extra notebook two");
        extraNotebook.setCreationTimestamp(2);
        extraNotebook.setModificationTimestamp(2);
        extraNotebook.setDefaultNotebook(false);
        extraNotebook.setLastUsed(true);
        extraNotebook.setPublishingUri("Fake publishing uri two");
        extraNotebook.setPublishingOrder(1);
        extraNotebook.setPublishingAscending(false);
        extraNotebook.setPublishingPublicDescription("Fake public description two");
        extraNotebook.setStack("Fake notebook stack two");
        extraNotebook.setBusinessNotebookDescription("Fake business notebook description two");
        extraNotebook.setBusinessNotebookPrivilegeLevel(1);
        extraNotebook.setBusinessNotebookRecommended(false);

        SharedNotebookWrapper sharedNotebook;
        sharedNotebook.setId(3);
        sharedNotebook.setUserId(4);
        sharedNotebook.setNotebookGuid(extraNotebook.guid());
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

        extraNotebook.addSharedNotebook(sharedNotebook);

        m_state = STATE_SENT_ADD_EXTRA_NOTEBOOK_TWO_REQUEST;
        emit addNotebookRequest(extraNotebook);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_NOTEBOOK_TWO_REQUEST)
    {
        m_initialNotebooks << notebook;

        m_state = STATE_SENT_LIST_NOTEBOOKS_REQUEST;
        emit listAllNotebooksRequest();
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onAddNotebookFailed(Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onUpdateNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onUpdateNotebookCompleted slot doesn't match "
                               "the pointer to the original modified Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findNotebookRequest(m_foundNotebook);
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onUpdateNotebookFailed(Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onFindNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_initialNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook in onFindNotebookCompleted slot doesn't match "
                               "the original Notebook";
            QNWARNING(errorDescription << "; original notebook: " << m_initialNotebook
                      << "\nFound notebook: " << notebook);
            emit failure(errorDescription);
            return;
        }

        m_foundNotebook = notebook;

        m_state = STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_ADD;
        emit findDefaultNotebookRequest(m_foundNotebook);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_modifiedNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindNotebookCompleted slot doesn't match "
                               "the pointer to the original modified Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_modifiedNotebook = notebook;
        m_foundNotebook = notebook;

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getNotebookCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription = "Error: found notebook which should have been expunged from local storage";
        QNWARNING(errorDescription << ": Notebook expunged from LocalStorageManager: " << m_modifiedNotebook
                  << "\nNotebook found in LocalStorageManager: " << m_foundNotebook);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindNotebookFailed(Notebook notebook, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getNotebookCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", notebook: " << notebook);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_ADD)
    {
        if (m_foundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindDefaultNotebookCompleted slot doesn't match "
                               "the pointer to the original added Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        if (m_foundNotebook != m_initialNotebook) {
            errorDescription = "Added and found notebooks in local storage don't match";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD;
        emit findLastUsedNotebookRequest(m_foundNotebook);
    }
    else if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_UPDATE)
    {
        errorDescription = "Error: found default notebook which should not have been in local storage";
        QNWARNING(errorDescription << ": Notebook found in LocalStorageManager: " << notebook);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultNotebookFailed(Notebook notebook, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_UPDATE) {
        m_state = STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_UPDATE;
        emit findLastUsedNotebookRequest(m_foundNotebook);
        return;
    }

    QNWARNING(errorDescription << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_UPDATE)
    {
        if (m_foundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindLastUsedNotebookCompleted slot doesn't match "
                               "the pointer to the original modified Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        if (m_foundNotebook != m_modifiedNotebook) {
            errorDescription = "Updated and found notebooks in local storage don't match";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_UPDATE;
        emit findDefaultOrLastUsedNotebookRequest(m_foundNotebook);
    }
    else if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD)
    {
        errorDescription = "Error: found last used notebook which should not have been in LocalStorageManager";
        QNWARNING(errorDescription << ": Notebook found in LocalStorageManager: " << notebook);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookFailed(Notebook notebook, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD) {
        m_state = STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD;
        emit findDefaultOrLastUsedNotebookRequest(m_foundNotebook);
        return;
    }

    QNWARNING(errorDescription << ": Notebook: " << notebook);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

    if ( (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD) ||
         (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_UPDATE) )
    {
        if (m_foundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                    "notebook pointer in onFindDefaultOrLastUsedNotebookCompleted slot doesn't match "
                    "the pointer to the original Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        if (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD)
        {
            if (m_foundNotebook != m_initialNotebook) {
                errorDescription = "Added and found notebooks in local storage don't match";
                QNWARNING(errorDescription);
                emit failure(errorDescription);
                return;
            }

            // Ok, found notebook is good, modifying it now
            m_modifiedNotebook = m_initialNotebook;
            m_modifiedNotebook.setUpdateSequenceNumber(m_initialNotebook.updateSequenceNumber() + 1);
            m_modifiedNotebook.setName(m_initialNotebook.name() + "_modified");
            m_modifiedNotebook.setDefaultNotebook(false);
            m_modifiedNotebook.setLastUsed(true);
            m_modifiedNotebook.setModificationTimestamp(m_initialNotebook.modificationTimestamp() + 1);
            m_modifiedNotebook.setPublishingUri(m_initialNotebook.publishingUri() + "_modified");
            m_modifiedNotebook.setPublishingAscending(!m_initialNotebook.isPublishingAscending());
            m_modifiedNotebook.setPublishingPublicDescription(m_initialNotebook.publishingPublicDescription() + "_modified");
            m_modifiedNotebook.setStack(m_initialNotebook.stack() + "_modified");
            m_modifiedNotebook.setBusinessNotebookDescription(m_initialNotebook.businessNotebookDescription() + "_modified");

            m_state = STATE_SENT_UPDATE_REQUEST;
            emit updateNotebookRequest(m_modifiedNotebook);
        }
        else
        {
            if (m_foundNotebook != m_modifiedNotebook) {
                errorDescription = "Updated and found notebooks in local storage don't match";
                QNWARNING(errorDescription);
                emit failure(errorDescription);
                return;
            }

            m_state = STATE_SENT_EXPUNGE_REQUEST;
            emit expungeNotebookRequest(m_modifiedNotebook);
        }
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookFailed(Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ": Notebook: " << notebook);
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

void NotebookLocalStorageManagerAsyncTester::onExpungeNotebookCompleted(Notebook notebook)
{
    QString errorDescription;

    if (m_modifiedNotebook != notebook) {
        errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                "notebook pointer in onExpungeNotebookCompleted slot doesn't match "
                "the pointer to the original expunged Notebook";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findNotebookRequest(m_foundNotebook);
}

void NotebookLocalStorageManagerAsyncTester::onExpungeNotebookFailed(Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void NotebookLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(failure(QString)),
                     this, SIGNAL(failure(QString)));

    // Request --> slot connections
    QObject::connect(this, SIGNAL(getNotebookCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetNotebookCountRequest()));
    QObject::connect(this, SIGNAL(addNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(updateNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onFindNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findDefaultNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onFindDefaultNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findLastUsedNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onFindLastUsedNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findDefaultOrLastUsedNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onFindDefaultOrLastUsedNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(listAllNotebooksRequest()), m_pLocalStorageManagerThread,
                     SLOT(onListAllNotebooksRequest()));
    QObject::connect(this, SIGNAL(listAllSharedNotebooksRequest()), m_pLocalStorageManagerThread,
                     SLOT(onListAllSharedNotebooksRequest()));
    QObject::connect(this, SIGNAL(listSharedNotebooksPerNotebookRequest(QString)),
                     m_pLocalStorageManagerThread, SLOT(onListSharedNotebooksPerNotebookGuidRequest(QString)));
    QObject::connect(this, SIGNAL(expungeNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeNotebookRequest(Notebook)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getNotebookCountComplete(int)),
                     this, SLOT(onGetNotebookCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getNotebookCountFailed(QString)),
                     this, SLOT(onGetNotebookCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookComplete(Notebook)),
                     this, SLOT(onAddNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookFailed(Notebook,QString)),
                     this, SLOT(onAddNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNotebookComplete(Notebook)),
                     this, SLOT(onUpdateNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateNotebookFailed(Notebook,QString)),
                     this, SLOT(onUpdateNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNotebookComplete(Notebook)),
                     this, SLOT(onFindNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findNotebookFailed(Notebook,QString)),
                     this, SLOT(onFindNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findDefaultNotebookComplete(Notebook)),
                     this, SLOT(onFindDefaultNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findDefaultNotebookFailed(Notebook,QString)),
                     this, SLOT(onFindDefaultNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findLastUsedNotebookComplete(Notebook)),
                     this, SLOT(onFindLastUsedNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findLastUsedNotebookFailed(Notebook,QString)),
                     this, SLOT(onFindLastUsedNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findDefaultOrLastUsedNotebookComplete(Notebook)),
                     this, SLOT(onFindDefaultOrLastUsedNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findDefaultOrLastUsedNotebookFailed(Notebook,QString)),
                     this, SLOT(onFindDefaultOrLastUsedNotebookFailed(Notebook,QString)));
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
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeNotebookComplete(Notebook)),
                     this, SLOT(onExpungeNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeNotebookFailed(Notebook,QString)),
                     this, SLOT(onExpungeNotebookFailed(Notebook,QString)));
}

#undef HANDLE_WRONG_STATE

} // namespace qute_note
} // namespace test
