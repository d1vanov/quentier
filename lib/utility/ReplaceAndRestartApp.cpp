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

#include "ReplaceAndRestartApp.h"

#include <quentier/utility/Macros.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGlobalStatic>
#include <QProcess>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>
#include <QWriteLocker>

#include <iostream>
#include <utility>

namespace quentier {

////////////////////////////////////////////////////////////////////////////////

class AppReplacementPathsHolder
{
public:
    AppReplacementPathsHolder() = default;

    QString bundlePath()
    {
        QReadLocker locker(&m_lock);
        return m_bundlePath;
    }

    QString binaryPath()
    {
        QReadLocker locker(&m_lock);
        return m_binaryPath;
    }

    std::pair<QString, QString> paths()
    {
        QReadLocker locker(&m_lock);
        return std::make_pair(m_bundlePath, m_binaryPath);
    }

    void setBundlePath(QString bundlePath)
    {
        QWriteLocker locker(&m_lock);
        m_bundlePath = std::move(bundlePath);
    }

    void setBinaryPath(QString binaryPath)
    {
        QWriteLocker locker(&m_lock);
        m_binaryPath = std::move(binaryPath);
    }

    void setPaths(QString bundlePath, QString binaryPath)
    {
        QWriteLocker locker(&m_lock);
        m_bundlePath = std::move(bundlePath);
        m_binaryPath = std::move(binaryPath);
    }

private:
    Q_DISABLE_COPY(AppReplacementPathsHolder)

private:
    QReadWriteLock  m_lock;
    QString     m_bundlePath;
    QString     m_binaryPath;
};

////////////////////////////////////////////////////////////////////////////////

Q_GLOBAL_STATIC(AppReplacementPathsHolder, appReplacementPathsHolder)

////////////////////////////////////////////////////////////////////////////////

void setAppReplacementPaths(QString bundlePath, QString binaryPath)
{
    appReplacementPathsHolder->setPaths(
        std::move(bundlePath),
        std::move(binaryPath));
}

void replaceAndRestartApp(int argc, char * argv[], int delaySeconds)
{
    QString restartScriptFileNameTemplate =
        QStringLiteral("quentier_restart_script_XXXXXX.");

#ifdef Q_OS_WIN
    restartScriptFileNameTemplate += QStringLiteral("bat");
#else
    restartScriptFileNameTemplate += QStringLiteral("sh");
#endif

    QTemporaryFile restartScriptFile(restartScriptFileNameTemplate);
    if (Q_UNLIKELY(!restartScriptFile.open())) {
        std::cerr << "Failed to open temporary file to write restart script: "
            << restartScriptFile.errorString().toStdString() << std::endl;
        return;
    }

    QFileInfo restartScriptFileInfo(restartScriptFile);
    QTextStream restartScriptStrm(&restartScriptFile);

    if (delaySeconds > 0)
    {
#ifdef Q_OS_WIN
        // NOTE: hack implementing sleep via ping to nonexistent address
        restartScriptStrm << "ping 192.0.2.2 -n 1 -w ";
        restartScriptStrm << QString::number(delaySeconds * 1000);
        restartScriptStrm << " > nul\r\n";
#else
        restartScriptStrm << "#!/bin/sh\n";
        restartScriptStrm << "sleep ";
        restartScriptStrm << QString::number(delaySeconds);
        restartScriptStrm << "\n";
#endif
    }

    QCoreApplication app(argc, argv);

    // 1) Find existing app installation bundle path

    QString currentAppInstallationBundlePath;
    QString quentierBinaryPathWithinBundle;
    QString appFilePath = app.applicationFilePath();

#if defined(Q_OS_MAC)
    bool isAppBundle = true;
    int appSuffixIndex = appFilePath.lastIndexOf(QStringLiteral(".app"));
    if (Q_UNLIKELY(appSuffixIndex < 0))
    {
        isAppBundle = false;
        currentAppInstallationBundlePath = appFilePath;
        quentierBinaryPathWithinBundle.clear();
    }
    else
    {
        currentAppInstallationBundlePath =
            appFilePath.left(appSuffixIndex) + QStringLiteral(".app");

        quentierBinaryPathWithinBundle = appFilePath;
        quentierBinaryPathWithinBundle.remove(0, appSuffixIndex + 4);
    }
#elif defined(Q_OS_WIN)
    quentierBinaryPathWithinBundle = QStringLiteral("/bin/quentier/quentier.exe");
    currentAppInstallationBundlePath = appFilePath;
    currentAppInstallationBundlePath.chop(
        quentierBinaryPathWithinBundle.size());
#elif QUENTIER_PACKAGED_AS_APP_IMAGE
    currentAppInstallationBundlePath = appFilePath;
    quentierBinaryPathWithinBundle.clear();
#else
    Q_UNUSED(appFilePath)
    Q_UNUSED(currentAppInstallationBundlePath)
    Q_UNUSED(quentierBinaryPathWithinBundle)
#endif

    auto paths = appReplacementPathsHolder->paths();

    // 2) Write instructions to the script to replace the existing app
    // installation with the new one

    if (!currentAppInstallationBundlePath.isEmpty())
    {
#ifdef Q_OS_WIN
        auto nativeCurrentAppInstallationBundlePath = QDir::toNativeSeparators(
            currentAppInstallationBundlePath);

        // Delete previous backup in case it was left around
        restartScriptStrm << "rd /s /q \""
            << nativeCurrentAppInstallationBundlePath
            << "_backup\"\r\n";

        // Move existing app installation to backup location
        restartScriptStrm << "move /y \""
            << nativeCurrentAppInstallationBundlePath
            << "\" \""
            << nativeCurrentAppInstallationBundlePath
            << "_backup\"\r\n";

        // Move freshly downloaded app to the place where the existing
        // installation used to reside
        restartScriptStrm << "move /y \""
            << QDir::toNativeSeparators(paths.first)
            << "\" \""
            << nativeCurrentAppInstallationBundlePath
            << "\"\r\n";

        // Remove backup
        restartScriptStrm << "rd /s /q \""
            << nativeCurrentAppInstallationBundlePath
            << "_backup\"\r\n";
#else
        auto escapedCurrentAppInstallationBundlePath =
            currentAppInstallationBundlePath;

        escapedCurrentAppInstallationBundlePath.replace(
            QStringLiteral(" "),
            QStringLiteral("\\ "));

        // Delete previous backup in case it was left around
        restartScriptStrm << "rm -rf "
            << escapedCurrentAppInstallationBundlePath
            << "_backup\n";

        // Move existing app installation to backup location
        restartScriptStrm << "mv "
            << escapedCurrentAppInstallationBundlePath
            << " "
            << escapedCurrentAppInstallationBundlePath
            << "_backup\n";

        // Move freshly downloaded app to the place where the existing
        // installation used to reside
        auto escapedNewAppBundlePath = paths.first;
        escapedNewAppBundlePath.replace(
            QStringLiteral(" "),
            QStringLiteral("\\ "));

        restartScriptStrm << "mv " << escapedNewAppBundlePath
            << " " << escapedCurrentAppInstallationBundlePath << "\n";

        // Remove backup
        restartScriptStrm << "rm -rf "
            << escapedCurrentAppInstallationBundlePath
            << "_backup\n";
#endif
    }

    // 3) Write instructions to the script to start app within the new
    // installation
#ifdef Q_OS_MAC
    if (!isAppBundle)
    {
        restartScriptStrm << appFilePath;
        if (argc > 1) {
            restartScriptStrm << " ";
            restartScriptStrm << app.arguments().join(QStringLiteral(" "));
        }
    }
    else
    {
        restartScriptStrm << "open " << currentAppInstallationBundlePath
            << "\n";
    }
#else
    restartScriptStrm << app.applicationFilePath();
    if (argc > 1) {
        restartScriptStrm << " ";
        restartScriptStrm << app.arguments().join(QStringLiteral(" "));
    }
#endif

    restartScriptStrm.flush();

#ifndef Q_OS_WIN
    // 4) Make the script file executable
    int chmodRes = QProcess::execute(
        QStringLiteral("chmod"),
        QStringList()
            << QStringLiteral("755")
            << restartScriptFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(chmodRes != 0)) {
        std::cerr << "Failed to mark the temporary script file executable: "
            << restartScriptFileInfo.absoluteFilePath().toStdString()
            << std::endl;
        return;
    }
#endif

    // 5) Launch the script
    QString commandLine;
#ifndef Q_OS_WIN
    commandLine += QStringLiteral("sh ");
#endif
    commandLine += restartScriptFileInfo.absoluteFilePath();

    bool res = QProcess::startDetached(commandLine);
    if (Q_UNLIKELY(!res)) {
        std::cerr << "Failed to launch script for application restart"
            << std::endl;
    }
}

} // namespace quentier
