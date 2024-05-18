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

#include "AccountManager.h"

#include "AccountModel.h"
#include "AddAccountDialog.h"
#include "ManageAccountsDialog.h"

#include <lib/preferences/keys/Account.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/SuppressWarnings.h>
#include <quentier/utility/System.h>

#include <QDebug>
#include <QXmlStreamWriter>

#include <memory>
#include <utility>

namespace quentier {

AccountManager::AccountManager(QObject * parent) :
    QObject{parent}, m_accountModel{new AccountModel{this}}
{
    QObject::connect(
        m_accountModel.get(), &AccountModel::accountAdded, this,
        &AccountManager::accountAdded);

    QObject::connect(
        m_accountModel.get(), &AccountModel::accountDisplayNameChanged, this,
        &AccountManager::onAccountDisplayNameChanged);

    QObject::connect(
        m_accountModel.get(), &AccountModel::accountRemoved, this,
        &AccountManager::accountRemoved);

    detectAvailableAccounts();
}

AccountManager::~AccountManager() = default;

const QList<Account> & AccountManager::availableAccounts() const noexcept
{
    return m_accountModel->accounts();
}

AccountModel & AccountManager::accountModel()
{
    return *m_accountModel;
}

void AccountManager::setStartupAccount(const Account & account)
{
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::setStartupAccount: " << account);

    qputenv(
        preferences::keys::startupAccountNameEnvVar.data(),
        account.name().toLocal8Bit());

    qputenv(
        preferences::keys::startupAccountTypeEnvVar.data(),
        ((account.type() == Account::Type::Local) ? QByteArray{"1"}
                                                  : QByteArray{"0"}));

    qputenv(
        preferences::keys::startupAccountIdEnvVar.data(),
        QByteArray::number(account.id()));

    qputenv(
        preferences::keys::startupAccountEvernoteAccountTypeEnvVar.data(),
        QByteArray::number(static_cast<qint64>(account.evernoteAccountType())));

    qputenv(
        preferences::keys::startupAccountEvernoteHostEnvVar.data(),
        account.evernoteHost().toLocal8Bit());
}

Account AccountManager::startupAccount()
{
    QNDEBUG("account::AccountManager", "AccountManager::startupAccount");

    if (qEnvironmentVariableIsEmpty(
            preferences::keys::startupAccountNameEnvVar.data())) {
        QNDEBUG(
            "account::AccountManager",
            "Account name environment variable is not set or is empty");
        return Account{};
    }

    if (qEnvironmentVariableIsEmpty(
            preferences::keys::startupAccountTypeEnvVar.data())) {
        QNDEBUG(
            "account::AccountManager",
            "Account type environment variable is not set or is empty");
        return Account{};
    }

    const QByteArray accountType =
        qgetenv(preferences::keys::startupAccountTypeEnvVar.data());
    const bool isLocal = (accountType == QByteArray{"1"});

    if (!isLocal) {
        if (qEnvironmentVariableIsEmpty(
                preferences::keys::startupAccountIdEnvVar.data())) {
            QNDEBUG(
                "account::AccountManager",
                "Account id environment variable is not set or is empty");
            return Account{};
        }

        if (qEnvironmentVariableIsEmpty(
                preferences::keys::startupAccountEvernoteAccountTypeEnvVar
                    .data()))
        {
            QNDEBUG(
                "account::AccountManager",
                "Evernote account type environment variable is not set or is "
                "empty");
            return Account{};
        }

        if (qEnvironmentVariableIsEmpty(
                preferences::keys::startupAccountEvernoteHostEnvVar.data()))
        {
            QNDEBUG(
                "account::AccountManager",
                "Evernote host environment variable is not set or is empty");
            return Account{};
        }
    }

    const QString accountName = QString::fromLocal8Bit(
        qgetenv(preferences::keys::startupAccountNameEnvVar.data()));

    qevercloud::UserID id = -1;
    QString evernoteHost;
    auto evernoteAccountType = Account::EvernoteAccountType::Free;

    if (!isLocal) {
        bool conversionResult = false;

        id = static_cast<qevercloud::UserID>(qEnvironmentVariableIntValue(
            preferences::keys::startupAccountIdEnvVar.data(),
            &conversionResult));

        if (!conversionResult) {
            QNDEBUG(
                "account::AccountManager",
                "Could not convert the account id to integer");
            return Account{};
        }

        conversionResult = false;

        evernoteAccountType = static_cast<Account::EvernoteAccountType>(
            qEnvironmentVariableIntValue(
                preferences::keys::startupAccountEvernoteAccountTypeEnvVar
                    .data(),
                &conversionResult));

        if (!conversionResult) {
            QNDEBUG(
                "account::AccountManager",
                "Could not convert the Evernote account type to integer");
            return Account{};
        }

        evernoteHost = QString::fromLocal8Bit(qgetenv(
            preferences::keys::startupAccountEvernoteHostEnvVar.data()));
    }

    return findAccount(
        isLocal, accountName, id, evernoteAccountType, evernoteHost);
}

Account AccountManager::defaultAccount(AccountSource * accountSource)
{
    QString username = getCurrentUserName();
    if (Q_UNLIKELY(username.isEmpty())) {
        QNDEBUG(
            "account::AccountManager",
            "Couldn't get the current user's name, fallback "
                << "to \"Default user\"");
        username = QStringLiteral("Default user");
    }

    const auto & availableAccounts = m_accountModel->accounts();
    const auto it = std::find_if(
        availableAccounts.constBegin(), availableAccounts.constEnd(),
        [&username](const Account & account) {
            return account.type() == Account::Type::Local &&
                account.name() == username;
        });
    if (it != availableAccounts.constEnd()) {
        updateLastUsedAccount(*it);

        if (accountSource) {
            *accountSource = AccountSource::ExistingDefault;
        }

        return *it;
    }

    ErrorString errorDescription;
    auto account = createDefaultAccount(errorDescription);
    if (Q_UNLIKELY(account.isEmpty())) {
        ErrorString error(QT_TR_NOOP("Can't initialize the default account"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        throw AccountInitializationException(error);
    }

    updateLastUsedAccount(account);

    if (accountSource) {
        *accountSource = AccountSource::NewDefault;
    }

    return account;
}

Account AccountManager::currentAccount(AccountSource * accountSource)
{
    QNDEBUG("account::AccountManager", "AccountManager::currentAccount");

    auto account = startupAccount();
    if (!account.isEmpty()) {
        if (accountSource) {
            *accountSource = AccountSource::Startup;
        }

        return account;
    }

    account = lastUsedAccount();
    if (!account.isEmpty()) {
        if (accountSource) {
            *accountSource = AccountSource::LastUsed;
        }

        return account;
    }

    return defaultAccount(accountSource);
}

int AccountManager::execAddAccountDialog()
{
    QNDEBUG("account::AccountManager", "AccountManager::execAddAccountDialog");

    auto * parentWidget = qobject_cast<QWidget *>(parent());

    auto addAccountDialog = std::make_unique<AddAccountDialog>(
        m_accountModel->accounts(), parentWidget);

    addAccountDialog->setWindowModality(Qt::WindowModal);

    QObject::connect(
        addAccountDialog.get(),
        &AddAccountDialog::evernoteAccountAdditionRequested, this,
        &AccountManager::evernoteAccountAuthenticationRequested);

    QObject::connect(
        addAccountDialog.get(),
        &AddAccountDialog::localAccountAdditionRequested, this,
        &AccountManager::onLocalAccountAdditionRequested);

    return addAccountDialog->exec();
}

int AccountManager::execManageAccountsDialog()
{
    QNDEBUG(
        "account::AccountManager", "AccountManager::execManageAccountsDialog");

    auto * parentWidget = qobject_cast<QWidget *>(parent());
    const auto & availableAccounts = m_accountModel->accounts();

    const Account account = currentAccount();
    const auto currentAccountRow = availableAccounts.indexOf(account);

    auto manageAccountsDialog = std::make_unique<ManageAccountsDialog>(
        *this, currentAccountRow, parentWidget);

    manageAccountsDialog->setWindowModality(Qt::WindowModal);

    QObject::connect(
        manageAccountsDialog.get(),
        &ManageAccountsDialog::evernoteAccountAdditionRequested, this,
        &AccountManager::evernoteAccountAuthenticationRequested);

    QObject::connect(
        manageAccountsDialog.get(),
        &ManageAccountsDialog::localAccountAdditionRequested, this,
        &AccountManager::onLocalAccountAdditionRequested);

    QObject::connect(
        manageAccountsDialog.get(),
        &ManageAccountsDialog::revokeAuthentication, this,
        &AccountManager::revokeAuthenticationRequested);

    QObject::connect(
        this, &AccountManager::authenticationRevoked,
        manageAccountsDialog.get(),
        &ManageAccountsDialog::onAuthenticationRevoked);

    return manageAccountsDialog->exec();
}

Account AccountManager::createNewLocalAccount(QString name)
{
    if (!name.isEmpty()) {
        const auto & availableAccounts = m_accountModel->accounts();
        for (const auto & currentAccount: std::as_const(availableAccounts)) {
            if (currentAccount.type() != Account::Type::Local) {
                continue;
            }

            if (name.compare(currentAccount.name(), Qt::CaseInsensitive) == 0) {
                return Account{};
            }
        }
    }
    else {
        const QString baseName = QStringLiteral("Local account");
        int suffix = 0;
        name = baseName;
        const auto & availableAccounts = m_accountModel->accounts();
        while (true) {
            bool nameClashDetected = false;
            for (const auto & currentAccount: std::as_const(availableAccounts))
            {
                if (currentAccount.type() != Account::Type::Local) {
                    continue;
                }

                if (!name.compare(currentAccount.name(), Qt::CaseInsensitive)) {
                    nameClashDetected = true;
                    ++suffix;
                    name = baseName + QStringLiteral("_") +
                        QString::number(suffix);
                    break;
                }
            }

            if (!nameClashDetected) {
                break;
            }
        }
    }

    ErrorString errorDescription;
    Account account = createLocalAccount(name, name, errorDescription);
    if (Q_UNLIKELY(account.isEmpty())) {
        ErrorString error{QT_TR_NOOP("Can't create a new local account")};
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING("account::AccountManager", error);
        Q_EMIT notifyError(std::move(error));
        return Account();
    }

    return account;
}

void AccountManager::switchAccount(const Account & account)
{
    // print the entire account because here not only the name but also the type
    // of the account we are switching to matters a lot
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::switchAccount: " << account);

    // See whether this account is within a list of already available accounts,
    // if not, add it there
    bool accountIsAvailable = false;
    const auto type = account.type();
    const bool isLocal = (account.type() == Account::Type::Local);
    const auto & availableAccounts = m_accountModel->accounts();
    for (const auto & availableAccount: std::as_const(availableAccounts)) {
        if (availableAccount.type() != type) {
            continue;
        }

        if (!isLocal && (availableAccount.id() != account.id())) {
            continue;
        }
        else if (isLocal && (availableAccount.name() != account.name())) {
            continue;
        }

        accountIsAvailable = true;
        break;
    }

    Account complementedAccount = account;
    if (!accountIsAvailable) {
        if (!createAccountInfo(account)) {
            return;
        }

        if (m_accountModel->addAccount(account)) {
            Q_EMIT accountAdded(account);
        }
    }
    else {
        readComplementaryAccountInfo(complementedAccount);
    }

    updateLastUsedAccount(complementedAccount);
    Q_EMIT switchedAccount(complementedAccount);
}

void AccountManager::onAuthenticationRevoked(
    const bool success, ErrorString errorDescription,
    const qevercloud::UserID userId)
{
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::onAuthenticationRevoked: success = "
            << (success ? "true" : "false") << ", error description = "
            << errorDescription << ", user id = " << userId);

    Q_EMIT authenticationRevoked(success, std::move(errorDescription), userId);
}

void AccountManager::onLocalAccountAdditionRequested(
    const QString & name, const QString & fullName)
{
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::onLocalAccountAdditionRequested: name = "
            << name << ", full name = " << fullName);

    // Double-check that no local account with such name already exists
    const auto & availableAccounts = m_accountModel->accounts();
    for (const auto & availableAccount: std::as_const(availableAccounts)) {
        if (availableAccount.type() != Account::Type::Local) {
            continue;
        }

        if (Q_UNLIKELY(availableAccount.name() == name)) {
            ErrorString error{
                QT_TR_NOOP("Can't add a local account: another account with "
                           "the same name already exists")};
            QNWARNING("account::AccountManager", error);
            Q_EMIT notifyError(std::move(error));
            return;
        }
    }

    ErrorString errorDescription;
    const auto account = createLocalAccount(name, fullName, errorDescription);
    if (Q_UNLIKELY(account.isEmpty())) {
        ErrorString error{QT_TR_NOOP("Can't create a new local account")};
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING("account::AccountManager", error);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    switchAccount(account);
}

void AccountManager::onAccountDisplayNameChanged(Account account)
{
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::onAccountDisplayNameChanged: " << account.name());

    const bool isLocal = (account.type() == Account::Type::Local);
    ErrorString errorDescription;

    const QString accountType =
        evernoteAccountTypeToString(account.evernoteAccountType());

    Q_UNUSED(writeAccountInfo(
        account.name(), account.displayName(), isLocal, account.id(),
        accountType, account.evernoteHost(), account.shardId(),
        errorDescription))

    Q_EMIT accountUpdated(account);
}

void AccountManager::detectAvailableAccounts()
{
    QNDEBUG(
        "account::AccountManager", "AccountManager::detectAvailableAccounts");

    const QString appPersistenceStoragePath = applicationPersistentStoragePath();

    const QString localAccountsStoragePath =
        appPersistenceStoragePath + QStringLiteral("/LocalAccounts");

    const QDir localAccountsStorageDir{localAccountsStoragePath};

    const auto localAccountsDirInfos = localAccountsStorageDir.entryInfoList(
        QDir::Filters(QDir::AllDirs | QDir::NoDotAndDotDot));

    const QString evernoteAccountsStoragePath =
        appPersistenceStoragePath + QStringLiteral("/EvernoteAccounts");

    const QDir evernoteAccountsStorageDir{evernoteAccountsStoragePath};
    const auto evernoteAccountsDirInfos =
        evernoteAccountsStorageDir.entryInfoList(
            QDir::Filters(QDir::AllDirs | QDir::NoDotAndDotDot));

    const auto numPotentialLocalAccountDirs = localAccountsDirInfos.size();
    const auto numPotentialEvernoteAccountDirs =
        evernoteAccountsDirInfos.size();

    const auto numPotentialAccountDirs =
        numPotentialLocalAccountDirs + numPotentialEvernoteAccountDirs;

    QList<Account> availableAccounts;
    availableAccounts.reserve(numPotentialAccountDirs);

    for (const auto & accountDirInfo: std::as_const(localAccountsDirInfos)) {
        QNTRACE(
            "account::AccountManager",
            "Checking potential local account dir: "
                << accountDirInfo.absoluteFilePath());

        const QFileInfo accountBasicInfoFileInfo{
            accountDirInfo.absoluteFilePath() +
            QStringLiteral("/accountInfo.txt")};

        if (!accountBasicInfoFileInfo.exists()) {
            QNTRACE(
                "account::AccountManager",
                "Found no accountInfo.txt file in this dir, skipping it");
            continue;
        }

        const QString accountName = accountDirInfo.fileName();
        qevercloud::UserID userId = -1;

        Account availableAccount{accountName, Account::Type::Local, userId};
        readComplementaryAccountInfo(availableAccount);
        availableAccounts << availableAccount;

        QNDEBUG(
            "account::AccountManager",
            "Found available local account: name = "
                << accountName << ", dir "
                << accountDirInfo.absoluteFilePath());
    }

    for (const auto & accountDirInfo: std::as_const(evernoteAccountsDirInfos)) {
        QNTRACE(
            "account::AccountManager",
            "Checking potential Evernote account dir: "
                << accountDirInfo.absoluteFilePath());

        const QFileInfo accountBasicInfoFileInfo{
            accountDirInfo.absoluteFilePath() +
            QStringLiteral("/accountInfo.txt")};

        if (!accountBasicInfoFileInfo.exists()) {
            QNTRACE(
                "account::AccountManager",
                "Found no accountInfo.txt file in this dir, skipping it");
            continue;
        }

        QString accountName = accountDirInfo.fileName();
        qevercloud::UserID userId = -1;

        // The account dir for Evernote accounts is encoded
        // as "<account_name>_<host>_<user_id>"
        const auto accountNameSize = accountName.size();
        const auto lastUnderlineIndex =
            accountName.lastIndexOf(QStringLiteral("_"));
        if (lastUnderlineIndex < 0 || lastUnderlineIndex >= accountNameSize)
        {
            QNTRACE(
                "account::AccountManager",
                "Dir " << accountName
                       << " doesn't seem to be an account dir: it "
                       << "doesn't start with \"local_\" and "
                       << "doesn't contain \"_<user id>\" at the end");
            continue;
        }

        const QString userIdStr =
            accountName.right(accountNameSize - lastUnderlineIndex - 1);

        bool conversionResult = false;
        userId = static_cast<qevercloud::UserID>(
            userIdStr.toInt(&conversionResult));

        if (Q_UNLIKELY(!conversionResult)) {
            QNTRACE(
                "account::AccountManager",
                "Skipping dir " << accountName
                                << " as it doesn't seem to end with user id, "
                                << "the attempt to convert it to int fails");
            continue;
        }

        const auto preLastUnderlineIndex = accountName.lastIndexOf(
            QStringLiteral("_"),
            std::max<decltype(lastUnderlineIndex)>(lastUnderlineIndex - 1, 1));

        if (preLastUnderlineIndex < 0 ||
            preLastUnderlineIndex >= lastUnderlineIndex)
        {
            QNTRACE(
                "account::AccountManager",
                "Dir " << accountName
                       << " doesn't seem to be an account dir: it "
                       << "doesn't start with \"local_\" and doesn't "
                       << "contain \"_<host>_<user id>\" at the end");
            continue;
        }

        const QString evernoteHost = accountName.mid(
            preLastUnderlineIndex + 1,
            lastUnderlineIndex - preLastUnderlineIndex - 1);

        accountName.remove(
            preLastUnderlineIndex, accountNameSize - preLastUnderlineIndex);

        Account availableAccount{
            accountName, Account::Type::Evernote, userId,
            Account::EvernoteAccountType::Free, evernoteHost};

        readComplementaryAccountInfo(availableAccount);
        availableAccounts << availableAccount;

        QNDEBUG(
            "account::AccountManager",
            "Found available Evernote account: name = "
                << accountName << ", user id = " << userId
                << ", Evernote account type = "
                << availableAccount.evernoteAccountType()
                << ", Evernote host = " << availableAccount.evernoteHost()
                << ", dir " << accountDirInfo.absoluteFilePath());
    }

    m_accountModel->setAccounts(availableAccounts);
}

Account AccountManager::createDefaultAccount(ErrorString & errorDescription)
{
    QNDEBUG("account::AccountManager", "AccountManager::createDefaultAccount");

    QString username = getCurrentUserName();
    if (Q_UNLIKELY(username.isEmpty())) {
        QNDEBUG(
            "account::AccountManager",
            "Couldn't get the current user's name, fallback to \"Default "
            "user\"");
        username = QStringLiteral("Default user");
    }

    QString fullName = getCurrentUserFullName();
    if (Q_UNLIKELY(fullName.isEmpty())) {
        QNDEBUG(
            "account::AccountManager",
            "Couldn't get the current user's full name, fallback to \"Defaulr "
            "user\"");
        fullName = QStringLiteral("Default user");
    }

    // Need to check whether the default account already exists
    Account account{username, Account::Type::Local, qevercloud::UserID(-1)};
    account.setDisplayName(fullName);

    const auto & availableAccounts = m_accountModel->accounts();
    if (availableAccounts.contains(account)) {
        QNDEBUG(
            "account::AccountManager", "The default account already exists");
        return account;
    }

    return createLocalAccount(username, fullName, errorDescription);
}

Account AccountManager::createLocalAccount(
    const QString & name, const QString & displayName,
    ErrorString & errorDescription)
{
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::createLocalAccount: name = "
            << name << ", display name = " << displayName);

    const bool res = writeAccountInfo(
        name, displayName,
        /* is local = */ true,
        /* user id = */ -1,
        /* Evernote account type = */ QString(),
        /* Evernote host = */ QString(),
        /* shard id = */ QString(), errorDescription);

    if (Q_UNLIKELY(!res)) {
        return Account();
    }

    Account availableAccount{
        name, Account::Type::Local, qevercloud::UserID(-1)};
    availableAccount.setDisplayName(displayName);
    m_accountModel->addAccount(availableAccount);

    return Account{name, Account::Type::Local};
}

bool AccountManager::createAccountInfo(const Account & account)
{
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::createAccountInfo: " << account.name());

    const bool isLocal = (account.type() == Account::Type::Local);

    const QString evernoteAccountType =
        evernoteAccountTypeToString(account.evernoteAccountType());

    ErrorString errorDescription;

    const bool res = writeAccountInfo(
        account.name(), account.displayName(), isLocal, account.id(),
        evernoteAccountType, account.evernoteHost(), account.shardId(),
        errorDescription);

    if (Q_UNLIKELY(!res)) {
        Q_EMIT notifyError(errorDescription);
        return false;
    }

    return true;
}

bool AccountManager::writeAccountInfo(
    const QString & name, const QString & displayName, const bool isLocal,
    const qevercloud::UserID id, const QString & evernoteAccountType,
    const QString & evernoteHost, const QString & shardId,
    ErrorString & errorDescription)
{
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::writeAccountInfo: name = "
            << name << ", display name = " << displayName << ", is local = "
            << (isLocal ? "true" : "false") << ", user id = " << id
            << ", Evernote account type = " << evernoteAccountType
            << ", Evernote host = " << evernoteHost
            << ", shard id = " << shardId);

