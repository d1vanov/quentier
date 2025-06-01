/*
 * Copyright 2019-2025 Dmitry Ivanov
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

#include "FetchNotes.h"
#include "WikiArticlesFetcher.h"
#include "WikiArticlesFetchingTracker.h"

#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QThread>
#include <QTimer>

namespace quentier {

[[nodiscard]] bool fetchNotes(
    const QList<qevercloud::Notebook> & notebooks,
    const QList<qevercloud::Tag> & tags, const quint32 minTagsPerNote,
    const quint32 numNotes, local_storage::ILocalStoragePtr localStorage)
{
    auto * fetcher = new WikiArticlesFetcher{
        notebooks, tags, minTagsPerNote, numNotes, std::move(localStorage)};

    auto * wikiArticlesFetcherThread = new QThread;

    wikiArticlesFetcherThread->setObjectName(
        QStringLiteral("WikiArticlesFetcherThread"));

    QObject::connect(
        wikiArticlesFetcherThread, &QThread::finished,
        wikiArticlesFetcherThread, &QThread::deleteLater);

    wikiArticlesFetcherThread->start();
    fetcher->moveToThread(wikiArticlesFetcherThread);

    WikiArticlesFetchingTracker tracker;

    QObject::connect(
        fetcher, &WikiArticlesFetcher::finished, &tracker,
        &WikiArticlesFetchingTracker::onWikiArticlesFetchingFinished);

    QObject::connect(
        fetcher, &WikiArticlesFetcher::failure, &tracker,
        &WikiArticlesFetchingTracker::onWikiArticlesFetchingFailed);

    QObject::connect(
        fetcher, &WikiArticlesFetcher::progress, &tracker,
        &WikiArticlesFetchingTracker::onWikiArticlesFetchingProgressUpdate);

    auto status = utility::EventLoopWithExitStatus::ExitStatus::Failure;
    {
        utility::EventLoopWithExitStatus loop;

        QObject::connect(
            &tracker, &WikiArticlesFetchingTracker::finished, &loop,
            &utility::EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &tracker, &WikiArticlesFetchingTracker::failure, &loop,
            &utility::EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);
        slotInvokingTimer.singleShot(0, fetcher, SLOT(start()));

        Q_UNUSED(loop.exec())
        status = loop.exitStatus();
    }

    fetcher->deleteLater();
    wikiArticlesFetcherThread->quit();

    if (status == utility::EventLoopWithExitStatus::ExitStatus::Success) {
        return true;
    }

    return false;
}

} // namespace quentier
