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
#include <quentier/synchronization/types/ISyncChunksDataCounters.h>

#include <qevercloud/utility/ToRange.h>

#include <QDateTime>
#include <QTextStream>

#include <algorithm>
#include <utility>

namespace quentier {

namespace {

[[nodiscard]] double computeSyncChunkDataProcessingProgress(
    const synchronization::ISyncChunksDataCounters & counters)
{
    double total = 0.0;
    double processed = 0.0;

    const auto totalSavedSearches = counters.totalSavedSearches();
    if (totalSavedSearches != 0) {
        total += static_cast<double>(totalSavedSearches);
        processed += static_cast<double>(counters.addedSavedSearches());
        processed += static_cast<double>(counters.updatedSavedSearches());
    }

    const auto totalExpungedSavedSearches =
        counters.totalExpungedSavedSearches();
    if (totalExpungedSavedSearches != 0) {
        total += static_cast<double>(totalExpungedSavedSearches);
        processed += static_cast<double>(counters.expungedSavedSearches());
    }

    const auto totalTags = counters.totalTags();
    if (totalTags != 0) {
        total += static_cast<double>(totalTags);
        processed += static_cast<double>(counters.addedTags());
        processed += static_cast<double>(counters.updatedTags());
    }

    const auto totalExpungedTags = counters.totalExpungedTags();
    if (totalExpungedTags != 0) {
        total += static_cast<double>(totalExpungedTags);
        processed += static_cast<double>(counters.expungedTags());
    }

    const auto totalNotebooks = counters.totalNotebooks();
    if (totalNotebooks != 0) {
        total += static_cast<double>(totalNotebooks);
        processed += static_cast<double>(counters.addedNotebooks());
        processed += static_cast<double>(counters.updatedNotebooks());
    }

    const auto totalExpungedNotebooks = counters.totalExpungedNotebooks();
    if (totalExpungedNotebooks != 0) {
        total += static_cast<double>(totalExpungedNotebooks);
        processed += static_cast<double>(counters.expungedNotebooks());
    }

    const auto totalLinkedNotebooks = counters.totalLinkedNotebooks();
    if (totalLinkedNotebooks != 0) {
        total += static_cast<double>(totalLinkedNotebooks);
        processed += static_cast<double>(counters.addedLinkedNotebooks());
        processed += static_cast<double>(counters.updatedLinkedNotebooks());
    }

    const auto totalExpungedLinkedNotebooks =
        counters.totalExpungedLinkedNotebooks();
    if (totalExpungedLinkedNotebooks != 0) {
        total += static_cast<double>(totalExpungedLinkedNotebooks);
        processed += static_cast<double>(counters.expungedLinkedNotebooks());
    }

    if (Q_UNLIKELY(processed > total)) {
        QNWARNING(
            "quentier::synchronization",
            "Invalid sync chunk data counters: " << counters);
    }

    if (total == 0.0) {
        return 0.0;
    }

    return std::clamp(processed / total, 0.0, 1.0) * 100.0;
}

[[nodiscard]] double computeSyncChunksDownloadProgressPercentage(
    const qint32 highestDownloadedUsn, const qint32 highestServerUsn,
    const qint32 lastPreviousUsn)
{
    const double numerator = highestDownloadedUsn - lastPreviousUsn;
    const double denominator = highestServerUsn - lastPreviousUsn;
    return std::min(numerator / denominator * 100.0, 100.0);
}

[[nodiscard]] QString linkedNotebookInfo(
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    QString res;
    QTextStream strm{&res};

    if (linkedNotebook.username()) {
        strm << *linkedNotebook.username();
    }
    else {
        strm << "<no username>";
    }

    strm << " (";
    if (linkedNotebook.guid()) {
        strm << *linkedNotebook.guid();
    }
    else {
        strm << "<no guid>";
    }
    strm << ")";

    return res;
}

[[nodiscard]] double computePercentage(
    const quint32 doneCount, const quint32 totalCount)
{
    return std::clamp(
        static_cast<double>(doneCount) / static_cast<double>(totalCount) *
            100.0,
        0.0, 100.0);
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

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
    QNDEBUG(
        "quentier::synchronization",
        "SyncEventsTracker::onUserOwnSyncChunksDownloadProgress: "
            << "highest downloaded usn = " << highestDownloadedUsn
            << ", highest server usn = " << highestServerUsn
            << ", last previous usn = " << lastPreviousUsn);

    if (Q_UNLIKELY(
            (highestServerUsn <= lastPreviousUsn) ||
            (highestDownloadedUsn <= lastPreviousUsn)))
    {
        QNWARNING(
            "quentier::synchronization",
            "Received incorrect user own sync chunks "
                << "download progress state: highest downloaded USN = "
                << highestDownloadedUsn
                << ", highest server USN = " << highestServerUsn
                << ", last previous USN = " << lastPreviousUsn);
        return;
    }

    const double percentage = computeSyncChunksDownloadProgressPercentage(
        highestDownloadedUsn, highestServerUsn, lastPreviousUsn);

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Downloading sync chunks") << ": "
             << QString::number(percentage, 'f', 2) << "%";
    }