    Account account{
        name, (isLocal ? Account::Type::Local : Account::Type::Evernote), id,
        Account::EvernoteAccountType::Free, evernoteHost, shardId};

    const QDir accountPersistentStorageDir{
        accountPersistentStoragePath(account)};

    if (!accountPersistentStorageDir.exists()) {
        const bool res = accountPersistentStorageDir.mkpath(
            accountPersistentStorageDir.absolutePath());

        if (Q_UNLIKELY(!res)) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't create a directory for the account storage"));

            errorDescription.details() =
                accountPersistentStorageDir.absolutePath();

            QNWARNING("account::AccountManager", errorDescription);
            return false;
        }
    }

    QFile accountInfo{
        accountPersistentStorageDir.absolutePath() +
        QStringLiteral("/accountInfo.txt")};

    const bool open = accountInfo.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open)) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't open the new account info file for writing"));
        errorDescription.details() = accountInfo.fileName();

        const QString errorString = accountInfo.errorString();
        if (!errorString.isEmpty()) {
            errorDescription.details() += QStringLiteral(": ");
            errorDescription.details() += errorString;
        }

        QNWARNING("account::AccountManager", errorDescription);
        return false;
    }

    QXmlStreamWriter writer{&accountInfo};
    writer.writeStartDocument();
    writer.writeStartElement(QStringLiteral("data"));

    if (!name.isEmpty()) {
        writer.writeStartElement(QStringLiteral("accountName"));
        writer.writeCharacters(name);
        writer.writeEndElement();
    }

    if (!displayName.isEmpty()) {
        writer.writeStartElement(QStringLiteral("displayName"));
        writer.writeCDATA(displayName);
        writer.writeEndElement();
    }

    writer.writeStartElement(QStringLiteral("accountType"));

    writer.writeCharacters(
        isLocal ? QStringLiteral("Local") : QStringLiteral("Evernote"));

    writer.writeEndElement();

    if (!evernoteAccountType.isEmpty()) {
        writer.writeStartElement(QStringLiteral("evernoteAccountType"));
        writer.writeCharacters(evernoteAccountType);
        writer.writeEndElement();
    }

    if (!evernoteHost.isEmpty()) {
        writer.writeStartElement(QStringLiteral("evernoteHost"));
        writer.writeCharacters(evernoteHost);
        writer.writeEndElement();
    }

    if (!shardId.isEmpty()) {
        writer.writeStartElement(QStringLiteral("shardId"));
        writer.writeCharacters(shardId);
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();

    accountInfo.close();
    return true;
}

