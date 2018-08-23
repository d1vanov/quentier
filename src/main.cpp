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
#include "SystemTrayIconManager.h"
#include "exception/LocalStorageVersionTooHighException.h"
#include "initialization/Initialize.h"
#include "initialization/LoadDependencies.h"
#include <quentier/utility/QuentierApplication.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/exception/DatabaseLockedException.h>
#include <quentier/exception/DatabaseOpeningException.h>
#include <quentier/exception/IQuentierException.h>
#include <QScopedPointer>
#include <iostream>
#include <exception>

using namespace quentier;

int main(int argc, char *argv[])
{
    // Loading the dependencies manually - required on Windows
    loadDependencies();

    ParseCommandLineResult parseCmdResult;
    parseCommandLine(argc, argv, parseCmdResult);
    if (parseCmdResult.m_shouldQuit)
    {
        if (!parseCmdResult.m_errorDescription.isEmpty()) {
            std::cerr << parseCmdResult.m_errorDescription.nonLocalizedString().toLocal8Bit().constData();
            return 1;
        }

        std::cout << parseCmdResult.m_responseMessage.toLocal8Bit().constData();
        return 0;
    }

    QuentierApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("Quentier"));
    app.setQuitOnLastWindowClosed(false);

    bool res = initialize(app, parseCmdResult.m_cmdOptions);
    if (!res) {
        return 1;
    }

    QScopedPointer<MainWindow> pMainWindow;

    try
    {
        pMainWindow.reset(new MainWindow);

        bool shouldStartMinimizedToSystemTray = false;
        auto startMinimizedToTrayIt = parseCmdResult.m_cmdOptions.find(QStringLiteral("startMinimizedToTray"));
        if (startMinimizedToTrayIt != parseCmdResult.m_cmdOptions.end()) {
            shouldStartMinimizedToSystemTray = true;
        }
        else {
            const SystemTrayIconManager & systemTrayIconManager = pMainWindow->systemTrayIconManager();
            shouldStartMinimizedToSystemTray = systemTrayIconManager.shouldStartMinimizedToSystemTray();
        }

        bool shouldStartMinimized = false;
        if (!shouldStartMinimizedToSystemTray)
        {
            auto startMinimizedIt = parseCmdResult.m_cmdOptions.find(QStringLiteral("startMinimized"));
            if (startMinimizedIt != parseCmdResult.m_cmdOptions.end()) {
                shouldStartMinimized = true;
            }
        }

        if (shouldStartMinimized) {
            pMainWindow->showMinimized();
        }
        else if (!shouldStartMinimizedToSystemTray) {
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
    catch(const quentier::LocalStorageVersionTooHighException & versionTooHighException)
    {
        criticalMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start"),
                           QObject::tr("Local storage is too new for used libquentier version to handle"),
                           QObject::tr("The version of local storage persistence for the account you are trying to open "
                                       "is higher than the version supported by the currently used build of libquentier. "
                                       "It means that this account's data has already been opened using a more recent version "
                                       "of libquentier which has changed the data layout somehow. The current version of "
                                       "libquentier cannot work with this version of data as it doesn't know what exactly "
                                       "has changed in the data layout and how to work with it. Please upgrade your versions "
                                       "of libquentier and Quentier and try again."));
        qWarning() << QStringLiteral("Caught LocalStorageVersionTooHighException: ") << versionTooHighException.nonLocalizedErrorMessage();
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
        internalErrorMessageBox(Q_NULLPTR, QObject::tr("Quentier cannot start, exception occurred: ") + QString::fromUtf8(exception.what()));
        qWarning() << QStringLiteral("Caught IQuentierException: ") << exception.what();
        return 1;
    }

    return app.exec();
}
