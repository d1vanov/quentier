/*
 * Copyright 2019-2020 Dmitry Ivanov
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
    m_numNewNotebooks(numNotebooks)
{
    createConnections(localStorageManagerAsync);
}

NotebookController::~NotebookController()
{}

void NotebookController::start()
{
    QNDEBUG("wiki2account", "NotebookController::start");

    if (!m_targetNotebookName.isEmpty())
    {
        Notebook notebook;
        notebook.setLocalUid(QString());
        notebook.setName(m_targetNotebookName);

        m_findNotebookRequestId = QUuid::createUuid();

        QNDEBUG("wiki2account", "Emitting request to find notebook by name: "
            << m_targetNotebookName << ", request id = "
            << m_findNotebookRequestId);

        Q_EMIT findNotebook(notebook, m_findNotebookRequestId);
        return;
    }

    if (m_numNewNotebooks > 0)
    {
        m_listNotebooksRequestId = QUuid::createUuid();

        QNDEBUG("wiki2account", "Emitting request to list notebooks: "
            << "request id = " << m_listNotebooksRequestId);

        Q_EMIT listNotebooks(
            LocalStorageManager::ListObjectsOption::ListAll,
            0,
            0,
            LocalStorageManager::ListNotebooksOrder::NoOrder,
            LocalStorageManager::OrderDirection::Ascending,
            QString(),
            m_listNotebooksRequestId);

        return;
    }

    ErrorString errorDescription(
        QT_TR_NOOP("Neither target notebook name nor number of new notebooks "
                   "is set"));

    QNWARNING("wiki2account", errorDescription);
    Q_EMIT failure(errorDescription);
}

void NotebookController::onListNotebooksComplete(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
    QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG("wiki2account", "NotebookController::onListNotebooksComplete: "
        << "flag = " << flag << ", limit = " << limit << ", offset = " << offset
        << ", order = " << order << ", order direction = " << orderDirection
        << ", linked notebook guid = " << linkedNotebookGuid
        << ", num listed notebooks = " << foundNotebooks.size()
        << ", request id = " << requestId);

    m_listNotebooksRequestId = QUuid();

    m_notebookLocalUidsByNames.reserve(foundNotebooks.size());

    for(const auto & notebook: qAsConst(foundNotebooks))
    {
        if (Q_UNLIKELY(!notebook.hasName())) {
            continue;
        }

        m_notebookLocalUidsByNames[notebook.name()] = notebook.localUid();
    }

    createNextNewNotebook();
}

void NotebookController::onListNotebooksFailed(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription,
    QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNWARNING("wiki2account", "NotebookController::onListNotebooksFailed: "
        << "flag = " << flag << ", limit = " << limit << ", offset = " << offset
        << ", order = " << order << ", order direction = " << orderDirection
        << ", linked notebook guid = " << linkedNotebookGuid
        << ", error description = " << errorDescription
        << ", request id = " << requestId);

    clear();
    Q_EMIT failure(errorDescription);
}

void NotebookController::onAddNotebookComplete(
    Notebook notebook, QUuid requestId)
{
    if (requestId != m_addNotebookRequestId) {
        return;
    }

    QNDEBUG("wiki2account", "NotebookController::onAddNotebookComplete: "
        << notebook << "\nRequest id = " << requestId);

    m_addNotebookRequestId = QUuid();

    if (Q_UNLIKELY(!notebook.hasName())) {
        ErrorString errorDescription(
            QT_TR_NOOP("Created notebook has no name"));
        clear();
        Q_EMIT failure(errorDescription);
        return;
    }

    if (!m_targetNotebookName.isEmpty()) {
        m_targetNotebook = notebook;
        QNDEBUG("wiki2account", "Created target notebook, finishing");
        Q_EMIT finished();
        return;
    }

    m_newNotebooks.push_back(notebook);
    if (m_newNotebooks.size() == static_cast<int>(m_numNewNotebooks)) {
        QNDEBUG("wiki2account", "Created the last needed new notebook, "
            << "finishing");
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

    QNWARNING("wiki2account", "NotebookController::onAddNotebookFailed: "
        << errorDescription << ", request id = " << requestId << ", notebook: "
        << notebook);

    clear();
    Q_EMIT failure(errorDescription);
}

void NotebookController::onFindNotebookComplete(
    Notebook foundNotebook, QUuid requestId)
{
    if (requestId != m_findNotebookRequestId) {
        return;
    }

    QNDEBUG("wiki2account", "NotebookController::onFindNotebookComplete: "
        << foundNotebook << ", request id = " << requestId);

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

    QNDEBUG("wiki2account", "NotebookController::onFindNotebookFailed: "
        << errorDescription << ", request id = " << requestId << ", notebook: "
        << notebook);

    m_findNotebookRequestId = QUuid();

    notebook = Notebook();
    notebook.setName(m_targetNotebookName);
    m_addNotebookRequestId = QUuid::createUuid();

    QNDEBUG("wiki2account", "Emitting request to add notebook: " << notebook
        << "\nRequest id: " << m_addNotebookRequestId);

    Q_EMIT addNotebook(notebook, m_addNotebookRequestId);
}

void NotebookController::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QObject::connect(
        this,
        &NotebookController::listNotebooks,
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListNotebooksRequest);

    QObject::connect(
        this,
        &NotebookController::addNotebook,
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddNotebookRequest);

    QObject::connect(
        this,
        &NotebookController::findNotebook,
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNotebookRequest);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listNotebooksComplete,
        this,
        &NotebookController::onListNotebooksComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listNotebooksFailed,
        this,
        &NotebookController::onListNotebooksFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addNotebookComplete,
        this,
        &NotebookController::onAddNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addNotebookFailed,
        this,
        &NotebookController::onAddNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookComplete,
        this,
        &NotebookController::onFindNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookFailed,
        this,
        &NotebookController::onFindNotebookFailed);
}

void NotebookController::clear()
{
    QNDEBUG("wiki2account", "NotebookController::clear");

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
    QNDEBUG("wiki2account", "NotebookController::createNextNewNotebook: "
        << "index = " << m_lastNewNotebookIndex);

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

    QNDEBUG("wiki2account", "Emitting request to add notebook: "
        << notebook << "\nRequest id: " << m_addNotebookRequestId);

    Q_EMIT addNotebook(notebook, m_addNotebookRequestId);
}

} // namespace quentier
