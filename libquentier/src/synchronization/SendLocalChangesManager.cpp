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

#include "SendLocalChangesManager.h"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/Utility.h>
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <QTimerEvent>

namespace quentier {

SendLocalChangesManager::SendLocalChangesManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                 QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                 QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_noteStore(pNoteStore),
    m_lastUpdateCount(0),
    m_lastUpdateCountByLinkedNotebookGuid(),
    m_shouldRepeatIncrementalSync(false),
    m_active(false),
    m_paused(false),
    m_requestedToStop(false),
    m_connectedToLocalStorage(false),
    m_receivedDirtyLocalStorageObjectsFromUsersAccount(false),
    m_receivedAllDirtyLocalStorageObjects(false),
    m_listDirtyTagsRequestId(),
    m_listDirtySavedSearchesRequestId(),
    m_listDirtyNotebooksRequestId(),
    m_listDirtyNotesRequestId(),
    m_listLinkedNotebooksRequestId(),
    m_listDirtyTagsFromLinkedNotebooksRequestIds(),
    m_listDirtyNotebooksFromLinkedNotebooksRequestIds(),
    m_listDirtyNotesFromLinkedNotebooksRequestIds(),
    m_tags(),
    m_savedSearches(),
    m_notebooks(),
    m_notes(),
    m_linkedNotebookGuidsAndSharedNotebookGlobalIds(),
    m_lastProcessedLinkedNotebookGuidIndex(-1),
    m_authenticationTokensAndShardIdsByLinkedNotebookGuid(),
    m_authenticationTokenExpirationTimesByLinkedNotebookGuid(),
    m_pendingAuthenticationTokensForLinkedNotebooks(false),
    m_updateTagRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_updateNotebookRequestIds(),
    m_updateNoteRequestIds(),
    m_findNotebookRequestIds(),
    m_notebooksByGuidsCache(),
    m_sendTagsPostponeTimerId(0),
    m_sendSavedSearchesPostponeTimerId(0),
    m_sendNotebooksPostponeTimerId(0),
    m_sendNotesPostponeTimerId(0)
{}

bool SendLocalChangesManager::active() const
{
    return m_active;
}

void SendLocalChangesManager::start(const qint32 updateCount,
                                    QHash<QString,qint32> updateCountByLinkedNotebookGuid)
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::start: update count = ") << updateCount
            << QStringLiteral(", update count by linked notebook guid = ") << updateCountByLinkedNotebookGuid);

    if (m_paused) {
        m_lastUpdateCount = updateCount;
        m_lastUpdateCountByLinkedNotebookGuid = updateCountByLinkedNotebookGuid;
        resume();
        return;
    }

    clear();
    m_active = true;
    m_lastUpdateCount = updateCount;
    m_lastUpdateCountByLinkedNotebookGuid = updateCountByLinkedNotebookGuid;

    requestStuffFromLocalStorage();
}

void SendLocalChangesManager::pause()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::pause"));
    m_paused = true;
    m_active = false;
    emit paused(/* pending authentication = */ false);
}

void SendLocalChangesManager::stop()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::stop"));
    m_requestedToStop = true;
    m_active = false;
    emit stopped();
}

void SendLocalChangesManager::resume()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::resume"));

    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    if (m_paused)
    {
        m_active = true;
        m_paused = false;

        if (m_receivedAllDirtyLocalStorageObjects) {
            sendLocalChanges();
        }
        else {
            start(m_lastUpdateCount, m_lastUpdateCountByLinkedNotebookGuid);
        }
    }
}

void SendLocalChangesManager::onAuthenticationTokensForLinkedNotebooksReceived(QHash<QString,QPair<QString,QString> > authenticationTokensByLinkedNotebookGuid,
                                                                               QHash<QString,qevercloud::Timestamp> authenticationTokenExpirationTimesByLinkedNotebookGuid)
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::onAuthenticationTokensForLinkedNotebooksReceived"));

    if (!m_pendingAuthenticationTokensForLinkedNotebooks) {
        QNDEBUG(QStringLiteral("Authenticaton tokens for linked notebooks were not requested by this object, won't do anything"));
        return;
    }

    m_pendingAuthenticationTokensForLinkedNotebooks = false;
    m_authenticationTokensAndShardIdsByLinkedNotebookGuid = authenticationTokensByLinkedNotebookGuid;
    m_authenticationTokenExpirationTimesByLinkedNotebookGuid = authenticationTokenExpirationTimesByLinkedNotebookGuid;

    sendLocalChanges();
}

#define CHECK_PAUSED() \
    if (m_paused && !m_requestedToStop) { \
        QNDEBUG(QStringLiteral("SendLocalChangesManager is being paused, returning without any actions")); \
        return; \
    }

#define CHECK_STOPPED() \
    if (m_requestedToStop && !hasPendingRequests()) { \
        QNDEBUG(QStringLiteral("SendLocalChangesManager is requested to stop and has no pending requests, finishing sending the local changes")); \
        finalize(); \
        return; \
    }

