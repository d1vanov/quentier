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

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Utility.h"
#include "SymbolsUnpacker.h"
#include "../src/utility/HumanReadableVersionInfo.h"
#include <VersionInfo.h>
#include <quentier/utility/VersionInfo.h>
#include <QDir>
#include <QDesktopServices>
#include <QThreadPool>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#endif

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || (defined(_MSC_VER) && (_MSC_VER <= 1600))
#define QNSIGNAL(className, methodName, ...) SIGNAL(methodName(__VA_ARGS__))
#define QNSLOT(className, methodName, ...) SLOT(methodName(__VA_ARGS__))
#else
#define QNSIGNAL(className, methodName, ...) &className::methodName
#define QNSLOT(className, methodName, ...) &className::methodName
#endif

MainWindow::MainWindow(const QString & quentierSymbolsFileLocation,
                       const QString & libquentierSymbolsFileLocation,
                       const QString & stackwalkBinaryLocation,
                       const QString & minidumpLocation,
                       QWidget * parent) :
    QMainWindow(parent),
    m_pUi(new Ui::MainWindow),
    m_numPendingSymbolsUnpackers(0),
    m_minidumpLocation(),
    m_stackwalkBinary(),
    m_unpackedSymbolsRootPath(),
    m_symbolsUnpackingErrors(),
    m_output(),
    m_error()
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Quentier crashed"));

    m_minidumpLocation = nativePathToUnixPath(minidumpLocation);
    m_pUi->minidumpFilePathLineEdit->setText(m_minidumpLocation);

    m_stackwalkBinary = nativePathToUnixPath(stackwalkBinaryLocation);
    QFileInfo stackwalkBinaryInfo(m_stackwalkBinary);
    if (Q_UNLIKELY(!stackwalkBinaryInfo.exists())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: minidump stackwalk utility file doesn't exist") +
                                                     QString::fromUtf8(": ") + QDir::toNativeSeparators(m_stackwalkBinary));
        return;
    }
    else if (Q_UNLIKELY(!stackwalkBinaryInfo.isFile())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: the path to minidump stackwalk utility doesn't point to an actual file") +
                                                     QString::fromUtf8(": ") + QDir::toNativeSeparators(m_stackwalkBinary));
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString tmpDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
    QString tmpDirPath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif

    m_unpackedSymbolsRootPath = tmpDirPath + QString::fromUtf8("/Quentier_debugging_symbols/symbols");
    bool res = removeDir(m_unpackedSymbolsRootPath);
    if (Q_UNLIKELY(!res)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: the directory containing the unpacked debugging symbols already exists and can't be removed") +
                                                     QString::fromUtf8(": ") + QDir::toNativeSeparators(m_unpackedSymbolsRootPath));
        return;
    }

    QDir unpackRootDir(m_unpackedSymbolsRootPath);
    res = unpackRootDir.mkpath(m_unpackedSymbolsRootPath);
    if (!res) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: the directory for the unpacked debugging symbols can't be created") +
                                                     QString::fromUtf8(": ") + QDir::toNativeSeparators(m_unpackedSymbolsRootPath));
        return;
    }

    QString output = QString::fromUtf8("Version info:\n\n");
    output += versionInfos();
    output += QString::fromUtf8("\n\n");
    output += tr("Loading debugging symbols, please wait");
    output += QString::fromUtf8("...");
    m_pUi->stackTracePlainTextEdit->setPlainText(output);

    SymbolsUnpacker * pQuentierSymbolsUnpacker = new SymbolsUnpacker(quentierSymbolsFileLocation,
                                                                     m_unpackedSymbolsRootPath);
    QObject::connect(pQuentierSymbolsUnpacker, QNSIGNAL(SymbolsUnpacker,finished,bool,QString),
                     this, QNSLOT(MainWindow,onSymbolsUnpackerFinished,bool,QString));
    ++m_numPendingSymbolsUnpackers;

    SymbolsUnpacker * pLibquentierSymbolsUnpacker = new SymbolsUnpacker(libquentierSymbolsFileLocation,
                                                                        m_unpackedSymbolsRootPath);
    QObject::connect(pLibquentierSymbolsUnpacker, QNSIGNAL(SymbolsUnpacker,finished,bool,QString),
                     this, QNSLOT(MainWindow,onSymbolsUnpackerFinished,bool,QString));
    ++m_numPendingSymbolsUnpackers;

    QThreadPool::globalInstance()->start(pQuentierSymbolsUnpacker);
    QThreadPool::globalInstance()->start(pLibquentierSymbolsUnpacker);
}

