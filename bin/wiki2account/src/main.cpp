/*
 * Copyright 2019 Dmitry Ivanov
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

#include "PrepareLocalStorageManager.h"
#include "PrepareNotebooks.h"
#include "ProcessStartupAccount.h"
#include "ProcessNoteOptions.h"
#include "ProcessNotebookOptions.h"

#include <lib/account/AccountManager.h>
#include <lib/initialization/Initialize.h>

#include <quentier/logging/QuentierLogger.h>

#include <QApplication>
#include <QTextStream>
#include <QThread>

#include <iostream>

using namespace quentier;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("wiki2account"));

    QHash<QString,CommandLineParser::CommandLineOptionData> availableCmdOptions;
    composeCommonAvailableCommandLineOptions(availableCmdOptions);

    auto & newAccountData = availableCmdOptions[QStringLiteral("new-account")];
    newAccountData.m_description =
        QStringLiteral("add notes created from wiki articles to a new local account");

    ParseCommandLineResult parseCmdResult;
    parseCommandLine(argc, argv, availableCmdOptions, parseCmdResult);
    if (parseCmdResult.m_shouldQuit)
    {
        if (!parseCmdResult.m_errorDescription.isEmpty()) {
            std::cerr << parseCmdResult.m_errorDescription.nonLocalizedString()
                         .toLocal8Bit().constData();
            return 1;
        }

        std::cout << parseCmdResult.m_responseMessage.toLocal8Bit().constData();
        return 0;
    }

    if (!processStorageDirCommandLineOption(parseCmdResult.m_cmdOptions)) {
        return 1;
    }

    // Initialize logging
    QUENTIER_INITIALIZE_LOGGING();
    QUENTIER_SET_MIN_LOG_LEVEL(Trace);

    Account account = processStartupAccount(parseCmdResult.m_cmdOptions);
    if (account.isEmpty()) {
        return 1;
    }

    QString targetNotebookName;
    quint32 numNewNotebooks = 0;
    bool res = processNotebookOptions(parseCmdResult.m_cmdOptions,
                                      targetNotebookName, numNewNotebooks);
    if (!res) {
        return 1;
    }

    quint32 numNotes = 0;
    res = processNoteOptions(parseCmdResult.m_cmdOptions, numNotes);
    if (!res) {
        return 1;
    }

    QThread * pLocalStorageManagerThread = new QThread;
    pLocalStorageManagerThread->setObjectName(QStringLiteral("LocalStorageManagerThread"));
    QObject::connect(pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));
    pLocalStorageManagerThread->start();

    ErrorString errorDescription;
    LocalStorageManagerAsync * pLocalStorageManager = prepareLocalStorageManager(
        account, *pLocalStorageManagerThread, errorDescription);
    if (!pLocalStorageManager) {
        pLocalStorageManagerThread->quit();
        return 1;
    }

    errorDescription.clear();
    QList<Notebook> notebooks = prepareNotebooks(targetNotebookName,
                                                 numNewNotebooks,
                                                 *pLocalStorageManager,
                                                 errorDescription);
    if (notebooks.isEmpty()) {
        pLocalStorageManagerThread->quit();
        return 1;
    }

    // TODO: continue from here

    return app.exec();
}
