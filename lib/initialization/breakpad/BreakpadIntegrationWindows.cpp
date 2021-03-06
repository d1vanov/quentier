/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#include <quentier/utility/SuppressWarnings.h>

SAVE_WARNINGS

CLANG_SUPPRESS_WARNING(-Wshorten-64-to-32)
CLANG_SUPPRESS_WARNING(-Wsign-conversion)
GCC_SUPPRESS_WARNING(-Wconversion)
MSVC_SUPPRESS_WARNING(4365)
MSVC_SUPPRESS_WARNING(4244)
MSVC_SUPPRESS_WARNING(4305)

#include <client/windows/crash_generation/client_info.h>
#include <client/windows/crash_generation/crash_generation_client.h>
#include <client/windows/crash_generation/minidump_generator.h>
#include <client/windows/handler/exception_handler.h>

RESTORE_WARNINGS

#include <QByteArray>
#include <QDir>
#include <QGlobalStatic>
#include <QList>
#include <QStandardPaths>

#include <tchar.h>
#include <Windows.h>

#include <iostream>
#include <string>

namespace quentier {

Q_GLOBAL_STATIC(std::wstring, quentierMinidumpsStorageFolderPath)
Q_GLOBAL_STATIC(std::wstring, quentierCrashHandlerFilePath)
Q_GLOBAL_STATIC(std::wstring, quentierCrashHandlerArgs)

bool ShowDumpResults(
    const wchar_t * dump_path, const wchar_t * minidump_id, void * context,
    EXCEPTION_POINTERS * exinfo, MDRawAssertionInfo * assertion, bool succeeded)
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
    pQuentierCrashHandlerArgs = quentierCrashHandlerArgs;

    // Hope it doesn't cause the resize; it shouldn't unless the dump_path
    // is unexpectedly huge
    pQuentierCrashHandlerArgs->append(dump_path);
    pQuentierCrashHandlerArgs->append(L"\\\\");
    pQuentierCrashHandlerArgs->append(minidump_id);
    pQuentierCrashHandlerArgs->append(L".dmp");

    std::wstring * pQuentierCrashHandlerFilePath = NULL;

    pQuentierCrashHandlerFilePath = quentierCrashHandlerFilePath;

    const TCHAR * crashHandlerFilePath = pQuentierCrashHandlerFilePath->c_str();
    TCHAR * argsData = const_cast<TCHAR*>(pQuentierCrashHandlerArgs->c_str());

    if (CreateProcess(
            crashHandlerFilePath,
            argsData,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo))
    {
        WaitForSingleObject(processInfo.hProcess,INFINITE);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    }

    return succeeded;
}

static google_breakpad::ExceptionHandler * pBreakpadHandler = nullptr;

void setupBreakpad(const QApplication & app)
{
    QString appFilePath = app.applicationFilePath();
    QFileInfo appFileInfo(appFilePath);

#define CONVERT_PATH(path)                                                     \
    path = QDir::toNativeSeparators(path);                                     \
    path.replace(QString::fromUtf8("\\"), QString::fromUtf8("\\\\"));          \
    path.prepend(QString::fromUtf8("\""));                                     \
    path.append(QString::fromUtf8("\""))

    QString minidumpsStorageFolderPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    CONVERT_PATH(minidumpsStorageFolderPath);
    // NOTE: removing quotes from this path for two reasons:
    // 1. This path shouldn't contain spaces so it's not necessary to escape
    //    them
    // 2. Minidump file path would be appended to this path in exception
    //    handler, so the resulting path would otherwise have an opening quote,
    //    quote in the middle of the path and no closing quote
    minidumpsStorageFolderPath.remove(0, 1);
    minidumpsStorageFolderPath.chop(1);

    *quentierMinidumpsStorageFolderPath =
        minidumpsStorageFolderPath.toStdWString();

    QString crashHandlerFilePath =
        appFileInfo.absolutePath() +
        QString::fromUtf8("/quentier_crash_handler.exe");

    CONVERT_PATH(crashHandlerFilePath);
    // NOTE: removing quotes from this path because it would be the first
    // argument to WinAPI's CreateProcess call and this one shouldn't be
    // surrounded by quotes
    crashHandlerFilePath.chop(1);
    crashHandlerFilePath.remove(0, 1);

    *quentierCrashHandlerFilePath = crashHandlerFilePath.toStdWString();

    QString quentierSymbolsFilePath, libquentierSymbolsFilePath;

    findCompressedSymbolsFiles(
        app,
        quentierSymbolsFilePath,
        libquentierSymbolsFilePath);

    CONVERT_PATH(quentierSymbolsFilePath);
    CONVERT_PATH(libquentierSymbolsFilePath);

    QString minidumpStackwalkFilePath =
        appFileInfo.absolutePath() +
        QString::fromUtf8("/quentier_minidump_stackwalk.exe");

    CONVERT_PATH(minidumpStackwalkFilePath);

    std::wstring * pQuentierCrashHandlerArgs = NULL;
    pQuentierCrashHandlerArgs = quentierCrashHandlerArgs;

    const wchar_t * minidumpsStorageFolderPathData =
        quentierMinidumpsStorageFolderPath->c_str();

    *pQuentierCrashHandlerArgs = quentierSymbolsFilePath.toStdWString();
    pQuentierCrashHandlerArgs->append(L" ");

    pQuentierCrashHandlerArgs->append(
        libquentierSymbolsFilePath.toStdWString());

    pQuentierCrashHandlerArgs->append(L" ");
    pQuentierCrashHandlerArgs->append(minidumpStackwalkFilePath.toStdWString());
    pQuentierCrashHandlerArgs->append(L" ");

    // NOTE: will need to append the path to generated minidump to this byte
    // array => will increase its reserved buffer and pray that the appending
    // of the path to generated minidump won't cause the resize; or, if it does,
    // that the resize succeeds and doesn't break the program about to crash
    pQuentierCrashHandlerArgs->reserve(
        static_cast<size_t>(pQuentierCrashHandlerArgs->size() + 1000));

    pBreakpadHandler = new google_breakpad::ExceptionHandler(
        std::wstring(minidumpsStorageFolderPathData),
        nullptr,
        ShowDumpResults,
        nullptr,
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        MiniDumpNormal,
        (const wchar_t*)nullptr,
        nullptr);
}

void detachBreakpad()
{
    if (pBreakpadHandler) {
        delete pBreakpadHandler;
        pBreakpadHandler = nullptr;
    }
}

} // namespace quentier
