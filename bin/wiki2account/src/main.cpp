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

#include <lib/account/AccountManager.h>
#include <lib/initialization/Initialize.h>

#include <quentier/logging/QuentierLogger.h>

#include <QApplication>
#include <QTextStream>

#include <iostream>

using namespace quentier;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("wiki2account"));

    ParseCommandLineResult parseCmdResult;
    parseCommandLine(argc, argv, parseCmdResult);
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

    QScopedPointer<Account> pStartupAccount;
    if (!processAccountCommandLineOption(parseCmdResult.m_cmdOptions, pStartupAccount)) {
        return 1;
    }

    if (pStartupAccount.isNull())
    {
        AccountManager accountManager;
        QVector<Account> availableAccounts = accountManager.availableAccounts();

        QString availableAccountsInfo;
        QTextStream strm(&availableAccountsInfo,
                         QIODevice::ReadWrite | QIODevice::Text);

        strm << "Available accounts:\n";
        size_t counter = 1;
        for(auto it = availableAccounts.constBegin(),
            end = availableAccounts.constEnd(); it != end; ++it)
        {
            const Account & availableAccount = *it;
            bool isEvernoteAccount =
                (availableAccount.type() == Account::Type::Evernote);

            strm << " " << counter++ << ") "
                 << availableAccount.name()
                 << " (" << availableAccount.displayName() << "), "
                 << (isEvernoteAccount ? "Evernote" : "local")
                 << ", ";
            if (isEvernoteAccount) {
                strm << availableAccount.evernoteHost();
            }
        }

        // TODO: offer the user to choose startup account
    }

    // TODO: implement further
    return app.exec();
}
