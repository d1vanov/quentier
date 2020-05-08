/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include <quentier/exception/IQuentierException.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>

#include <QDir>
#include <QNetworkProxy>
#include <QObject>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QDebug)

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
        virtual const QString exceptionDisplayName() const override;
    };

public:
    AccountManager(QObject * parent = nullptr);
    ~AccountManager();

    const QVector<Account> & availableAccounts() const;

    AccountModel & accountModel();

    /**
     * Sets the account which should be used on app's startup. This method is
     * intended to be called in the early stages of the app's startup, when
     * the account which needs to be used is found within command line arguments
     * or from any other source but before the app has constructed its main
     * widgets.
     *
     * The passed in account's description is stored in the environment
     * variables so that even another instance of AccountManager can retrieve
     * the previously stored startup account in startupAccount method.
     */
    void setStartupAccount(const Account & account);

    /**
     * Provides the account which should be used on app's startup. If no account
     * was previously specified via setStartupAccount method, the result is the
     * same as from calling currentAccount
     */
    Account startupAccount();

    /**
     * Tries to restore the last used account from the app settings
     *
     * @return                  Non-empty account in case of success, empty one
     *                          otherwise
     */
    Account lastUsedAccount();

    /**
     * @brief The AccountSource enum describes the source of account returned
     * from currentAccount method
     */
    enum class AccountSource
    {
        Startup = 0,
        LastUsed,
        NewDefault
    };

    friend QDebug & operator<<(QDebug & dbg, const AccountSource source);

    /**
     * Attempts to retrieve the last used account from the app settings, in case
     * of failure creates and returns the default local account
     *
     * @param pAccountSource    If not nullptr, after the call *pAccountSource
     *                          would contain the source of the returned account
     */
    Account currentAccount(AccountSource * pAccountSource = nullptr);

    int execAddAccountDialog();
    int execManageAccountsDialog();

    /**
     * Attempts to create a new local account
     *
     * @param name              If specified, name to be used for the new
     *                          account; if name is not set, it would be chosen
     *                          automatically
     * @return                  Either non-empty Account object if creation was
     *                          successful or empty Account object otherwise
     */
    Account createNewLocalAccount(QString name = QString());

Q_SIGNALS:
    void evernoteAccountAuthenticationRequested(
        QString host, QNetworkProxy proxy);

    void switchedAccount(Account account);
    void accountUpdated(Account account);
    void notifyError(ErrorString error);
    void accountAdded(Account account);
    void accountRemoved(Account account);

    void revokeAuthenticationRequested(qevercloud::UserID userId);

// private signals
    void authenticationRevoked(
        bool success, ErrorString errorDescription, qevercloud::UserID userId);

public Q_SLOTS:
    void switchAccount(const Account & account);

    void onAuthenticationRevoked(
        bool success, ErrorString errorDescription, qevercloud::UserID userId);

private Q_SLOTS:
    void onLocalAccountAdditionRequested(QString name, QString fullName);
    void onAccountDisplayNameChanged(Account account);

private:
    void detectAvailableAccounts();

    Account createDefaultAccount(ErrorString & errorDescription);

    Account createLocalAccount(
        const QString & name, const QString & displayName,
        ErrorString & errorDescription);

    bool createAccountInfo(const Account & account);

    bool writeAccountInfo(
        const QString & name, const QString & displayName,
        const bool isLocal, const qevercloud::UserID id,
        const QString & evernoteAccountType,
        const QString & evernoteHost, const QString & shardId,
        ErrorString & errorDescription);

    QString evernoteAccountTypeToString(
        const Account::EvernoteAccountType type) const;

    void readComplementaryAccountInfo(Account & account);

    /**
     * Tries to find the account corresponding to the specified environment
     * variables specifying the details of the account
     *
     * @return              Non-empty account in case of success, empty one
     *                      otherwise
     */
    Account accountFromEnvVarHints();

    Account findAccount(
        const bool isLocal, const QString & accountName,
        const qevercloud::UserID id, const Account::EvernoteAccountType type,
        const QString & evernoteHost);

    void updateLastUsedAccount(const Account & account);

private:
    QScopedPointer<AccountModel>   m_pAccountModel;
};

} // namespace quentier

#endif // QUENTIER_LIB_ACCOUNT_ACCOUNT_MANAGER_H
