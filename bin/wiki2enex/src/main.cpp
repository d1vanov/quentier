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

#include "WikiArticleFetcher.h"

#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QNetworkAccessManager>
#include <QTimer>
#include <QUrl>

#include <iostream>

using namespace quentier;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " wiki-article-url" << std::endl;
        return 1;
    }

    QUrl url(QString::fromLocal8Bit(argv[1]));
    if (!url.isValid()) {
        std::cerr << "Not a valid URL" << std::endl;
        return 1;
    }

    QNetworkAccessManager networkAccessManager;
    ErrorString errorDescription;
    Note note;

    int status = -1;
    {
        QTimer timer;
        timer.setInterval(600000);
        timer.setSingleShot(true);

        WikiArticleFetcher fetcher(&networkAccessManager, url);

        EventLoopWithExitStatus loop;
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout),
                         &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&fetcher,
                         QNSIGNAL(WikiArticleFetcher,finished),
                         &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&fetcher,
                         QNSIGNAL(WikiArticleFetcher,failure,ErrorString),
                         &loop, QNSLOT(EventLoopWithExitStatus,
                                       exitAsFailureWithErrorString,ErrorString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(100);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &fetcher, SLOT(start()));
        status = loop.exec();
        errorDescription = loop.errorDescription();
        note = fetcher.note();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        std::cerr << "Failed to fetch wiki article in time" << std::endl;
        return 1;
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
        std::cerr << "Failed to fetch wiki article: "
            << errorDescription.nonLocalizedString().toStdString() << std::endl;
        return 1;
    }

    // TODO: convert note to ENEX
    return 0;
}
