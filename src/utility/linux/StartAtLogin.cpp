/*
 * Copyright 2018 Dmitry Ivanov
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
#include <quentier/logging/QuentierLogger.h>
#include <QFileInfo>
#include <QFile>
#include <QDir>

#define QUENTIER_AUTOSTART_DESKTOP_FILE_PATH QStringLiteral("~/.config/autostart/Quentier.desktop")

namespace quentier {

bool setStartQuentierAtLoginOption(const bool shouldStartAtLogin,
                                   ErrorString & errorDescription,
                                   const StartQuentierAtLoginOption::type option)
{
    QNDEBUG(QStringLiteral("setStartQuentierAtLoginOption (Linux): should start at login = ")
            << (shouldStartAtLogin ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", option = ") << option);

    QFileInfo autoStartDesktopFileInfo(QUENTIER_AUTOSTART_DESKTOP_FILE_PATH);
    if (autoStartDesktopFileInfo.exists())
    {
        // First need to remove any existing autostart configuration
        if (!QFile::remove(QUENTIER_AUTOSTART_DESKTOP_FILE_PATH)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP("StartAtLogin", "failed to remove previous autostart configuration"));
            QNWARNING(errorDescription);
            return false;
        }
    }

    // If the app shouldn't start at login, that should be all
    if (!shouldStartAtLogin) {
        return true;
    }

    QDir autoStartDesktopFileDir = autoStartDesktopFileInfo.absoluteDir();
    if (Q_UNLIKELY(!autoStartDesktopFileDir.exists()))
    {
        bool res = autoStartDesktopFileDir.mkpath(autoStartDesktopFileDir.absolutePath());
        if (Q_UNLIKELY(!res)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP("StartAtLogin", "can't create directory for app's autostart config"));
            errorDescription.details() = autoStartDesktopFileDir.absolutePath();
            QNWARNING(errorDescription);
            return false;
        }
    }

    // Now compose the new .desktop file with start at login option set to the relevant value
    QString desktopFileContents = QStringLiteral("[Desktop Entry]\nType=Application\nExec=quentier ");

    if (option == StartQuentierAtLoginOption::MinimizedToTray) {
        desktopFileContents += QStringLiteral("--startMinimizedToTray");
    }
    else if (option == StartQuentierAtLoginOption::Minimized) {
        desktopFileContents += QStringLiteral("--startMinimized");
    }

    desktopFileContents += QStringLiteral(" %u\nName=Quentier\nIcon=quentier\nTerminal=false\nGenericName=Note taking app\n"
                                          "Comment=Note taking app integrated with Evernote\nCategories=Qt;Network;Office;TextTools;\n"
                                          "Keywords=Quentier;note;Evernote;\n");

    QFile autoStartDesktopFile(QUENTIER_AUTOSTART_DESKTOP_FILE_PATH);
    if (!autoStartDesktopFile.open(QIODevice::WriteOnly)) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("StartAtLogin", "failed to open the autostart desktop file for writing"));
        QNWARNING(errorDescription);
        return false;
    }

    Q_UNUSED(autoStartDesktopFile.write(desktopFileContents.toUtf8()))

    if (!autoStartDesktopFile.flush()) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("StartAtLogin", "failed to flush data into the autostart desktop file"));
        QNWARNING(errorDescription);
        return false;
    }

    autoStartDesktopFile.close();
    return true;
}

} // namespace quentier