MainWindow::~MainWindow()
{
    delete m_pUi;
}

void MainWindow::onMinidumpStackwalkReadyReadStandardOutput()
{
    QProcess * pStackwalkProcess = qobject_cast<QProcess*>(sender());
    if (Q_UNLIKELY(!pStackwalkProcess)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: can't cast the invoker of minidump stackwalk stdout update to QProcess"));
        return;
    }

    m_output += readData(*pStackwalkProcess, /* from stdout = */ true);

    QString output;

    if (!m_symbolsUnpackingErrors.isEmpty()) {
        output = m_symbolsUnpackingErrors;
    }

    output += m_output;

    m_pUi->stackTracePlainTextEdit->setPlainText(output);
}

void MainWindow::onMinidumpStackwalkReadyReadStandardError()
{
    QProcess * pStackwalkProcess = qobject_cast<QProcess*>(sender());
    if (Q_UNLIKELY(!pStackwalkProcess)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: can't cast the invoker of minidump stackwalk stderr update to QProcess"));
        return;
    }

    m_error += readData(*pStackwalkProcess, /* from stdout = */ false);
}

void MainWindow::onMinidumpStackwalkProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    QString output;

    if (!m_symbolsUnpackingErrors.isEmpty()) {
        output = m_symbolsUnpackingErrors;
        output += QString::fromUtf8("\n");
    }

    output += QString::fromUtf8("Version info:\n\n");
    output += versionInfos();
    output += QString::fromUtf8("\n\n");
    output += tr("Stacktrace extraction finished, exit code") + QString::fromUtf8(": ") + QString::number(exitCode) + QString::fromUtf8("\n");
    output += m_output;
    output += QString::fromUtf8("\n\n");
    output += m_error;

    m_pUi->stackTracePlainTextEdit->setPlainText(output);
}

void MainWindow::onSymbolsUnpackerFinished(bool status, QString errorDescription)
{
    if (m_numPendingSymbolsUnpackers != 0) {
        --m_numPendingSymbolsUnpackers;
    }

    if (!status)
    {
        if (m_symbolsUnpackingErrors.isEmpty()) {
            m_symbolsUnpackingErrors = tr("Errors detected during symbols unpacking") + QString::fromUtf8(":\n\n");
        }

        m_symbolsUnpackingErrors += errorDescription;
        m_symbolsUnpackingErrors += QString::fromUtf8("\n");
    }

    if (m_numPendingSymbolsUnpackers != 0) {
        return;
    }

    QProcess * pStackwalkProcess = new QProcess(this);
    QObject::connect(pStackwalkProcess, QNSIGNAL(QProcess,readyReadStandardOutput),
                     this, QNSLOT(MainWindow,onMinidumpStackwalkReadyReadStandardOutput));
    QObject::connect(pStackwalkProcess, QNSIGNAL(QProcess,readyReadStandardError),
                     this, QNSLOT(MainWindow,onMinidumpStackwalkReadyReadStandardError));
    QObject::connect(pStackwalkProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                     this, SLOT(onMinidumpStackwalkProcessFinished(int,QProcess::ExitStatus)));

    QStringList stackwalkArgs;
    stackwalkArgs.reserve(2);
    stackwalkArgs << QDir::fromNativeSeparators(m_minidumpLocation);
    stackwalkArgs << m_unpackedSymbolsRootPath;

    pStackwalkProcess->start(m_stackwalkBinary, stackwalkArgs, QIODevice::ReadOnly);
}

QString MainWindow::readData(QProcess & process, const bool fromStdout)
{
    QByteArray output = (fromStdout
                         ? process.readAllStandardOutput()
                         : process.readAllStandardError());
    return QString::fromUtf8(output);
}

QString MainWindow::versionInfos() const
{
    QString result = QString::fromUtf8("libquentier: ");

    result += quentier::libquentierRuntimeInfo();
    result += QString::fromUtf8("\n");

    result += QString::fromUtf8("Quentier: ");
    result += quentier::quentierVersion();
    result += QString::fromUtf8(", build info: ");
    result += quentier::quentierBuildInfo();

    result += QString::fromUtf8("\n\nBuilt with Qt ");
    result += QString::fromUtf8(QT_VERSION_STR);
    result += QString::fromUtf8(", uses Qt ");
    result += QString::fromUtf8(qVersion());
    return result;
}