QString AccountManager::evernoteAccountTypeToString(
    const Account::EvernoteAccountType type) const
{
    QString evernoteAccountType;
    switch (type) {
    case Account::EvernoteAccountType::Plus:
        evernoteAccountType = QStringLiteral("Plus");
        break;
    case Account::EvernoteAccountType::Premium:
        evernoteAccountType = QStringLiteral("Premium");
        break;
    case Account::EvernoteAccountType::Business:
        evernoteAccountType = QStringLiteral("Business");
        break;
    default:
        evernoteAccountType = QStringLiteral("Free");
        break;
    }

    return evernoteAccountType;
}

void AccountManager::readComplementaryAccountInfo(Account & account)
{
    QNTRACE(
        "account::AccountManager",
        "AccountManager::readComplementaryAccountInfo: " << account.name());

    if (Q_UNLIKELY(account.isEmpty())) {
        QNDEBUG(
            "account::AccountManager",
            "The account is empty: " << account.name());
        return;
    }

    if (Q_UNLIKELY(account.name().isEmpty())) {
        QNDEBUG(
            "account::AccountManager",
            "The account name is empty: " << account.name());
        return;
    }

    const QDir accountPersistentStorageDir{
        accountPersistentStoragePath(account)};

    if (!accountPersistentStorageDir.exists()) {
        QNDEBUG(
            "account::AccountManager",
            "No persistent storage dir exists for this account: "
                << account.name());
        return;
    }

    QFile accountInfo{
        accountPersistentStorageDir.absolutePath() +
        QStringLiteral("/accountInfo.txt")};

    const bool open = accountInfo.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't read the complementary account info: can't open "
                       "file for reading")};

        errorDescription.details() = accountInfo.fileName();

        const QString errorString = accountInfo.errorString();
        if (!errorString.isEmpty()) {
            errorDescription.details() += QStringLiteral(": ");
            errorDescription.details() += errorString;
        }

        QNWARNING("account::AccountManager", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QXmlStreamReader reader{&accountInfo};

    QString currentElementName;
    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartDocument()) {
            continue;
        }

        if (reader.isDTD()) {
            continue;
        }

        if (reader.isEndDocument()) {
            continue;
        }

        if (reader.isStartElement()) {
            currentElementName = reader.name().toString();
            continue;
        }

        if (reader.isCharacters()) {
            if (currentElementName.isEmpty()) {
                continue;
            }

            if (currentElementName == QStringLiteral("evernoteAccountType")) {
                QString evernoteAccountType = reader.text().toString();
                if (evernoteAccountType.isEmpty() ||
                    evernoteAccountType == QStringLiteral("Free"))
                {
                    account.setEvernoteAccountType(
                        Account::EvernoteAccountType::Free);
                }
                else if (evernoteAccountType == QStringLiteral("Plus")) {
                    account.setEvernoteAccountType(
                        Account::EvernoteAccountType::Plus);
                }
                else if (evernoteAccountType == QStringLiteral("Premium")) {
                    account.setEvernoteAccountType(
                        Account::EvernoteAccountType::Premium);
                }
                else if (evernoteAccountType == QStringLiteral("Business")) {
                    account.setEvernoteAccountType(
                        Account::EvernoteAccountType::Business);
                }
            }
            else if (currentElementName == QStringLiteral("evernoteHost")) {
                account.setEvernoteHost(reader.text().toString());
            }
            else if (currentElementName == QStringLiteral("shardId")) {
                account.setShardId(reader.text().toString());
            }
        }

        if (reader.isCDATA() &&
            (currentElementName == QStringLiteral("displayName"))) {
            account.setDisplayName(reader.text().toString());
        }
    }

    if (reader.hasError()) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't read the entire complementary account info, "
                       "error reading XML")};

        errorDescription.details() = reader.errorString();
        QNWARNING("account::AccountManager", errorDescription);
        Q_EMIT notifyError(std::move(errorDescription));
    }

    accountInfo.close();

    QNTRACE(
        "account::AccountManager",
        "Account after reading in the complementary info: " << account);
}

