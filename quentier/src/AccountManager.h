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

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/exception/IQuentierException.h>
#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QDir>

namespace quentier {

class AccountManager: public QObject
{
    Q_OBJECT
public:
    class AccountInitializationException: public IQuentierException
    {
    public:
        explicit AccountInitializationException(const ErrorString & message);

    protected:
        virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
    };

public:
    AccountManager(QObject * parent = Q_NULLPTR);

    const QVector<Account> & availableAccounts() const
    { return m_availableAccounts; }

    /**
     * Attempts to retrieve the last used account from the app settings, in case of failure creates and returns
     * the default local account
     */
    Account currentAccount();

    void raiseAddAccountDialog();
    void raiseManageAccountsDialog();

Q_SIGNALS:
    void evernoteAccountAuthenticationRequested(QString host);
    void switchedAccount(Account account);
    void accountUpdated(Account account);
    void notifyError(ErrorString error);

public Q_SLOTS:
    void switchAccount(const Account & account);

private Q_SLOTS:
    void onLocalAccountAdditionRequested(QString name, QString fullName);
    void onAccountDisplayNameChanged(Account account);

private:
    void detectAvailableAccounts();

    QSharedPointer<Account> createDefaultAccount(ErrorString & errorDescription);
    QSharedPointer<Account> createLocalAccount(const QString & name, const QString & displayName,
                                               ErrorString & errorDescription);
    bool createAccountInfo(const Account & account);

    bool writeAccountInfo(const QString & name, const QString & displayName, const bool isLocal,
                          const qevercloud::UserID id, const QString & evernoteAccountType,
                          const QString & evernoteHost, ErrorString & errorDescription);

    QString evernoteAccountTypeToString(const Account::EvernoteAccountType::type type) const;

    void readComplementaryAccountInfo(Account & account);

    QDir accountStorageDir(const QString & name, const bool isLocal, const qevercloud::UserID id, const QString & evernoteHost) const;

    // Tries to restore the last used account from the app settings;
    // in case of success returns non-null pointer to account, null otherwise
    QSharedPointer<Account> lastUsedAccount() const;

    void updateLastUsedAccount(const Account & account);

private:
    QVector<Account>   m_availableAccounts;
};

} // namespace quentier

#endif // QUENTIER_ACCOUNT_MANAGER_H
