/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/exception/IQuentierException.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>

#include <QDir>
#include <QNetworkProxy>
#include <QObject>
#include <QList>

#include <memory>

class QDebug;

namespace quentier {

class AccountModel;

class AccountManager : public QObject
{
    Q_OBJECT
public:
    class AccountInitializationException : public IQuentierException
    {
    public:
        explicit AccountInitializationException(const ErrorString & message);

    protected:
        [[nodiscard]] QString exceptionDisplayName() const override;
    };

public:
    AccountManager(QObject * parent = nullptr);
    ~AccountManager();

    [[nodiscard]] const QList<Account> & availableAccounts() const noexcept;
    [[nodiscard]] AccountModel & accountModel();

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
    [[nodiscard]] Account startupAccount();

    /**
     * Tries to restore the last used account from the app settings
     *
     * @return                  Non-empty account in case of success, empty one
     *                          otherwise
     */
    [[nodiscard]] Account lastUsedAccount();

    /**
     * @brief The AccountSource enum describes the source of account returned
     * from currentAccount method
     */
    enum class AccountSource
    {
        Startup = 0,
        LastUsed,
        ExistingDefault,
        NewDefault
    };

    friend QDebug & operator<<(QDebug & dbg, const AccountSource source);

    /**
     * Either finds existing default account or creates new default account
     *
     * @param pAccountSource    If not nullptr, after the call *pAccountSource
     *                          would contain the source of the returned default
     *                          account
     */
    [[nodiscard]] Account defaultAccount(
        AccountSource * pAccountSource = nullptr);

    /**
     * Attempts to retrieve the last used account from the app settings, in case
     * of failure creates and returns the default local account
     *
     * @param pAccountSource    If not nullptr, after the call *pAccountSource
     *                          would contain the source of the returned account
     */
    [[nodiscard]] Account currentAccount(
        AccountSource * pAccountSource = nullptr);

    [[nodiscard]] int execAddAccountDialog();
    [[nodiscard]] int execManageAccountsDialog();

    /**
     * Attempts to create a new local account
     *
     * @param name              If specified, name to be used for the new
     *                          account; if name is not set, it would be chosen
     *                          automatically
     * @return                  Either non-empty Account object if creation was
     *                          successful or empty Account object otherwise
     */
    [[nodiscard]] Account createNewLocalAccount(QString name = QString{});

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

    [[nodiscard]] Account createDefaultAccount(ErrorString & errorDescription);

    [[nodiscard]] Account createLocalAccount(
        const QString & name, const QString & displayName,
        ErrorString & errorDescription);

    [[nodiscard]] bool createAccountInfo(const Account & account);

    [[nodiscard]] bool writeAccountInfo(
        const QString & name, const QString & displayName, const bool isLocal,
        const qevercloud::UserID id, const QString & evernoteAccountType,
        const QString & evernoteHost, const QString & shardId,
        ErrorString & errorDescription);

    [[nodiscard]] QString evernoteAccountTypeToString(
        const Account::EvernoteAccountType type) const;

    void readComplementaryAccountInfo(Account & account);

    /**
     * Tries to find the account corresponding to the specified environment
     * variables specifying the details of the account
     *
     * @return              Non-empty account in case of success, empty one
     *                      otherwise
     */
    [[nodiscard]] Account accountFromEnvVarHints() const;

    [[nodiscard]] Account findAccount(
        const bool isLocal, const QString & accountName,
        const qevercloud::UserID id, const Account::EvernoteAccountType type,
        const QString & evernoteHost);

    void updateLastUsedAccount(const Account & account);

private:
    std::unique_ptr<AccountModel> m_pAccountModel;
};

} // namespace quentier