void SendLocalChangesManager::onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                       size_t limit, size_t offset,
                                                       LocalStorageManager::ListTagsOrder::type order,
                                                       LocalStorageManager::OrderDirection::type orderDirection,
                                                       QString linkedNotebookGuid, QList<Tag> tags, QUuid requestId)
{
    CHECK_PAUSED();

    bool userTagsListCompleted = (requestId == m_listDirtyTagsRequestId);
    auto it = m_listDirtyTagsFromLinkedNotebooksRequestIds.end();
    if (!userTagsListCompleted) {
        it = m_listDirtyTagsFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userTagsListCompleted || (it != m_listDirtyTagsFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG(QStringLiteral("SendLocalChangesManager::onListDirtyTagsCompleted: flag = ") << flag
                << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset
                << QStringLiteral(", order = ") << order << QStringLiteral(", orderDirection = ")
                << orderDirection << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid
                << QStringLiteral(", requestId = ") << requestId);

        m_tags << tags;

        if (userTagsListCompleted) {
            QNTRACE(QStringLiteral("User's tags list is completed"));
            m_listDirtyTagsRequestId = QUuid();
        }
        else {
            QNTRACE(QStringLiteral("Tags list is completed for one of linked notebooks"));
            Q_UNUSED(m_listDirtyTagsFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                                    size_t limit, size_t offset,
                                                    LocalStorageManager::ListTagsOrder::type order,
                                                    LocalStorageManager::OrderDirection::type orderDirection,
                                                    QString linkedNotebookGuid, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool userTagsListCompleted = (requestId == m_listDirtyTagsRequestId);
    auto it = m_listDirtyTagsFromLinkedNotebooksRequestIds.end();
    if (!userTagsListCompleted) {
        it = m_listDirtyTagsFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userTagsListCompleted || (it != m_listDirtyTagsFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING(QStringLiteral("SendLocalChangesManager::onListDirtyTagsFailed: flag = ") << flag
                << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset
                << QStringLiteral(", order = ") << order << QStringLiteral(", orderDirection = ") << orderDirection
                << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid << QStringLiteral(", error description = ")
                << errorDescription << QStringLiteral(", requestId = ") << requestId);

        if (userTagsListCompleted) {
            m_listDirtyTagsRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyTagsFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        QNLocalizedString error = QT_TR_NOOP("error listing dirty tags from local storage");
        error += QStringLiteral(": ");
        error += errorDescription;
        emit failure(error);
    }
}

void SendLocalChangesManager::onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                                size_t limit, size_t offset,
                                                                LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                                QList<SavedSearch> savedSearches, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_listDirtySavedSearchesRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("SendLocalChangesManager::onListDirtySavedSearchesCompleted: flag = ") << flag
            << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ")
            << order << QStringLiteral(", orderDirection = ") << orderDirection << QStringLiteral(", requestId = ") << requestId);

    m_savedSearches << savedSearches;
    m_listDirtySavedSearchesRequestId = QUuid();

    CHECK_STOPPED();

    checkListLocalStorageObjectsCompletion();
}

void SendLocalChangesManager::onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                             size_t limit, size_t offset,
                                                             LocalStorageManager::ListSavedSearchesOrder::type order,
                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                             QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_listDirtySavedSearchesRequestId) {
        return;
    }

    QNWARNING(QStringLiteral("SendLocalChangesManager::onListDirtySavedSearchesFailed: flag = ") << flag
              << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset
              << QStringLiteral(", order = ") << order << QStringLiteral(", orderDirection = ") << orderDirection
              << QStringLiteral(", errorDescription = ") << errorDescription << QStringLiteral(", requestId = ") << requestId);

    m_listDirtySavedSearchesRequestId = QUuid();

    CHECK_STOPPED();

    QNLocalizedString error = QT_TR_NOOP("error listing dirty saved searches from local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    emit failure(error);
}

void SendLocalChangesManager::onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                            size_t limit, size_t offset,
                                                            LocalStorageManager::ListNotebooksOrder::type order,
                                                            LocalStorageManager::OrderDirection::type orderDirection,
                                                            QString linkedNotebookGuid, QList<Notebook> notebooks, QUuid requestId)
{
    CHECK_PAUSED();

    bool userNotebooksListCompleted = (requestId == m_listDirtyNotebooksRequestId);
    auto it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end();
    if (!userNotebooksListCompleted) {
        it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userNotebooksListCompleted || (it != m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG(QStringLiteral("SendLocalChangesManager::onListDirtyNotebooksCompleted: flag = ") << flag
                << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset
                << QStringLiteral(", order = ") << order << QStringLiteral(", orderDirection = ") << orderDirection
                << QStringLiteral(", linkedNotebookGuid = ") << linkedNotebookGuid << QStringLiteral(", requestId = ") << requestId);

        m_notebooks << notebooks;

        if (userNotebooksListCompleted) {
            m_listDirtyNotebooksRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyNotebooksFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                                         size_t limit, size_t offset,
                                                         LocalStorageManager::ListNotebooksOrder::type order,
                                                         LocalStorageManager::OrderDirection::type orderDirection,
                                                         QString linkedNotebookGuid, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool userNotebooksListCompleted = (requestId == m_listDirtyNotebooksRequestId);
    auto it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end();
    if (!userNotebooksListCompleted) {
        it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userNotebooksListCompleted || (it != m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING(QStringLiteral("SendLocalChangesManager::onListDirtyNotebooksFailed: flag = ") << flag
                  << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset
                  << QStringLiteral(", order = ") << order << QStringLiteral(", orderDirection = ") << orderDirection
                  << QStringLiteral(", linkedNotebookGuid = ") << linkedNotebookGuid << QStringLiteral(", errorDescription = ")
                  << errorDescription);

        if (userNotebooksListCompleted) {
            m_listDirtyNotebooksRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyNotebooksFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        QNLocalizedString error = QT_TR_NOOP("error listing dirty notebooks from local storage");
        error += QStringLiteral(": ");
        error += errorDescription;
        emit failure(error);
    }
}

void SendLocalChangesManager::onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                        bool withResourceBinaryData,
                                                        size_t limit, size_t offset,
                                                        LocalStorageManager::ListNotesOrder::type order,
                                                        LocalStorageManager::OrderDirection::type orderDirection,
                                                        QList<Note> notes, QUuid requestId)
{
    CHECK_PAUSED();

    bool userNotesListCompleted = (requestId == m_listDirtyNotesRequestId);
    auto it = m_listDirtyNotesFromLinkedNotebooksRequestIds.end();
    if (!userNotesListCompleted) {
        it = m_listDirtyNotesFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userNotesListCompleted || (it != m_listDirtyNotesFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG(QStringLiteral("SendLocalChangesManager::onListDirtyNotesCompleted: flag = ") << flag
                << QStringLiteral(", withResourceBinaryData = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false"))
                << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order
                << QStringLiteral(", orderDirection = ") << orderDirection << QStringLiteral(", requestId = ") << requestId);

        m_notes << notes;

        if (userNotesListCompleted) {
            m_listDirtyNotesRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyNotesFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                     bool withResourceBinaryData, size_t limit, size_t offset,
                                                     LocalStorageManager::ListNotesOrder::type order,
                                                     LocalStorageManager::OrderDirection::type orderDirection,
                                                     QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool userNotesListCompleted = (requestId == m_listDirtyNotesRequestId);
    auto it = m_listDirtyNotesFromLinkedNotebooksRequestIds.end();
    if (!userNotesListCompleted) {
        it = m_listDirtyNotesFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userNotesListCompleted || (it != m_listDirtyNotesFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING(QStringLiteral("SendLocalChangesManager::onListDirtyNotesFailed: flag = ") << flag
                  << QStringLiteral(", withResourceBinaryData = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false"))
                  << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset
                  << QStringLiteral(", order = ") << order << QStringLiteral(", orderDirection = ") << orderDirection
                  << QStringLiteral(", requestId = ") << requestId);

        if (userNotesListCompleted) {
            m_listDirtyNotesRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyNotesFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        QNLocalizedString error = QT_TR_NOOP("Error listing dirty notes from local storage");
        error += QStringLiteral(": ");
        error += errorDescription;
        emit failure(error);
    }
}

void SendLocalChangesManager::onListLinkedNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                             size_t limit, size_t offset,
                                                             LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                             QList<LinkedNotebook> linkedNotebooks, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("SendLocalChangesManager::onListLinkedNotebooksCompleted: flag = ") << flag
            << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset
            << QStringLiteral(", order = ") << order << QStringLiteral(", orderDirection = ") << orderDirection
            << QStringLiteral(", requestId = ") << requestId);

    const int numLinkedNotebooks = linkedNotebooks.size();
    m_linkedNotebookGuidsAndSharedNotebookGlobalIds.reserve(std::max(numLinkedNotebooks, 0));

    QString sharedNotebookGlobalId;
    for(int i = 0; i < numLinkedNotebooks; ++i)
    {
        const LinkedNotebook & linkedNotebook = linkedNotebooks[i];
        if (!linkedNotebook.hasGuid()) {
            QNLocalizedString error = QT_TR_NOOP("found linked notebook without guid set in local storage");
            emit failure(error);
            QNWARNING(error << QStringLiteral(", linked notebook: ") << linkedNotebook);
            return;
        }

        sharedNotebookGlobalId.resize(0);
        if (linkedNotebook.hasSharedNotebookGlobalId()) {
            sharedNotebookGlobalId = linkedNotebook.sharedNotebookGlobalId();
        }

        m_linkedNotebookGuidsAndSharedNotebookGlobalIds << QPair<QString, QString>(linkedNotebook.guid(), sharedNotebookGlobalId);
    }

    m_listLinkedNotebooksRequestId = QUuid();
    CHECK_STOPPED();
    checkListLocalStorageObjectsCompletion();
}

void SendLocalChangesManager::onListLinkedNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                                          size_t limit, size_t offset,
                                                          LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                          LocalStorageManager::OrderDirection::type orderDirection,
                                                          QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNWARNING(QStringLiteral("SendLocalChangesManager::onListLinkedNotebooksFailed: flag = ") << flag
              << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ") << offset
              << QStringLiteral(", order = ") << order << QStringLiteral(", orderDirection = ") << orderDirection
              << QStringLiteral(", errorDescription = ") << errorDescription << QStringLiteral(", requestId = ") << requestId);

    m_listLinkedNotebooksRequestId = QUuid();
    CHECK_STOPPED();

    QNLocalizedString error = QT_TR_NOOP("error listing linked notebooks from local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    emit failure(error);
}

void SendLocalChangesManager::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("SendLocalChangesManager::onUpdateTagCompleted: tag = ") << tag << QStringLiteral("\nRequest id = ") << requestId);
    Q_UNUSED(m_updateTagRequestIds.erase(it));

    CHECK_STOPPED();
}

void SendLocalChangesManager::onUpdateTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_updateTagRequestIds.erase(it));
    CHECK_STOPPED();

    QNLocalizedString error = QT_TR_NOOP("Couldn't update tag in local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    QNWARNING(error << QStringLiteral("; tag: ") << tag);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("SendLocalChangesManager::onUpdateSavedSearchCompleted: search = ") << savedSearch
            << QStringLiteral("\nRequest id = ") << requestId);
    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it));

    CHECK_STOPPED();
}

void SendLocalChangesManager::onUpdateSavedSearchFailed(SavedSearch savedSearch, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it));
    CHECK_STOPPED();

    QNLocalizedString error = QT_TR_NOOP("Couldn't update saved search in local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    QNWARNING(error << QStringLiteral("; saved search: ") << savedSearch);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("SendLocalChangesManager::onUpdateNotebookCompleted: notebook = ") << notebook
            << QStringLiteral("\nRequest id = ") << requestId);
    Q_UNUSED(m_updateNotebookRequestIds.erase(it));

    CHECK_STOPPED();
}

