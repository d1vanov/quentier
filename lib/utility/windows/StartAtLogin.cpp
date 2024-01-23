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

// WinAPI headers
#include <objbase.h>
#include <ObjIdl.h>
#include <Windows.h>
#include <WinNls.h>
#include <ShlGuid.h>
#include <ShObjIdl.h>

namespace quentier {

HRESULT createShortcut(
    LPCWSTR pszTargetfile, LPCWSTR pszTargetargs,
    LPCWSTR pszLinkfile, LPCWSTR pszDescription,
    int iShowmode, LPCWSTR pszCurdir,
    LPCWSTR pszIconfile, int iIconindex)
{
    HRESULT hRes;
    IShellLink * shellLink = nullptr;
    IPersistFile * persistFile = nullptr;

    CoInitialize(nullptr);
    hRes = E_INVALIDARG;

    if ( (pszTargetfile != nullptr) && (wcslen(pszTargetfile) > 0) &&
         (pszTargetargs != nullptr) &&
         (pszLinkfile != nullptr) && (wcslen(pszLinkfile) > 0) &&
         (iShowmode >= 0) &&
         (pszCurdir != nullptr) &&
         (pszIconfile != nullptr) &&
         (iIconindex >= 0) )
    {
        hRes = CoCreateInstance(
            CLSID_ShellLink, // pre-defined CLSID of the IShellLink object
            nullptr, // pointer to parent interface if part of aggregate
            CLSCTX_INPROC_SERVER, // caller and called code are in same process
            IID_IShellLink, // pre-defined interface of the IShellLink object
            (LPVOID*)&shellLink); // Returns a pointer to the IShellLink object

        if (SUCCEEDED(hRes))
        {
            // Set the fields in the IShellLink object
            hRes = shellLink->SetPath(pszTargetfile);
            hRes = shellLink->SetArguments(pszTargetargs);

            if (pszDescription && (wcslen(pszDescription) > 0)) {
                hRes = shellLink->SetDescription(pszDescription);
            }

            if (iShowmode > 0) {
                hRes = shellLink->SetShowCmd(iShowmode);
            }

            if (wcslen(pszCurdir) > 0) {
                hRes = shellLink->SetWorkingDirectory(pszCurdir);
            }

            if (wcslen(pszIconfile) > 0 && iIconindex >= 0) {
                hRes = shellLink->SetIconLocation(pszIconfile, iIconindex);
            }

            // Use the IPersistFile object to save the shell link
            hRes = shellLink->QueryInterface(
                IID_IPersistFile, // pre-defined interface of IPersistFile
                (LPVOID*)&persistFile); // returns a pointer to IPersistFile

            if (SUCCEEDED(hRes)) {
                hRes = persistFile->Save(pszLinkfile, TRUE);
                persistFile->Release();
            }

            shellLink->Release();
        }

    }

    CoUninitialize();

    return (hRes);
}

bool setStartQuentierAtLoginOption(
    const bool shouldStartAtLogin, ErrorString & errorDescription,
    const StartQuentierAtLoginOption::type option)
{
    QNDEBUG("utility", "setStartQuentierAtLoginOption (Windows): should start "
        << "at login = " << (shouldStartAtLogin ? "true" : "false")
        << ", option = " << option);

    const QString quentierAutostartShortcutFilePath =
        QStringLiteral("C:/Users/") +
         QString::fromLocal8Bit(qgetenv("USERNAME")) +
         QStringLiteral("/AppData/Roaming/Microsoft/Windows/Start Menu/"
                        "Programs/Startup/quentier.lnk");

    QFileInfo autoStartShortcutFileInfo{quentierAutostartShortcutFilePath};
    if (autoStartShortcutFileInfo.exists())
    {
        // First need to remove any existing autostart configuration
        if (!QFile::remove(quentierAutostartShortcutFilePath))
        {
            errorDescription.setBase(QT_TRANSLATE_NOOP(
                "StartAtLogin",
                "failed to remove previous autostart configuration"));

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

    QDir autoStartShortcutFileDir = autoStartShortcutFileInfo.absoluteDir();
    if (Q_UNLIKELY(!autoStartShortcutFileDir.exists()))
    {
        const bool res = autoStartShortcutFileDir.mkpath(
            autoStartShortcutFileDir.absolutePath());

        if (Q_UNLIKELY(!res))
        {
            errorDescription.setBase(
                QT_TRANSLATE_NOOP("StartAtLogin", "can't create directory for "
                                  "app's autostart config"));

            errorDescription.details() = autoStartShortcutFileDir.absolutePath();
            QNWARNING("utility", errorDescription);
            return false;
        }
    }

    QString quentierAppPath = QDir::toNativeSeparators(
        QCoreApplication::applicationFilePath());

    if (quentierAppPath.contains(QStringLiteral(" "))) {
        quentierAppPath.prepend(QStringLiteral("\""));
        quentierAppPath.append(QStringLiteral("\""));
    }

    QString args = ((option == StartQuentierAtLoginOption::MinimizedToTray)
        ? QStringLiteral("--startMinimizedToTray")
        : ((option == StartQuentierAtLoginOption::Minimized)
            ? QStringLiteral("--startMinimized")
            : QString()));

    HRESULT hres = createShortcut(
        quentierAppPath.toStdWString().c_str(),
        args.toStdWString().c_str(),
        QDir::toNativeSeparators(autoStartShortcutFileInfo.absoluteFilePath())
        .toStdWString().c_str(),
        nullptr,
        0,
        QDir::toNativeSeparators(QCoreApplication::applicationDirPath())
        .toStdWString().c_str(),
        quentierAppPath.toStdWString().c_str(),
        0);

    if (!SUCCEEDED(hres))
    {
        errorDescription.setBase(
            QT_TRANSLATE_NOOP("StartAtLogin", "can't create shortcut to app "
                              "in startup folder"));

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