    Q_EMIT message(std::move(msg));
}

void SyncEventsTracker::onUserOwnSyncChunksDownloaded()
{
    QNDEBUG(
        "quentier::synchronization",
        "SyncEventsTracker::onUserOwnSyncChunksDownloaded");

    m_userOwnSyncChunksDownloadedTimestamp =
        QDateTime::currentMSecsSinceEpoch();

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Downloaded sync chunks, parsing tags, notebooks and "
                   "saved searches from them")
             << "...";
    }

    Q_EMIT message(std::move(msg));
}

void SyncEventsTracker::onUserOwnSyncChunksDataProcessingProgress(
    const synchronization::ISyncChunksDataCountersPtr & counters)
{
    Q_ASSERT(counters);

    QNDEBUG(
        "quentier::synchronization",
        "SyncEventsTracker::onUserOwnSyncChunksDataProcessingProgress: "
            << *counters);

    const double percentage = computeSyncChunkDataProcessingProgress(*counters);
    if (!updateSyncChunkDataProcessingProgressImpl(
            percentage, m_userOwnSyncChunksDownloadedTimestamp,
            m_lastUserOwnSyncChunksDataProcessingProgressPercentage))
    {
        return;
    }

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Processing data from sync chunks") << ": "
             << QString::number(percentage, 'f', 2);
    }

    Q_EMIT message(std::move(msg));
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

        m_linkedNotebookProgressInfos[*linkedNotebook.guid()] =
            LinkedNotebookProgressInfo{};
    }
}

void SyncEventsTracker::onLinkedNotebookSyncChunksDownloadProgress(
    const qint32 highestDownloadedUsn, const qint32 highestServerUsn,
    const qint32 lastPreviousUsn,
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "quentier::synchronization",
            "Skipping sync chunks download progress for a linked notebook "
                << "without guid: " << linkedNotebook);
        return;
    }

    QNDEBUG(
        "quentier::synchronization",
        "SyncEventsTracker::onLinkedNotebookSyncChunksDownloadProgress: "
            << "highest downloaded usn = " << highestDownloadedUsn
            << ", highest server usn = " << highestServerUsn
            << ", last previous usn = " << lastPreviousUsn
            << ", linked notebook: " << linkedNotebookInfo(linkedNotebook));

    const double percentage = computeSyncChunksDownloadProgressPercentage(
        highestDownloadedUsn, highestServerUsn, lastPreviousUsn);

    auto & linkedNotebookProgressInfo =
        m_linkedNotebookProgressInfos[*linkedNotebook.guid()];

    linkedNotebookProgressInfo.m_syncChunksDownloadProgressPercentage =
        percentage;

    updateLinkedNotebookSyncChunksDownloadProgress();
}

void SyncEventsTracker::onLinkedNotebookSyncChunksDownloaded(
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "quentier::synchronization",
            "Skipping sync chunks downloaded message for a linked notebook "
                << "without guid: " << linkedNotebook);
        return;
    }

    QNDEBUG(
        "quentier::synchronization",
        "SyncEventsTracker::onLinkedNotebookSyncChunksDownloaded: "
            << linkedNotebookInfo(linkedNotebook));

    auto & linkedNotebookProgressInfo =
        m_linkedNotebookProgressInfos[*linkedNotebook.guid()];

    linkedNotebookProgressInfo.m_syncChunksDownloaded = true;
    linkedNotebookProgressInfo.m_syncChunksDownloadProgressPercentage = 100.0;
    checkAllLinkedNotebookSyncChunksDownloaded();
}