void SendLocalChangesManager::onUpdateNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_updateNotebookRequestIds.erase(it));
    CHECK_STOPPED();

    QNLocalizedString error = QT_TR_NOOP("Couldn't update notebook in local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    QNWARNING(error << QStringLiteral("; notebook: ") << notebook);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateNoteCompleted(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("SendLocalChangesManager::onUpdateNoteCompleted: note = ") << note << QStringLiteral("\nRequest id = ") << requestId);
    Q_UNUSED(m_updateNoteRequestIds.erase(it));

    CHECK_STOPPED();
}

void SendLocalChangesManager::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                                 QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_updateNoteRequestIds.erase(it));
    CHECK_STOPPED();

    QNLocalizedString error = QT_TR_NOOP("Couldn't update note in local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    QNWARNING(error << QStringLiteral("; note: ") << note);
    emit failure(error);
}

void SendLocalChangesManager::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_findNotebookRequestIds.find(requestId);
    if (it == m_findNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("SendLocalChangesManager::onFindNotebookCompleted: notebook = ") << notebook
            << QStringLiteral("\nRequest id = ") << requestId);

    if (!notebook.hasGuid()) {
        QNLocalizedString errorDescription = QT_TR_NOOP("found notebook without guid within notebooks requested "
                                                        "from local storage by guid");
        QNWARNING(errorDescription << QStringLiteral(", notebook: ") << notebook);
        emit failure(errorDescription);
        return;
    }

    m_notebooksByGuidsCache[notebook.guid()] = notebook;
    Q_UNUSED(m_findNotebookRequestIds.erase(it));

    CHECK_STOPPED();

    if (m_findNotebookRequestIds.isEmpty()) {
        checkAndSendNotes();
    }
}

void SendLocalChangesManager::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_findNotebookRequestIds.find(requestId);
    if (it == m_findNotebookRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_findNotebookRequestIds.erase(it));
    CHECK_STOPPED();

    QNWARNING(errorDescription << QStringLiteral("; notebook: ") << notebook);
    emit failure(errorDescription);
}

void SendLocalChangesManager::timerEvent(QTimerEvent * pEvent)
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::timerEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNLocalizedString errorDescription = QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent");
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    if (m_paused) {
        QNDEBUG(QStringLiteral("SendLocalChangesManager is being paused"));
        killAllTimers();
        return;
    }

    int timerId = pEvent->timerId();
    killTimer(timerId);
    QNDEBUG(QStringLiteral("Killed timer with id ") << timerId);

    if (timerId == m_sendTagsPostponeTimerId)
    {
        m_sendTagsPostponeTimerId = 0;
        sendTags();
    }
    else if (timerId == m_sendSavedSearchesPostponeTimerId)
    {
        m_sendSavedSearchesPostponeTimerId = 0;
        sendSavedSearches();
    }
    else if (timerId == m_sendNotebooksPostponeTimerId)
    {
        m_sendNotebooksPostponeTimerId = 0;
        sendNotebooks();
    }
    else if (timerId == m_sendNotesPostponeTimerId)
    {
        m_sendNotesPostponeTimerId = 0;
        checkAndSendNotes();
    }
}

