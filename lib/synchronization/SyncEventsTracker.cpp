/*
 * Copyright 2024 Dmitry Ivanov
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

#include "SyncEventsTracker.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/synchronization/ISyncEventsNotifier.h>

#include <QTextStream>

#include <utility>

namespace quentier {

SyncEventsTracker::SyncEventsTracker(QObject * parent) : QObject{parent} {}

void SyncEventsTracker::start(synchronization::ISyncEventsNotifier * notifier)
{
    QNINFO("quentier::synchronization", "SyncEventsTracker::start");

    Q_ASSERT(notifier);

    stopImpl();

    m_active = true;
    m_notifier = notifier;
    connectToSyncEvents();
}

void SyncEventsTracker::stop()
{
    QNINFO("quentier::synchronization", "SyncEventsTracker::stop");
    stopImpl();
}

void SyncEventsTracker::onUserOwnSyncChunksDownloadProgress(
    const qint32 highestDownloadedUsn, const qint32 highestServerUsn,
    const qint32 lastPreviousUsn)
{
    // TODO: implement
    Q_UNUSED(highestDownloadedUsn)
    Q_UNUSED(highestServerUsn)
    Q_UNUSED(lastPreviousUsn)
}

void SyncEventsTracker::onUserOwnSyncChunksDownloaded()
{
    // TODO: implement
}

void SyncEventsTracker::onUserOwnSyncChunksDataProcessingProgress(
    const synchronization::ISyncChunksDataCountersPtr & counters)
{
    // TODO: implement
    Q_UNUSED(counters)
}

void SyncEventsTracker::onLinkedNotebooksDataDownloadingStarted(
    const QList<qevercloud::LinkedNotebook> & linkedNotebooks)
{
    if (QuentierIsLogLevelActive(LogLevel::Debug)) {
        QString str;
        QTextStream strm{&str};

        for (const auto & linkedNotebook: std::as_const(linkedNotebooks)) {
            strm << linkedNotebook.guid().value_or(QStringLiteral("<none>"))
                 << "; ";
        }

        strm.flush();

        QNDEBUG(
            "quentier::synchronization",
            "SyncEventsTracker::onLinkedNotebooksDataDownloadingStarted: "
                << str);
    }

    for (const auto & linkedNotebook: std::as_const(linkedNotebooks)) {
        if (Q_UNLIKELY(!linkedNotebook.guid())) {
            QNWARNING(
                "quentier::synchronization",
                "Skipping linked notebook without guid: " << linkedNotebook);
            continue;
        }

        m_linkedNotebookProgressCounters[*linkedNotebook.guid()] =
            LinkedNotebookProgressInfo{};
    }
}

void SyncEventsTracker::onLinkedNotebookSyncChunksDownloadProgress(
    const qint32 highestDownloadedUsn, const qint32 highestServerUsn,
    const qint32 lastPreviousUsn,
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    // TODO: implement
    Q_UNUSED(highestDownloadedUsn)
    Q_UNUSED(highestServerUsn)
    Q_UNUSED(lastPreviousUsn)
    Q_UNUSED(linkedNotebook)
}

void SyncEventsTracker::onLinkedNotebookSyncChunksDownloaded(
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    // TODO: implement
    Q_UNUSED(linkedNotebook)
}

void SyncEventsTracker::onLinkedNotebookSyncChunksDataProcessingProgress(
    const synchronization::ISyncChunksDataCountersPtr & counters,
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    // TODO: implement
    Q_UNUSED(counters)
    Q_UNUSED(linkedNotebook)
}

void SyncEventsTracker::onUserOwnNotesDownloadProgress(
    const quint32 notesDownloaded, const quint32 totalNotesToDownload)
{
    // TODO: implement
    Q_UNUSED(notesDownloaded)
    Q_UNUSED(totalNotesToDownload)
}

void SyncEventsTracker::onLinkedNotebookNotesDownloadProgress(
    const quint32 notesDownloaded, const quint32 totalNotesToDownload,
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    // TODO: implement
    Q_UNUSED(notesDownloaded)
    Q_UNUSED(totalNotesToDownload)
    Q_UNUSED(linkedNotebook)
}

void SyncEventsTracker::onUserOwnResourcesDownloadProgress(
    const quint32 resourcesDownloaded, const quint32 totalResourcesToDownload)
{
    // TODO: implement
    Q_UNUSED(resourcesDownloaded)
    Q_UNUSED(totalResourcesToDownload);
}

void SyncEventsTracker::onLinkedNotebookResourcesDownloadProgress(
    const quint32 resourcesDownloaded, const quint32 totalResourcesToDownload,
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    // TODO: implement
    Q_UNUSED(resourcesDownloaded)
    Q_UNUSED(totalResourcesToDownload)
    Q_UNUSED(linkedNotebook)
}

void SyncEventsTracker::onUserOwnSendStatusUpdate(
    const synchronization::ISendStatusPtr & sendStatus)
{
    // TODO: implement
    Q_UNUSED(sendStatus)
}

void SyncEventsTracker::onLinkedNotebookSendStatusUpdate(
    const qevercloud::Guid & linkedNotebookGuid,
    const synchronization::ISendStatusPtr & sendStatus)
{
    // TODO: implement
    Q_UNUSED(linkedNotebookGuid)
    Q_UNUSED(sendStatus)
}

void SyncEventsTracker::connectToSyncEvents()
{
    Q_ASSERT(m_notifier);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::syncChunksDownloadProgress, this,
        &SyncEventsTracker::onUserOwnSyncChunksDownloadProgress);

    QObject::connect(
        m_notifier, &synchronization::ISyncEventsNotifier::syncChunksDownloaded,
        this, &SyncEventsTracker::onUserOwnSyncChunksDownloaded);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::syncChunksDataProcessingProgress,
        this, &SyncEventsTracker::onUserOwnSyncChunksDataProcessingProgress);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::
            startLinkedNotebooksDataDownloading,
        this, &SyncEventsTracker::onLinkedNotebooksDataDownloadingStarted);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::
            linkedNotebookSyncChunksDownloadProgress,
        this, &SyncEventsTracker::onLinkedNotebookSyncChunksDownloadProgress);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::
            linkedNotebookSyncChunksDownloaded,
        this, &SyncEventsTracker::onLinkedNotebookSyncChunksDownloaded);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::
            linkedNotebookSyncChunksDataProcessingProgress,
        this,
        &SyncEventsTracker::onLinkedNotebookSyncChunksDataProcessingProgress);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::notesDownloadProgress, this,
        &SyncEventsTracker::onUserOwnNotesDownloadProgress);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::
            linkedNotebookNotesDownloadProgress,
        this, &SyncEventsTracker::onLinkedNotebookNotesDownloadProgress);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::resourcesDownloadProgress, this,
        &SyncEventsTracker::onUserOwnResourcesDownloadProgress);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::
            linkedNotebookResourcesDownloadProgress,
        this, &SyncEventsTracker::onLinkedNotebookResourcesDownloadProgress);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::userOwnSendStatusUpdate, this,
        &SyncEventsTracker::onUserOwnSendStatusUpdate);

    QObject::connect(
        m_notifier,
        &synchronization::ISyncEventsNotifier::linkedNotebookSendStatusUpdate,
        this, &SyncEventsTracker::onLinkedNotebookSendStatusUpdate);
}

void SyncEventsTracker::stopImpl()
{
    QNDEBUG("quentier::synchronization", "SyncEventsTracker::stopImpl");

    if (!m_active) {
        QNDEBUG("quentier::synchronization", "Already stopped");
        return;
    }

    Q_ASSERT(m_notifier);
    m_notifier->disconnect(this);

    m_active = false;

    m_syncDownloadStepFinished = false;
    m_syncSendStepFinished = false;

    m_userOwnSyncChunksDownloadedTimestamp = 0;
    m_lastUserOwnSyncChunksDataProcessingProgressPercentage = 0.0;
    m_userOwnNotesDownloadedPercentage = 0.0;
    m_userOwnResourcesDownloadedPercentage = 0.0;

    m_linkedNotebooksSyncChunksDownloadedTimestamp = 0;
    m_lastLinkedNotebookSyncChunksDataProcessingProgressPercentage = 0.0;
    m_linkedNotebooksNotesDownloadedPercentage = 0.0;
    m_linkedNotebooksResourcesDownloadedPercentage = 0.0;
    m_linkedNotebookProgressCounters.clear();
}

} // namespace quentier
