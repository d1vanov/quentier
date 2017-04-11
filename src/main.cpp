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
#include <quentier/exception/DatabaseLockedException.h>
#include <quentier/exception/DatabaseOpeningException.h>
#include <quentier/exception/IQuentierException.h>
#include <QDebug>
#include <QScopedPointer>
#include <iostream>
#include <exception>

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

    QScopedPointer<MainWindow> pMainWindow;

    try
    {
        pMainWindow.reset(new MainWindow);

        const SystemTrayIconManager & systemTrayIconManager = pMainWindow->systemTrayIconManager();
        if (!systemTrayIconManager.shouldStartMinimizedToSystemTray()) {
            pMainWindow->show();
        }
        else {
            QNDEBUG(QStringLiteral("Not showing the main window on startup because the start "
                                   "minimized to system tray was requested"));
        }
    }
    catch(const quentier::DatabaseLockedException & dbLockedException)
    {
        criticalMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start"),
                           QObject::tr("Database is locked"),
                           QObject::tr("Quentier cannot start because its database is locked. "
                                       "It should only happen if another instance is already running "
                                       "and using the same account. Please either use the already running "
                                       "instance or quit it before opening the new one. If there is no "
                                       "already running instance, please report the problem to the developers "
                                       "of Quentier and try restarting your computer. Sorry for the inconvenience."
                                       "\n\n"
                                       "Exception message: ") + dbLockedException.localizedErrorMessage());
        qWarning() << QStringLiteral("Caught DatabaseLockedException: ") << dbLockedException.nonLocalizedErrorMessage();
        return 1;
    }
    catch(const quentier::DatabaseOpeningException & dbOpeningException)
    {
        criticalMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start"),
                           QObject::tr("Failed to open the local storage database"),
                           QObject::tr("Quentier cannot start because it could not open the database. "
                                       "On Windows it might happen if another instance is already running "
                                       "and using the same account. Otherwise it might indicate the corruption "
                                       "of account database file.\n\nException message: ") + dbOpeningException.localizedErrorMessage());
        qWarning() << QStringLiteral("Caught DatabaseOpeningException: ") << dbOpeningException.nonLocalizedErrorMessage();
        return 1;
    }
    catch(const quentier::IQuentierException & exception)
    {
        internalErrorMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start, exception occurred: ") + exception.localizedErrorMessage());
        qWarning() << QStringLiteral("Caught IQuentierException: ") << exception.nonLocalizedErrorMessage();
        return 1;
    }
    catch(const std::exception & exception)
    {
        internalErrorMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start, exception occurred: ") + exception.what());
        qWarning() << QStringLiteral("Caught IQuentierException: ") << exception.what();
        return 1;
    }

    return app.exec();
}
