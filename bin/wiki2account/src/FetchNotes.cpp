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

#include "FetchNotes.h"
#include "WikiArticlesFetcher.h"
#include "WikiArticlesFetchingTracker.h"

#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QThread>
#include <QTimer>

namespace quentier {

bool FetchNotes(
    const QList<Notebook> & notebooks, const QList<Tag> & tags,
    const quint32 minTagsPerNote, const quint32 numNotes,
    LocalStorageManagerAsync & localStorageManager)
{
    WikiArticlesFetcher * pFetcher = new WikiArticlesFetcher(
        notebooks, tags, minTagsPerNote,
        numNotes, localStorageManager);

    QThread * pWikiArticlerFetcherThread = new QThread;
    pWikiArticlerFetcherThread->setObjectName(
        QStringLiteral("WikiArticlesFetcherThread"));
    QObject::connect(pWikiArticlerFetcherThread, QNSIGNAL(QThread,finished),
                     pWikiArticlerFetcherThread, QNSLOT(QThread,deleteLater));
    pWikiArticlerFetcherThread->start();
    pFetcher->moveToThread(pWikiArticlerFetcherThread);

    WikiArticlesFetchingTracker tracker;
    QObject::connect(pFetcher,
                     QNSIGNAL(WikiArticlesFetcher,finished),
                     &tracker,
                     QNSLOT(WikiArticlesFetchingTracker,
                            onWikiArticlesFetchingFinished));
    QObject::connect(pFetcher,
                     QNSIGNAL(WikiArticlesFetcher,failure,ErrorString),
                     &tracker,
                     QNSLOT(WikiArticlesFetchingTracker,
                            onWikiArticlesFetchingFailed,ErrorString));
    QObject::connect(pFetcher,
                     QNSIGNAL(WikiArticlesFetcher,progress,double),
                     &tracker,
                     QNSLOT(WikiArticlesFetchingTracker,
                            onWikiArticlesFetchingProgressUpdate,double));

    int status = -1;
    {
        EventLoopWithExitStatus loop;
        QObject::connect(&tracker,
                         QNSIGNAL(WikiArticlesFetchingTracker,finished),
                         &loop,
                         QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&tracker,
                         QNSIGNAL(WikiArticlesFetchingTracker,failure,ErrorString),
                         &loop,
                         QNSLOT(EventLoopWithExitStatus,
                                exitAsFailureWithErrorString,ErrorString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);
        slotInvokingTimer.singleShot(0, pFetcher, SLOT(start()));

        status = loop.exec();
    }

    pFetcher->deleteLater();
    pWikiArticlerFetcherThread->quit();

    if (status == EventLoopWithExitStatus::ExitStatus::Success) {
        return true;
    }

    return false;
}

} // namespace quentier
