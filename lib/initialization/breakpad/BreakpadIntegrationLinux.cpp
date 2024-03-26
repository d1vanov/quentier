/*
 * Copyright 2017-2024 Dmitry Ivanov
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

// clang-format off
CLANG_SUPPRESS_WARNING(-Wshorten-64-to-32)
CLANG_SUPPRESS_WARNING(-Wsign-conversion)

#if defined(__clang__) && defined(__clang_major__) && (__clang_major__ >= 6)
CLANG_SUPPRESS_WARNING(-Wimplicit-int-conversion)
#endif

GCC_SUPPRESS_WARNING(-Wconversion)
MSVC_SUPPRESS_WARNING(4365)
MSVC_SUPPRESS_WARNING(4244)
MSVC_SUPPRESS_WARNING(4305)
// clang-format on

#include <client/linux/handler/exception_handler.h>

RESTORE_WARNINGS

#include <QFileInfo>
#include <QGlobalStatic>
#include <QProcess>
#include <QStringList>

#include <unistd.h>

namespace quentier {

Q_GLOBAL_STATIC(QString, quentierCrashHandlerFilePath)
Q_GLOBAL_STATIC(QString, quentierSymbolsFilePath)
Q_GLOBAL_STATIC(QString, libquentierSymbolsFilePath)
Q_GLOBAL_STATIC(QString, quentierMinidumpStackwalkFilePath)

namespace {

[[nodiscard]] bool dumpCallback(
    const google_breakpad::MinidumpDescriptor & descriptor, void * context,
    bool succeeded)
{
    Q_UNUSED(context)

    pid_t p = fork();
    if (p == 0) {
        QString minidumpFileLocation =
            QString::fromLocal8Bit(descriptor.path());

        QStringList crashHandlerArgs;

        crashHandlerArgs << *quentierSymbolsFilePath;
        crashHandlerArgs << *libquentierSymbolsFilePath;
        crashHandlerArgs << *quentierMinidumpStackwalkFilePath;
        crashHandlerArgs << minidumpFileLocation;

        auto * processHandle = new QProcess;

        QObject::connect(
            processHandle, SIGNAL(finished(int, QProcess::ExitStatus)),
            processHandle, SLOT(deleteLater()));

        QString * crashHandlerFilePath = quentierCrashHandlerFilePath;
        processHandle->start(*crashHandlerFilePath, crashHandlerArgs);
        processHandle->waitForFinished(-1);
        return true;
    }
    else {
        printf("Dump path: %s\n", descriptor.path());
        return succeeded;
    }
}

} // namespace

static google_breakpad::MinidumpDescriptor * gBreakpadDescriptor = nullptr;
static google_breakpad::ExceptionHandler * gBreakpadHandler = nullptr;

void setupBreakpad(const QApplication & app)
{
    const QString appFilePath = app.applicationFilePath();
    const QFileInfo appFileInfo{appFilePath};

    *quentierCrashHandlerFilePath = appFileInfo.absolutePath() +
        QString::fromUtf8("/quentier_crash_handler");

    *quentierMinidumpStackwalkFilePath = appFileInfo.absolutePath() +
        QString::fromUtf8("/quentier_minidump_stackwalk");

    findCompressedSymbolsFiles(
        app, *quentierSymbolsFilePath, *libquentierSymbolsFilePath);

    gBreakpadDescriptor = new google_breakpad::MinidumpDescriptor("/tmp");

    gBreakpadHandler = new google_breakpad::ExceptionHandler(
        *gBreakpadDescriptor, nullptr, dumpCallback, nullptr, true, -1);
}

void detachBreakpad()
{
    if (gBreakpadHandler) {
        delete gBreakpadHandler;
        gBreakpadHandler = nullptr;
    }

    if (gBreakpadDescriptor) {
        delete gBreakpadDescriptor;
        gBreakpadDescriptor = nullptr;
    }
}

} // namespace quentier
