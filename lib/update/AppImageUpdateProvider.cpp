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
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>
#include <QTextStream>

#include <AppImageUpdaterBridge>

#include <cstdio>

namespace quentier {

AppImageUpdateProvider::AppImageUpdateProvider(QObject * parent) :
    IUpdateProvider(nullptr, parent)
{}

AppImageUpdateProvider::~AppImageUpdateProvider() = default;

bool AppImageUpdateProvider::canCancelUpdate()
{
    // AppImageUpdateProvider only downloads the update in fact, it doesn't
    // mess with the existing installation so it can be safely interrupted
    return true;
}

bool AppImageUpdateProvider::inProgress()
{
    return m_pDeltaRevisioner.get();
}

void AppImageUpdateProvider::prepareForRestart()
{
    QString replaceAppImageScriptFileNameTemplate =
        QStringLiteral("replace_quentier_appimage_XXXXXX.sh");

    QTemporaryFile replaceAppImageScriptFile(
        replaceAppImageScriptFileNameTemplate);

    if (Q_UNLIKELY(!replaceAppImageScriptFile.open())) {
        QNWARNING("Failed to open temporary file to write restart script: "
            << replaceAppImageScriptFile.errorString());
        return;
    }

    QFileInfo replaceAppImageScriptFileInfo(replaceAppImageScriptFile);
    QTextStream replaceAppImageScriptStrm(&replaceAppImageScriptFile);
    auto escapedOldVersionFilePath = m_oldVersionFilePath;

    escapedOldVersionFilePath.replace(
        QStringLiteral(" "),
        QStringLiteral("\\ "));

    // Delete previous backup in case it was left around
    replaceAppImageScriptStrm << "rm -rf "
        << escapedOldVersionFilePath
        << ".bak\n";

    // Move old file to backup
    replaceAppImageScriptStrm << "mv "
        << escapedOldVersionFilePath << " "
        << escapedOldVersionFilePath << ".bak\n";

    // Move freshly downloaded app to the place where the existing
    // installation used to reside
    auto escapedNewVersionFilePath = m_newVersionFilePath;

    escapedNewVersionFilePath.replace(
        QStringLiteral(" "),
        QStringLiteral("\\ "));

    replaceAppImageScriptStrm << "mv " << escapedNewVersionFilePath
        << " " << escapedOldVersionFilePath << "\n";

    // Remove backup
    replaceAppImageScriptStrm << "rm -rf "
        << escapedOldVersionFilePath << ".bak\n";

    replaceAppImageScriptStrm.flush();

    // Make the script file executable
    int chmodRes = QProcess::execute(
        QStringLiteral("chmod"),
        QStringList()
            << QStringLiteral("755")
            << replaceAppImageScriptFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(chmodRes != 0)) {
        QNWARNING("Failed to mark the AppImage replacement script file "
            << "executable: "
            << replaceAppImageScriptFileInfo.absoluteFilePath());
        return;
    }

    // Execute the script
    int scriptRes = QProcess::execute(
        QStringLiteral("sh"),
        QStringList() << replaceAppImageScriptFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(scriptRes != 0)) {
        QNWARNING("Failed to execute AppImage replacement script");
    }
}

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

void AppImageUpdateProvider::cancel()
{
    QNDEBUG("AppImageUpdateProvider::cancel");

    recycleDeltaRevisioner();
    Q_EMIT cancelled();
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

    m_oldVersionFilePath = oldVersionPath;

    m_newVersionFilePath =
        newVersionDetails[QStringLiteral("AbsolutePath")].toString();

    QVariantMap updateProviderInfoMap;
    updateProviderInfoMap[QStringLiteral("OldVersionPath")] = oldVersionPath;

    updateProviderInfoMap[QStringLiteral("NewVersionPath")] =
        m_newVersionFilePath;

    recycleDeltaRevisioner();

    Q_EMIT finished(
        /* status = */ true,
        ErrorString(),
        /* needs restart = */ true,
        UpdateProvider::APPIMAGE,
        QJsonObject::fromVariantMap(updateProviderInfoMap));
}

void AppImageUpdateProvider::onError(qint16 errorCode)
{
    auto errorDescription = AppImageUpdaterBridge::errorCodeToString(errorCode);

    QNDEBUG("AppImageUpdateProvider::onError: error code = " << errorCode
        << ": " << errorDescription);

    ErrorString error(QT_TR_NOOP("Failed to update AppImage"));
    error.details() = errorDescription;

    recycleDeltaRevisioner();

    Q_EMIT finished(
        /* status = */ false,
        error,
        /* needs restart = */ false,
        UpdateProvider::APPIMAGE,
        {});
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

void AppImageUpdateProvider::recycleDeltaRevisioner()
{
    QNDEBUG("AppImageUpdateProvider::recycleDeltaRevisioner");

    m_pDeltaRevisioner->disconnect(this);
    m_pDeltaRevisioner->deleteLater();

    Q_UNUSED(m_pDeltaRevisioner.release())
    m_pDeltaRevisioner.reset();
}

} // namespace quentier
