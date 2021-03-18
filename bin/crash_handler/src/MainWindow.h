/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#ifndef QUENTIER_CRASH_HANDLER_MAINWINDOW_H
#define QUENTIER_CRASH_HANDLER_MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QString>

namespace Ui {
class MainWindow;
}

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(
        const QString & quentierSymbolsFileLocation,
        const QString & libquentierSymbolsFileLocation,
        const QString & stackwalkBinaryLocation,
        const QString & minidumpLocation, QWidget * parent = nullptr);

    ~MainWindow() override;

private Q_SLOTS:
    void onMinidumpStackwalkReadyReadStandardOutput();
    void onMinidumpStackwalkReadyReadStandardError();

    void onMinidumpStackwalkProcessFinished(
        int exitCode, QProcess::ExitStatus ExitStatus);

    void onSymbolsUnpackerFinished(bool status, QString errorDescription);

private:
    [[nodiscard]] QString readData(QProcess & process, bool fromStdout);
    [[nodiscard]] QString versionInfos() const;

private:
    Ui::MainWindow * m_pUi;

    int m_numPendingSymbolsUnpackers = 0;

    QString m_minidumpLocation;
    QString m_stackwalkBinary;
    QString m_unpackedSymbolsRootPath;
    QString m_symbolsUnpackingErrors;
    QString m_output;
    QString m_error;
};

#endif // QUENTIER_CRASH_HANDLER_MAINWINDOW_H
