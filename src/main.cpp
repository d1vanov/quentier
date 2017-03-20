/*
 * Copyright 2016 Dmitry Ivanov
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

#include "MainWindow.h"
#include "LoadDependencies.h"
#include "SetupApplicationIcon.h"
#include "CommandLineParser.h"
#include "SystemTrayIconManager.h"
#include "SettingsNames.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/QuentierApplication.h>
#include <quentier/utility/Utility.h>
#include <quentier/utility/DesktopServices.h>
#include <iostream>

int main(int argc, char *argv[])
{
    quentier::CommandLineParser cmdParser(argc, argv);
    if (cmdParser.shouldQuit())
    {
        if (cmdParser.hasError()) {
            std::cerr << cmdParser.errorDescription().nonLocalizedString().toLocal8Bit().constData();
            return 1;
        }

        std::cout << cmdParser.responseMessage().toLocal8Bit().constData();
        return 0;
    }

    typedef CommandLineParser::CommandLineOptions CmdOptions;
    CmdOptions cmdOptions = cmdParser.options();

    CmdOptions::const_iterator storageDirIt = cmdOptions.find(QStringLiteral("storageDir"));
    if (storageDirIt != cmdOptions.constEnd()) {
        QString storageDir = storageDirIt.value().toString();
        qputenv(LIBQUENTIER_PERSISTENCE_STORAGE_PATH, storageDir.toLocal8Bit());
    }

    CmdOptions::const_iterator overrideSystemTrayAvailabilityIt =
            cmdOptions.find(QStringLiteral("overrideSystemTrayAvailability"));
    if (overrideSystemTrayAvailabilityIt != cmdOptions.constEnd())
    {
        bool overrideSystemTrayAvailability = overrideSystemTrayAvailabilityIt.value().toBool();
        qputenv(OVERRIDE_SYSTEM_TRAY_AVAILABILITY_ENV_VAR,
                (overrideSystemTrayAvailability ? QByteArray("1") : QByteArray("0")));
    }

    quentier::QuentierApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("Quentier"));
    app.setQuitOnLastWindowClosed(false);

    QUENTIER_INITIALIZE_LOGGING();
    QUENTIER_SET_MIN_LOG_LEVEL(Trace);
    QUENTIER_ADD_STDOUT_LOG_DESTINATION();

    loadDependencies();

    quentier::initializeLibquentier();
    quentier::setupApplicationIcon(app);

    MainWindow mainWindow;

    const SystemTrayIconManager & systemTrayIconManager = mainWindow.systemTrayIconManager();
    if (!systemTrayIconManager.shouldStartMinimizedToSystemTray()) {
        mainWindow.show();
    }
    else {
        QNDEBUG(QStringLiteral("Not showing the main window on startup because the start "
                               "minimized to system tray was requested"));
    }

    return app.exec();
}
