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

#include "SuppressWarningsMacro.h"

SUPPRESS_WARNINGS
#include <client/linux/handler/exception_handler.h>
RESTORE_WARNINGS

#include <QStringList>
#include <QFileInfo>
#include <QProcess>

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
#include <QGlobalStatic>
#endif

#include <unistd.h>

namespace quentier {

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
Q_GLOBAL_STATIC(QString, quentierCrashHandlerFilePath)
Q_GLOBAL_STATIC(QString, quentierSymbolsFilePath)
Q_GLOBAL_STATIC(QString, libquentierSymbolsFilePath)
Q_GLOBAL_STATIC(QString, quentierMinidumpStackwalkFilePath)
#else
static QString quentierCrashHandlerFilePath;
static QString quentierSymbolsFilePath;
static QString libquentierSymbolsFilePath;
static QString quentierMinidumpStackwalkFilePath;
#endif

static bool dumpCallback(const google_breakpad::MinidumpDescriptor & descriptor,
                         void * context, bool succeeded)
{
    Q_UNUSED(context)

    pid_t p = fork();
    if (p == 0)
    {
        QString minidumpFileLocation = QString::fromLocal8Bit(descriptor.path());

        QStringList crashHandlerArgs;

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
        crashHandlerArgs << *quentierSymbolsFilePath;
        crashHandlerArgs << *libquentierSymbolsFilePath;
        crashHandlerArgs << *quentierMinidumpStackwalkFilePath;
#else
        crashHandlerArgs << quentierSymbolsFilePath;
        crashHandlerArgs << libquentierSymbolsFilePath;
        crashHandlerArgs << quentierMinidumpStackwalkFilePath;
#endif
        crashHandlerArgs << minidumpFileLocation;

        QProcess * pProcessHandle = new QProcess();
        QObject::connect(pProcessHandle, SIGNAL(finished(int,QProcess::ExitStatus)),
                         pProcessHandle, SLOT(deleteLater()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
        QString * pQuentierCrashHandlerFilePath = quentierCrashHandlerFilePath;
#else
        QString * pQuentierCrashHandlerFilePath = &quentierCrashHandlerFilePath;
#endif

        Q_UNUSED(pProcessHandle->start(*pQuentierCrashHandlerFilePath,
                                       crashHandlerArgs))
        pProcessHandle->waitForFinished(-1);
        return true;
    }
    else
    {
        printf("Dump path: %s\n", descriptor.path());
        return succeeded;
    }
}

static google_breakpad::MinidumpDescriptor * pBreakpadDescriptor = NULL;
static google_breakpad::ExceptionHandler * pBreakpadHandler = NULL;

void setupBreakpad(const QApplication & app)
{
    QString appFilePath = app.applicationFilePath();
    QFileInfo appFileInfo(appFilePath);

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    *quentierCrashHandlerFilePath =
        appFileInfo.absolutePath() +
        QString::fromUtf8("/quentier_crash_handler");

    *quentierMinidumpStackwalkFilePath =
        appFileInfo.absolutePath() +
        QString::fromUtf8("/quentier_minidump_stackwalk");

    findCompressedSymbolsFiles(app, *quentierSymbolsFilePath,
                               *libquentierSymbolsFilePath);
#else
    quentierCrashHandlerFilePath =
        appFileInfo.absolutePath() +
        QString::fromUtf8("/quentier_crash_handler");

    quentierMinidumpStackwalkFilePath =
        appFileInfo.absolutePath() +
        QString::fromUtf8("/quentier_minidump_stackwalk");

    findCompressedSymbolsFiles(app, quentierSymbolsFilePath,
                               libquentierSymbolsFilePath);
#endif

    pBreakpadDescriptor = new google_breakpad::MinidumpDescriptor("/tmp");
    pBreakpadHandler = new google_breakpad::ExceptionHandler(*pBreakpadDescriptor,
                                                             NULL, dumpCallback,
                                                             NULL, true, -1);
}

} // namespace quentier
