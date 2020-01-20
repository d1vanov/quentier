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

#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/Notebook.h>

#include <QHash>
#include <QList>
#include <QObject>
#include <QUuid>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)

/**
 * @brief The NotebookController class processes the notebook options for
 * wiki2account utility
 *
 * If target notebook name was specified, it ensures that such notebook exists
 * within the account. If number of new notebooks was specified, this class
 * creates these notebooks and returns them - with names and local uids
 */
class NotebookController: public QObject
{
    Q_OBJECT
public:
    explicit NotebookController(
        const QString & targetNotebookName,
        const quint32 numNotebooks,
        LocalStorageManagerAsync & localStorageManagerAsync,
        QObject * parent = nullptr);

    virtual ~NotebookController();

    const Notebook & targetNotebook() const { return m_targetNotebook; }
    const QList<Notebook> & newNotebooks() const { return m_newNotebooks; }

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);

    // private signals
    void listNotebooks(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void addNotebook(Notebook notebook, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onListNotebooksComplete(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
        QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onAddNotebookComplete(Notebook notebook, QUuid requestId);

    void onAddNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onFindNotebookComplete(Notebook foundNotebook, QUuid requestId);

    void onFindNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void clear();
    void createNextNewNotebook();

private:
    QString     m_targetNotebookName;
    quint32     m_numNewNotebooks;

    QHash<QString, QString>     m_notebookLocalUidsByNames;

    QUuid       m_findNotebookRequestId;
    QUuid       m_addNotebookRequestId;
    QUuid       m_listNotebooksRequestId;

    Notebook            m_targetNotebook;
    QList<Notebook>     m_newNotebooks;
    qint32      m_lastNewNotebookIndex;
};

} // namespace quentier
