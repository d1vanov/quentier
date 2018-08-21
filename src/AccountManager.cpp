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

#include "AccountManager.h"
#include "SettingsNames.h"
#include "dialogs/AddAccountDialog.h"
#include "dialogs/ManageAccountsDialog.h"
#include "models/AccountsModel.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/Utility.h>
#include <quentier/logging/QuentierLogger.h>
#include <QXmlStreamWriter>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/scope_exit.hpp>
#endif

namespace quentier {

AccountManager::AccountManager(QObject * parent) :
    QObject(parent),
    m_pAccountsModel(new AccountsModel(this))
{
    QObject::connect(m_pAccountsModel.data(), QNSIGNAL(AccountsModel,accountAdded,Account),
                     this, QNSIGNAL(AccountManager,accountAdded,Account));
    QObject::connect(m_pAccountsModel.data(), QNSIGNAL(AccountsModel,accountDisplayNameChanged,Account),
                     this, QNSLOT(AccountManager,onAccountDisplayNameChanged,Account));

    detectAvailableAccounts();
}

AccountManager::~AccountManager()
{}

const QVector<Account> & AccountManager::availableAccounts() const
{
    return m_pAccountsModel->accounts();
}

AccountsModel & AccountManager::accountsModel()
{
    return *m_pAccountsModel;
}

Account AccountManager::currentAccount(bool * pCreatedDefaultAccount)
{
    QNDEBUG(QStringLiteral("AccountManager::currentAccount"));

    if (pCreatedDefaultAccount) {
        *pCreatedDefaultAccount = false;
    }

    QSharedPointer<Account> pManuallySpecifiedAccount = accountFromEnvVarHints();
    if (!pManuallySpecifiedAccount.isNull()) {
        return *pManuallySpecifiedAccount;
    }

    QSharedPointer<Account> pLastUsedAccount = lastUsedAccount();
    if (pLastUsedAccount.isNull())
    {
        ErrorString errorDescription;
        pLastUsedAccount = createDefaultAccount(errorDescription, pCreatedDefaultAccount);
        if (Q_UNLIKELY(pLastUsedAccount.isNull())) {
            ErrorString error(QT_TR_NOOP("Can't initialize the default account"));
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            throw AccountInitializationException(error);
        }

        updateLastUsedAccount(*pLastUsedAccount);
    }

    return *pLastUsedAccount;
}

void AccountManager::raiseAddAccountDialog()
{
    QNDEBUG(QStringLiteral("AccountManager::raiseAddAccountDialog"));

    QWidget * parentWidget = qobject_cast<QWidget*>(parent());

    QScopedPointer<AddAccountDialog> addAccountDialog(new AddAccountDialog(m_pAccountsModel->accounts(), parentWidget));
    addAccountDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(addAccountDialog.data(), QNSIGNAL(AddAccountDialog,evernoteAccountAdditionRequested,QString,QNetworkProxy),
                     this, QNSIGNAL(AccountManager,evernoteAccountAuthenticationRequested,QString,QNetworkProxy));
    QObject::connect(addAccountDialog.data(), QNSIGNAL(AddAccountDialog,localAccountAdditionRequested,QString,QString),
                     this, QNSLOT(AccountManager,onLocalAccountAdditionRequested,QString,QString));
    Q_UNUSED(addAccountDialog->exec())
}

void AccountManager::raiseManageAccountsDialog()
{
    QNDEBUG(QStringLiteral("AccountManager::raiseManageAccountsDialog"));

    QWidget * parentWidget = qobject_cast<QWidget*>(parent());
    const QVector<Account> & availableAccounts = m_pAccountsModel->accounts();

    Account account = currentAccount();
    int currentAccountRow = availableAccounts.indexOf(account);

    QScopedPointer<ManageAccountsDialog> manageAccountsDialog(new ManageAccountsDialog(*m_pAccountsModel, currentAccountRow, parentWidget));
    manageAccountsDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(manageAccountsDialog.data(), QNSIGNAL(ManageAccountsDialog,evernoteAccountAdditionRequested,QString,QNetworkProxy),
                     this, QNSIGNAL(AccountManager,evernoteAccountAuthenticationRequested,QString,QNetworkProxy));
    QObject::connect(manageAccountsDialog.data(), QNSIGNAL(ManageAccountsDialog,localAccountAdditionRequested,QString,QString),
                     this, QNSLOT(AccountManager,onLocalAccountAdditionRequested,QString,QString));
    Q_UNUSED(manageAccountsDialog->exec())
}

void AccountManager::switchAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("AccountManager::switchAccount: ") << account);

    // See whether this account is within a list of already available accounts, if not, add it there
    bool accountIsAvailable = false;
    Account::Type::type type = account.type();
    bool isLocal = (account.type() == Account::Type::Local);
    const QVector<Account> & availableAccounts = m_pAccountsModel->accounts();
    for(auto it = availableAccounts.constBegin(), end = availableAccounts.constEnd(); it != end; ++it)
    {
        const Account & availableAccount = *it;

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
    if (!accountIsAvailable)
    {
        bool res = createAccountInfo(account);
        if (!res) {
            return;
        }

        if (m_pAccountsModel->addAccount(account)) {
            Q_EMIT accountAdded(account);
        }
    }
    else
    {
        readComplementaryAccountInfo(complementedAccount);
    }

    updateLastUsedAccount(complementedAccount);
    Q_EMIT switchedAccount(complementedAccount);
}