Account AccountManager::lastUsedAccount()
{
    QNDEBUG("account::AccountManager", "AccountManager::lastUsedAccount");

    ApplicationSettings appSettings;

    appSettings.beginGroup(preferences::keys::accountGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};

    const QVariant name =
        appSettings.value(preferences::keys::lastUsedAccountName);
    if (name.isNull()) {
        QNDEBUG(
            "account::AccountManager", "Can't find last used account's name");
        return Account{};
    }

    const QString accountName = name.toString();
    if (accountName.isEmpty()) {
        QNDEBUG("account::AccountManager", "Last used account's name is empty");
        return Account{};
    }

    const QVariant type =
        appSettings.value(preferences::keys::lastUsedAccountType);
    if (type.isNull()) {
        QNDEBUG(
            "account::AccountManager", "Can't find last used account's type");
        return Account{};
    }

    const bool isLocal = type.toBool();

    qevercloud::UserID id = -1;
    Account::EvernoteAccountType evernoteAccountType =
        Account::EvernoteAccountType::Free;

    QString evernoteHost;
    if (!isLocal) {
        QVariant userId =
            appSettings.value(preferences::keys::lastUsedAccountId);
        if (userId.isNull()) {
            QNDEBUG(
                "account::AccountManager", "Can't find last used account's id");
            return Account{};
        }

        bool conversionResult = false;
        id = userId.toInt(&conversionResult);
        if (!conversionResult) {
            QNDEBUG(
                "account::AccountManager",
                "Can't convert the last used account's id to int");
            return Account{};
        }

        const QVariant userEvernoteAccountHost =
            appSettings.value(preferences::keys::lastUsedAccountEvernoteHost);
        if (userEvernoteAccountHost.isNull()) {
            QNDEBUG(
                "account::AccountManager",
                "Can't find last used account's Evernote host");
            return Account{};
        }

        evernoteHost = userEvernoteAccountHost.toString();

        QVariant userEvernoteAccountType = appSettings.value(
            preferences::keys::lastUsedAccountEvernoteAccountType);

        if (userEvernoteAccountType.isNull()) {
            QNDEBUG(
                "account::AccountManager",
                "Can't find last used account's Evernote account type");
            return Account{};
        }

        conversionResult = false;

        evernoteAccountType = static_cast<Account::EvernoteAccountType>(
            userEvernoteAccountType.toInt(&conversionResult));

        if (!conversionResult) {
            QNDEBUG(
                "account::AccountManager",
                "Can't convert the last used account's Evernote account type "
                "to int");
            return Account{};
        }
    }

    return findAccount(
        isLocal, accountName, id, evernoteAccountType, evernoteHost);
}

