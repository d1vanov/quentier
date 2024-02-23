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
#include <QTextStream>

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

    const QString quentierAutostartDesktopFilePath =
        QDir::homePath() +
        QStringLiteral("/.config/autostart/Quentier.desktop");

    const QFileInfo autoStartDesktopFileInfo{quentierAutostartDesktopFilePath};
    if (autoStartDesktopFileInfo.exists()) {
        // First need to remove any existing autostart configuration
        if (!QFile::remove(quentierAutostartDesktopFilePath)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP(
                "StartAtLogin",
                "failed to remove previous autostart configuration"));

            QNWARNING("utility", errorDescription);
            return false;
        }
    }

    // If the app shouldn't start at login, that should be all
    if (!shouldStartAtLogin) {
        ApplicationSettings appSettings;
        appSettings.beginGroup(preferences::keys::startAtLoginGroup.data());
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::shouldStartAtLogin.data(), false);
        return true;
    }

    QDir autoStartDesktopFileDir = autoStartDesktopFileInfo.absoluteDir();
    if (Q_UNLIKELY(!autoStartDesktopFileDir.exists())) {
        const bool res = autoStartDesktopFileDir.mkpath(
            autoStartDesktopFileDir.absolutePath());

        if (Q_UNLIKELY(!res)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP(
                "StartAtLogin",
                "can't create directory for app's autostart config"));

            errorDescription.details() = autoStartDesktopFileDir.absolutePath();
            QNWARNING("utility", errorDescription);
            return false;
        }
    }

    // Now compose the new .desktop file with start at login option set to
    // the relevant value
    QString desktopFileContents;
    QTextStream strm{&desktopFileContents};

    strm << "[Desktop Entry]\nType=Application\nExec=";
    strm << QCoreApplication::applicationFilePath();
    strm << " ";

    if (option == StartQuentierAtLoginOption::MinimizedToTray) {
        strm << "--startMinimizedToTray";
    }
    else if (option == StartQuentierAtLoginOption::Minimized) {
        strm << "--startMinimized";
    }

    strm << " %u\nName=Quentier\nIcon=quentier\nTerminal=false\n"
        "GenericName=Note taking app\nComment=Note taking app "
        "integrated with Evernote\nCategories=Qt;Network;Office;"
        "TextTools;\nKeywords=Quentier;note;Evernote;\n";

    QFile autoStartDesktopFile{quentierAutostartDesktopFilePath};
    if (!autoStartDesktopFile.open(QIODevice::WriteOnly)) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "StartAtLogin",
            "failed to open the autostart desktop file for writing"));

        QNWARNING("utility", errorDescription);
        return false;
    }

    autoStartDesktopFile.write(desktopFileContents.toUtf8());

    if (!autoStartDesktopFile.flush()) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "StartAtLogin",
            "failed to flush data into the autostart desktop file"));

        QNWARNING("utility", errorDescription);
        return false;
    }

    autoStartDesktopFile.close();

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::startAtLoginGroup.data());
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::shouldStartAtLogin.data(), true);
    appSettings.setValue(
        preferences::keys::startAtLoginOption.data(), static_cast<int>(option));

    return true;
}

} // namespace quentier