void AccountManager::onLocalAccountAdditionRequested(QString name, QString fullName)
{
    QNDEBUG(QStringLiteral("AccountManager::onLocalAccountAdditionRequested: name = ")
            << name << QStringLiteral(", full name = ") << fullName);

    // Double-check that no local account with such name already exists
    const QVector<Account> & availableAccounts = m_pAccountsModel->accounts();
    for(auto it = availableAccounts.constBegin(), end = availableAccounts.constEnd(); it != end; ++it)
    {
        const Account & availableAccount = *it;
        if (availableAccount.type() != Account::Type::Local) {
            continue;
        }

        if (Q_UNLIKELY(availableAccount.name() == name)) {
            ErrorString error(QT_TR_NOOP("Can't add a local account: another account with the same name already exists"));
            QNWARNING(error);
            Q_EMIT notifyError(error);
            return;
        }
    }

    ErrorString errorDescription;
    QSharedPointer<Account> pNewAccount = createLocalAccount(name, fullName, errorDescription);
    if (Q_UNLIKELY(pNewAccount.isNull())) {
        ErrorString error(QT_TR_NOOP("Can't create a new local account"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    switchAccount(*pNewAccount);
}

void AccountManager::onAccountDisplayNameChanged(Account account)
{
    QNDEBUG(QStringLiteral("AccountManager::onAccountDisplayNameChanged: ") << account);

    bool isLocal = (account.type() ==  Account::Type::Local);
    ErrorString errorDescription;
    Q_UNUSED(writeAccountInfo(account.name(), account.displayName(), isLocal, account.id(),
                              evernoteAccountTypeToString(account.evernoteAccountType()),
                              account.evernoteHost(), account.shardId(), errorDescription))

    Q_EMIT accountUpdated(account);
}

void AccountManager::detectAvailableAccounts()
{
    QNDEBUG(QStringLiteral("AccountManager::detectAvailableAccounts"));

    QString appPersistenceStoragePath = applicationPersistentStoragePath();

    QString localAccountsStoragePath = appPersistenceStoragePath + QStringLiteral("/LocalAccounts");
    QDir localAccountsStorageDir(localAccountsStoragePath);
    QFileInfoList localAccountsDirInfos = localAccountsStorageDir.entryInfoList(QDir::Filters(QDir::AllDirs | QDir::NoDotAndDotDot));

    QString evernoteAccountsStoragePath = appPersistenceStoragePath + QStringLiteral("/EvernoteAccounts");
    QDir evernoteAccountsStorageDir(evernoteAccountsStoragePath);
    QFileInfoList evernoteAccountsDirInfos = evernoteAccountsStorageDir.entryInfoList(QDir::Filters(QDir::AllDirs | QDir::NoDotAndDotDot));

    int numPotentialLocalAccountDirs = localAccountsDirInfos.size();
    int numPotentialEvernoteAccountDirs = evernoteAccountsDirInfos.size();
    int numPotentialAccountDirs = numPotentialLocalAccountDirs + numPotentialEvernoteAccountDirs;

    QVector<Account> availableAccounts;
    availableAccounts.reserve(numPotentialAccountDirs);

    for(int i = 0; i < numPotentialLocalAccountDirs; ++i)
    {
        const QFileInfo & accountDirInfo = localAccountsDirInfos[i];
        QNTRACE(QStringLiteral("Examining potential local account dir: ") << accountDirInfo.absoluteFilePath());

        QFileInfo accountBasicInfoFileInfo(accountDirInfo.absoluteFilePath() + QStringLiteral("/accountInfo.txt"));
        if (!accountBasicInfoFileInfo.exists()) {
            QNTRACE(QStringLiteral("Found no accountInfo.txt file in this dir, skipping it"));
            continue;
        }

        QString accountName = accountDirInfo.baseName();
        qevercloud::UserID userId = -1;

        Account availableAccount(accountName, Account::Type::Local, userId);
        readComplementaryAccountInfo(availableAccount);
        availableAccounts << availableAccount;
        QNDEBUG(QStringLiteral("Found available local account: name = ") << accountName
                << QStringLiteral(", dir ") << accountDirInfo.absoluteFilePath());
    }

    for(int i = 0; i < numPotentialEvernoteAccountDirs; ++i)
    {
        const QFileInfo & accountDirInfo = evernoteAccountsDirInfos[i];
        QNTRACE(QStringLiteral("Examining potential Evernote account dir: ") << accountDirInfo.absoluteFilePath());

        QFileInfo accountBasicInfoFileInfo(accountDirInfo.absoluteFilePath() + QStringLiteral("/accountInfo.txt"));
        if (!accountBasicInfoFileInfo.exists()) {
            QNTRACE(QStringLiteral("Found no accountInfo.txt file in this dir, skipping it"));
            continue;
        }

        QString accountName = accountDirInfo.fileName();
        qevercloud::UserID userId = -1;

        // The account dir for Evernote accounts is encoded as "<account_name>_<host>_<user_id>"
        int accountNameSize = accountName.size();
        int lastUnderlineIndex = accountName.lastIndexOf(QStringLiteral("_"));
        if ((lastUnderlineIndex < 0) || (lastUnderlineIndex >= accountNameSize)) {
            QNTRACE(QStringLiteral("Dir ") << accountName
                    << QStringLiteral(" doesn't seem to be an account dir: it doesn't start with \"local_\" and "
                                      "doesn't contain \"_<user id>\" at the end"));
            continue;
        }

        QStringRef userIdStrRef = accountName.rightRef(accountNameSize - lastUnderlineIndex - 1);
        bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        userId = static_cast<qevercloud::UserID>(userIdStrRef.toInt(&conversionResult));
#else
        userId = static_cast<qevercloud::UserID>(userIdStrRef.toString().toInt(&conversionResult));
#endif
        if (Q_UNLIKELY(!conversionResult)) {
            QNTRACE(QStringLiteral("Skipping dir ") << accountName
                    << QStringLiteral(" as it doesn't seem to end with user id, the attempt to convert it to int fails"));
            continue;
        }

        int preLastUnderlineIndex = accountName.lastIndexOf(QStringLiteral("_"), std::max(lastUnderlineIndex - 1, 1));
        if ((preLastUnderlineIndex < 0) || (preLastUnderlineIndex >= lastUnderlineIndex)) {
            QNTRACE(QStringLiteral("Dir ") << accountName
                    << QStringLiteral(" doesn't seem to be an account dir: it doesn't start with \"local_\" and "
                                      "doesn't contain \"_<host>_<user id>\" at the end"));
            continue;
        }

        QString evernoteHost = accountName.mid(preLastUnderlineIndex + 1, lastUnderlineIndex - preLastUnderlineIndex - 1);
        accountName.remove(preLastUnderlineIndex, accountNameSize - preLastUnderlineIndex);

        Account availableAccount(accountName, Account::Type::Evernote, userId, Account::EvernoteAccountType::Free, evernoteHost);
        readComplementaryAccountInfo(availableAccount);
        availableAccounts << availableAccount;
        QNDEBUG(QStringLiteral("Found available Evernote account: name = ") << accountName
                << QStringLiteral(", user id = ") << userId << QStringLiteral(", Evernote account type = ")
                << availableAccount.evernoteAccountType() << QStringLiteral(", Evernote host = ")
                << availableAccount.evernoteHost() << QStringLiteral(", dir ") << accountDirInfo.absoluteFilePath());
    }

    m_pAccountsModel->setAccounts(availableAccounts);
}

QSharedPointer<Account> AccountManager::createDefaultAccount(ErrorString & errorDescription, bool * pCreatedDefaultAccount)
{
    QNDEBUG(QStringLiteral("AccountManager::createDefaultAccount"));

    if (pCreatedDefaultAccount) {
        *pCreatedDefaultAccount = false;
    }

    QString username = getCurrentUserName();
    if (Q_UNLIKELY(username.isEmpty())) {
        QNDEBUG(QStringLiteral("Couldn't get the current user's name, fallback to \"Default user\""));
        username = QStringLiteral("Default user");
    }

    QString fullName = getCurrentUserFullName();
    if (Q_UNLIKELY(fullName.isEmpty())) {
        QNDEBUG(QStringLiteral("Couldn't get the current user's full name, fallback to \"Defaulr user\""));
        fullName = QStringLiteral("Default user");
    }

    // Need to check whether the default account already exists
    QSharedPointer<Account> pDefaultAccount(new Account(username, Account::Type::Local, qevercloud::UserID(-1)));
    pDefaultAccount->setDisplayName(fullName);

    const QVector<Account> & availableAccounts = m_pAccountsModel->accounts();
    if (availableAccounts.contains(*pDefaultAccount)) {
        QNDEBUG(QStringLiteral("The default account already exists"));
        return pDefaultAccount;
    }

    if (pCreatedDefaultAccount) {
        *pCreatedDefaultAccount = true;
    }

    return createLocalAccount(username, fullName, errorDescription);
}

QSharedPointer<Account> AccountManager::createLocalAccount(const QString & name,
                                                           const QString & displayName,
                                                           ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("AccountManager::createLocalAccount: name = ") << name
            << QStringLiteral(", display name = ") << displayName);

    bool res = writeAccountInfo(name, displayName, /* is local = */ true, /* user id = */ -1,
                                /* Evernote account type = */ QString(), /* Evernote host = */ QString(),
                                /* shard id = */ QString(), errorDescription);
    if (Q_UNLIKELY(!res)) {
        return QSharedPointer<Account>();
    }

    Account availableAccount(name, Account::Type::Local, qevercloud::UserID(-1));
    availableAccount.setDisplayName(displayName);
    m_pAccountsModel->addAccount(availableAccount);

    QSharedPointer<Account> result(new Account(name, Account::Type::Local));
    return result;
}

bool AccountManager::createAccountInfo(const Account & account)
{
    QNDEBUG(QStringLiteral("AccountManager::createAccountInfo: ") << account);

    bool isLocal = (account.type() == Account::Type::Local);

    QString evernoteAccountType = evernoteAccountTypeToString(account.evernoteAccountType());

    ErrorString errorDescription;
    bool res = writeAccountInfo(account.name(), account.displayName(), isLocal, account.id(),
                                evernoteAccountType, account.evernoteHost(), account.shardId(),
                                errorDescription);
    if (Q_UNLIKELY(!res)) {
        Q_EMIT notifyError(errorDescription);
        return false;
    }

    return true;
}

bool AccountManager::writeAccountInfo(const QString & name, const QString & displayName,
                                      const bool isLocal, const qevercloud::UserID id,
                                      const QString & evernoteAccountType,
                                      const QString & evernoteHost,
                                      const QString & shardId,
                                      ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("AccountManager::writeAccountInfo: name = ") << name
            << QStringLiteral(", display name = ") << displayName
            << QStringLiteral(", is local = ") << (isLocal ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", user id = ") << id << QStringLiteral(", Evernote account type = ")
            << evernoteAccountType << QStringLiteral(", Evernote host = ") << evernoteHost
            << QStringLiteral(", shard id = ") << shardId);

    Account account(name, (isLocal ? Account::Type::Local : Account::Type::Evernote),
                    id, Account::EvernoteAccountType::Free, evernoteHost, shardId);

    QDir accountPersistentStorageDir(accountPersistentStoragePath(account));
    if (!accountPersistentStorageDir.exists())
    {
        bool res = accountPersistentStorageDir.mkpath(accountPersistentStorageDir.absolutePath());
        if (Q_UNLIKELY(!res)) {
            errorDescription.setBase(QT_TR_NOOP("Can't create a directory for the account storage"));
            errorDescription.details() = accountPersistentStorageDir.absolutePath();
            QNWARNING(errorDescription);
            return false;
        }
    }

    QFile accountInfo(accountPersistentStorageDir.absolutePath() + QStringLiteral("/accountInfo.txt"));

    bool open = accountInfo.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open))
    {
        errorDescription.setBase(QT_TR_NOOP("Can't open the new account info file for writing"));
        errorDescription.details() = accountInfo.fileName();

        QString errorString = accountInfo.errorString();
        if (!errorString.isEmpty()) {
            errorDescription.details() += QStringLiteral(": ");
            errorDescription.details() += errorString;
        }

        QNWARNING(errorDescription);
        return false;
    }

    QXmlStreamWriter writer(&accountInfo);

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
    writer.writeCharacters(isLocal ? QStringLiteral("Local") : QStringLiteral("Evernote"));
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

