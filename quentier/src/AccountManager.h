/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_ACCOUNT_MANAGER_H
#define QUENTIER_ACCOUNT_MANAGER_H

#include "AvailableAccount.h"
#include <quentier/types/Account.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/exception/IQuentierException.h>
#include <QObject>
#include <QSharedPointer>
#include <QVector>

class AccountManager: public QObject
{
    Q_OBJECT
public:
    class AccountInitializationException: public quentier::IQuentierException
    {
    public:
        explicit AccountInitializationException(const quentier::QNLocalizedString & message);

    protected:
        virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
    };

public:
    AccountManager(QObject * parent = Q_NULLPTR);

    const QVector<AvailableAccount> & availableAccounts() const
    { return m_availableAccounts; }

    /**
     * Attempts to retrieve the last used account from the app settings, in case of failure creates and returns
     * the default local account
     */
    quentier::Account currentAccount();

    void raiseAddAccountDialog();
    void raiseManageAccountsDialog();

Q_SIGNALS:
    void switchedAccount(quentier::Account account);
    void notifyError(quentier::QNLocalizedString error);

private:
    void detectAvailableAccounts();

    QSharedPointer<quentier::Account> createDefaultAccount(quentier::QNLocalizedString & errorDescription);

    // Tries to restore the last used account from the app settings;
    // in case of success returns non-null pointer to account, null otherwise
    QSharedPointer<quentier::Account> lastUsedAccount() const;

    void updateLastUsedAccount(const quentier::Account & account);

private:
    QVector<AvailableAccount>   m_availableAccounts;
};

#endif // QUENTIER_ACCOUNT_MANAGER_H
