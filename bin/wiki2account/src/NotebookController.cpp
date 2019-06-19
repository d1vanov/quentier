/*
 * Copyright 2019 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NotebookController.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

NotebookController::NotebookController(
        const QString & targetNotebookName,
        const quint32 numNotebooks,
        LocalStorageManagerAsync & localStorageManagerAsync,
        QObject * parent) :
    QObject(parent),
    m_targetNotebookName(targetNotebookName),
    m_numNewNotebooks(numNotebooks),
    m_lastNewNotebookIndex(1)
{
    createConnections(localStorageManagerAsync);
}

NotebookController::~NotebookController()
{}

void NotebookController::start()
{
    QNDEBUG(QStringLiteral("NotebookController::start"));

    if (!m_targetNotebookName.isEmpty())
    {
        Notebook notebook;
        notebook.setLocalUid(QString());
        notebook.setName(m_targetNotebookName);
        m_findNotebookRequestId = QUuid::createUuid();
        QNDEBUG(QStringLiteral("Emitting request to find notebook by name: ")
                << m_targetNotebookName << QStringLiteral(", request id = ")
                << m_findNotebookRequestId);
        Q_EMIT findNotebook(notebook, m_findNotebookRequestId);
        return;
    }

    if (m_numNewNotebooks > 0)
    {
        m_listNotebooksRequestId = QUuid::createUuid();
        QNDEBUG(QStringLiteral("Emitting request to list notebooks: request id = ")
                << m_listNotebooksRequestId);
        Q_EMIT listNotebooks(LocalStorageManager::ListObjectsOption::ListAll, 0,
                             0, LocalStorageManager::ListNotebooksOrder::NoOrder,
                             LocalStorageManager::OrderDirection::Ascending,
                             QString(), m_listNotebooksRequestId);
        return;
    }

    ErrorString errorDescription(QT_TR_NOOP("Neither target notebook name nor "
                                            "number of new notebooks is set"));
    QNWARNING(errorDescription);
    Q_EMIT failure(errorDescription);
}

void NotebookController::onListNotebooksComplete(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
    QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookController::onListNotebooksComplete: flag = ")
            << flag << QStringLiteral(", limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ")
            << order << QStringLiteral(", order direction = ") << orderDirection
            << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid
            << QStringLiteral(", num listed notebooks = ") << foundNotebooks.size()
            << QStringLiteral(", request id = ") << requestId);

    m_listNotebooksRequestId = QUuid();

    m_notebookLocalUidsByNames.reserve(foundNotebooks.size());
    for(auto it = foundNotebooks.constBegin(),
        end = foundNotebooks.constEnd(); it != end; ++it)
    {
        if (Q_UNLIKELY(!it->hasName())) {
            continue;
        }

        m_notebookLocalUidsByNames[it->name()] = it->localUid();
    }

    createNextNewNotebook();
}

void NotebookController::onListNotebooksFailed(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription,
    QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNWARNING(QStringLiteral("NotebookController::onListNotebooksFailed: flag = ")
              << flag << QStringLiteral(", limit = ") << limit
              << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ")
              << order << QStringLiteral(", order direction = ") << orderDirection
              << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid
              << QStringLiteral(", error description = ") << errorDescription
              << QStringLiteral(", request id = ") << requestId);

    clear();
    Q_EMIT failure(errorDescription);
}

void NotebookController::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookController::onAddNotebookComplete: ")
            << notebook << QStringLiteral("\nRequest id = ") << requestId);

    m_addNotebookRequestId = QUuid();

    if (Q_UNLIKELY(!notebook.hasName())) {
        ErrorString errorDescription(QT_TR_NOOP("Created notebook has no name"));
        clear();
        Q_EMIT failure(errorDescription);
        return;
    }

    if (!m_targetNotebookName.isEmpty()) {
        m_targetNotebook = notebook;
        QNDEBUG(QStringLiteral("Created target notebook, finishing"));
        Q_EMIT finished();
        return;
    }

    m_newNotebooks.push_back(notebook);
    if (m_newNotebooks.size() == static_cast<int>(m_numNewNotebooks)) {
        QNDEBUG(QStringLiteral("Created the last needed new notebook, finishing"));
        Q_EMIT finished();
        return;
    }

    m_notebookLocalUidsByNames[notebook.name()] = notebook.localUid();
    createNextNewNotebook();
}

void NotebookController::onAddNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNWARNING(QStringLiteral("NotebookController::onAddNotebookFailed: ")
              << errorDescription << QStringLiteral(", request id = ")
              << requestId << QStringLiteral(", notebook: ") << notebook);

    clear();
    Q_EMIT failure(errorDescription);
}

void NotebookController::onFindNotebookComplete(
    Notebook foundNotebook, QUuid requestId)
{
    if (requestId != m_findNotebookRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookController::onFindNotebookComplete: ")
            << foundNotebook << QStringLiteral(", request id = ") << requestId);

    m_findNotebookRequestId = QUuid();

    m_targetNotebook = foundNotebook;
    Q_EMIT finished();
}

void NotebookController::onFindNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_findNotebookRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookController::onFindNotebookFailed: ")
            << errorDescription << QStringLiteral(", request id = ") << requestId
            << QStringLiteral(", notebook: ") << notebook);

    m_findNotebookRequestId = QUuid();

    notebook = Notebook();
    notebook.setName(m_targetNotebookName);
    m_addNotebookRequestId = QUuid::createUuid();
    QNDEBUG(QStringLiteral("Emitting request to add notebook: ")
            << notebook << QStringLiteral("\nRequest id: ")
            << m_addNotebookRequestId);
    Q_EMIT addNotebook(notebook, m_addNotebookRequestId);
}

void NotebookController::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QObject::connect(this,
                     QNSIGNAL(NotebookController,listNotebooks,
                              LocalStorageManager::ListObjectsOptions,size_t,
                              size_t,LocalStorageManager::ListNotebooksOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onListNotebooksRequest,
                            LocalStorageManager::ListObjectsOptions,size_t,
                            size_t,LocalStorageManager::ListNotebooksOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NotebookController,addNotebook,Notebook,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onAddNotebookRequest,
                            Notebook,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NotebookController,findNotebook,Notebook,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindNotebookRequest,
                            Notebook,QUuid));

    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listNotebooksComplete,
                              LocalStorageManager::ListObjectsOptions,size_t,
                              size_t,LocalStorageManager::ListNotebooksOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,QList<Notebook>,QUuid),
                     this,
                     QNSLOT(NotebookController,onListNotebooksComplete,
                            LocalStorageManager::ListObjectsOptions,size_t,
                            size_t,LocalStorageManager::ListNotebooksOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,QList<Notebook>,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listNotebooksFailed,
                              LocalStorageManager::ListObjectsOptions,size_t,
                              size_t,LocalStorageManager::ListNotebooksOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,ErrorString,QUuid),
                     this,
                     QNSLOT(NotebookController,onListNotebooksFailed,
                            LocalStorageManager::ListObjectsOptions,size_t,
                            size_t,LocalStorageManager::ListNotebooksOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NotebookController,onAddNotebookComplete,
                            Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(NotebookController,onAddNotebookFailed,
                            Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NotebookController,onFindNotebookComplete,
                            Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(NotebookController,onFindNotebookFailed,
                            Notebook,ErrorString,QUuid));
}

void NotebookController::clear()
{
    QNDEBUG(QStringLiteral("NotebookController::clear"));

    m_notebookLocalUidsByNames.clear();

    m_findNotebookRequestId = QUuid();
    m_addNotebookRequestId = QUuid();
    m_listNotebooksRequestId = QUuid();

    m_targetNotebook.clear();
    m_newNotebooks.clear();

    m_lastNewNotebookIndex = 1;
}

void NotebookController::createNextNewNotebook()
{
    QNDEBUG(QStringLiteral("NotebookController::createNextNewNotebook: index = ")
            << m_lastNewNotebookIndex);

    QString baseName = QStringLiteral("wiki notes notebook");
    QString name;
    qint32 index = m_lastNewNotebookIndex;

    while(true)
    {
        name = baseName;
        if (index > 1) {
            name += QStringLiteral(" #") + QString::number(index);
        }

        auto it = m_notebookLocalUidsByNames.find(name);
        if (it == m_notebookLocalUidsByNames.end()) {
            break;
        }

        ++index;
    }

    Notebook notebook;
    notebook.setName(name);
    m_addNotebookRequestId = QUuid::createUuid();
    QNDEBUG(QStringLiteral("Emitting request to add notebook: ")
            << notebook << QStringLiteral("\nRequest id: ")
            << m_addNotebookRequestId);
    Q_EMIT addNotebook(notebook, m_addNotebookRequestId);
}

} // namespace quentier
