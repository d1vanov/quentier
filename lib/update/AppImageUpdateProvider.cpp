/*
 * Copyright 2020-2025 Dmitry Ivanov
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
#include <quentier/utility/FileSystem.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>
#include <QTextStream>

#include <AppImageUpdaterBridge>

#include <cstdio>

namespace quentier {

AppImageUpdateProvider::AppImageUpdateProvider(QObject * parent) :
    IUpdateProvider{nullptr, parent}
{}

AppImageUpdateProvider::~AppImageUpdateProvider() = default;

bool AppImageUpdateProvider::canCancelUpdate()
{
    return m_canCancelUpdate;
}

bool AppImageUpdateProvider::inProgress()
{
    return m_deltaRevisioner.get();
}

void AppImageUpdateProvider::run()
{
    QNDEBUG("update::AppImageUpdateProvider", "AppImageUpdateProvider::run");

    if (m_deltaRevisioner) {
        QNDEBUG("update::AppImageUpdateProvider", "Update is already running");
        return;
    }

    m_deltaRevisioner = std::make_unique<AppImageDeltaRevisioner>();

    QObject::connect(
        m_deltaRevisioner.get(), &AppImageDeltaRevisioner::started, this,
        &AppImageUpdateProvider::onStarted);

    QObject::connect(
        m_deltaRevisioner.get(), &AppImageDeltaRevisioner::finished, this,
        &AppImageUpdateProvider::onFinished);

    QObject::connect(
        m_deltaRevisioner.get(), &AppImageDeltaRevisioner::error, this,
        &AppImageUpdateProvider::onError);

    QObject::connect(
        m_deltaRevisioner.get(), &AppImageDeltaRevisioner::progress, this,
        &AppImageUpdateProvider::onProgress);

    QObject::connect(
        m_deltaRevisioner.get(), &AppImageDeltaRevisioner::logger, this,
        &AppImageUpdateProvider::onLogEntry);

    m_deltaRevisioner->start();
}

void AppImageUpdateProvider::cancel()
{
    QNDEBUG("update::AppImageUpdateProvider", "AppImageUpdateProvider::cancel");

    recycleDeltaRevisioner();
    Q_EMIT cancelled();
}

void AppImageUpdateProvider::onStarted()
{
    QNDEBUG(
        "update::AppImageUpdateProvider", "AppImageUpdateProvider::onStarted");
}

void AppImageUpdateProvider::onFinished(
    QJsonObject newVersionDetails, QString oldVersionPath)
{
    QNDEBUG(
        "update::AppImageUpdateProvider",
        "AppImageUpdateProvider::onFinished: old version path = "
            << oldVersionPath
            << ", new version details = " << newVersionDetails);

    auto newVersionPath =
        newVersionDetails[QStringLiteral("AbsolutePath")].toString();

    recycleDeltaRevisioner();

    m_canCancelUpdate = false;

    ErrorString errorDescription;
    if (!replaceAppImage(
            oldVersionPath, std::move(newVersionPath), errorDescription))
    {
        m_canCancelUpdate = true;

        Q_EMIT finished(
            /* status = */ false, errorDescription,
            /* needs restart = */ false, UpdateProvider::APPIMAGE);

        return;
    }

    m_canCancelUpdate = true;

    Q_EMIT finished(
        /* status = */ true, ErrorString{},
        /* needs restart = */ true, UpdateProvider::APPIMAGE);
}

void AppImageUpdateProvider::onError(qint16 errorCode)
{
    auto errorDescription = AppImageUpdaterBridge::errorCodeToString(errorCode);

    QNDEBUG(
        "update::AppImageUpdateProvider",
        "AppImageUpdateProvider::onError: error code = " << errorCode << ": "
                                                         << errorDescription);

    ErrorString error{QT_TR_NOOP("Failed to update AppImage")};
    error.details() = std::move(errorDescription);

    recycleDeltaRevisioner();

    Q_EMIT finished(
        /* status = */ false, error,
        /* needs restart = */ false, UpdateProvider::APPIMAGE);
}

