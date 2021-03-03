/*
 * Copyright 2018-2021 Dmitry Ivanov
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

#define QUENTIER_AUTOSTART_DESKTOP_FILE_PATH                                   \
    QDir::homePath() + QStringLiteral("/.config/autostart/Quentier.desktop")

namespace quentier {

bool setStartQuentierAtLoginOption(
    const bool shouldStartAtLogin, ErrorString & errorDescription,
    const StartQuentierAtLoginOption option)
{
    QNDEBUG(
        "utility",
        "setStartQuentierAtLoginOption (Linux): should start at "
            << "login = " << (shouldStartAtLogin ? "true" : "false")
            << ", option = " << option);

    QFileInfo autoStartDesktopFileInfo(QUENTIER_AUTOSTART_DESKTOP_FILE_PATH);
    if (autoStartDesktopFileInfo.exists()) {
        // First need to remove any existing autostart configuration
        if (!QFile::remove(QUENTIER_AUTOSTART_DESKTOP_FILE_PATH)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP(
                "StartAtLogin",
                "failed to remove previous "
                "autostart configuration"));

            QNWARNING("utility", errorDescription);
            return false;
        }
    }

    // If the app shouldn't start at login, that should be all
    if (!shouldStartAtLogin) {
        ApplicationSettings appSettings;
        appSettings.beginGroup(preferences::keys::startAtLoginGroup);
        appSettings.setValue(preferences::keys::shouldStartAtLogin, false);
        appSettings.endGroup();
        return true;
    }

    QDir autoStartDesktopFileDir = autoStartDesktopFileInfo.absoluteDir();
    if (Q_UNLIKELY(!autoStartDesktopFileDir.exists())) {
        bool res = autoStartDesktopFileDir.mkpath(
            autoStartDesktopFileDir.absolutePath());

        if (Q_UNLIKELY(!res)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP(
                "StartAtLogin",
                "can't create directory "
                "for app's autostart config"));

            errorDescription.details() = autoStartDesktopFileDir.absolutePath();
            QNWARNING("utility", errorDescription);
            return false;
        }
    }

    // Now compose the new .desktop file with start at login option set to
    // the relevant value
    QString desktopFileContents =
        QStringLiteral("[Desktop Entry]\nType=Application\nExec=");

    desktopFileContents += QCoreApplication::applicationFilePath();
    desktopFileContents += QStringLiteral(" ");

    if (option == StartQuentierAtLoginOption::MinimizedToTray) {
        desktopFileContents += QStringLiteral("--startMinimizedToTray");
    }
    else if (option == StartQuentierAtLoginOption::Minimized) {
        desktopFileContents += QStringLiteral("--startMinimized");
    }

    desktopFileContents += QStringLiteral(
        " %u\nName=Quentier\nIcon=quentier\nTerminal=false\n"
        "GenericName=Note taking app\nComment=Note taking app "
        "integrated with Evernote\nCategories=Qt;Network;Office;"
        "TextTools;\nKeywords=Quentier;note;Evernote;\n");

    QFile autoStartDesktopFile(QUENTIER_AUTOSTART_DESKTOP_FILE_PATH);
    if (!autoStartDesktopFile.open(QIODevice::WriteOnly)) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "StartAtLogin",
            "failed to open the autostart "
            "desktop file for writing"));

        QNWARNING("utility", errorDescription);
        return false;
    }

    Q_UNUSED(autoStartDesktopFile.write(desktopFileContents.toUtf8()))

    if (!autoStartDesktopFile.flush()) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "StartAtLogin",
            "failed to flush data into "
            "the autostart desktop file"));

        QNWARNING("utility", errorDescription);
        return false;
    }

    autoStartDesktopFile.close();

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::startAtLoginGroup);
    appSettings.setValue(preferences::keys::shouldStartAtLogin, true);

    appSettings.setValue(
        preferences::keys::startAtLoginOption, static_cast<int>(option));

    appSettings.endGroup();

    return true;
}

} // namespace quentier