QString AccountManager::evernoteAccountTypeToString(const Account::EvernoteAccountType::type type) const
{
    QString evernoteAccountType;
    switch(type)
    {
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
    QNDEBUG(QStringLiteral("AccountManager::readComplementaryAccountInfo: ") << account);

    if (Q_UNLIKELY(account.isEmpty())) {
        QNDEBUG(QStringLiteral("The account is empty"));
        return;
    }

    if (Q_UNLIKELY(account.name().isEmpty())) {
        QNDEBUG(QStringLiteral("The account name is empty"));
        return;
    }

    QDir accountPersistentStorageDir(accountPersistentStoragePath(account));
    if (!accountPersistentStorageDir.exists()) {
        QNDEBUG(QStringLiteral("No persistent storage dir exists for this account"));
        return;
    }

    QFile accountInfo(accountPersistentStorageDir.absolutePath() + QStringLiteral("/accountInfo.txt"));

    bool open = accountInfo.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open))
    {
        ErrorString errorDescription(QT_TR_NOOP("Can't read the complementary account info: "
                                                "can't open file for reading"));
        errorDescription.details() = accountInfo.fileName();

        QString errorString = accountInfo.errorString();
        if (!errorString.isEmpty()) {
            errorDescription.details() += QStringLiteral(": ");
            errorDescription.details() += errorString;
        }

        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QXmlStreamReader reader(&accountInfo);

    QString currentElementName;
    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

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

        if (reader.isCharacters())
        {
            if (currentElementName.isEmpty()) {
                continue;
            }

            if (currentElementName == QStringLiteral("evernoteAccountType"))
            {
                QString evernoteAccountType = reader.text().toString();
                if (evernoteAccountType.isEmpty() || (evernoteAccountType == QStringLiteral("Free"))) {
                    account.setEvernoteAccountType(Account::EvernoteAccountType::Free);
                }
                else if (evernoteAccountType == QStringLiteral("Plus")) {
                    account.setEvernoteAccountType(Account::EvernoteAccountType::Plus);
                }
                else if (evernoteAccountType == QStringLiteral("Premium")) {
                    account.setEvernoteAccountType(Account::EvernoteAccountType::Premium);
                }
                else if (evernoteAccountType == QStringLiteral("Business")) {
                    account.setEvernoteAccountType(Account::EvernoteAccountType::Business);
                }
            }
            else if (currentElementName == QStringLiteral("evernoteHost"))
            {
                account.setEvernoteHost(reader.text().toString());
            }
            else if (currentElementName == QStringLiteral("shardId"))
            {
                account.setShardId(reader.text().toString());
            }
        }

        if (reader.isCDATA() && (currentElementName == QStringLiteral("displayName"))) {
            account.setDisplayName(reader.text().toString());
        }
    }

    if (reader.hasError()) {
        ErrorString errorDescription(QT_TR_NOOP("Can't read the entire complementary account info, error reading XML"));
        errorDescription.details() = reader.errorString();
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
    }

    accountInfo.close();

    QNDEBUG(QStringLiteral("Account after reading in the complementary info: ") << account);
}

