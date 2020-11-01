/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#include "ProcessStartupAccount.h"

#include <lib/account/AccountManager.h>
#include <lib/initialization/Initialize.h>

#include <quentier/utility/Compat.h>

#include <iostream>
#include <memory>

namespace quentier {

namespace {

Account createNewLocalAccount(
    const QString & name, AccountManager & accountManager)
{
    QString accountName = name;
    int suffix = 2;
    Account account;

    while (true) {
        account = accountManager.createNewLocalAccount(accountName);
        if (account.isEmpty()) {
            accountName = name + QStringLiteral(" (") +
                QString::number(suffix) + QStringLiteral(")");
            ++suffix;
            continue;
        }

        break;
    }

    return account;
}

} // namespace

Account processStartupAccount(const CommandLineParser::Options & options)
{
    std::unique_ptr<Account> pAccount;
    if (!processAccountCommandLineOption(options, pAccount)) {
        return Account();
    }

    if (pAccount) {
        return *pAccount;
    }

    AccountManager accountManager;
    if (options.contains(QStringLiteral("new-account"))) {
        return createNewLocalAccount(
            QStringLiteral("Wiki notes"), accountManager);
    }

    auto availableAccounts = accountManager.availableAccounts();
    QString availableAccountsInfo;

    QTextStream strm(
        &availableAccountsInfo, QIODevice::ReadWrite | QIODevice::Text);

    strm << "Available accounts:\n";
    size_t counter = 1;

    for (const auto & availableAccount: qAsConst(availableAccounts)) {
        bool isEvernoteAccount =
            (availableAccount.type() == Account::Type::Evernote);

        strm << " " << counter++ << ") " << availableAccount.name() << " ("
             << availableAccount.displayName() << "), "
             << (isEvernoteAccount ? "Evernote" : "local") << ", ";

        if (isEvernoteAccount) {
            strm << availableAccount.evernoteHost();
        }

        strm << "\n";
    }

    strm << "Enter the number of account to which new notes should be added\n"
         << "If you want to create a new account, write \"new\"\n"
         << "> ";
    strm.flush();

    QTextStream stdoutStrm(stdout);
    stdoutStrm << availableAccountsInfo;
    stdoutStrm.flush();

    QTextStream stdinStrm(stdin);
    int accountNum = -1;
    while (true) {
        QString line = stdinStrm.readLine().trimmed();
        if (line == QStringLiteral("new")) {
            return createNewLocalAccount(
                QStringLiteral("Wiki notes"), accountManager);
        }

        bool conversionResult = false;
        accountNum = line.toInt(&conversionResult);
        if (!conversionResult) {
            stdoutStrm << "Failed to parse account number, please try again\n"
                       << "> ";
            continue;
        }

        --accountNum;
        if (accountNum < 0 || accountNum >= availableAccounts.size()) {
            stdoutStrm << "Invalid account number, please try again\n> ";
            continue;
        }

        break;
    }

    return availableAccounts[accountNum];
}

} // namespace quentier
