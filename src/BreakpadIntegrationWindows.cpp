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
#include <client/windows/crash_generation/minidump_generator.h>
#include <client/windows/crash_generation/client_info.h>
#include <client/windows/crash_generation/crash_generation_client.h>
RESTORE_WARNINGS

#include <QList>
#include <QByteArray>
#include <QDir>

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
#include <QGlobalStatic>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#endif

#include <windows.h>
#include <tchar.h>
#include <string>

#include <iostream>

namespace quentier {

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
Q_GLOBAL_STATIC(std::wstring, quentierMinidumpsStorageFolderPath)
Q_GLOBAL_STATIC(std::wstring, quentierCrashHandlerFilePath)
Q_GLOBAL_STATIC(std::wstring, quentierCrashHandlerArgs)
Q_GLOBAL_STATIC(std::string, quentierCrashHandlerFilePathMultiByte)
Q_GLOBAL_STATIC(std::string, quentierCrashHandlerArgsMultiByte)
#else
static std::wstring quentierMinidumpsStorageFolderPath;
static std::wstring quentierCrashHandlerFilePath;
static std::wstring quentierCrashHandlerArgs;
static std::string quentierCrashHandlerFilePathMultiByte;
static std::string quentierCrashHandlerArgsMultiByte;
#endif

bool ShowDumpResults(const wchar_t * dump_path,
                     const wchar_t * minidump_id,
                     void * context,
                     EXCEPTION_POINTERS * exinfo,
                     MDRawAssertionInfo * assertion,
                     bool succeeded)
{
    Q_UNUSED(context)
    Q_UNUSED(exinfo)
    Q_UNUSED(assertion)

    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = (WORD)1;

    PROCESS_INFORMATION processInfo;
    ZeroMemory(&processInfo, sizeof(processInfo));

    std::wstring * pQuentierCrashHandlerArgs = NULL;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    pQuentierCrashHandlerArgs = quentierCrashHandlerArgs;
#else
    pQuentierCrashHandlerArgs = &quentierCrashHandlerArgs;
#endif

    // Hope it doesn't cause the resize; it shouldn't unless the dump_path is unexpectedly huge
    pQuentierCrashHandlerArgs->append(dump_path);
    pQuentierCrashHandlerArgs->append(L"\\\\");
    pQuentierCrashHandlerArgs->append(minidump_id);
    pQuentierCrashHandlerArgs->append(L".dmp");

    std::wstring * pQuentierCrashHandlerFilePath = NULL;
    std::string * pQuentierCrashHandlerFilePathMultiByte = NULL;
    std::string * pQuentierCrashHandlerArgsMultiByte = NULL;

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    pQuentierCrashHandlerFilePath = quentierCrashHandlerFilePath;
    pQuentierCrashHandlerFilePathMultiByte = quentierCrashHandlerFilePathMultiByte;
    pQuentierCrashHandlerArgsMultiByte = quentierCrashHandlerArgsMultiByte;
#else
    pQuentierCrashHandlerFilePath = &quentierCrashHandlerFilePath;
    pQuentierCrashHandlerFilePathMultiByte = &quentierCrashHandlerFilePathMultiByte;
    pQuentierCrashHandlerArgsMultiByte = &quentierCrashHandlerArgsMultiByte;
#endif

    int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, pQuentierCrashHandlerFilePath->c_str(), int(pQuentierCrashHandlerFilePath->length() + 1), 0, 0, 0, 0);
    pQuentierCrashHandlerFilePathMultiByte->assign(sizeNeeded, 0);
    WideCharToMultiByte(CP_ACP, 0, pQuentierCrashHandlerFilePath->c_str(), int(pQuentierCrashHandlerFilePath->length() + 1),
                        &(*pQuentierCrashHandlerFilePathMultiByte)[0], sizeNeeded, 0, 0);

    sizeNeeded = WideCharToMultiByte(CP_ACP, 0, pQuentierCrashHandlerArgs->c_str(), int(pQuentierCrashHandlerArgs->length() + 1), 0, 0, 0, 0);
    pQuentierCrashHandlerArgsMultiByte->assign(sizeNeeded, 0);
    WideCharToMultiByte(CP_ACP, 0, pQuentierCrashHandlerArgs->c_str(), int(pQuentierCrashHandlerArgs->length() + 1),
                        &(*pQuentierCrashHandlerArgsMultiByte)[0], sizeNeeded, 0, 0);

    const TCHAR * crashHandlerFilePath = pQuentierCrashHandlerFilePathMultiByte->c_str();
    TCHAR * argsData = const_cast<TCHAR*>(pQuentierCrashHandlerArgsMultiByte->c_str());