QSharedPointer<Account> AccountManager::accountFromEnvVarHints()
{
    QNDEBUG(QStringLiteral("AccountManager::accountFromEnvVarHints"));

#if QT_VERSION < QT_VERSION_CHECK(5, 1, 0)
#define qEnvironmentVariableIsEmpty(x) qgetenv(x).isEmpty()
#endif

    if (qEnvironmentVariableIsEmpty(ACCOUNT_NAME_ENV_VAR)) {
        QNDEBUG(QStringLiteral("Account name environment variable is not set or is empty"));
        return QSharedPointer<Account>();
    }

    if (qEnvironmentVariableIsEmpty(ACCOUNT_TYPE_ENV_VAR)) {
        QNDEBUG(QStringLiteral("Account type environment variable is not set or is empty"));
        return QSharedPointer<Account>();
    }

    QByteArray accountType = qgetenv(ACCOUNT_TYPE_ENV_VAR);
    bool isLocal = (accountType == QByteArray("1"));

    if (!isLocal)
    {
        if (qEnvironmentVariableIsEmpty(ACCOUNT_ID_ENV_VAR)) {
            QNDEBUG(QStringLiteral("Account id environment variable is not set or is empty"));
            return QSharedPointer<Account>();
        }

        if (qEnvironmentVariableIsEmpty(ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR)) {
            QNDEBUG(QStringLiteral("Evernote account type environment variable is not set or is empty"));
            return QSharedPointer<Account>();
        }

        if (qEnvironmentVariableIsEmpty(ACCOUNT_EVERNOTE_HOST_ENV_VAR)) {
            QNDEBUG(QStringLiteral("Evernote host environment variable is not set or is empty"));
            return QSharedPointer<Account>();
        }
    }

    QString accountName = QString::fromLocal8Bit(qgetenv(ACCOUNT_NAME_ENV_VAR));

    qevercloud::UserID id = -1;
    Account::EvernoteAccountType::type evernoteAccountType = Account::EvernoteAccountType::Free;
    QString evernoteHost;

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#define qEnvironmentVariableIntValue(x, ok) qgetenv(x).toInt(ok)
#endif

    if (!isLocal)
    {
        bool conversionResult = false;
        id = static_cast<qevercloud::UserID>(qEnvironmentVariableIntValue(ACCOUNT_ID_ENV_VAR, &conversionResult));
        if (!conversionResult) {
            QNDEBUG(QStringLiteral("Could not convert the account id to integer"));
            return QSharedPointer<Account>();
        }

        conversionResult = false;
        evernoteAccountType = static_cast<Account::EvernoteAccountType::type>(qEnvironmentVariableIntValue(ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR, &conversionResult));
        if (!conversionResult) {
            QNDEBUG(QStringLiteral("Could not convert the Evernote account type to integer"));
            return QSharedPointer<Account>();
        }

        evernoteHost = QString::fromLocal8Bit(qgetenv(ACCOUNT_EVERNOTE_HOST_ENV_VAR));
    }

    return findAccount(isLocal, accountName, id, evernoteAccountType, evernoteHost);
}