Account AccountManager::findAccount(
    const bool isLocal, const QString & accountName,
    const qevercloud::UserID id,
    const Account::EvernoteAccountType evernoteAccountType,
    const QString & evernoteHost)
{
    QNDEBUG("account::AccountManager", "AccountManager::findAccount");

    const QString appPersistenceStoragePath =
        applicationPersistentStoragePath();

    QString accountDirName =
        (isLocal ? (QStringLiteral("LocalAccounts/") + accountName)
                 : (QStringLiteral("EvernoteAccounts/") + accountName +
                    QStringLiteral("_") + evernoteHost + QStringLiteral("_") +
                    QString::number(id)));

    const QFileInfo accountFileInfo{
        appPersistenceStoragePath + QStringLiteral("/") + accountDirName +
        QStringLiteral("/accountInfo.txt")};

    if (!accountFileInfo.exists()) {
        return Account{};
    }

    Account account{
        accountName, (isLocal ? Account::Type::Local : Account::Type::Evernote),
        id, evernoteAccountType, evernoteHost};

    readComplementaryAccountInfo(account);
    return account;
}

void AccountManager::updateLastUsedAccount(const Account & account)
{
    QNDEBUG(
        "account::AccountManager",
        "AccountManager::updateLastUsedAccount: " << account.name());

    ApplicationSettings appSettings;

    appSettings.beginGroup(preferences::keys::accountGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};

    appSettings.setValue(
        preferences::keys::lastUsedAccountName, account.name());

    appSettings.setValue(
        preferences::keys::lastUsedAccountType,
        (account.type() == Account::Type::Local));

    appSettings.setValue(preferences::keys::lastUsedAccountId, account.id());

    appSettings.setValue(
        preferences::keys::lastUsedAccountEvernoteAccountType,
        static_cast<qint64>(account.evernoteAccountType()));

    appSettings.setValue(
        preferences::keys::lastUsedAccountEvernoteHost, account.evernoteHost());
}

AccountManager::AccountInitializationException::AccountInitializationException(
    const ErrorString & message) :
    IQuentierException(message)
{}

QString
AccountManager::AccountInitializationException::exceptionDisplayName() const
{
    return QStringLiteral("AccountInitializationException");
}

////////////////////////////////////////////////////////////////////////////////

QDebug & operator<<(QDebug & dbg, const AccountManager::AccountSource source)
{
    using AccountSource = AccountManager::AccountSource;

    switch (source) {
    case AccountSource::Startup:
        dbg << "startup";
        break;
    case AccountSource::LastUsed:
        dbg << "last used";
        break;
    case AccountSource::NewDefault:
        dbg << "new default";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(source) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