void SendLocalChangesManager::createConnections()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::createConnections"));

    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this,
                     QNSIGNAL(SendLocalChangesManager,requestLocalUnsynchronizedTags,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListTagsRequest,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::connect(this,
                     QNSIGNAL(SendLocalChangesManager,requestLocalUnsynchronizedSavedSearches,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListSavedSearchesRequest,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::connect(this,
                     QNSIGNAL(SendLocalChangesManager,requestLocalUnsynchronizedNotebooks,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListNotebooksRequest,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::connect(this,
                     QNSIGNAL(SendLocalChangesManager,requestLocalUnsynchronizedNotes,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListNotesRequest,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::connect(this,
                     QNSIGNAL(SendLocalChangesManager,requestLinkedNotebooksList,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListLinkedNotebooksRequest,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::connect(this, QNSIGNAL(SendLocalChangesManager,updateTag,Tag,QUuid), &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onUpdateTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(SendLocalChangesManager,updateSavedSearch,SavedSearch,QUuid), &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(SendLocalChangesManager,updateNotebook,Notebook,QUuid), &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(SendLocalChangesManager,updateNote,Note,Notebook,bool,bool,QUuid), &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,Notebook,bool,bool,QUuid));

    QObject::connect(this, QNSIGNAL(SendLocalChangesManager,findNotebook,Notebook,QUuid), &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listTagsComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<Tag>,QString,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListDirtyTagsCompleted,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<Tag>,QString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listTagsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListDirtyTagsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QString,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListDirtySavedSearchesCompleted,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListDirtySavedSearchesFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<Notebook>,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListDirtyNotebooksCompleted,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<Notebook>,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListDirtyNotebooksFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listNotesComplete,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QList<Note>,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListDirtyNotesCompleted,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QList<Note>,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListDirtyNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listLinkedNotebooksComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListLinkedNotebooksOrder::type, LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QString,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListLinkedNotebooksCompleted,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listLinkedNotebooksFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                              LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                     this,
                     QNSLOT(SendLocalChangesManager,onListLinkedNotebooksFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                            LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onUpdateTagCompleted,Tag,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onUpdateTagFailed,Tag,QNLocalizedString,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onUpdateSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onUpdateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onUpdateNotebookCompleted,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onUpdateNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onUpdateNoteCompleted,Note,bool,bool,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onUpdateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(SendLocalChangesManager,onFindNotebookCompleted,Notebook,QUuid));

    m_connectedToLocalStorage = true;
}

void SendLocalChangesManager::disconnectFromLocalStorage()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::disconnectFromLocalStorage"));

    // Disconnect local signals from localStorageManagerThread's slots
    QObject::disconnect(this,
                        QNSIGNAL(SendLocalChangesManager,requestLocalUnsynchronizedTags,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                        &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onListTagsRequest,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::disconnect(this,
                        QNSIGNAL(SendLocalChangesManager,requestLocalUnsynchronizedSavedSearches,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                        &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onListSavedSearchesRequest,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::disconnect(this,
                        QNSIGNAL(SendLocalChangesManager,requestLocalUnsynchronizedNotebooks,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                        &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onListNotebooksRequest,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::disconnect(this,
                        QNSIGNAL(SendLocalChangesManager,requestLocalUnsynchronizedNotes,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                 LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                        &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onListNotesRequest,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                               LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::disconnect(this,
                        QNSIGNAL(SendLocalChangesManager,requestLinkedNotebooksList,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                        &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onListLinkedNotebooksRequest,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));

    QObject::disconnect(this, QNSIGNAL(SendLocalChangesManager,updateTag,Tag,QUuid), &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onUpdateTagRequest,Tag,QUuid));
    QObject::disconnect(this, QNSIGNAL(SendLocalChangesManager,updateSavedSearch,SavedSearch,QUuid), &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));
    QObject::disconnect(this, QNSIGNAL(SendLocalChangesManager,updateNotebook,Notebook,QUuid), &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::disconnect(this, QNSIGNAL(SendLocalChangesManager,updateNote,Note,Notebook,bool,bool,QUuid), &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,Notebook,bool,bool,QUuid));

    QObject::disconnect(this, QNSIGNAL(SendLocalChangesManager,findNotebook,Notebook,QUuid), &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));

    // Disconnect localStorageManagerThread's signals from local slots
    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listTagsComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QList<Tag>,QString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListDirtyTagsCompleted,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QList<Tag>,QString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listTagsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListDirtyTagsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListDirtySavedSearchesCompleted,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListDirtySavedSearchesFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<Notebook>,QString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListDirtyNotebooksCompleted,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<Notebook>,QString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListDirtyNotebooksFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listNotesComplete,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                 LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QList<Note>,QString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListDirtyNotesCompleted,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                               LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QList<Note>,QString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                 LocalStorageManager::ListNotesOrder::type, LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListDirtyNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                               LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listLinkedNotebooksComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListLinkedNotebooksCompleted,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listLinkedNotebooksFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                 LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                        this,
                        QNSLOT(SendLocalChangesManager,onListLinkedNotebooksFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                               LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onUpdateTagCompleted,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QNLocalizedString,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onUpdateTagFailed,Tag,QNLocalizedString,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onUpdateSavedSearchCompleted,SavedSearch,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onUpdateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onUpdateNotebookCompleted,Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QNLocalizedString,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onUpdateNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onUpdateNoteCompleted,Note,bool,bool,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onUpdateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onFindNotebookCompleted,Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                        this, QNSLOT(SendLocalChangesManager,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));

    m_connectedToLocalStorage = false;
}

void SendLocalChangesManager::requestStuffFromLocalStorage(const QString & linkedNotebookGuid)
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::requestStuffFromLocalStorage: linked notebook guid = ") << linkedNotebookGuid);

    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    bool emptyLinkedNotebookGuid = linkedNotebookGuid.isEmpty();

    // WARNING: this flag assumes that all local-but-not-yet-synchronized objects would have dirty flag set!
    LocalStorageManager::ListObjectsOptions listDirtyObjectsFlag =
            LocalStorageManager::ListDirty | LocalStorageManager::ListNonLocal;

    size_t limit = 0, offset = 0;
    LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;

    LocalStorageManager::ListTagsOrder::type tagsOrder = LocalStorageManager::ListTagsOrder::NoOrder;
    QUuid listDirtyTagsRequestId = QUuid::createUuid();
    if (emptyLinkedNotebookGuid) {
        m_listDirtyTagsRequestId = listDirtyTagsRequestId;
    }
    else {
        Q_UNUSED(m_listDirtyTagsFromLinkedNotebooksRequestIds.insert(listDirtyTagsRequestId));
    }
    emit requestLocalUnsynchronizedTags(listDirtyObjectsFlag, limit, offset, tagsOrder,
                                        orderDirection, linkedNotebookGuid, listDirtyTagsRequestId);

    if (emptyLinkedNotebookGuid)
    {
        LocalStorageManager::ListSavedSearchesOrder::type savedSearchesOrder = LocalStorageManager::ListSavedSearchesOrder::NoOrder;
        m_listDirtySavedSearchesRequestId = QUuid::createUuid();
        emit requestLocalUnsynchronizedSavedSearches(listDirtyObjectsFlag, limit, offset, savedSearchesOrder,
                orderDirection, linkedNotebookGuid, m_listDirtySavedSearchesRequestId);
    }

    LocalStorageManager::ListNotebooksOrder::type notebooksOrder = LocalStorageManager::ListNotebooksOrder::NoOrder;
    QUuid listDirtyNotebooksRequestId = QUuid::createUuid();
    if (emptyLinkedNotebookGuid) {
        m_listDirtyNotebooksRequestId = listDirtyNotebooksRequestId;
    }
    else {
        Q_UNUSED(m_listDirtyNotebooksFromLinkedNotebooksRequestIds.insert(listDirtyNotebooksRequestId));
    }
    emit requestLocalUnsynchronizedNotebooks(listDirtyObjectsFlag, limit, offset, notebooksOrder,
                                             orderDirection, linkedNotebookGuid, listDirtyNotebooksRequestId);

    LocalStorageManager::ListNotesOrder::type notesOrder = LocalStorageManager::ListNotesOrder::NoOrder;
    QUuid listDirtyNotesRequestId = QUuid::createUuid();
    if (emptyLinkedNotebookGuid) {
        m_listDirtyNotesRequestId = listDirtyNotesRequestId;
    }
    else {
        Q_UNUSED(m_listDirtyNotesFromLinkedNotebooksRequestIds.insert(listDirtyNotesRequestId));
    }
    emit requestLocalUnsynchronizedNotes(listDirtyObjectsFlag, /* with resource binary data = */ true,
                                         limit, offset, notesOrder, orderDirection, linkedNotebookGuid,
                                         listDirtyNotesRequestId);

    if (emptyLinkedNotebookGuid)
    {
        LocalStorageManager::ListObjectsOptions linkedNotebooksListOption = LocalStorageManager::ListAll;
        LocalStorageManager::ListLinkedNotebooksOrder::type linkedNotebooksOrder = LocalStorageManager::ListLinkedNotebooksOrder::NoOrder;
        m_listLinkedNotebooksRequestId = QUuid::createUuid();
        emit requestLinkedNotebooksList(linkedNotebooksListOption, limit, offset, linkedNotebooksOrder, orderDirection, m_listLinkedNotebooksRequestId);
    }
}

void SendLocalChangesManager::checkListLocalStorageObjectsCompletion()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::checkListLocalStorageObjectsCompletion"));

    if (!m_listDirtyTagsRequestId.isNull()) {
        QNTRACE(QStringLiteral("The last request for the list of new and dirty tags was not processed yet"));
        return;
    }

    if (!m_listDirtySavedSearchesRequestId.isNull()) {
        QNTRACE(QStringLiteral("The last request for the list of new and dirty saved searches was not processed yet"));
        return;
    }

    if (!m_listDirtyNotebooksRequestId.isNull()) {
        QNTRACE(QStringLiteral("The last request for the list of new and dirty notebooks was not processed yet"));
        return;
    }

    if (!m_listDirtyNotesRequestId.isNull()) {
        QNTRACE(QStringLiteral("The last request for the list of new and dirty notes was not processed yet"));
        return;
    }

    if (!m_listLinkedNotebooksRequestId.isNull()) {
        QNTRACE(QStringLiteral("The last request for the list of linked notebooks was not processed yet"));
        return;
    }

    m_receivedDirtyLocalStorageObjectsFromUsersAccount = true;
    QNTRACE(QStringLiteral("Received all dirty objects from user's own account from local storage"));
    emit receivedUserAccountDirtyObjects();

    if ((m_lastProcessedLinkedNotebookGuidIndex < 0) && (m_linkedNotebookGuidsAndSharedNotebookGlobalIds.size() > 0))
    {
        QNTRACE(QStringLiteral("There are ") << m_linkedNotebookGuidsAndSharedNotebookGlobalIds.size()
                << QStringLiteral(" linked notebook guids, need to receive the dirty objects from them as well"));

        const int numLinkedNotebookGuids = m_linkedNotebookGuidsAndSharedNotebookGlobalIds.size();
        for(int i = 0; i < numLinkedNotebookGuids; ++i)
        {
            const QPair<QString, QString> & guidAndShareKey = m_linkedNotebookGuidsAndSharedNotebookGlobalIds[i];
            requestStuffFromLocalStorage(guidAndShareKey.first);
        }

        return;
    }
    else if (m_linkedNotebookGuidsAndSharedNotebookGlobalIds.size() > 0)
    {
        if (!m_listDirtyTagsFromLinkedNotebooksRequestIds.isEmpty()) {
            QNTRACE(QStringLiteral("There are pending requests to list tags from linked notebooks from local storage: ")
                    << m_listDirtyTagsFromLinkedNotebooksRequestIds.size());
            return;
        }

        if (!m_listDirtyNotebooksFromLinkedNotebooksRequestIds.isEmpty()) {
            QNTRACE(QStringLiteral("There are pending requests to list notebooks from linked notebooks from local storage: ")
                    << m_listDirtyNotebooksFromLinkedNotebooksRequestIds.size());
            return;
        }

        if (!m_listDirtyNotesFromLinkedNotebooksRequestIds.isEmpty()) {
            QNTRACE(QStringLiteral("There are pending requests to list notes from linked notebooks from local storage: ")
                    << m_listDirtyNotesFromLinkedNotebooksRequestIds.size());
            return;
        }
    }

    m_receivedAllDirtyLocalStorageObjects = true;
    QNTRACE(QStringLiteral("All relevant objects from local storage have been listed"));
    emit receivedAllDirtyObjects();

    if (!m_tags.isEmpty() || !m_savedSearches.isEmpty() || !m_notebooks.isEmpty() || !m_notes.isEmpty()) {
        sendLocalChanges();
    }
    else {
        QNINFO(QStringLiteral("No modified or new synchronizable objects were found in the local storage, nothing to send to the remote service"));
        finalize();
    }
}

void SendLocalChangesManager::sendLocalChanges()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::sendLocalChanges"));

    if (!checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
        return;
    }

    // TODO: consider checking the expiration time of user's own authentication token here

#define CHECK_RATE_LIMIT() \
    if (rateLimitIsActive()) { \
        return; \
    }

    sendTags();
    CHECK_RATE_LIMIT()

    sendSavedSearches();
    CHECK_RATE_LIMIT()

    sendNotebooks();
    CHECK_RATE_LIMIT()

    findNotebooksForNotes();
}

void SendLocalChangesManager::sendTags()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::sendTags"));

    QNLocalizedString errorDescription;

    typedef QList<Tag>::iterator Iter;
    for(Iter it = m_tags.begin(); it != m_tags.end(); )
    {
        Tag & tag = *it;

        errorDescription.clear();
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = qevercloud::EDAMErrorCode::UNKNOWN;

        QString linkedNotebookAuthToken;
        if (tag.hasLinkedNotebookGuid())
        {
            auto cit = m_authenticationTokensAndShardIdsByLinkedNotebookGuid.find(tag.linkedNotebookGuid());
            if (cit != m_authenticationTokensAndShardIdsByLinkedNotebookGuid.end())
            {
                linkedNotebookAuthToken = cit.value().first;
            }
            else
            {
                errorDescription = QT_TR_NOOP("couldn't find authentication token for linked notebook when attempting "
                                              "to create or update a tag from it");
                QNWARNING(errorDescription << QStringLiteral(", tag: ") << tag);

                auto sit = std::find_if(m_linkedNotebookGuidsAndSharedNotebookGlobalIds.begin(),
                                        m_linkedNotebookGuidsAndSharedNotebookGlobalIds.end(),
                                        CompareGuidAndSharedNotebookGlobalIdByGuid(tag.linkedNotebookGuid()));
                if (sit == m_linkedNotebookGuidsAndSharedNotebookGlobalIds.end()) {
                    QNWARNING(QStringLiteral("The linked notebook the tag refers to was not found within the list of linked notebooks "
                                             "received from local storage!"));
                }

                emit failure(errorDescription);
                return;
            }
        }

        bool creatingTag = !tag.hasUpdateSequenceNumber();
        if (creatingTag) {
            QNTRACE(QStringLiteral("Sending new tag: ") << tag);
            errorCode = m_noteStore.createTag(tag, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }
        else {
            QNTRACE(QStringLiteral("Sending modified tag: ") << tag);
            errorCode = m_noteStore.updateTag(tag, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds <= 0) {
                errorDescription += QStringLiteral("\n");
                errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
                errorDescription += QStringLiteral(": ");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (Q_UNLIKELY(timerId == 0)) {
                errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                              "due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_sendTagsPostponeTimerId = timerId;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (!tag.hasLinkedNotebookGuid())
            {
                handleAuthExpiration();
            }
            else
            {
                auto cit = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(tag.linkedNotebookGuid());
                if (cit == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end())
                {
                    errorDescription = QT_TR_NOOP("couldn't find the expiration time of linked notebook auth token");
                    QNWARNING(errorDescription << QStringLiteral(", linked notebook guid = ") << tag.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
                else if (checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
                    errorDescription = QT_TR_NOOP("unexpected AUTH_EXPIRED error: authentication tokens for all linked notebooks "
                                                  "are still valid");
                    QNWARNING(errorDescription << QStringLiteral(", linked notebook guid = ") << tag.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
            }

            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
        {
            QNINFO(QStringLiteral("Encountered DATA_CONFLICT exception while trying to send new and/or modified tags, "
                                  "it means the incremental sync should be repeated before sending the changes to the service"));
            emit conflictDetected();
            pause();
            return;
        }
        else if (errorCode != 0) {
            QNLocalizedString error = QT_TR_NOOP("can't send new and/or modified tags to the service");
            error += QStringLiteral(": ");
            error += errorDescription;
            emit failure(error);
            return;
        }

        if (!m_shouldRepeatIncrementalSync)
        {
            QNTRACE(QStringLiteral("Checking if we are still in sync with the remote service"));

            if (!tag.hasUpdateSequenceNumber()) {
                errorDescription = QT_TR_NOOP("tag's update sequence number is not set after it being sent to the service");
                emit failure(errorDescription);
                return;
            }

            int * pLastUpdateCount = Q_NULLPTR;
            if (!tag.hasLinkedNotebookGuid())
            {
                pLastUpdateCount = &m_lastUpdateCount;
                QNTRACE(QStringLiteral("Current tag does not belong to linked notebook"));
            }
            else
            {
                QNTRACE(QStringLiteral("Current tag belongs to linked notebook with guid ") << tag.linkedNotebookGuid());

                auto lit = m_lastUpdateCountByLinkedNotebookGuid.find(tag.linkedNotebookGuid());
                if (lit == m_lastUpdateCountByLinkedNotebookGuid.end()) {
                    errorDescription = QT_TR_NOOP("can't find update count per linked notebook guid on attempt to check "
                                                  "the update count of tag sent to the service");
                    emit failure(errorDescription);
                    return;
                }

                pLastUpdateCount = &lit.value();
            }

            if (tag.updateSequenceNumber() == *pLastUpdateCount + 1) {
                *pLastUpdateCount = tag.updateSequenceNumber();
                QNTRACE(QStringLiteral("The client is in sync with the service; updated corresponding last update count to ") << *pLastUpdateCount);
            }
            else {
                m_shouldRepeatIncrementalSync = true;
                emit shouldRepeatIncrementalSync();
                QNTRACE(QStringLiteral("The client is not in sync with the service"));
            }
        }

        it = m_tags.erase(it);
    }
}

void SendLocalChangesManager::sendSavedSearches()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::sendSavedSearches"));

    QNLocalizedString errorDescription;

    typedef QList<SavedSearch>::iterator Iter;
    for(Iter it = m_savedSearches.begin(); it != m_savedSearches.end(); )
    {
        SavedSearch & search = *it;

        errorDescription.clear();
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = qevercloud::EDAMErrorCode::UNKNOWN;

        bool creatingSearch = !search.hasUpdateSequenceNumber();
        if (creatingSearch) {
            QNTRACE(QStringLiteral("Sending new saved search: ") << search);
            errorCode = m_noteStore.createSavedSearch(search, errorDescription, rateLimitSeconds);
        }
        else {
            QNTRACE(QStringLiteral("Sending modified saved search: ") << search);
            errorCode = m_noteStore.updateSavedSearch(search, errorDescription, rateLimitSeconds);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds <= 0) {
                errorDescription += QStringLiteral("\n");
                errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
                errorDescription += QStringLiteral(": ");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (Q_UNLIKELY(timerId == 0)) {
                errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                              "due to trta limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_sendSavedSearchesPostponeTimerId = timerId;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            handleAuthExpiration();
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
        {
            QNINFO(QStringLiteral("Encountered DATA_CONFLICT exception while trying to send new and/or modified saved searches, "
                                  "it means the incremental sync should be repeated before sending the changes to the service"));
            emit conflictDetected();
            pause();
            return;
        }
        else if (errorCode != 0)
        {
            QNLocalizedString error = QT_TR_NOOP("can't send new and/or modified tags to the service");
            error += QStringLiteral(": ");
            error += errorDescription;
            emit failure(error);
            return;
        }

        if (!m_shouldRepeatIncrementalSync)
        {
            QNTRACE(QStringLiteral("Checking if we are still in sync with the remote service"));

            if (!search.hasUpdateSequenceNumber()) {
                errorDescription = QT_TR_NOOP("Internal error: saved search's update sequence number is not set "
                                              "after being send to the service");
                emit failure(errorDescription);
                return;
            }

            if (search.updateSequenceNumber() == m_lastUpdateCount + 1) {
                m_lastUpdateCount = search.updateSequenceNumber();
                QNTRACE(QStringLiteral("The client is in sync with the service; updated last update count to ") << m_lastUpdateCount);
            }
            else {
                m_shouldRepeatIncrementalSync = true;
                emit shouldRepeatIncrementalSync();
                QNTRACE(QStringLiteral("The client is not in sync with the service"));
            }
        }

        it = m_savedSearches.erase(it);
    }
}

void SendLocalChangesManager::sendNotebooks()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::sendNotebooks"));

    QNLocalizedString errorDescription;

    typedef QList<Notebook>::iterator Iter;
    for(Iter it = m_notebooks.begin(); it != m_notebooks.end(); )
    {
        Notebook & notebook = *it;

        errorDescription.clear();
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = qevercloud::EDAMErrorCode::UNKNOWN;

        QString linkedNotebookAuthToken;
        if (notebook.hasLinkedNotebookGuid())
        {
            auto cit = m_authenticationTokensAndShardIdsByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
            if (cit != m_authenticationTokensAndShardIdsByLinkedNotebookGuid.end())
            {
                linkedNotebookAuthToken = cit.value().first;
            }
            else
            {
                errorDescription = QT_TR_NOOP("couldn't find authenticaton token for linked notebook when attempting "
                                              "to create or update a notebook");
                QNWARNING(errorDescription << QStringLiteral(", notebook: ") << notebook);

                auto sit = std::find_if(m_linkedNotebookGuidsAndSharedNotebookGlobalIds.begin(),
                                        m_linkedNotebookGuidsAndSharedNotebookGlobalIds.end(),
                                        CompareGuidAndSharedNotebookGlobalIdByGuid(notebook.linkedNotebookGuid()));
                if (sit == m_linkedNotebookGuidsAndSharedNotebookGlobalIds.end()) {
                    QNWARNING(QStringLiteral("The linked notebook the notebook refers to was not found within the list of linked notebooks "
                                             "received from local storage"));
                }

                emit failure(errorDescription);
                return;
            }
        }

        bool creatingNotebook = !notebook.hasUpdateSequenceNumber();
        if (creatingNotebook) {
            QNTRACE(QStringLiteral("Sending new notebook: ") << notebook);
            errorCode = m_noteStore.createNotebook(notebook, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }
        else {
            QNTRACE(QStringLiteral("Sending modified notebook: ") << notebook);
            errorCode = m_noteStore.updateNotebook(notebook, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds <= 0) {
                errorDescription += QStringLiteral("\n");
                errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
                errorDescription += QStringLiteral(": ");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (Q_UNLIKELY(timerId == 0)) {
                errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                              "due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_sendNotebooksPostponeTimerId = timerId;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (!notebook.hasLinkedNotebookGuid()) {
                handleAuthExpiration();
            }
            else
            {
                auto cit = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
                if (cit == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end())
                {
                    errorDescription = QT_TR_NOOP("couldn't find the expiration time of linked notebook auth token");
                    QNWARNING(errorDescription << QStringLiteral(", linked notebook guid = ") << notebook.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
                else if (checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
                    errorDescription = QT_TR_NOOP("unexpected AUTH_EXPIRED error: authentication tokens for all linked notebooks "
                                                  "are still valid");
                    QNWARNING(errorDescription << QStringLiteral(", linked notebook guid = ") << notebook.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
            }

            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
        {
            QNINFO(QStringLiteral("Encountered DATA_CONFLICT exception while trying to send new and/or modified notebooks, "
                                  "it means the incremental sync should be repeated before sending the changes to the service"));
            emit conflictDetected();
            pause();
            return;
        }
        else if (errorCode != 0)
        {
            QNLocalizedString error = QT_TR_NOOP("can't send new and/or mofidied notebooks to the service");
            error += QStringLiteral(": ");
            error += errorDescription;
            emit failure(error);
            return;
        }

        if (!m_shouldRepeatIncrementalSync)
        {
            QNTRACE(QStringLiteral("Checking if we are still in sync with the remote service"));

            if (!notebook.hasUpdateSequenceNumber()) {
                errorDescription = QT_TR_NOOP("notebook's update sequence number is not set after it being sent to the service");
                emit failure(errorDescription);
                return;
            }

            int * pLastUpdateCount = Q_NULLPTR;
            if (!notebook.hasLinkedNotebookGuid())
            {
                pLastUpdateCount = &m_lastUpdateCount;
                QNTRACE(QStringLiteral("Current notebook does not belong to linked notebook"));
            }
            else
            {
                QNTRACE(QStringLiteral("Current notebook belongs to linked notebook with guid ") << notebook.linkedNotebookGuid());

                auto lit = m_lastUpdateCountByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
                if (lit == m_lastUpdateCountByLinkedNotebookGuid.end()) {
                    errorDescription = QT_TR_NOOP("can't find update count per linked notebook guid on attempt to check "
                                                  "the update count of tag sent to the service");
                    emit failure(errorDescription);
                    return;
                }

                pLastUpdateCount = &lit.value();
            }

            if (notebook.updateSequenceNumber() == *pLastUpdateCount + 1) {
                *pLastUpdateCount = notebook.updateSequenceNumber();
                QNTRACE(QStringLiteral("The client is in sync with the service; updated last update count to ") << *pLastUpdateCount);
            }
            else {
                m_shouldRepeatIncrementalSync = true;
                emit shouldRepeatIncrementalSync();
                QNTRACE(QStringLiteral("The client is not in sync with the service"));
            }
        }

        it = m_notebooks.erase(it);
    }
}

void SendLocalChangesManager::checkAndSendNotes()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::checkAndSendNotes"));

    if (m_tags.isEmpty() && m_notebooks.isEmpty() && m_findNotebookRequestIds.isEmpty()) {
        sendNotes();
    }
}

void SendLocalChangesManager::sendNotes()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::sendNotes"));

    QNLocalizedString errorDescription;

    typedef QList<Note>::iterator Iter;
    for(Iter it = m_notes.begin(); it != m_notes.end(); )
    {
        Note & note = *it;

        errorDescription.clear();
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = qevercloud::EDAMErrorCode::UNKNOWN;

        if (!note.hasNotebookGuid()) {
            errorDescription = QT_TR_NOOP("found note without notebook guid set");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
            emit failure(errorDescription);
            return;
        }

        auto nit = m_notebooksByGuidsCache.find(note.notebookGuid());
        if (nit == m_notebooksByGuidsCache.end()) {
            errorDescription = QT_TR_NOOP("can't find notebook for one of notes to be sent to the remote service");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
            emit failure(errorDescription);
            return;
        }

        const Notebook & notebook = nit.value();

        QString linkedNotebookAuthToken;
        if (notebook.hasLinkedNotebookGuid())
        {
            auto cit = m_authenticationTokensAndShardIdsByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
            if (cit != m_authenticationTokensAndShardIdsByLinkedNotebookGuid.end())
            {
                linkedNotebookAuthToken = cit.value().first;
            }
            else
            {
                errorDescription = QT_TR_NOOP("couldn't find authenticaton token for linked notebook when attempting "
                                              "to create or update a notebook");
                QNWARNING(errorDescription << QStringLiteral(", notebook: ") << notebook);

                auto sit = std::find_if(m_linkedNotebookGuidsAndSharedNotebookGlobalIds.begin(),
                                        m_linkedNotebookGuidsAndSharedNotebookGlobalIds.end(),
                                        CompareGuidAndSharedNotebookGlobalIdByGuid(notebook.linkedNotebookGuid()));
                if (sit == m_linkedNotebookGuidsAndSharedNotebookGlobalIds.end()) {
                    QNWARNING("The linked notebook the notebook refers to was not found within the list of linked notebooks "
                              "received from local storage");
                }

                emit failure(errorDescription);
                return;
            }
        }

        bool creatingNote = !note.hasUpdateSequenceNumber();
        if (creatingNote) {
            QNTRACE(QStringLiteral("Sending new note: ") << note);
            errorCode = m_noteStore.createNote(note, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }
        else {
            QNTRACE(QStringLiteral("Sending modified note: ") << note);
            errorCode = m_noteStore.updateNote(note, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds <= 0) {
                errorDescription += QStringLiteral("\n");
                errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
                errorDescription += QStringLiteral(": ");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (timerId == 0) {
                errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                              "due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_sendNotesPostponeTimerId = timerId;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (!notebook.hasLinkedNotebookGuid()) {
                handleAuthExpiration();
            }
            else
            {
                auto cit = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
                if (cit == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end())
                {
                    errorDescription = QT_TR_NOOP("couldn't find the expiration time of linked notebook auth token");
                    QNWARNING(errorDescription << QStringLiteral(", linked notebook guid = ") << notebook.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
                else if (checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
                    errorDescription = QT_TR_NOOP("unexpected AUTH_EXPIRED error: authentication tokens for all linked notebooks "
                                                  "are still valid");
                    QNWARNING(errorDescription << QStringLiteral(", linked notebook guid = ") << notebook.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
            }

            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
        {
            QNINFO(QStringLiteral("Encountered DATA_CONFLICT exception while trying to send new and/or modified notes, "
                                  "it means the incremental sync should be repeated before sending the changes to the service"));
            emit conflictDetected();
            pause();
            return;
        }
        else if (errorCode != 0)
        {
            QNLocalizedString error = QT_TR_NOOP("can't send new and/or mofidied notes to the service");
            error += QStringLiteral(": ");
            error += errorDescription;
            emit failure(error);
            return;
        }

        if (!m_shouldRepeatIncrementalSync)
        {
            QNTRACE(QStringLiteral("Checking if we are still in sync with the remote service"));

            if (!note.hasUpdateSequenceNumber()) {
                errorDescription = QT_TR_NOOP("note's update sequence number is not set after it being sent to the service");
                emit failure(errorDescription);
                return;
            }

            int * pLastUpdateCount = Q_NULLPTR;
            if (!notebook.hasLinkedNotebookGuid())
            {
                pLastUpdateCount = &m_lastUpdateCount;
                QNTRACE(QStringLiteral("Current note does not belong to linked notebook"));
            }
            else
            {
                QNTRACE(QStringLiteral("Current note belongs to linked notebook with guid ") << notebook.linkedNotebookGuid());

                auto lit = m_lastUpdateCountByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
                if (lit == m_lastUpdateCountByLinkedNotebookGuid.end()) {
                    errorDescription = QT_TR_NOOP("can't find update count per linked notebook guid on attempt to check "
                                                  "the update count of tag sent to the service");
                    emit failure(errorDescription);
                    return;
                }

                pLastUpdateCount = &lit.value();
            }

            if (note.updateSequenceNumber() == *pLastUpdateCount + 1) {
                *pLastUpdateCount = note.updateSequenceNumber();
                QNTRACE(QStringLiteral("The client is in sync with the service; updated last update count to ") << *pLastUpdateCount);
            }
            else {
                m_shouldRepeatIncrementalSync = true;
                emit shouldRepeatIncrementalSync();
                QNTRACE(QStringLiteral("The client is not in sync with the service"));
            }
        }

        it = m_notes.erase(it);
    }

    // If we got here, we are actually done
    QNINFO(QStringLiteral("Sent all locally added/updated notes back to the Evernote service"));
    finalize();
}

void SendLocalChangesManager::findNotebooksForNotes()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::findNotebooksForNotes"));

    m_findNotebookRequestIds.clear();
    QSet<QString> notebookGuids;

    // NOTE: enforce limited scope for CIter typedef
    {
        auto notebooksByGuidsCacheEnd = m_notebooksByGuidsCache.end();

        typedef QList<Note>::const_iterator CIter;
        CIter notesEnd = m_notes.end();
        for(CIter it = m_notes.begin(); it != notesEnd; ++it)
        {
            const Note & note = *it;
            if (!note.hasNotebookGuid()) {
                continue;
            }

            auto nit = m_notebooksByGuidsCache.find(note.notebookGuid());
            if (nit == notebooksByGuidsCacheEnd) {
                Q_UNUSED(notebookGuids.insert(note.notebookGuid()));
            }
        }
    }

    if (!notebookGuids.isEmpty())
    {
        Notebook dummyNotebook;
        dummyNotebook.unsetLocalUid();

        typedef QSet<QString>::const_iterator CIter;
        CIter notebookGuidsEnd = notebookGuids.end();
        for(CIter it = notebookGuids.begin(); it != notebookGuidsEnd; ++it)
        {
            const QString & notebookGuid = *it;
            dummyNotebook.setGuid(notebookGuid);

            QUuid requestId = QUuid::createUuid();
            emit findNotebook(dummyNotebook, requestId);
            Q_UNUSED(m_findNotebookRequestIds.insert(requestId));

            QNTRACE(QStringLiteral("Sent find notebook request for notebook guid ") << notebookGuid << QStringLiteral(", request id = ") << requestId);
        }
    }
    else
    {
        checkAndSendNotes();
    }
}

bool SendLocalChangesManager::rateLimitIsActive() const
{
    return ( (m_sendTagsPostponeTimerId > 0) ||
             (m_sendSavedSearchesPostponeTimerId > 0) ||
             (m_sendNotebooksPostponeTimerId > 0) ||
             (m_sendNotesPostponeTimerId > 0) );
}

bool SendLocalChangesManager::hasPendingRequests() const
{
    QUuid emptyId;
    return ( (m_listDirtyTagsRequestId != emptyId) ||
             (m_listDirtySavedSearchesRequestId != emptyId) ||
             (m_listDirtyNotebooksRequestId != emptyId) ||
             (m_listDirtyNotesRequestId != emptyId) ||
             (m_listLinkedNotebooksRequestId != emptyId) ||
             !m_listDirtyTagsFromLinkedNotebooksRequestIds.isEmpty() ||
             !m_listDirtyNotebooksFromLinkedNotebooksRequestIds.isEmpty() ||
             !m_listDirtyNotesFromLinkedNotebooksRequestIds.isEmpty() ||
             !m_updateTagRequestIds.isEmpty() ||
             !m_updateSavedSearchRequestIds.isEmpty() ||
             !m_updateNotebookRequestIds.isEmpty() ||
             !m_updateNoteRequestIds.isEmpty() ||
             !m_findNotebookRequestIds.isEmpty() );
}

void SendLocalChangesManager::finalize()
{
    emit finished(m_lastUpdateCount, m_lastUpdateCountByLinkedNotebookGuid);
    clear();
    disconnectFromLocalStorage();
    m_active = false;
}

void SendLocalChangesManager::clear()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::clear"));

    m_lastUpdateCount = 0;
    m_lastUpdateCountByLinkedNotebookGuid.clear();

    m_shouldRepeatIncrementalSync = false;
    m_paused = false;
    m_requestedToStop = false;

    m_receivedDirtyLocalStorageObjectsFromUsersAccount = false;
    m_receivedAllDirtyLocalStorageObjects = false;

    QUuid emptyId;
    m_listDirtyTagsRequestId = emptyId;
    m_listDirtySavedSearchesRequestId = emptyId;
    m_listDirtyNotebooksRequestId = emptyId;
    m_listDirtyNotesRequestId = emptyId;
    m_listLinkedNotebooksRequestId = emptyId;

    m_listDirtyTagsFromLinkedNotebooksRequestIds.clear();
    m_listDirtyNotebooksFromLinkedNotebooksRequestIds.clear();
    m_listDirtyNotesFromLinkedNotebooksRequestIds.clear();

    m_tags.clear();
    m_savedSearches.clear();
    m_notebooks.clear();
    m_notes.clear();

    m_linkedNotebookGuidsAndSharedNotebookGlobalIds.clear();
    m_lastProcessedLinkedNotebookGuidIndex = -1;
    m_pendingAuthenticationTokensForLinkedNotebooks = false;

    // NOTE: don't clear auth tokens by linked notebook guid as well as their expiration timestamps, these might be useful later on

    m_updateTagRequestIds.clear();
    m_updateSavedSearchRequestIds.clear();
    m_updateNotebookRequestIds.clear();
    m_updateNoteRequestIds.clear();

    m_findNotebookRequestIds.clear();
    m_notebooksByGuidsCache.clear();    // NOTE: don't get any ideas on preserving the cache, it can easily get stale
                                        // especially when disconnected from local storage

    killAllTimers();
}

void SendLocalChangesManager::killAllTimers()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::killAllTimers"));

    if (m_sendTagsPostponeTimerId > 0) {
        killTimer(m_sendTagsPostponeTimerId);
    }
    m_sendTagsPostponeTimerId = 0;

    if (m_sendSavedSearchesPostponeTimerId > 0) {
        killTimer(m_sendSavedSearchesPostponeTimerId);
    }
    m_sendSavedSearchesPostponeTimerId = 0;

    if (m_sendNotebooksPostponeTimerId > 0) {
        killTimer(m_sendNotebooksPostponeTimerId);
    }
    m_sendNotebooksPostponeTimerId = 0;

    if (m_sendNotesPostponeTimerId > 0) {
        killTimer(m_sendNotesPostponeTimerId);
    }
    m_sendNotesPostponeTimerId = 0;
}

bool SendLocalChangesManager::checkAndRequestAuthenticationTokensForLinkedNotebooks()
{
    QNDEBUG(QStringLiteral("SendLocalChangesManager::checkAndRequestAuthenticationTokensForLinkedNotebooks"));

    if (m_linkedNotebookGuidsAndSharedNotebookGlobalIds.isEmpty()) {
        QNDEBUG(QStringLiteral("The list of linked notebook guids and share keys is empty"));
        return true;
    }

    const int numLinkedNotebookGuids = m_linkedNotebookGuidsAndSharedNotebookGlobalIds.size();
    for(int i = 0; i < numLinkedNotebookGuids; ++i)
    {
        const QPair<QString, QString> & guidAndShareKey = m_linkedNotebookGuidsAndSharedNotebookGlobalIds[i];
        const QString & guid = guidAndShareKey.first;
        if (guid.isEmpty()) {
            QNLocalizedString error = QT_TR_NOOP("found empty linked notebook guid within the list of linked notebook guids and share keys");
            QNWARNING(error);
            emit failure(error);
            return false;
        }

        auto it = m_authenticationTokensAndShardIdsByLinkedNotebookGuid.find(guid);
        if (it == m_authenticationTokensAndShardIdsByLinkedNotebookGuid.end()) {
            QNDEBUG(QStringLiteral("Authentication token for linked notebook with guid ") << guid
                    << QStringLiteral(" was not found; will request authentication tokens for all linked notebooks at once"));
            emit requestAuthenticationTokensForLinkedNotebooks(m_linkedNotebookGuidsAndSharedNotebookGlobalIds);
            m_pendingAuthenticationTokensForLinkedNotebooks = true;
            return false;
        }

        auto eit = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(guid);
        if (eit == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end()) {
            QNLocalizedString error = QT_TR_NOOP("can't find cached expiration time of linked notebook's authentication token");
            QNWARNING(error << QStringLiteral(", linked notebook guid = ") << guid);
            emit failure(error);
            return false;
        }

        const qevercloud::Timestamp & expirationTime = eit.value();
        const qevercloud::Timestamp currentTime = QDateTime::currentMSecsSinceEpoch();
        if (currentTime - expirationTime < SIX_HOURS_IN_MSEC) {
            QNDEBUG(QStringLiteral("Authentication token for linked notebook with guid ") << guid
                    << QStringLiteral(" is too close to expiration: its expiration time is ") << printableDateTimeFromTimestamp(expirationTime)
                    << QStringLiteral(", current time is ") << printableDateTimeFromTimestamp(currentTime)
                    << QStringLiteral("; will request new authentication tokens for all linked notebooks"));
            emit requestAuthenticationTokensForLinkedNotebooks(m_linkedNotebookGuidsAndSharedNotebookGlobalIds);
            m_pendingAuthenticationTokensForLinkedNotebooks = true;
            return false;
        }
    }

    QNDEBUG(QStringLiteral("Got authentication tokens for all linked notebooks, can proceed with "
                           "their synchronization"));

    return true;
}

void SendLocalChangesManager::handleAuthExpiration()
{
    QNINFO(QStringLiteral("Got AUTH_EXPIRED error, pausing and requesting new authentication token"));
    m_paused = true;
    emit paused(/* pending authentication = */ true);
    emit requestAuthenticationToken();
}

} // namespace quentier

