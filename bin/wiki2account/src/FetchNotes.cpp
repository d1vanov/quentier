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

#include "FetchNotes.h"
#include "WikiArticlesFetcher.h"
#include "WikiArticlesFetchingTracker.h"

#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QThread>
#include <QTimer>

namespace quentier {

bool fetchNotes(
    const QList<Notebook> & notebooks, const QList<Tag> & tags,
    const quint32 minTagsPerNote, const quint32 numNotes,
    LocalStorageManagerAsync & localStorageManager)
{
    auto * pFetcher = new WikiArticlesFetcher(
        notebooks,
        tags,
        minTagsPerNote,
        numNotes,
        localStorageManager);

    auto * pWikiArticlerFetcherThread = new QThread;

    pWikiArticlerFetcherThread->setObjectName(
        QStringLiteral("WikiArticlesFetcherThread"));

    QObject::connect(
        pWikiArticlerFetcherThread,
        &QThread::finished,
        pWikiArticlerFetcherThread,
        &QThread::deleteLater);

    pWikiArticlerFetcherThread->start();
    pFetcher->moveToThread(pWikiArticlerFetcherThread);

    WikiArticlesFetchingTracker tracker;

    QObject::connect(
        pFetcher,
        &WikiArticlesFetcher::finished,
        &tracker,
        &WikiArticlesFetchingTracker::onWikiArticlesFetchingFinished);

    QObject::connect(
        pFetcher,
        &WikiArticlesFetcher::failure,
        &tracker,
        &WikiArticlesFetchingTracker::onWikiArticlesFetchingFailed);

    QObject::connect(
        pFetcher,
        &WikiArticlesFetcher::progress,
        &tracker,
        &WikiArticlesFetchingTracker::onWikiArticlesFetchingProgressUpdate);

    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        EventLoopWithExitStatus loop;

        QObject::connect(
            &tracker,
            &WikiArticlesFetchingTracker::finished,
            &loop,
            &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &tracker,
            &WikiArticlesFetchingTracker::failure,
            &loop,
            &EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);
        slotInvokingTimer.singleShot(0, pFetcher, SLOT(start()));

        Q_UNUSED(loop.exec())
        status = loop.exitStatus();
    }

    pFetcher->deleteLater();
    pWikiArticlerFetcherThread->quit();

    if (status == EventLoopWithExitStatus::ExitStatus::Success) {
        return true;
    }

    return false;
}

} // namespace quentier