void SyncEventsTracker::onLinkedNotebookSyncChunksDataProcessingProgress(
    const synchronization::ISyncChunksDataCountersPtr & counters,
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "quentier::synchronization",
            "Skipping sync chunks data processing progress for a linked "
            "notebook without guid: "
                << linkedNotebook);
        return;
    }

    Q_ASSERT(counters);

    QNDEBUG(
        "quentier::synchronization",
        "SyncEventsTracker::onLinkedNotebookSyncChunksDataProcessingProgress: "
            << *counters
            << "\nLinked notebook: " << linkedNotebookInfo(linkedNotebook));

    auto & linkedNotebookProgressInfo =
        m_linkedNotebookProgressInfos[*linkedNotebook.guid()];

    linkedNotebookProgressInfo.m_syncChunksDataProcesssingProgressPercentage =
        computeSyncChunkDataProcessingProgress(*counters);

    updateLinkedNotebookSyncChunkDataProcessingProgress();
}

void SyncEventsTracker::onUserOwnNotesDownloadProgress(
    const quint32 notesDownloaded, const quint32 totalNotesToDownload)
{
    const double percentage =
        computePercentage(notesDownloaded, totalNotesToDownload);

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - m_userOwnNotesDownloadedPercentage > 10.0) {
        QNINFO(
            "quentier::synchronization",
            "SyncEventsTracker::onUserOwnNotesDownloadProgress: "
                << "notes downloaded = " << notesDownloaded
                << ", total notes to download = " << totalNotesToDownload);
        m_userOwnNotesDownloadedPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier::synchronization",
            "SyncEventsTracker::onUserOwnNotesDownloadProgress: "
                << "notes downloaded = " << notesDownloaded
                << ", total notes to download = " << totalNotesToDownload);
    }

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Downloading notes") << ": " << notesDownloaded << " of "
             << totalNotesToDownload;
    }

    Q_EMIT message(std::move(msg));
}

void SyncEventsTracker::onLinkedNotebookNotesDownloadProgress(
    const quint32 notesDownloaded, const quint32 totalNotesToDownload,
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "quentier::synchronization",
            "Skipping notes download progress for a linked notebook without "
                << "guid: " << linkedNotebook);
        return;
    }

    const double percentage =
        computePercentage(notesDownloaded, totalNotesToDownload);

    auto & linkedNotebookProgressInfo =
        m_linkedNotebookProgressInfos[*linkedNotebook.guid()];

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage -
            linkedNotebookProgressInfo.m_notesDownloadProgressPercentage >
        10.0)
    {
        QNINFO(
            "quentier::synchronization",
            "SyncEventsTracker::onLinkedNotebookNotesDownloadProgress: "
                << "notes downloaded = " << notesDownloaded
                << ", total notes to download = " << totalNotesToDownload
                << ", linked notebook: " << linkedNotebookInfo(linkedNotebook));

        linkedNotebookProgressInfo.m_notesDownloadProgressPercentage =
            percentage;

        updateLinkedNotebookNotesDownloadProgress();
        return;
    }

    QNDEBUG(
        "quentier::synchronization",
        "SyncEventsTracker::onLinkedNotebookNotesDownloadProgress: "
            << "notes downloaded = " << notesDownloaded
            << ", total notes to download = " << totalNotesToDownload
            << ", linked notebook: " << linkedNotebookInfo(linkedNotebook));
}

void SyncEventsTracker::onUserOwnResourcesDownloadProgress(
    const quint32 resourcesDownloaded, const quint32 totalResourcesToDownload)
{
    const double percentage =
        computePercentage(resourcesDownloaded, totalResourcesToDownload);

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - m_userOwnResourcesDownloadedPercentage > 10.0) {
        QNINFO(
            "quentier::synchronization",
            "SyncEventsTracker::onUserOwnResourcesDownloadProgress: "
                << "resources downloaded = " << resourcesDownloaded
                << ", total resources to download = "
                << totalResourcesToDownload);
        m_userOwnResourcesDownloadedPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier::synchronization",
            "SyncEventsTracker::onUserOwnResourcesDownloadProgress: "
                << "resources downloaded = " << resourcesDownloaded
                << ", total resources to download = "
                << totalResourcesToDownload);
    }

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Downloading resources") << ": " << resourcesDownloaded
             << " of " << totalResourcesToDownload;
    }

    Q_EMIT message(std::move(msg));
}

