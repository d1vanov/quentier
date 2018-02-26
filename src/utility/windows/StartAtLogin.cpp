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
#include <QApplication>

#define QUENTIER_AUTOSTART_SHORTCUT_FILE_PATH (QStringLiteral("C:\\Users\\") + QString::fromLocal8Bit(qgetenv("USERNAME")) + \
                                               QStringLiteral("\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"))

namespace quentier {

bool setStartQuentierAtLoginOption(const bool shouldStartAtLogin,
                                   ErrorString & errorDescription,
                                   const StartQuentierAtLoginOption::type option)
{
    QNDEBUG(QStringLiteral("setStartQuentierAtLoginOption (Windows): should start at login = ")
            << (shouldStartAtLogin ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", option = ") << option);

    QFileInfo autoStartShortcutFileInfo(QUENTIER_AUTOSTART_SHORTCUT_FILE_PATH);
    if (autoStartShortcutFileInfo.exists())
    {
        // First need to remove any existing autostart configuration
        if (!QFile::remove(QUENTIER_AUTOSTART_SHORTCUT_FILE_PATH)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP("StartAtLogin", "failed to remove previous autostart configuration"));
            QNWARNING(errorDescription);
            return false;
        }
    }

    // If the app shouldn't start at login, that should be all
    if (!shouldStartAtLogin) {
        return true;
    }

    QDir autoStartShortcutFileDir = autoStartShortcutFileInfo.absoluteDir();
    if (Q_UNLIKELY(!autoStartShortcutFileDir.exists()))
    {
        bool res = autoStartShortcutFileDir.mkpath(autoStartShortcutFileDir.absolutePath());
        if (Q_UNLIKELY(!res)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP("StartAtLogin", "can't create directory for app's autostart config"));
            errorDescription.details() = autoStartShortcutFileDir.absolutePath();
            QNWARNING(errorDescription);
            return false;
        }
    }

    QString quentierAppPath = qApp->applicationDirPath() + "/quentier.exe";
    QFile quentierAppFile(quentierAppPath);

    // FIXME: but how to pass argument to this link?
    bool res = quentierAppFile.link(autoStartShortcutFileDir.absolutePath() + QStringLiteral("/Quentier.lnk"));
    if (!res) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("StartAtLogin", "failed to create shortcut to Quentier in auto start folder"));
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

} // namespace quentier
