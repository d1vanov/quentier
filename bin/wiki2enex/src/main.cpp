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

#include "WikiArticleFetcher.h"

#include <quentier/enml/Factory.h>
#include <quentier/enml/IConverter.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/EventLoopWithExitStatus.h>

#include <qevercloud/types/Note.h>

#include <QCoreApplication>
#include <QDebug>
#include <QList>
#include <QTime>
#include <QTimer>
#include <QUrl>

#include <iostream>

using namespace quentier;

int main(int argc, char * argv[])
{
    QCoreApplication app{argc, argv};
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("wiki2enex"));

    const QStringList args = app.arguments();
    if (args.size() != 2) {
        qWarning() << "Usage: " << app.applicationName()
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

    auto enmlConverter = quentier::enml::createConverter();

    QList<qevercloud::Note> notes;
    notes << qevercloud::Note{};
    auto & note = notes.back();

    ErrorString errorDescription;
    auto status = utility::EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(600000);
        timer.setSingleShot(true);

        WikiArticleFetcher fetcher(enmlConverter, url);
        utility::EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &utility::EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &fetcher, &WikiArticleFetcher::finished, &loop,
            &utility::EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &fetcher, &WikiArticleFetcher::failure, &loop,
            &utility::EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(100);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &fetcher, SLOT(start()));
        Q_UNUSED(loop.exec())
        status = loop.exitStatus();
        errorDescription = loop.errorDescription();
        note = fetcher.note();
    }

    if (status == utility::EventLoopWithExitStatus::ExitStatus::Timeout) {
        qWarning() << "Failed to fetch wiki article in time\n";
        return 1;
    }

    if (status == utility::EventLoopWithExitStatus::ExitStatus::Failure) {
        qWarning() << "Failed to fetch wiki article: "
                   << errorDescription.nonLocalizedString() << "\n";
        return 1;
    }

    const auto res = enmlConverter->exportNotesToEnex(
        notes, QHash<QString, QString>{}, enml::IConverter::EnexExportTags::No);

    if (!res.isValid()) {
        qWarning() << "Failed to convert the fetched note to ENEX: "
                   << res.error().nonLocalizedString() << "\n";
        return 1;
    }

    std::cout << res.get().toStdString() << std::endl;
    return 0;
}