void SyncEventsTracker::onLinkedNotebookResourcesDownloadProgress(
    const quint32 resourcesDownloaded, const quint32 totalResourcesToDownload,
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "quentier::synchronization",
            "Skipping resources download progress for a linked notebook "
                << "without guid: " << linkedNotebook);
        return;
    }

    const double percentage =
        computePercentage(resourcesDownloaded, totalResourcesToDownload);

    auto & linkedNotebookProgressInfo =
        m_linkedNotebookProgressInfos[*linkedNotebook.guid()];

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage -
            linkedNotebookProgressInfo.m_resourcesDownloadProgressPercentage >
        10.0)
    {
        QNINFO(
            "quentier::synchronization",
            "SyncEventsTracker::onLinkedNotebookResourcesDownloadProgress: "
                << "resources downloaded = " << resourcesDownloaded
                << ", total resources to download = "
                << totalResourcesToDownload
                << ", linked notebook: " << linkedNotebookInfo(linkedNotebook));

        linkedNotebookProgressInfo.m_resourcesDownloadProgressPercentage =
            percentage;

        updateLinkedNotebookResourcesDownloadProgress();
        return;
    }

    QNDEBUG(
        "quentier::synchronization",
        "SyncEventsTracker::onLinkedNotebookResourcesDownloadProgress: "
            << "resources downloaded = " << resourcesDownloaded
            << ", total resources to download = " << totalResourcesToDownload
            << ", linked notebook: " << linkedNotebookInfo(linkedNotebook));
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
    m_linkedNotebookProgressInfos.clear();
}

double
SyncEventsTracker::computeOverallLinkedNotebookSyncChunksDownloadProgress()
    const noexcept
{
    if (m_linkedNotebookProgressInfos.isEmpty()) {
        return 0.0;
    }

    double sumPercentage = 0.0;
    for (const auto it: qevercloud::toRange(m_linkedNotebookProgressInfos)) {
        sumPercentage += it.value().m_syncChunksDownloadProgressPercentage;
    }

    return computeOverallLinkedNotebookPercentage(sumPercentage);
}

double
SyncEventsTracker::computeOverallLinkedNotebookSyncChunkDataProcessingProgress()
    const noexcept
{
    if (m_linkedNotebookProgressInfos.isEmpty()) {
        return 0.0;
    }

    double sumPercentage = 0.0;
    for (const auto it: qevercloud::toRange(m_linkedNotebookProgressInfos)) {
        sumPercentage +=
            it.value().m_syncChunksDataProcesssingProgressPercentage;
    }

    return computeOverallLinkedNotebookPercentage(sumPercentage);
}

double SyncEventsTracker::computeOverallLinkedNotebookNotesDownloadProgress()
    const noexcept
{
    if (m_linkedNotebookProgressInfos.isEmpty()) {
        return 0.0;
    }

    double sumPercentage = 0.0;
    for (const auto it: qevercloud::toRange(m_linkedNotebookProgressInfos)) {
        sumPercentage += it.value().m_notesDownloadProgressPercentage;
    }

    return computeOverallLinkedNotebookPercentage(sumPercentage);
}

double
SyncEventsTracker::computeOverallLinkedNotebookResourcesDownloadProgress()
    const noexcept
{
    if (m_linkedNotebookProgressInfos.isEmpty()) {
        return 0.0;
    }

    double sumPercentage = 0.0;
    for (const auto it: qevercloud::toRange(m_linkedNotebookProgressInfos)) {
        sumPercentage += it.value().m_resourcesDownloadProgressPercentage;
    }

    return computeOverallLinkedNotebookPercentage(sumPercentage);
}

double SyncEventsTracker::computeOverallLinkedNotebookPercentage(
    const double percentage) const noexcept
{
    Q_ASSERT(!m_linkedNotebookProgressInfos.isEmpty());

    return std::clamp(
        percentage / static_cast<double>(m_linkedNotebookProgressInfos.size()),
        0.0, 1.0);
}

void SyncEventsTracker::checkAllLinkedNotebookSyncChunksDownloaded()
{
    if (m_linkedNotebooksSyncChunksDownloadedTimestamp != 0) {
        return;
    }

    for (const auto it: qevercloud::toRange(m_linkedNotebookProgressInfos)) {
        if (!it.value().m_syncChunksDownloaded) {
            return;
        }
    }

    m_linkedNotebooksSyncChunksDownloadedTimestamp =
        QDateTime::currentMSecsSinceEpoch();

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Downloaded sync chunks from linked notebooks, parsing "
                   "data from them")
             << "...";
    }

    Q_EMIT message(std::move(msg));
}

