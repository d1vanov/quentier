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
    m_numNewNotebooks(numNotebooks)
{
    createConnections(localStorageManagerAsync);
}

NotebookController::~NotebookController()
{}

void NotebookController::start()
{
    QNDEBUG(QStringLiteral("NotebookController::start"));

    // TODO: implement
}

void NotebookController::onListNotebooksComplete(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
    QUuid requestId)
{
    if (requestId != m_listNotebookRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookController::onListNotebooksComplete: flag = ")
            << flag << QStringLiteral(", limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ")
            << order << QStringLiteral(", order direction = ") << orderDirection
            << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid
            << QStringLiteral(", num listed notebooks = ") << foundNotebooks.size()
            << QStringLiteral(", request id = ") << requestId);

    m_listNotebookRequestId = QUuid();

    m_notebookLocalUidsByNames.reserve(foundNotebooks.size());
    for(auto it = foundNotebooks.constBegin(),
        end = foundNotebooks.constEnd(); it != end; ++it)
    {
        if (Q_UNLIKELY(!it->hasName())) {
            continue;
        }

        m_notebookLocalUidsByNames[it->name()] = it->localUid();
    }

    // TODO: continue
}

void NotebookController::onListNotebooksFailed(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription,
    QUuid requestId)
{
    if (requestId != m_listNotebookRequestId) {
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

    // TODO: create the next new notebook
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

    // TODO: try to add notebook with the target name
}

void NotebookController::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    // TODO: implement
    Q_UNUSED(localStorageManagerAsync)
}

void NotebookController::clear()
{
    QNDEBUG(QStringLiteral("NotebookController::clear"));

    m_notebookLocalUidsByNames.clear();

    m_findNotebookRequestId = QUuid();
    m_addNotebookRequestId = QUuid();
    m_listNotebookRequestId = QUuid();

    m_targetNotebook.clear();
    m_newNotebooks.clear();
}

} // namespace quentier
