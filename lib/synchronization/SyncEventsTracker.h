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

#pragma once

#include <quentier/synchronization/Fwd.h>
#include <quentier/synchronization/types/Fwd.h>

#include <qevercloud/types/LinkedNotebook.h>
#include <qevercloud/types/TypeAliases.h>

#include <QHash>
#include <QObject>
#include <QString>
#include <QtGlobal>

namespace quentier {

class SyncEventsTracker : public QObject
{
    Q_OBJECT
public:
    explicit SyncEventsTracker(QObject * parent = nullptr);

public Q_SLOTS:
    void start(synchronization::ISyncEventsNotifier * notifier);
    void stop();

Q_SIGNALS:
    void message(QString msg);

private Q_SLOTS:
    void onUserOwnSyncChunksDownloadProgress(
        qint32 highestDownloadedUsn, qint32 highestServerUsn,
        qint32 lastPreviousUsn);

    void onUserOwnSyncChunksDownloaded();

    void onUserOwnSyncChunksDataProcessingProgress(
        const synchronization::ISyncChunksDataCountersPtr & counters);

    void onLinkedNotebooksDataDownloadingStarted(
        const QList<qevercloud::LinkedNotebook> & linkedNotebooks);

    void onLinkedNotebookSyncChunksDownloadProgress(
        qint32 highestDownloadedUsn, qint32 highestServerUsn,
        qint32 lastPreviousUsn,
        const qevercloud::LinkedNotebook & linkedNotebook);

    void onLinkedNotebookSyncChunksDownloaded(
        const qevercloud::LinkedNotebook & linkedNotebook);

    void onLinkedNotebookSyncChunksDataProcessingProgress(
        const synchronization::ISyncChunksDataCountersPtr & counters,
        const qevercloud::LinkedNotebook & linkedNotebook);

    void onUserOwnNotesDownloadProgress(
        quint32 notesDownloaded, quint32 totalNotesToDownload);

    void onLinkedNotebookNotesDownloadProgress(
        quint32 notesDownloaded, quint32 totalNotesToDownload,
        const qevercloud::LinkedNotebook & linkedNotebook);

    void onUserOwnResourcesDownloadProgress(
        quint32 resourcesDownloaded, quint32 totalResourcesToDownload);

    void onDownloadFinished(bool dataDownloaded);

    void onLinkedNotebookResourcesDownloadProgress(
        quint32 resourcesDownloaded, quint32 totalResourcesToDownload,
        const qevercloud::LinkedNotebook & linkedNotebook);

    void onUserOwnSendStatusUpdate(
        const synchronization::ISendStatusPtr & sendStatus);

    void onLinkedNotebookSendStatusUpdate(
        const qevercloud::Guid & linkedNotebookGuid,
        const synchronization::ISendStatusPtr & sendStatus);

private:
    void connectToSyncEvents();
    void stopImpl();

private:
    struct LinkedNotebookProgressInfo
    {
        bool m_syncChunksDownloaded = false;

        double m_syncChunksDownloadProgressPercentage = 0.0;
        double m_syncChunksDataProcesssingProgressPercentage = 0.0;
        double m_notesDownloadProgressPercentage = 0.0;
        double m_resourcesDownloadProgressPercentage = 0.0;
    };

    [[nodiscard]] double
    computeOverallLinkedNotebookSyncChunksDownloadProgress() const noexcept;

    [[nodiscard]] double
    computeOverallLinkedNotebookSyncChunkDataProcessingProgress()
        const noexcept;

    [[nodiscard]] double computeOverallLinkedNotebookNotesDownloadProgress()
        const noexcept;

    [[nodiscard]] double computeOverallLinkedNotebookResourcesDownloadProgress()
        const noexcept;

    [[nodiscard]] double computeOverallLinkedNotebookPercentage(
        const double percentage) const noexcept;

    void checkAllLinkedNotebookSyncChunksDownloaded();
    void updateLinkedNotebookSyncChunksDownloadProgress();
    void updateLinkedNotebookSyncChunkDataProcessingProgress();
    void updateLinkedNotebookNotesDownloadProgress();
    void updateLinkedNotebookResourcesDownloadProgress();

    [[nodiscard]] bool updateSyncChunkDataProcessingProgressImpl(
        double percentage, qint64 syncChunksDownloadTimestamp,
        double & lastPercentage);

private:
    synchronization::ISyncEventsNotifier * m_notifier = nullptr;
    bool m_active = false;

    bool m_syncDownloadStepFinished = false;
    bool m_syncSendStepFinished = false;

    qint64 m_userOwnSyncChunksDownloadedTimestamp = 0;
    double m_lastUserOwnSyncChunksDataProcessingProgressPercentage = 0.0;
    double m_userOwnNotesDownloadedPercentage = 0.0;
    double m_userOwnResourcesDownloadedPercentage = 0.0;

    qint64 m_linkedNotebooksSyncChunksDownloadedTimestamp = 0;
    double m_lastLinkedNotebookSyncChunksDataProcessingProgressPercentage = 0.0;
    double m_linkedNotebooksNotesDownloadedPercentage = 0.0;
    double m_linkedNotebooksResourcesDownloadedPercentage = 0.0;
    QHash<qevercloud::Guid, LinkedNotebookProgressInfo>
        m_linkedNotebookProgressInfos;
};

} // namespace quentier