void SyncEventsTracker::updateLinkedNotebookSyncChunksDownloadProgress()
{
    const double percentage =
        computeOverallLinkedNotebookSyncChunksDownloadProgress();

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Downloading linked notebook sync chunks") << ": "
             << QString::number(percentage, 'f', 2) << "%";
    }

    Q_EMIT message(std::move(msg));
}

void SyncEventsTracker::updateLinkedNotebookSyncChunkDataProcessingProgress()
{
    if (m_linkedNotebooksSyncChunksDownloadedTimestamp == 0) {
        // If not all linked notebook sync chunks are downloaded yet, it's too
        // early to update their data processing progress
        return;
    }

    const double percentage =
        computeOverallLinkedNotebookSyncChunkDataProcessingProgress();
    if (!updateSyncChunkDataProcessingProgressImpl(
            percentage, m_linkedNotebooksSyncChunksDownloadedTimestamp,
            m_lastLinkedNotebookSyncChunksDataProcessingProgressPercentage))
    {
        return;
    }

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Processing data from linked notebook sync chunks") << ": "
             << QString::number(percentage, 'f', 2);
    }

    Q_EMIT message(std::move(msg));
}

void SyncEventsTracker::updateLinkedNotebookNotesDownloadProgress()
{
    const double percentage =
        computeOverallLinkedNotebookNotesDownloadProgress();

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - m_linkedNotebooksNotesDownloadedPercentage > 10.0) {
        QNINFO(
            "quentier::synchronization",
            "SyncEventsTracker::updateLinkedNotebookNotesDownloadProgress: "
                << "percentage = " << percentage);

        m_linkedNotebooksNotesDownloadedPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier::synchronization",
            "SyncEventsTracker::updateLinkedNotebookNotesDownloadProgress: "
                << "percentage = " << percentage);
    }

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Downloading notes from linked notebooks") << ": "
             << percentage << "%";
    }

    Q_EMIT message(std::move(msg));
}

void SyncEventsTracker::updateLinkedNotebookResourcesDownloadProgress()
{
    const double percentage =
        computeOverallLinkedNotebookResourcesDownloadProgress();

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - m_linkedNotebooksResourcesDownloadedPercentage > 10.0) {
        QNINFO(
            "quentier::synchronization",
            "SyncEventsTracker::updateLinkedNotebookResourcesDownloadProgress: "
                << "percentage = " << percentage);

        m_linkedNotebooksResourcesDownloadedPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier::synchronization",
            "SyncEventsTracker::updateLinkedNotebookResourcesDownloadProgress: "
                << "percentage = " << percentage);
    }

    QString msg;
    {
        QTextStream strm{&msg};
        strm << tr("Downloading resources from linked notebooks") << ": "
             << percentage << "%";
    }

    Q_EMIT message(std::move(msg));
}

bool SyncEventsTracker::updateSyncChunkDataProcessingProgressImpl(
    const double percentage, const qint64 syncChunksDownloadTimestamp,
    double & lastPercentage)
{
    // On small accounts stuff from sync chunks is processed rather quickly.
    // So for the first few seconds won't show progress notifications
    // to prevent unreadable text blinking
    const auto now = QDateTime::currentMSecsSinceEpoch();
    if (syncChunksDownloadTimestamp == 0 ||
        now - syncChunksDownloadTimestamp < 2000)
    {
        QNDEBUG("quentier::synchronization", "Percentage = " << percentage);
        return false;
    }

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - lastPercentage > 10.0) {
        QNINFO(
            "quentier::synchronization",
            (&lastPercentage ==
                     &m_lastUserOwnSyncChunksDataProcessingProgressPercentage
                 ? "User own"
                 : "Linked notebook")
                << " sync chunk data processing percentage = " << percentage);
        lastPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier::synchronization",
            (&lastPercentage ==
                     &m_lastUserOwnSyncChunksDataProcessingProgressPercentage
                 ? "User own"
                 : "Linked notebook")
                << " sync chunk data processing percentage = " << percentage);
    }

    return true;
}

} // namespace quentier
