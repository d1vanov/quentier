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

#include "ProcessStartupAccount.h"

#include <lib/account/AccountManager.h>
#include <lib/initialization/Initialize.h>

namespace quentier {

Account * processStartupAccount(const CommandLineParser::CommandLineOptions & options)
{
    QScopedPointer<Account> pAccount;
    if (!processAccountCommandLineOption(options, pAccount)) {
        return Q_NULLPTR;
    }

    if (!pAccount.isNull()) {
        return pAccount.take();
    }

    AccountManager accountManager;
    if (options.contains(QStringLiteral("new-account"))) {
        // TODO: create new account within account manager and return it
        return Q_NULLPTR;
    }

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

        strm << "\n";
    }

    strm << "Enter the number of account to which new notes should be added\n"
         << "If you want to create a new account, write \"new\"\n"
         << ">";

    // TODO: offer the user to choose startup account

    return Q_NULLPTR;
}

} // namespace quentier
