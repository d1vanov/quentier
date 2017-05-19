/*
 * Copyright 2017 Dmitry Ivanov
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

#include "BreakpadIntegration.h"

#include "SuppressWarningsMacro.h"

SUPPRESS_WARNINGS
#include <client/windows/handler/exception_handler.h>
RESTORE_WARNINGS

#include <QList>
#include <QByteArray>
#include <QDir>

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
#include <QGlobalStatic>
#endif


#include <windows.h>
#include <tchar.h>

namespace quentier {

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
Q_GLOBAL_STATIC(QByteArray, quentierMinidumpsStorageFolderPath)
Q_GLOBAL_STATIC(QByteArray, quentierCrashHandlerFilePath)
Q_GLOBAL_STATIC(QByteArray, quentierCrashHandlerArgs)
#else
static QByteArray quentierMinidumpsStorageFolderPath;
static QByteArray quentierCrashHandlerFilePath;
static QByteArray quentierCrashHandlerArgs;
#endif

}

bool ShowDumpResults(const wchar_t* dump_path,
                     const wchar_t* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded)
{
    DWORD dwLastError = 0;
    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = (WORD)1;

    PROCESS_INFORMATION processInfo;
    ZeroMemory(&processInfo, sizeof(processInfo));

    // Hope it doesn't cause the resize; it shouldn't unless the dump_path is unexpectedly huge
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    quentierCrashHandlerArgs->append((const char*)dump_path);
    const TCHAR * crashHandlerFilePath = (const TCHAR*)(quentierCrashHandlerFilePath->constData())
    const TCHAR * argsData = (const TCHAR*)(quentierCrashHandlerArgs->constData())
#else
    quentierCrashHandlerArgs.append((const char*)dump_path);
    const TCHAR * crashHandlerFilePath = (const TCHAR*)(quentierCrashHandlerFilePath.constData())
    const TCHAR * argsData = (const TCHAR*)(quentierCrashHandlerArgs.constData())
#endif

    Q_UNUSED(CreateProcess(crashHandlerFilePath, argsData, NULL, NULL, FALSE,
                           0, NULL, NULL, &startupInfo, &processInfo))
    return succeeded;
}

static google_breakpad::ExceptionHandler * pBreakpadHandler = NULL;

void setupBreakpad(const QApplication & app)
{
    QString appFilePath = app.applicationFilePath();
    QFileInfo appFileInfo(appFilePath);

#define CONVERT_PATH(path) \
    path = QDir::toNativeSeparators(path);
    path.replace(QString::fromUtf8("\\"), QString::fromUtf8("\\\\"))

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString minidumpsStorageFolderPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
    QString minidumpsStorageFolderPath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif

    CONVERT_PATH(minidumpsStorageFolderPath);

    QString crashHandlerFilePath = appFileInfo.absolutePath() + QString::fromUtf8("/quentier_crash_handler");
    CONVERT_PATH(crashHandlerFilePath);

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    *quentierCrashHandlerFilePath = crashHandlerFilePath.utf16();
#else
    quentierCrashHandlerFilePath = crashHandlerFilePath.utf16();
#endif

    QString symbolsFilePath = appFileInfo.absolutePath() + QString::fromUtf8("/quentier.syms");
    CONVERT_PATH(symbolsFilePath);

    QString minidumpStackwalkFilePath = appFileInfo.absolutePath() + QString::fromUtf8("/quentier_minidump_stackwalk");
    CONVERT_PATH(minidumpStackwalkFilePath);

    QByteArray * pQuentierCrashHandlerArgs = NULL;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    pQuentierCrashHandlerArgs = quentierCrashHandlerArgs;
    const TCHAR * minidumpsStorageFolderPathData = (const TCHAR*)quentierMinidumpsStorageFolderPath->constData();
#else
    pQuentierCrashHandlerArgs = &quentierCrashHandlerArgs;
    const TCHAR * minidumpsStorageFolderPathData = (const TCHAR*)quentierMinidumpsStorageFolderPath.constData();
#endif

    *pQuentierCrashHandlerArgs = symbolsFilePath.utf16();
    *pQuentierCrashHandlerArgs += QString::fromUtf8(" ").utf16();
    *pQuentierCrashHandlerArgs += minidumpStackwalkFilePath.utf16();
    *pQuentierCrashHandlerArgs += QString::fromUtf8(" ").utf16();

    // NOTE: will need to append the path to generated minidump to this byte array = will increase its reserved buffer
    // and pray that the appending of the path to generated minidump won't cause the resize; or, if it does, that
    // the resize succeeds and doesn't break the program about to crash
    pQuentierCrashHandlerArgs->reserve(quentierCrashHandlerArgs.size() + 1000);

    pBreakpadHandler = new google_breakpad::ExceptionHandler(minidumpsStorageFolderPathData,
                                                             NULL,
                                                             ShowDumpResults,
                                                             NULL,
                                                             google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                             google_breakpad::MiniDumpNormal,
                                                             NULL,
                                                             NULL);
}

} // namespace quentier
