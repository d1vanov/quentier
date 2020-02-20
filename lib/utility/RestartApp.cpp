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

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>

#include <iostream>

namespace quentier {

void restartApp(int argc, char * argv[], int delaySeconds)
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

    restartScriptFile.setAutoRemove(false);

    QTextStream restartScriptStrm(&restartScriptFile);

    QString commandLine;
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
    restartScriptStrm << app.applicationFilePath();
    if (argc > 1) {
        restartScriptStrm << " ";
        restartScriptStrm << app.arguments().join(QStringLiteral(" "));
    }

    restartScriptStrm.flush();

    QFileInfo restartScriptFileInfo(restartScriptFile);

    bool res = QProcess::startDetached(
        restartScriptFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(!res)) {
        std::cerr << "Failed to launch script for application restart"
            << std::endl;
    }
}

} // namespace quentier
