/*
 * Copyright 2018-2024 Dmitry Ivanov
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

#include "../StartAtLogin.h"

#include <lib/preferences/keys/StartAtLogin.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStringList>

namespace quentier {

bool setStartQuentierAtLoginOption(
    const bool shouldStartAtLogin, ErrorString & errorDescription,
    const StartQuentierAtLoginOption::type option)
{
    QNDEBUG("utility", "setStartQuentierAtLoginOption (Darwin): should start "
        << "at login = " << (shouldStartAtLogin ? "true" : "false")
        << ", option = " << option);

    const QString quentierLaunchPlistFilePath = 
        QDir::homePath() +
        QStringLiteral("/Library/LaunchAgents/org.quentier.Quentier.plist");

    const QFileInfo plistFileInfo{quentierLaunchPlistFilePath};
    if (plistFileInfo.exists())
    {
        // Need to unload whichever previous configuration

        QStringList args;
        args << QStringLiteral("unload");
        args << QStringLiteral("~/Library/LaunchAgents/") +
            plistFileInfo.fileName();

        const int res =
            QProcess::execute(QStringLiteral("/bin/launchctl"), args);
        if (res == -2)
        {
            errorDescription.setBase(
                QT_TRANSLATE_NOOP("StartAtLogin", "can't execute launchd "
                                  "to unload previous configuration"));

            QNWARNING("utility", errorDescription);
            return false;
        }
        else if (res == -1)
        {
            errorDescription.setBase(
                QT_TRANSLATE_NOOP("StartAtLogin", "launchd crashed during "
                                  "the attempt to unload previous "
                                  "configuration"));

            QNWARNING("utility", errorDescription);
            return false;
        }
        else if (res != 0)
        {
            errorDescription.setBase(
                QT_TRANSLATE_NOOP("StartAtLogin", "launchd returned with "
                                  "error during the attempt to unload previous "
                                  "configuration"));

            QNWARNING("utility", errorDescription);
            return false;
        }

        // Now can remove the launchd config file
        if (!QFile::remove(quentierLaunchPlistFilePath))
        {
            errorDescription.setBase(
                QT_TRANSLATE_NOOP("StartAtLogin", "failed to remove previous "
                                  "launchd configuration file"));

            QNWARNING("utility", errorDescription);
            return false;
        }
    }

    // If the app shouldn't start at login, that should be all
    if (!shouldStartAtLogin)
    {
        ApplicationSettings appSettings;
        appSettings.beginGroup(preferences::keys::startAtLoginGroup.data());
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::shouldStartAtLogin.data(), false);
        return true;
    }

    QDir plistFileDir = plistFileInfo.absoluteDir();
    if (Q_UNLIKELY(!plistFileDir.exists()))
    {
        const bool res = plistFileDir.mkpath(plistFileDir.absolutePath());
        if (Q_UNLIKELY(!res))
        {
            errorDescription.setBase(
                QT_TRANSLATE_NOOP("StartAtLogin", "can't create directory "
                                  "for app's launchd config"));

            errorDescription.details() = plistFileDir.absolutePath();
            QNWARNING("utility", errorDescription);
            return false;
        }
    }

    // Now compose the new plist file with start at login option set to
    // the relevant value
    QSettings plist{quentierLaunchPlistFilePath, QSettings::NativeFormat};
    plist.setValue(QStringLiteral("Label"), plistFileInfo.completeBaseName());

    QStringList appArgsList;
    appArgsList.reserve(2);
    appArgsList << QCoreApplication::applicationFilePath();

    if (option == StartQuentierAtLoginOption::MinimizedToTray) {
        appArgsList << QStringLiteral("--startMinimizedToTray");
    }
    else if (option == StartQuentierAtLoginOption::Minimized) {
        appArgsList << QStringLiteral("--startMinimized");
    }

    plist.setValue(QStringLiteral("ProgramArguments"), appArgsList);

    plist.setValue(
        QStringLiteral("ProcessType"),
        QStringLiteral("Interactive"));

    plist.setValue(QStringLiteral("RunAtLoad"), true);
    plist.setValue(QStringLiteral("KeepAlive"), false);

    // Now need to tell launchd to load the new configuration
    QStringList args;
    args << QStringLiteral("load");

    args << QDir::homePath() + QStringLiteral("/Library/LaunchAgents/") +
        plistFileInfo.fileName();

    const int res = QProcess::execute(QStringLiteral("/bin/launchctl"), args);
    if (res == -2)
    {
        errorDescription.setBase(
            QT_TRANSLATE_NOOP("StartAtLogin", "can't execute launchd to "
                              "load the new configuration"));

        QNWARNING("utility", errorDescription);
        return false;
    }
    else if (res == -1)
    {
        errorDescription.setBase(
            QT_TRANSLATE_NOOP("StartAtLogin", "launchd crashed during "
                              "the attempt to load new configuration"));

        QNWARNING("utility", errorDescription);
        return false;
    }
    else if (res != 0)
    {
        errorDescription.setBase(
            QT_TRANSLATE_NOOP("StartAtLogin", "launchd returned with error "
                              "during the attempt to load new configuration"));

        QNWARNING("utility", errorDescription);
        return false;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::startAtLoginGroup.data());
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::shouldStartAtLogin.data(), true);
    appSettings.setValue(preferences::keys::startAtLoginOption.data(), option);

    return true;
}

} // namespace quentier
