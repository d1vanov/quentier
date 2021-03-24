/*
 * Copyright 2019-2021 Dmitry Ivanov
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

#include <quentier/local_storage/LocalStorageManager.h>

#include <qevercloud/types/Notebook.h>

#include <QHash>
#include <QList>
#include <QObject>
#include <QUuid>

namespace quentier {

class LocalStorageManagerAsync;

/**
 * @brief The NotebookController class processes the notebook options for
 * wiki2account utility
 *
 * If target notebook name was specified, it ensures that such notebook exists
 * within the account. If number of new notebooks was specified, this class
 * creates these notebooks and returns them - with names and local uids
 */
class NotebookController final : public QObject
{
    Q_OBJECT
public:
    explicit NotebookController(
        QString targetNotebookName, quint32 numNotebooks,
        LocalStorageManagerAsync & localStorageManagerAsync,
        QObject * parent = nullptr);

    ~NotebookController() override;

    [[nodiscard]] const qevercloud::Notebook & targetNotebook() const noexcept
    {
        return m_targetNotebook;
    }

    [[nodiscard]] const QList<qevercloud::Notebook> & newNotebooks()
        const noexcept
    {
        return m_newNotebooks;
    }

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);

    // private signals
    void listNotebooks(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void addNotebook(qevercloud::Notebook notebook, QUuid requestId);
    void findNotebook(qevercloud::Notebook notebook, QUuid requestId);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onListNotebooksComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<qevercloud::Notebook> foundNotebooks,
        QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onAddNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onAddNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onFindNotebookComplete(
        qevercloud::Notebook foundNotebook, QUuid requestId);

    void onFindNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void clear();
    void createNextNewNotebook();

private:
    QString m_targetNotebookName;
    quint32 m_numNewNotebooks;

    QHash<QString, QString> m_notebookLocalUidsByNames;

    QUuid m_findNotebookRequestId;
    QUuid m_addNotebookRequestId;
    QUuid m_listNotebooksRequestId;

    qevercloud::Notebook m_targetNotebook;
    QList<qevercloud::Notebook> m_newNotebooks;
    qint32 m_lastNewNotebookIndex = 1;
};

} // namespace quentier
