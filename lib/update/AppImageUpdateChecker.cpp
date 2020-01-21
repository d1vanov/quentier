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
    QNDEBUG("AppImageUpdateChecker::checkForUpdates");

    if (m_pDeltaRevisioner) {
        QNDEBUG("Checking for updates is already in progress");
        return;
    }

    m_pDeltaRevisioner = std::make_unique<AppImageDeltaRevisioner>(
        QCoreApplication::applicationFilePath());

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
}

void AppImageUpdateChecker::onCheckForUpdatesReady(
    bool ready, QJsonObject updateInfo)
{
    QNDEBUG("AppImageUpdateChecker::onCheckForUpdatesReady: updates available = "
        << (ready ? "true" : "false") << ", update info: " << updateInfo);

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

    QNWARNING("AppImageUpdateChecker::onCheckForUpdatesError: error code = "
        << errorCode << ": " << errorDescription);

    ErrorString error(QT_TR_NOOP("Failed to check for AppImage updates"));
    error.details() = errorDescription;

    Q_EMIT failure(error);
}

} // namespace quentier