QSharedPointer<Account> AccountManager::lastUsedAccount()
{
    QNDEBUG(QStringLiteral("AccountManager::lastUsedAccount"));

    QSharedPointer<Account> result;

    ApplicationSettings appSettings;

    appSettings.beginGroup(ACCOUNT_SETTINGS_GROUP);

    BOOST_SCOPE_EXIT(&appSettings) { \
        appSettings.endGroup();
    } BOOST_SCOPE_EXIT_END

    QVariant name = appSettings.value(LAST_USED_ACCOUNT_NAME);
    if (name.isNull()) {
        QNDEBUG(QStringLiteral("Can't find last used account's name"));
        return result;
    }

    QString accountName = name.toString();
    if (accountName.isEmpty()) {
        QNDEBUG(QStringLiteral("Last used account's name is empty"));
        return result;
    }

    QVariant type = appSettings.value(LAST_USED_ACCOUNT_TYPE);
    if (type.isNull()) {
        QNDEBUG(QStringLiteral("Can't find last used account's type"));
        return result;
    }

    bool isLocal = type.toBool();

    qevercloud::UserID id = -1;
    Account::EvernoteAccountType::type evernoteAccountType = Account::EvernoteAccountType::Free;
    QString evernoteHost;

    if (!isLocal)
    {
        QVariant userId = appSettings.value(LAST_USED_ACCOUNT_ID);
        if (userId.isNull()) {
            QNDEBUG(QStringLiteral("Can't find last used account's id"));
            return result;
        }

        bool conversionResult = false;
        id = userId.toInt(&conversionResult);
        if (!conversionResult) {
            QNDEBUG(QStringLiteral("Can't convert the last used account's id to int"));
            return result;
        }

        QVariant userEvernoteAccountHost = appSettings.value(LAST_USED_ACCOUNT_EVERNOTE_HOST);
        if (userEvernoteAccountHost.isNull()) {
            QNDEBUG(QStringLiteral("Can't find last used account's Evernote host"));
            return result;
        }
        evernoteHost = userEvernoteAccountHost.toString();

        QVariant userEvernoteAccountType = appSettings.value(LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE);
        if (userEvernoteAccountType.isNull()) {
            QNDEBUG(QStringLiteral("Can't find last used account's Evernote account type"));
            return result;
        }

        conversionResult = false;
        evernoteAccountType = static_cast<Account::EvernoteAccountType::type>(userEvernoteAccountType.toInt(&conversionResult));
        if (!conversionResult) {
            QNDEBUG(QStringLiteral("Can't convert the last used account's Evernote account type to int"));
            return result;
        }
    }

    return findAccount(isLocal, accountName, id, evernoteAccountType, evernoteHost);
}

