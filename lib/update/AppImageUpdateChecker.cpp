/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include "AppImageUpdateChecker.h"
#include "AppImageUpdateProvider.h"
#include "appimageupdaterbridge_enums.hpp"

#include <quentier/logging/QuentierLogger.h>

#include <QCoreApplication>

#include <AppImageUpdaterBridge>

namespace quentier {

AppImageUpdateChecker::AppImageUpdateChecker(QObject * parent) :
    IUpdateChecker{parent}
{}

AppImageUpdateChecker::~AppImageUpdateChecker() = default;

void AppImageUpdateChecker::checkForUpdates()
{
    QNDEBUG(
        "update::AppImageUpdateChecker",
        "AppImageUpdateChecker::checkForUpdates");

    if (m_deltaRevisioner) {
        QNDEBUG(
            "update::AppImageUpdateChecker",
            "Checking for updates is already in progress");
        return;
    }

    m_deltaRevisioner = std::make_unique<AppImageDeltaRevisioner>();

    QObject::connect(
        m_deltaRevisioner.get(), &AppImageDeltaRevisioner::updateAvailable,
        this, &AppImageUpdateChecker::onCheckForUpdatesReady);

    QObject::connect(
        m_deltaRevisioner.get(), &AppImageDeltaRevisioner::error, this,
        &AppImageUpdateChecker::onCheckForUpdatesError);

    QObject::connect(
        m_deltaRevisioner.get(), &AppImageDeltaRevisioner::logger, this,
        &AppImageUpdateChecker::onLogEntry);

    m_deltaRevisioner->checkForUpdate();
}

void AppImageUpdateChecker::onCheckForUpdatesReady(
    const bool ready, QJsonObject updateInfo)
{
    QNDEBUG(
        "update::AppImageUpdateChecker",
        "AppImageUpdateChecker::onCheckForUpdatesReady: updates available = "
            << (ready ? "true" : "false") << ", update info: " << updateInfo);

    m_deltaRevisioner->deleteLater();
    Q_UNUSED(m_deltaRevisioner.release());

    if (!ready) {
        Q_EMIT noUpdatesAvailable();
        return;
    }

    auto provider = std::make_shared<AppImageUpdateProvider>();
    Q_EMIT updatesAvailable(std::move(provider));
}

void AppImageUpdateChecker::onCheckForUpdatesError(const qint16 errorCode)
{
    auto errorDescription = AppImageUpdaterBridge::errorCodeToString(errorCode);

    QNWARNING(
        "update::AppImageUpdateChecker",
        "AppImageUpdateChecker::onCheckForUpdatesError: "
            << "error code = " << errorCode << ": " << errorDescription);

    ErrorString error{QT_TR_NOOP("Failed to check for AppImage updates")};
    error.details() = std::move(errorDescription);

    Q_EMIT failure(std::move(error));
}

void AppImageUpdateChecker::onLogEntry(
    QString message, const QString & appImagePath)
{
    message = message.trimmed();

    if (message.startsWith(QStringLiteral("FATAL"))) {
        QNERROR(
            "update::AppImageUpdateChecker",
            "[" << appImagePath << "]: " << message);
    }
    else if (message.startsWith(QStringLiteral("WARNING"))) {
        QNWARNING(
            "update::AppImageUpdateChecker",
            "[" << appImagePath << "]: " << message);
    }
    else {
        QNDEBUG(
            "update::AppImageUpdateChecker",
            "[" << appImagePath << "]: " << message);
    }
}

} // namespace quentier
