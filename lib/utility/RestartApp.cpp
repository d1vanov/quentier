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

#include "RestartApp.h"

#include <quentier/utility/Macros.h>

#include <lib/utility/VersionInfo.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegExp>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>

#include <iostream>

namespace quentier {

void restartApp(int argc, char * argv[], int delaySeconds)
{
    QString restartScriptFileNameTemplate =
        QStringLiteral("/quentier_restart_script_XXXXXX.");

#ifdef Q_OS_WIN
    restartScriptFileNameTemplate += QStringLiteral("bat");
#else
    restartScriptFileNameTemplate += QStringLiteral("sh");
#endif

    QTextStream Cerr(stderr);

    QTemporaryFile restartScriptFile(
        QDir::tempPath() + restartScriptFileNameTemplate);

    // Keep the file for a while in case its contents would need to be
    // investigated
    restartScriptFile.setAutoRemove(false);

    if (Q_UNLIKELY(!restartScriptFile.open())) {
        Cerr << "Failed to open temporary file to write restart script: "
            << restartScriptFile.errorString() << "\n";
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
    QString appFilePath = app.applicationFilePath();

    // Write instructions to the script to start app within the new installation
#if defined(Q_OS_MAC)
    int appSuffixIndex = appFilePath.lastIndexOf(QStringLiteral(".app"));
    if (Q_UNLIKELY(appSuffixIndex < 0))
    {
        appFilePath.replace(
            QStringLiteral(" "),
            QStringLiteral("\\ "));

        restartScriptStrm << appFilePath;
        if (argc > 1) {
            restartScriptStrm << " ";
            restartScriptStrm << app.arguments().join(QStringLiteral(" "));
        }
    }
    else
    {
        QString currentAppInstallationBundlePath =
            appFilePath.left(appSuffixIndex) + QStringLiteral(".app");

        currentAppInstallationBundlePath.replace(
            QStringLiteral(" "),
            QStringLiteral("\\ "));

        restartScriptStrm << "open " << currentAppInstallationBundlePath
            << "\n";
    }
#elif defined(Q_OS_WIN)
    appFilePath.prepend(QStringLiteral("\""));
    appFilePath.append(QStringLiteral("\""));

    restartScriptStrm << appFilePath;
    if (argc > 1) {
        restartScriptStrm << " ";
        restartScriptStrm << app.arguments().join(QStringLiteral(" "));
    }
#else
#if QUENTIER_PACKAGED_AS_APP_IMAGE
    Cerr << "Preparing Quentier AppImage for restart\n";

    auto appImageFilePath = QProcessEnvironment::systemEnvironment().value(
        QStringLiteral("APPIMAGE"));

    Cerr << "AppImage file path from process envitonment: " << appImageFilePath
        << "\n";

    if (!appImageFilePath.isEmpty())
    {
        // Check if it's AppImageLauncher's path, if so then use the map file
        // to get the actual AppImage path
        QRegExp rx(QStringLiteral("/run/user/*/appimagelauncherfs/*.AppImage"));
        rx.setPatternSyntax(QRegExp::Wildcard);

        if (rx.exactMatch(appImageFilePath))
        {
            Cerr << "AppImageLauncher is installed\n";
            auto arguments = QCoreApplication::arguments();
            if(!arguments.isEmpty()) {
                appImageFilePath = QFileInfo(arguments.at(0)).absolutePath() +
                    QStringLiteral("/") +
                    QFileInfo(arguments.at(0)).fileName();
                Cerr << "AppImage file path from program arguments: "
                    << appImageFilePath << "\n";
            }
            else {
                appImageFilePath = QString();
            }
        }
    }

    if (!appImageFilePath.isEmpty()) {
        appFilePath = appImageFilePath;
    }
#endif

    Cerr << "Quentier app file path to be restarted: " << appFilePath << "\n";

    appFilePath.replace(
        QStringLiteral(" "),
        QStringLiteral("\\ "));

    restartScriptStrm << appFilePath;
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
        Cerr << "Failed to mark the temporary script file executable: "
            << restartScriptFileInfo.absoluteFilePath()
            << "\n";
        return;
    }
#endif

    // 5) Launch the script
    QStringList args;
#ifndef Q_OS_WIN
    args += QStringLiteral("sh ");
#endif
    args += restartScriptFileInfo.absoluteFilePath();

    QString program = args.takeFirst();
    bool res = QProcess::startDetached(program, args);
    if (Q_UNLIKELY(!res)) {
        Cerr << "Failed to launch script for application restart\n";
    }
}

} // namespace quentier
