/*
 * Copyright 2016-2020 Dmitry Ivanov
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
#include "ParseCommandLine.h"
#include "SetupStartupSettings.h"

#include <lib/exception/LocalStorageVersionTooHighException.h>
#include <lib/initialization/Initialize.h>
#include <lib/initialization/LoadDependencies.h>
#include <lib/tray/SystemTrayIconManager.h>
#include <lib/utility/ExitCodes.h>
#include <lib/utility/RestartApp.h>

#include <VersionInfo.h>

#include <quentier/local_storage/LocalStorageOpenException.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/QuentierApplication.h>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#include <QSessionManager>
#endif

#include <QTime>

#include <exception>
#include <iostream>
#include <memory>

using namespace quentier;

int main(int argc, char * argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    qsrand(static_cast<quint32>(QTime::currentTime().msec()));
#endif

    // Loading the dependencies manually - required on Windows
    loadDependencies();

#ifdef QUENTIER_PACKAGED_AS_APP_IMAGE
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif

    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
#endif

    QuentierApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("Quentier"));
    app.setQuitOnLastWindowClosed(false);

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    auto restartHintSetter = [](QSessionManager & manager) {
        manager.setRestartHint(QSessionManager::RestartNever);
    };

    QObject::connect(
        &app, &QuentierApplication::saveStateRequest, &app, restartHintSetter);

    QObject::connect(
        &app, &QuentierApplication::commitDataRequest, &app, restartHintSetter);

    app.setFallbackSessionManagementEnabled(false);
#endif // Qt 5.14.0

    initializeAppVersion(app);

    ParseCommandLineResult parseCmdResult;
    parseCommandLine(argc, argv, parseCmdResult);
    if (!parseCmdResult.m_errorDescription.isEmpty()) {
        std::cerr << parseCmdResult.m_errorDescription.nonLocalizedString()
                         .toLocal8Bit()
                         .constData();
        return 1;
    }

    bool res = initialize(app, parseCmdResult.m_cmdOptions);
    if (!res) {
        return 1;
    }

    setupStartupSettings();

    std::unique_ptr<MainWindow> pMainWindow;
    try {
        pMainWindow.reset(new MainWindow);

        bool shouldStartMinimizedToSystemTray = false;

        auto startMinimizedToTrayIt = parseCmdResult.m_cmdOptions.find(
            QStringLiteral("startMinimizedToTray"));

        if (startMinimizedToTrayIt != parseCmdResult.m_cmdOptions.end()) {
            shouldStartMinimizedToSystemTray = true;
        }
        else {
            const auto & systemTrayIconManager =
                pMainWindow->systemTrayIconManager();

            shouldStartMinimizedToSystemTray =
                systemTrayIconManager.shouldStartMinimizedToSystemTray();
        }

        bool shouldStartMinimized = false;
        if (!shouldStartMinimizedToSystemTray) {
            auto startMinimizedIt = parseCmdResult.m_cmdOptions.find(
                QStringLiteral("startMinimized"));
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
            QNDEBUG(
                "quentier:main",
                "Not showing the main window on startup "
                "because the start minimized to system tray "
                "was requested");
        }
    }
    catch (const quentier::local_storage::LocalStorageOpenException & e) {
        criticalMessageBox(
            nullptr, QObject::tr("Quentier cannot start"),
            QObject::tr("Failed to open the local storage database"),
            QObject::tr("Quentier cannot start because it could not open "
                        "the database. On Windows it might happen if another "
                        "instance is already running and using the same "
                        "account. Otherwise it might indicate the corruption "
                        "of account database file.\n\nException message: ") +
                e.localizedErrorMessage());

        qWarning() << "Caught DatabaseOpeningException: "
                   << e.nonLocalizedErrorMessage();

        return 1;
    }
    catch (const quentier::LocalStorageVersionTooHighException & e) {
        criticalMessageBox(
            nullptr, QObject::tr("Quentier cannot start"),
            QObject::tr("Local storage is too new for used libquentier version "
                        "to handle"),
            QObject::tr("The version of local storage persistence for "
                        "the account you are trying to open is higher than "
                        "the version supported by the currently used build of "
                        "libquentier. It means that this account's data has "
                        "already been opened using a more recent version of "
                        "libquentier which has changed the data layout "
                        "somehow. The current version of libquentier cannot "
                        "work with this version of data as it doesn't know "
                        "what exactly has changed in the data layout and how "
                        "to work with it. Please upgrade your versions of "
                        "libquentier and Quentier and try again."));

        qWarning() << "Caught LocalStorageVersionTooHighException: "
                   << e.nonLocalizedErrorMessage();

        return 1;
    }
    catch (const quentier::IQuentierException & e) {
        internalErrorMessageBox(
            nullptr,
            QObject::tr("Quentier cannot start, exception occurred: ") +
                e.localizedErrorMessage());

        qWarning() << "Caught IQuentierException: "
                   << e.nonLocalizedErrorMessage();

        return 1;
    }
    catch (const std::exception & e) {
        internalErrorMessageBox(
            nullptr,
            QObject::tr("Quentier cannot start, exception occurred: ") +
                QString::fromUtf8(e.what()));

        qWarning() << "Caught IQuentierException: " << e.what();
        return 1;
    }

    int exitCode = app.exec();

    pMainWindow.reset();

    if (exitCode == gRestartExitCode) {
        exitCode = 0;
        restartApp(argc, argv);
    }

    finalize();
    return exitCode;
}
