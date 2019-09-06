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

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QCoreApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QTime>
#include <QTimer>
#include <QUrl>

#include <iostream>

using namespace quentier;

int main(int argc, char *argv[])
{
    qsrand(static_cast<quint32>(QTime::currentTime().msec()));

    QCoreApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("wiki2enex"));

    QStringList args = app.arguments();
    if (args.size() != 2) {
        qWarning() << "Usage: " << app.applicationName() << " wiki-article-url\n";
        return 1;
    }

    QUrl url(args[1]);
    if (!url.isValid()) {
        qWarning() << "Not a valid URL\n";
        return 1;
    }

    // Initialize logging
    QUENTIER_INITIALIZE_LOGGING();
    QUENTIER_SET_MIN_LOG_LEVEL(Trace);

    QNetworkAccessManager networkAccessManager;
    ENMLConverter enmlConverter;
    ErrorString errorDescription;

    QVector<Note> notes(1);
    Note & note = notes.back();

    int status = -1;
    {
        QTimer timer;
        timer.setInterval(600000);
        timer.setSingleShot(true);

        WikiArticleFetcher fetcher(&networkAccessManager, enmlConverter, url);

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
        qWarning() << "Failed to fetch wiki article in time\n";
        return 1;
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
        qWarning() << "Failed to fetch wiki article: "
            << errorDescription.nonLocalizedString() << "\n";
        return 1;
    }

    QString enex;
    errorDescription.clear();
    bool res = enmlConverter.exportNotesToEnex(notes, QHash<QString,QString>(),
                                               ENMLConverter::EnexExportTags::No,
                                               enex, errorDescription);
    if (!res) {
        qWarning() << "Failed to convert the fetched note to ENEX: "
            << errorDescription.nonLocalizedString() << "\n";
        return 1;
    }

    std::cout << enex.toStdString() << std::endl;
    return 0;
}