void AppImageUpdateProvider::onProgress(
    int percentage, const qint64 bytesReceived, const qint64 bytesTotal,
    const double indeterminateSpeed, const QString speedUnits)
{
    QNDEBUG(
        "update::AppImageUpdateProvider",
        "AppImageUpdateProvider::onProgress: percentage = "
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

void AppImageUpdateProvider::onLogEntry(
    QString message, const QString & appImagePath)
{
    message = message.trimmed();
    if (message.startsWith(QStringLiteral("FATAL"))) {
        QNERROR(
            "update::AppImageUpdateProvider",
            "[" << appImagePath << "]: " << message);
    }
    else if (message.startsWith(QStringLiteral("WARNING"))) {
        QNWARNING(
            "update::AppImageUpdateProvider",
            "[" << appImagePath << "]: " << message);
    }
    else {
        QNDEBUG(
            "update::AppImageUpdateProvider",
            "[" << appImagePath << "]: " << message);
    }
}

bool AppImageUpdateProvider::replaceAppImage(
    const QString & oldVersionPath, const QString & newVersionPath,
    ErrorString errorDescription)
{
    QNDEBUG(
        "update::AppImageUpdateProvider",
        "AppImageUpdateProvider::replaceAppImage");

    QString oldVersionBackupPath = oldVersionPath + QStringLiteral(".bak");

    auto removeBackup = [&] {
        QFileInfo oldVersionBackupInfo(oldVersionBackupPath);
        if (oldVersionBackupInfo.exists()) {
            if (oldVersionBackupInfo.isFile()) {
                if (!utility::removeFile(oldVersionBackupPath)) {
                    errorDescription.setBase(
                        QT_TR_NOOP("Failed to remove previous AppImage "
                                   "backup"));

                    errorDescription.details() = oldVersionBackupPath;
                    QNWARNING(
                        "update::AppImageUpdateProvider", errorDescription);
                    return false;
                }
            }
            else if (!utility::removeDir(oldVersionBackupPath)) {
                errorDescription.setBase(
                    QT_TR_NOOP("Failed to remove dir in place of AppImage "
                               "backup"));

                errorDescription.details() = oldVersionBackupPath;
            }
        }

        return true;
    };

    // 1) Remove backup if it exists
    if (!removeBackup()) {
        return false;
    }

    // 2) Backup the original AppImage
    ErrorString error;
    if (!utility::renameFile(oldVersionPath, oldVersionBackupPath, error)) {
        errorDescription.setBase(
            QT_TR_NOOP("Failed to backup old version of AppImage"));

        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("update::AppImageUpdateProvider", errorDescription);
        return false;
    }

    // 3) Move new AppImage in place of the old one
    error.clear();
    if (!utility::renameFile(newVersionPath, oldVersionPath, error)) {
        errorDescription.setBase(
            QT_TR_NOOP("Failed to move the new AppImage in place of "
                       "the old one"));

        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("update::AppImageUpdateProvider", errorDescription);

        // Best effort undo: try to restore the backup
        error.clear();
        if (!utility::renameFile(oldVersionBackupPath, oldVersionPath, error)) {
            QNWARNING(
                "update::AppImageUpdateProvider",
                "Failed to restore AppImage from backup: "
                    << error << "; backup path: " << oldVersionBackupPath);
        }

        return false;
    }

    // 4) Remove backup
    if (!removeBackup()) {
        return false;
    }

    return true;
}

void AppImageUpdateProvider::recycleDeltaRevisioner()
{
    QNDEBUG(
        "update::AppImageUpdateProvider",
        "AppImageUpdateProvider::recycleDeltaRevisioner");

    m_deltaRevisioner->disconnect(this);
    m_deltaRevisioner->deleteLater();

    Q_UNUSED(m_deltaRevisioner.release())
    m_deltaRevisioner.reset();
}

} // namespace quentier