    if (CreateProcess(crashHandlerFilePath, argsData, NULL, NULL, FALSE,
                      0, NULL, NULL, &startupInfo, &processInfo))
    {
        WaitForSingleObject(processInfo.hProcess,INFINITE);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    }

    return succeeded;
}

static google_breakpad::ExceptionHandler * pBreakpadHandler = NULL;

void setupBreakpad(const QApplication & app)
{
    QString appFilePath = app.applicationFilePath();
    QFileInfo appFileInfo(appFilePath);

#define CONVERT_PATH(path) \
    path = QDir::toNativeSeparators(path); \
    path.replace(QString::fromUtf8("\\"), QString::fromUtf8("\\\\"))

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString minidumpsStorageFolderPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
    QString minidumpsStorageFolderPath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif

    CONVERT_PATH(minidumpsStorageFolderPath);

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    *quentierMinidumpsStorageFolderPath = minidumpsStorageFolderPath.toStdWString();
#else
    quentierMinidumpsStorageFolderPath = minidumpsStorageFolderPath.toStdWString();
#endif

    QString crashHandlerFilePath = appFileInfo.absolutePath() + QString::fromUtf8("/quentier_crash_handler.exe");
    CONVERT_PATH(crashHandlerFilePath);

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    *quentierCrashHandlerFilePath = crashHandlerFilePath.toStdWString();
#else
    quentierCrashHandlerFilePath = crashHandlerFilePath.toStdWString();
#endif

    QString quentierSymbolsFilePath = appFileInfo.absolutePath() + QString::fromUtf8("/quentier.syms.compressed");
    CONVERT_PATH(quentierSymbolsFilePath);

    QString libquentierSymbolsFilePath = appFileInfo.absolutePath() + QString::fromUtf8("/libquentier.syms.compressed");
    CONVERT_PATH(libquentierSymbolsFilePath);

    QString minidumpStackwalkFilePath = appFileInfo.absolutePath() + QString::fromUtf8("/quentier_minidump_stackwalk.exe");
    CONVERT_PATH(minidumpStackwalkFilePath);

    std::wstring * pQuentierCrashHandlerArgs = NULL;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    pQuentierCrashHandlerArgs = quentierCrashHandlerArgs;
    const wchar_t * minidumpsStorageFolderPathData = quentierMinidumpsStorageFolderPath->c_str();
#else
    pQuentierCrashHandlerArgs = &quentierCrashHandlerArgs;
    const wchar_t * minidumpsStorageFolderPathData = quentierMinidumpsStorageFolderPath.c_str();
#endif

    *pQuentierCrashHandlerArgs = quentierSymbolsFilePath.toStdWString();
    pQuentierCrashHandlerArgs->append(L" ");
    pQuentierCrashHandlerArgs->append(libquentierSymbolsFilePath.toStdWString());
    pQuentierCrashHandlerArgs->append(L" ");
    pQuentierCrashHandlerArgs->append(minidumpStackwalkFilePath.toStdWString());
    pQuentierCrashHandlerArgs->append(L" ");

    // NOTE: will need to append the path to generated minidump to this byte array => will increase its reserved buffer
    // and pray that the appending of the path to generated minidump won't cause the resize; or, if it does, that
    // the resize succeeds and doesn't break the program about to crash
    pQuentierCrashHandlerArgs->reserve(static_cast<size_t>(pQuentierCrashHandlerArgs->size() + 1000));

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    quentierCrashHandlerArgsMultiByte->reserve(quentierCrashHandlerFilePath->capacity());
#else
    quentierCrashHandlerArgsMultiByte.reserve(quentierCrashHandlerFilePath.capacity());
#endif

    std::string * pQuentierCrashHandlerArgsMultiByte = NULL;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    pQuentierCrashHandlerArgsMultiByte = quentierCrashHandlerArgsMultiByte;
#else
    pQuentierCrashHandlerArgsMultiByte = &quentierCrashHandlerArgsMultiByte;
#endif
    pQuentierCrashHandlerArgsMultiByte->reserve(pQuentierCrashHandlerArgs->size());

    pBreakpadHandler = new google_breakpad::ExceptionHandler(std::wstring(minidumpsStorageFolderPathData),
                                                             NULL,
                                                             ShowDumpResults,
                                                             NULL,
                                                             google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                             MiniDumpNormal,
                                                             (const wchar_t*)NULL,
                                                             NULL);
}

} // namespace quentier
