/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_ACCOUNT_ACCOUNT_MANAGER_H
#define QUENTIER_LIB_ACCOUNT_ACCOUNT_MANAGER_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/exception/IQuentierException.h>

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QDir>
#include <QNetworkProxy>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountModel)

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
    ~AccountManager();

    const QVector<Account> & availableAccounts() const;

    AccountModel & accountModel();

    /**
     * Attempts to retrieve the last used account from the app settings, in case
     * of failure creates and returns the default local account
     *
     * @param pCreatedDefaultAccount        Optional pointer to bool parameter
     *                                      which, if not null, is used to report
     *                                      whether no existing account was found
     *                                      so the default account was created
     */
    Account currentAccount(bool * pCreatedDefaultAccount = Q_NULLPTR);

    int execAddAccountDialog();
    int execManageAccountsDialog();

    /**
     * Attempts to create a new local account
     *
     * @param name                          If specified, name to be used for
     *                                      the new account; if name is not set,
     *                                      it would be chosen automatically
     * @return                              Either non-empty Account object if
     *                                      creation was successful or empty
     *                                      Account object otherwise
     */
    Account createNewLocalAccount(QString name = QString());

Q_SIGNALS:
    void evernoteAccountAuthenticationRequested(QString host, QNetworkProxy proxy);
    void switchedAccount(Account account);
    void accountUpdated(Account account);
    void notifyError(ErrorString error);
    void accountAdded(Account account);
    void accountRemoved(Account account);

public Q_SLOTS:
    void switchAccount(const Account & account);

private Q_SLOTS:
    void onLocalAccountAdditionRequested(QString name, QString fullName);
    void onAccountDisplayNameChanged(Account account);

private:
    void detectAvailableAccounts();

    QSharedPointer<Account> createDefaultAccount(ErrorString & errorDescription,
                                                 bool * pCreatedDefaultAccount);
    QSharedPointer<Account> createLocalAccount(const QString & name,
                                               const QString & displayName,
                                               ErrorString & errorDescription);
    bool createAccountInfo(const Account & account);

    bool writeAccountInfo(const QString & name, const QString & displayName,
                          const bool isLocal, const qevercloud::UserID id,
                          const QString & evernoteAccountType,
                          const QString & evernoteHost, const QString & shardId,
                          ErrorString & errorDescription);

    QString evernoteAccountTypeToString(
        const Account::EvernoteAccountType::type type) const;

    void readComplementaryAccountInfo(Account & account);

    /**
     * Tries to find the account corresponding to the specified environment
     * variables specifying the details of the account
     *
     * @return              Non-null pointer to the account in case of success,
     *                      null pointer otherwise
     */
    QSharedPointer<Account> accountFromEnvVarHints();

    /**
     * Tries to restore the last used account from the app settings
     *
     * @return              Non-null pointer to the account in case of success,
     *                      null pointer otherwise
     */
    QSharedPointer<Account> lastUsedAccount();

    QSharedPointer<Account> findAccount(const bool isLocal,
                                        const QString & accountName,
                                        const qevercloud::UserID id,
                                        const Account::EvernoteAccountType::type type,
                                        const QString & evernoteHost);

    void updateLastUsedAccount(const Account & account);

private:
    QScopedPointer<AccountModel>   m_pAccountModel;
};

} // namespace quentier

#endif // QUENTIER_LIB_ACCOUNT_ACCOUNT_MANAGER_H
