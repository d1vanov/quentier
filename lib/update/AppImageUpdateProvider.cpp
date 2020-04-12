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

#include "AppImageUpdateProvider.h"

#include <quentier/logging/QuentierLogger.h>

#include <QCoreApplication>

#include <AppImageUpdaterBridge>

namespace quentier {

AppImageUpdateProvider::AppImageUpdateProvider(QObject * parent) :
    IUpdateProvider(nullptr, parent)
{}

AppImageUpdateProvider::~AppImageUpdateProvider() = default;

void AppImageUpdateProvider::run()
{
    QNDEBUG("AppImageUpdateProvider::run");

    if (m_pDeltaRevisioner) {
        QNDEBUG("Update is already running");
        return;
    }

    m_pDeltaRevisioner = std::make_unique<AppImageDeltaRevisioner>();

    QObject::connect(
        m_pDeltaRevisioner.get(),
        &AppImageDeltaRevisioner::started,
        this,
        &AppImageUpdateProvider::onStarted);

    QObject::connect(
        m_pDeltaRevisioner.get(),
        &AppImageDeltaRevisioner::finished,
        this,
        &AppImageUpdateProvider::onFinished);

    QObject::connect(
        m_pDeltaRevisioner.get(),
        &AppImageDeltaRevisioner::error,
        this,
        &AppImageUpdateProvider::onError);

    QObject::connect(
        m_pDeltaRevisioner.get(),
        &AppImageDeltaRevisioner::progress,
        this,
        &AppImageUpdateProvider::onProgress);

    m_pDeltaRevisioner->start();
}

void AppImageUpdateProvider::onStarted()
{
    QNDEBUG("AppImageUpdateProvider::onStarted");
}

void AppImageUpdateProvider::onFinished(
    QJsonObject newVersionDetails, QString oldVersionPath)
{
    QNDEBUG("AppImageUpdateProvider::onFinished: old version path = "
        << oldVersionPath << ", new version details = "
        << newVersionDetails);

    Q_EMIT finished(true, ErrorString(), true);
}

void AppImageUpdateProvider::onError(qint16 errorCode)
{
    auto errorDescription = AppImageUpdaterBridge::errorCodeToString(errorCode);

    QNDEBUG("AppImageUpdateProvider::onError: error code = " << errorCode
        << ": " << errorDescription);

    ErrorString error(QT_TR_NOOP("Failed to update AppImage"));
    error.details() = errorDescription;

    Q_EMIT finished(false, error, false);
}

void AppImageUpdateProvider::onProgress(
    int percentage, qint64 bytesReceived, qint64 bytesTotal,
    double indeterminateSpeed, QString speedUnits)
{
    QNDEBUG("AppImageUpdateProvider::onProgress: percentage = "
        << percentage << ", bytes received = " << bytesReceived
        << ", bytes total = " << bytesTotal << ", indeterminate speed = "
        << indeterminateSpeed << " " << speedUnits);

    // Ensure the percentage has proper value
    if (Q_UNLIKELY(percentage < 0)) {
        percentage = 0;
    }

    if (Q_UNLIKELY(percentage > 100)) {
        percentage = 100;
    }

    Q_EMIT progress(percentage * 0.01, QString());
}

} // namespace quentier
