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

#include "WikiArticleFetcher.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QCoreApplication>
#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QUrl>

#include <iostream>

using namespace quentier;

int main(int argc, char * argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    qsrand(static_cast<quint32>(QTime::currentTime().msec()));
#endif

    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("quentier.org"));
    QCoreApplication::setApplicationName(QStringLiteral("wiki2enex"));

    const QStringList args = QCoreApplication::arguments();
    if (args.size() != 2) {
        qWarning() << "Usage: " << QCoreApplication::applicationName()
                   << " wiki-article-url\n";
        return 1;
    }

    QUrl url{args[1]};
    if (!url.isValid()) {
        qWarning() << "Not a valid URL\n";
        return 1;
    }

    // Initialize logging
    QUENTIER_INITIALIZE_LOGGING();
    QUENTIER_SET_MIN_LOG_LEVEL(Trace);

    ENMLConverter enmlConverter;
    ErrorString errorDescription;

    QList<qevercloud::Note> notes;
    notes.push_back(qevercloud::Note{});
    auto & note = notes.back();

    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(600000);
        timer.setSingleShot(true);

        WikiArticleFetcher fetcher(enmlConverter, url);
        EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &fetcher, &WikiArticleFetcher::finished, &loop,
            &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &fetcher, &WikiArticleFetcher::failure, &loop,
            &EventLoopWithExitStatus::exitAsFailureWithErrorString);

        timer.start();
        QTimer::singleShot(0, &fetcher, SLOT(start()));
        Q_UNUSED(loop.exec())
        status = loop.exitStatus();
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

    bool res = enmlConverter.exportNotesToEnex(
        notes, QHash<QString, QString>(), ENMLConverter::EnexExportTags::No,
        enex, errorDescription);

    if (!res) {
        qWarning() << "Failed to convert the fetched note to ENEX: "
                   << errorDescription.nonLocalizedString() << "\n";
        return 1;
    }

    std::cout << enex.toStdString() << std::endl;
    return 0;
}