QSharedPointer<Account> AccountManager::findAccount(const bool isLocal,
                                                    const QString & accountName,
                                                    const qevercloud::UserID id,
                                                    const Account::EvernoteAccountType::type evernoteAccountType,
                                                    const QString & evernoteHost)
{
    QNDEBUG(QStringLiteral("AccountManager::findAccount"));

    QSharedPointer<Account> result;

    QString appPersistenceStoragePath = applicationPersistentStoragePath();
    QString accountDirName = (isLocal
                              ? (QStringLiteral("LocalAccounts/") + accountName)
                              : (QStringLiteral("EvernoteAccounts/") + accountName +
                                 QStringLiteral("_") + evernoteHost + QStringLiteral("_") +
                                 QString::number(id)));
    QFileInfo accountFileInfo(appPersistenceStoragePath + QStringLiteral("/") +
                              accountDirName + QStringLiteral("/accountInfo.txt"));
    if (accountFileInfo.exists()) {
        result = QSharedPointer<Account>(new Account(accountName,
                                                     (isLocal ? Account::Type::Local : Account::Type::Evernote),
                                                     id, evernoteAccountType, evernoteHost));
        readComplementaryAccountInfo(*result);
    }

    return result;
}

void AccountManager::updateLastUsedAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("AccountManager::updateLastUsedAccount: ") << account);

    ApplicationSettings appSettings;

    appSettings.beginGroup(ACCOUNT_SETTINGS_GROUP);

    appSettings.setValue(LAST_USED_ACCOUNT_NAME, account.name());
    appSettings.setValue(LAST_USED_ACCOUNT_TYPE, (account.type() == Account::Type::Local));
    appSettings.setValue(LAST_USED_ACCOUNT_ID, account.id());
    appSettings.setValue(LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE, account.evernoteAccountType());
    appSettings.setValue(LAST_USED_ACCOUNT_EVERNOTE_HOST, account.evernoteHost());

    appSettings.endGroup();
}

AccountManager::AccountInitializationException::AccountInitializationException(const ErrorString & message) :
    IQuentierException(message)
{}

const QString AccountManager::AccountInitializationException::exceptionDisplayName() const
{
    return QStringLiteral("AccountInitializationException");
}

} // namespace quentier
