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

#include <QTimer>

namespace quentier {

bool FetchNotes(const QList<Notebook> & notebooks, const QList<Tag> & tags,
                const quint32 minTagsPerNote, const quint32 numNotes,
                LocalStorageManagerAsync & localStorageManager)
{
    WikiArticlesFetcher fetcher(notebooks, tags, minTagsPerNote,
                                numNotes, localStorageManager);

    WikiArticlesFetchingTracker tracker;
    QObject::connect(&fetcher,
                     QNSIGNAL(WikiArticlesFetcher,finished),
                     &tracker,
                     QNSLOT(WikiArticlesFetchingTracker,
                            onWikiArticlesFetchingFinished));
    QObject::connect(&fetcher,
                     QNSIGNAL(WikiArticlesFetcher,failure,ErrorString),
                     &tracker,
                     QNSLOT(WikiArticlesFetchingTracker,
                            onWikiArticlesFetchingFailed,ErrorString));
    QObject::connect(&fetcher,
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
        slotInvokingTimer.singleShot(0, &fetcher, SLOT(start()));

        status = loop.exec();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Success) {
        return true;
    }

    return false;
}

} // namespace quentier
