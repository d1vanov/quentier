/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NotebookLocalStorageManagerAsyncTester.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <QThread>

namespace quentier {
namespace test {

NotebookLocalStorageManagerAsyncTester::NotebookLocalStorageManagerAsyncTester(QObject *parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThreadWorker(Q_NULLPTR),
    m_pLocalStorageManagerThread(Q_NULLPTR),
    m_userId(4),
    m_initialNotebook(),
    m_foundNotebook(),
    m_modifiedNotebook(),
    m_initialNotebooks(),
    m_allInitialSharedNotebooks(),
    m_initialSharedNotebooksPerNotebook()
{}

NotebookLocalStorageManagerAsyncTester::~NotebookLocalStorageManagerAsyncTester()
{
    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
    }
}

void NotebookLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "NotebookLocalStorageManagerAsyncTester";
    bool startFromScratch = true;
    bool overrideLock = false;

    if (m_pLocalStorageManagerThread) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = Q_NULLPTR;
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
        m_pLocalStorageManagerThreadWorker = Q_NULLPTR;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new QThread(this);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(username, m_userId, startFromScratch, overrideLock);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();
}

void NotebookLocalStorageManagerAsyncTester::onWorkerInitialized()
{
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

    SharedNotebook sharedNotebook;
    sharedNotebook.setId(1);
    sharedNotebook.setUserId(m_userId);
    sharedNotebook.setNotebookGuid(m_initialNotebook.guid());
    sharedNotebook.setEmail("Fake shared notebook email");
    sharedNotebook.setCreationTimestamp(1);
    sharedNotebook.setModificationTimestamp(1);
    sharedNotebook.setGlobalId("Fake shared notebook global id");
    sharedNotebook.setUsername("Fake shared notebook username");
    sharedNotebook.setPrivilegeLevel(1);
    sharedNotebook.setReminderNotifyEmail(true);
    sharedNotebook.setReminderNotifyApp(false);

    m_initialNotebook.addSharedNotebook(sharedNotebook);

    QNLocalizedString errorDescription;
    if (!m_initialNotebook.checkParameters(errorDescription)) {
        QNWARNING("Found invalid notebook: " << m_initialNotebook << ", error: " << errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addNotebookRequest(m_initialNotebook);
}

void NotebookLocalStorageManagerAsyncTester::onGetNotebookCountCompleted(int count, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        QNWARNING(errorDescription << ": " << m_state); \
        emit failure(errorDescription.nonLocalizedString()); \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = "GetNotebookCount returned result different from the expected one (1): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
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
            emit failure(errorDescription.nonLocalizedString());
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

        SharedNotebook sharedNotebookOne;
        sharedNotebookOne.setId(1);
        sharedNotebookOne.setUserId(4);
        sharedNotebookOne.setNotebookGuid(extraNotebook.guid());
        sharedNotebookOne.setEmail("Fake shared notebook email one");
        sharedNotebookOne.setCreationTimestamp(1);
        sharedNotebookOne.setModificationTimestamp(1);
        sharedNotebookOne.setGlobalId("Fake shared notebook global id one");
        sharedNotebookOne.setUsername("Fake shared notebook username one");
        sharedNotebookOne.setPrivilegeLevel(1);
        sharedNotebookOne.setReminderNotifyEmail(true);
        sharedNotebookOne.setReminderNotifyApp(false);

        extraNotebook.addSharedNotebook(sharedNotebookOne);

        SharedNotebook sharedNotebookTwo;
        sharedNotebookTwo.setId(2);
        sharedNotebookTwo.setUserId(4);
        sharedNotebookTwo.setNotebookGuid(extraNotebook.guid());
        sharedNotebookTwo.setEmail("Fake shared notebook email two");
        sharedNotebookTwo.setCreationTimestamp(1);
        sharedNotebookTwo.setModificationTimestamp(1);
        sharedNotebookTwo.setGlobalId("Fake shared notebook global id two");
        sharedNotebookTwo.setUsername("Fake shared notebook username two");
        sharedNotebookTwo.setPrivilegeLevel(1);
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

void NotebookLocalStorageManagerAsyncTester::onGetNotebookCountFailed(QNLocalizedString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onAddNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook in onAddNotebookCompleted doesn't match the original Notebook";
            QNWARNING(errorDescription << "; original notebook: " << m_initialNotebook
                      << "\nFound notebook: " << notebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_foundNotebook = Notebook();
        m_foundNotebook.setLocalUid(notebook.localUid());

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

        SharedNotebook sharedNotebook;
        sharedNotebook.setId(3);
        sharedNotebook.setUserId(4);
        sharedNotebook.setNotebookGuid(extraNotebook.guid());
        sharedNotebook.setEmail("Fake shared notebook email three");
        sharedNotebook.setCreationTimestamp(2);
        sharedNotebook.setModificationTimestamp(2);
        sharedNotebook.setGlobalId("Fake shared notebook global id three");
        sharedNotebook.setUsername("Fake shared notebook username three");
        sharedNotebook.setPrivilegeLevel(1);
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
        size_t limit = 0, offset = 0;
        LocalStorageManager::ListNotebooksOrder::type order = LocalStorageManager::ListNotebooksOrder::NoOrder;
        LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;
        QString linkedNotebookGuid;
        emit listAllNotebooksRequest(limit, offset, order, orderDirection, linkedNotebookGuid);
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onAddNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Notebook: " << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onUpdateNotebookCompleted slot doesn't match "
                               "the pointer to the original modified Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findNotebookRequest(m_foundNotebook);
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onUpdateNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Notebook: " << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_initialNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook in onFindNotebookCompleted slot doesn't match "
                               "the original Notebook";
            QNWARNING(errorDescription << "; original notebook: " << m_initialNotebook
                      << "\nFound notebook: " << notebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Attempt to find notebook by name now
        Notebook notebookToFindByName;
        notebookToFindByName.unsetLocalUid();
        notebookToFindByName.setName(m_initialNotebook.name());

        m_state = STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST;
        emit findNotebookRequest(notebook);
    }
    else if (m_state == STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST)
    {
        if (m_initialNotebook != notebook) {
            errorDescription = "Added and found by name notebooks in local storage don't match";
            QNWARNING(errorDescription << ": Notebook added to LocalStorageManager: " << m_initialNotebook
                      << "\nNotebook found in LocalStorageManager: " << notebook);
            emit failure(errorDescription.nonLocalizedString());
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
            emit failure(errorDescription.nonLocalizedString());
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
        emit failure(errorDescription.nonLocalizedString());
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getNotebookCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", requestId = " << requestId << ", Notebook: " << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_ADD)
    {
        if (m_foundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindDefaultNotebookCompleted slot doesn't match "
                               "the pointer to the original added Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        if (m_foundNotebook != m_initialNotebook) {
            errorDescription = "Added and found notebooks in local storage don't match";
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_state = STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD;
        emit findLastUsedNotebookRequest(m_foundNotebook);
    }
    else if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_UPDATE)
    {
        errorDescription = "Error: found default notebook which should not have been in local storage";
        QNWARNING(errorDescription << ": Notebook found in LocalStorageManager: " << notebook);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    if (m_state == STATE_SENT_FIND_DEFAULT_NOTEBOOK_AFTER_UPDATE) {
        m_state = STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_UPDATE;
        emit findLastUsedNotebookRequest(m_foundNotebook);
        return;
    }

    QNWARNING(errorDescription << ", requestId = " << requestId << ", Notebook: " << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_UPDATE)
    {
        if (m_foundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                               "notebook pointer in onFindLastUsedNotebookCompleted slot doesn't match "
                               "the pointer to the original modified Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        if (m_foundNotebook != m_modifiedNotebook) {
            errorDescription = "Updated and found notebooks in local storage don't match";
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_state = STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_UPDATE;
        emit findDefaultOrLastUsedNotebookRequest(m_foundNotebook);
    }
    else if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD)
    {
        errorDescription = "Error: found last used notebook which should not have been in LocalStorageManager";
        QNWARNING(errorDescription << ": Notebook found in LocalStorageManager: " << notebook);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindLastUsedNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    if (m_state == STATE_SENT_FIND_LAST_USED_NOTEBOOK_AFTER_ADD) {
        m_state = STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD;
        emit findDefaultOrLastUsedNotebookRequest(m_foundNotebook);
        return;
    }

    QNWARNING(errorDescription << ", requestId = " << requestId << ", Notebook: " << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if ( (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD) ||
         (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_UPDATE) )
    {
        if (m_foundNotebook != notebook) {
            errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                    "notebook pointer in onFindDefaultOrLastUsedNotebookCompleted slot doesn't match "
                    "the pointer to the original Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        if (m_state == STATE_SENT_FIND_DEFAULT_OR_LAST_USED_NOTEBOOK_AFTER_ADD)
        {
            if (m_foundNotebook != m_initialNotebook) {
                errorDescription = "Added and found notebooks in local storage don't match";
                QNWARNING(errorDescription);
                emit failure(errorDescription.nonLocalizedString());
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
                emit failure(errorDescription.nonLocalizedString());
                return;
            }

            m_state = STATE_SENT_EXPUNGE_REQUEST;
            emit expungeNotebookRequest(m_modifiedNotebook);
        }
    }
    HANDLE_WRONG_STATE();
}

void NotebookLocalStorageManagerAsyncTester::onFindDefaultOrLastUsedNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Notebook: " << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onListAllNotebooksCompleted(size_t limit, size_t offset,
                                                                         LocalStorageManager::ListNotebooksOrder::type order,
                                                                         LocalStorageManager::OrderDirection::type orderDirection,
                                                                         QString linkedNotebookGuid, QList<Notebook> notebooks, QUuid requestId)
{
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(linkedNotebookGuid)
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_initialNotebooks.size() != notebooks.size()) {
        errorDescription = "Sizes of listed and reference notebooks don't match";
        QNWARNING(errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    foreach(const Notebook & notebook, m_initialNotebooks) {
        if (!notebooks.contains(notebook)) {
            QNLocalizedString errorDescription = "One of initial notebooks is not found within listed notebooks";
            QNWARNING(errorDescription << ", notebook which was not found: " << notebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
    }

    m_state = STATE_SENT_LIST_ALL_SHARED_NOTEBOOKS_REQUEST;
    emit listAllSharedNotebooksRequest();
}

void NotebookLocalStorageManagerAsyncTester::onListAllNotebooksFailed(size_t limit, size_t offset,
                                                                      LocalStorageManager::ListNotebooksOrder::type order,
                                                                      LocalStorageManager::OrderDirection::type orderDirection,
                                                                      QString linkedNotebookGuid,
                                                                      QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(linkedNotebookGuid)

    QNWARNING(errorDescription << ", requestId = " << requestId);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onListAllSharedNotebooksCompleted(QList<SharedNotebook> sharedNotebooks, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_allInitialSharedNotebooks.size() != sharedNotebooks.size()) {
        errorDescription = "Sizes of listed and reference shared notebooks don't match";
        QNWARNING(errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    for(auto it = m_allInitialSharedNotebooks.constBegin(), end = m_allInitialSharedNotebooks.constEnd(); it != end; ++it)
    {
        const SharedNotebook & sharedNotebook = *it;
        if (!sharedNotebooks.contains(sharedNotebook)) {
            errorDescription = "One of initial shared notebooks is not found within listed shared notebooks";
            QNWARNING(errorDescription << ", shared notebook which was not found: " << sharedNotebook);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
    }

    m_state = STATE_SENT_LIST_SHARED_NOTEBOOKS_PER_NOTEBOOK_REQUEST;
    emit listSharedNotebooksPerNotebookRequest("00000000-0000-0000-c000-000000000001");
}

void NotebookLocalStorageManagerAsyncTester::onListAllSharedNotebooksFailed(QNLocalizedString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onListSharedNotebooksPerNotebookGuidCompleted(QString notebookGuid,
                                                                                           QList<SharedNotebook> sharedNotebooks,
                                                                                           QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_initialSharedNotebooksPerNotebook.size() != sharedNotebooks.size()) {
        errorDescription = "Sizes of listed and reference shared notebooks don't match";
        QNWARNING(errorDescription << ", notebook guid = " << notebookGuid);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    for(auto it = m_initialSharedNotebooksPerNotebook.constBegin(), end = m_initialSharedNotebooksPerNotebook.constEnd();
        it != end; ++it)
    {
        const SharedNotebook & sharedNotebook = *it;
        if (!sharedNotebooks.contains(sharedNotebook)) {
            errorDescription = "One of initial shared notebooks is not found within listed shared notebooks";
            QNWARNING(errorDescription << ", shared notebook which was not found: " << sharedNotebook
                      << ", notebook guid = " << notebookGuid);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
    }

    emit success();
}

void NotebookLocalStorageManagerAsyncTester::onListSharedNotebooksPerNotebookGuidFailed(QString notebookGuid,
                                                                                        QNLocalizedString errorDescription,
                                                                                        QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", notebook guid = " << notebookGuid);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onExpungeNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    QNLocalizedString errorDescription;

    if (m_modifiedNotebook != notebook) {
        errorDescription = "Internal error in NotebookLocalStorageManagerAsyncTester: "
                "notebook pointer in onExpungeNotebookCompleted slot doesn't match "
                "the pointer to the original expunged Notebook";
        QNWARNING(errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findNotebookRequest(m_foundNotebook);
}

void NotebookLocalStorageManagerAsyncTester::onExpungeNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Notebook: " << notebook);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::onFailure(QNLocalizedString errorDescription)
{
    QNWARNING("NotebookLocalStorageManagerAsyncTester::onFailure: " << errorDescription);
    emit failure(errorDescription.nonLocalizedString());
}

void NotebookLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,failure,QNLocalizedString),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFailure,QNLocalizedString));

    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,started), m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,init));
    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished), m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));

    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,initialized),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onWorkerInitialized));

    // Request --> slot connections
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,getNotebookCountRequest,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onGetNotebookCountRequest,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,addNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,updateNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,findNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,findDefaultNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindDefaultNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,findLastUsedNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindLastUsedNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,findDefaultOrLastUsedNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindDefaultOrLastUsedNotebookRequest,Notebook,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NotebookLocalStorageManagerAsyncTester,listAllNotebooksRequest,size_t,size_t,
                              LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     m_pLocalStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListAllNotebooksRequest,size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                            LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,listAllSharedNotebooksRequest,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListAllSharedNotebooksRequest,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,listSharedNotebooksPerNotebookRequest,QString,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListSharedNotebooksPerNotebookGuidRequest,QString,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,expungeNotebookRequest,Notebook,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNotebookRequest,Notebook,QUuid));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,getNotebookCountComplete,int,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onGetNotebookCountCompleted,int,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,getNotebookCountFailed,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onGetNotebookCountFailed,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onAddNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onAddNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onUpdateNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onUpdateNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFindNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findDefaultNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFindDefaultNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findDefaultNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFindDefaultNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findLastUsedNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFindLastUsedNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findLastUsedNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFindLastUsedNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findDefaultOrLastUsedNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFindDefaultOrLastUsedNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findDefaultOrLastUsedNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onFindDefaultOrLastUsedNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listAllNotebooksComplete,size_t,size_t,
                              LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,
                              QString,QList<Notebook>,QUuid),
                     this,
                     QNSLOT(NotebookLocalStorageManagerAsyncTester,onListAllNotebooksCompleted,size_t,size_t,
                            LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,
                            QString,QList<Notebook>,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listAllNotebooksFailed,size_t,size_t,
                              LocalStorageManager::ListNotebooksOrder::type,
                              LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid),
                     this,
                     QNSLOT(NotebookLocalStorageManagerAsyncTester,onListAllNotebooksFailed,size_t,size_t,
                            LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,
                            QString,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listAllSharedNotebooksComplete,
                                                                  QList<SharedNotebook>,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onListAllSharedNotebooksCompleted,
                                  QList<SharedNotebook>,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listAllSharedNotebooksFailed,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onListAllSharedNotebooksFailed,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSharedNotebooksPerNotebookGuidComplete,
                                                                  QString,QList<SharedNotebook>,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onListSharedNotebooksPerNotebookGuidCompleted,
                                  QString,QList<SharedNotebook>,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSharedNotebooksPerNotebookGuidFailed,
                                                                  QString,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onListSharedNotebooksPerNotebookGuidFailed,QString,QNLocalizedString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onExpungeNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookLocalStorageManagerAsyncTester,onExpungeNotebookFailed,Notebook,QNLocalizedString,QUuid));
}

#undef HANDLE_WRONG_STATE

} // namespace quentier
} // namespace test
