/*
 * Copyright 2020 Dmitry Ivanov
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
    IUpdateChecker(parent)
{}

AppImageUpdateChecker::~AppImageUpdateChecker() = default;

void AppImageUpdateChecker::checkForUpdates()
{
    QNDEBUG("update", "AppImageUpdateChecker::checkForUpdates");

    if (m_pDeltaRevisioner) {
        QNDEBUG("update", "Checking for updates is already in progress");
        return;
    }

    m_pDeltaRevisioner = std::make_unique<AppImageDeltaRevisioner>();

    QObject::connect(
        m_pDeltaRevisioner.get(),
        &AppImageDeltaRevisioner::updateAvailable,
        this,
        &AppImageUpdateChecker::onCheckForUpdatesReady);

    QObject::connect(
        m_pDeltaRevisioner.get(),
        &AppImageDeltaRevisioner::error,
        this,
        &AppImageUpdateChecker::onCheckForUpdatesError);

    QObject::connect(
        m_pDeltaRevisioner.get(),
        &AppImageDeltaRevisioner::logger,
        this,
        &AppImageUpdateChecker::onLogEntry);

    m_pDeltaRevisioner->checkForUpdate();
}

void AppImageUpdateChecker::onCheckForUpdatesReady(
    bool ready, QJsonObject updateInfo)
{
    QNDEBUG("update", "AppImageUpdateChecker::onCheckForUpdatesReady: updates "
        << "available = " << (ready ? "true" : "false") << ", update info: "
        << updateInfo);

    m_pDeltaRevisioner->deleteLater();
    Q_UNUSED(m_pDeltaRevisioner.release());

    if (!ready) {
        Q_EMIT noUpdatesAvailable();
        return;
    }

    auto provider = std::make_shared<AppImageUpdateProvider>();
    Q_EMIT updatesAvailable(provider);
}

void AppImageUpdateChecker::onCheckForUpdatesError(qint16 errorCode)
{
    auto errorDescription = AppImageUpdaterBridge::errorCodeToString(errorCode);

    QNWARNING("update", "AppImageUpdateChecker::onCheckForUpdatesError: "
        << "error code = " << errorCode << ": " << errorDescription);

    ErrorString error(QT_TR_NOOP("Failed to check for AppImage updates"));
    error.details() = errorDescription;

    Q_EMIT failure(error);
}

void AppImageUpdateChecker::onLogEntry(QString message, QString appImagePath)
{
    message = message.trimmed();
    if (message.startsWith(QStringLiteral("FATAL"))) {
        QNERROR("update", "[" << appImagePath << "]: " << message);
    }
    else if (message.startsWith(QStringLiteral("WARNING"))) {
        QNWARNING("update", "[" << appImagePath << "]: " << message);
    }
    else {
        QNDEBUG("update", "[" << appImagePath << "]: " << message);
    }
}

} // namespace quentier
