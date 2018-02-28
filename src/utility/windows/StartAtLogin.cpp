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

HRESULT createShortcut(LPSTR pszTargetfile, LPSTR pszTargetargs,
                       LPSTR pszLinkfile, LPSTR pszDescription,
                       int iShowmode, LPSTR pszCurdir,
                       LPSTR pszIconfile, int iIconindex)
{
    HRESULT hRes;
    IShellLink * pShellLink = NULL;
    IPersistFile * pPersistFile = NULL;
    WCHAR wszLinkfile[MAX_PATH];
    int numWideCharsWritten = 0;

    CoInitialize(NULL);
    hRes = E_INVALIDARG;

    if ( (pszTargetfile != NULL) && (strlen(pszTargetfile) > 0) &&
         (pszTargetargs != NULL) &&
         (pszLinkfile != NULL) && (strlen(pszLinkfile) > 0) &&
         (pszDescription != NULL) &&
         (iShowmode >= 0) &&
         (pszCurdir != NULL) &&
         (pszIconfile != NULL) &&
         (iIconindex >= 0) )
    {
        hRes = CoCreateInstance(CLSID_ShellLink, // pre-defined CLSID of the IShellLink object
                                NULL, // pointer to parent interface if part of aggregate
                                CLSCTX_INPROC_SERVER, // caller and called code are in same process
                                IID_IShellLink, // pre-defined interface of the IShellLink object
                                (LPVOID*)&pShellLink); // Returns a pointer to the IShellLink object
        if (SUCCEEDED(hRes))
        {
            // Set the fields in the IShellLink object
            hRes = pShellLink->SetPath(pszTargetfile);
            hRes = pShellLink->SetArguments(pszTargetargs);

            if (strlen(pszDescription) > 0) {
                hRes = pShellLink->SetDescription(pszDescription);
            }

            if (iShowmode > 0) {
                hRes = pShellLink->SetShowCmd(iShowmode);
            }

            if (strlen(pszCurdir) > 0) {
                hRes = pShellLink->SetWorkingDirectory(pszCurdir);
            }

            if (strlen(pszIconfile) > 0 && iIconindex >= 0) {
                hRes = pShellLink->SetIconLocation(pszIconfile, iIconindex);
            }

            // Use the IPersistFile object to save the shell link
            hRes = pShellLink->QueryInterface(IID_IPersistFile, // pre-defined interface of the IPersistFile object
                                              (LPVOID*)&pPersistFile); // returns a pointer to the IPersistFile object
            if (SUCCEEDED(hRes)) {
                numWideCharsWritten = MultiByteToWideChar(CP_ACP, 0, pszLinkfile, -1, wszLinkfile, MAX_PATH);
                hRes = pPersistFile->Save(wszLinkfile, TRUE);
                pPersistFile->Release();
            }

            pShellLink->Release();
        }

    }

    CoUninitialize();

    return (hRes);
}

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
        ApplicationSettings appSettings;
        appSettings.beginGroup(START_AUTOMATICALLY_AT_LOGIN_SETTINGS_GROUP_NAME);
        appSettings.setValue(SHOULD_START_AUTOMATICALLY_AT_LOGIN, false);
        appSettings.endGroup();
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
    const char * args = ((option == StartQuentierAtLoginOption::MinimizedToTray)
                         ? "--startMinimizedToTray"
                         : ((option == StartQuentierAtLoginOption::Minimized)
                            ? "--startMinimized"
                            : NULL));

    HRESULT hres = createShortcut(QDir::toNativeSeparators(quentierAppPath).toLocal8Bit().constData(),
                                  args, "quentier.lnk", NULL, 0,
                                  QDir::toNativeSeparators(qApp->applicationDirPath()).toLocal8Bit().constData(),
                                  QDir::toNativeSeparators(quentierAppPath).toLocal8Bit().constData(), 0);
    if (!SUCCEEDED(hres)) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("StartAtLogin", "can't create shortcut to app in startup folder"));
        QNWARNING(errorDescription);
        return false;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(START_AUTOMATICALLY_AT_LOGIN_SETTINGS_GROUP_NAME);
    appSettings.setValue(SHOULD_START_AUTOMATICALLY_AT_LOGIN, true);
    appSettings.setValue(START_AUTOMATICALLY_AT_LOGIN_OPTION, option);
    appSettings.endGroup();

    return true;
}

} // namespace quentier
