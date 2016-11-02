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
#include <dialogs/AddAccountDialog.h>
#include "dialogs/ManageAccountsDialog.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/logging/QuentierLogger.h>
#include <QXmlStreamWriter>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/scope_exit.hpp>
#endif

#define ACCOUNT_SETTINGS_GROUP QStringLiteral("AccountSettings")
#define LAST_USED_ACCOUNT_NAME QStringLiteral("LastUsedAccountName")
#define LAST_USED_ACCOUNT_TYPE QStringLiteral("LastUsedAccountType")
#define LAST_USED_ACCOUNT_ID QStringLiteral("LastUsedAccountId")
#define LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE QStringLiteral("LastUsedAccountEvernoteAccountType")

using namespace quentier;

AccountManager::AccountManager(QObject * parent) :
    QObject(parent),
    m_availableAccounts()
{
    detectAvailableAccounts();
}

Account AccountManager::currentAccount()
{
    QNDEBUG(QStringLiteral("AccountManager::currentAccount"));

    QSharedPointer<Account> pLastUsedAccount = lastUsedAccount();
    if (pLastUsedAccount.isNull())
    {
        QNLocalizedString errorDescription;
        pLastUsedAccount = createDefaultAccount(errorDescription);
        if (Q_UNLIKELY(pLastUsedAccount.isNull())) {
            QNLocalizedString error = QT_TR_NOOP("Can't initialize the default account");
            error += QStringLiteral(": ");
            error += errorDescription;
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

    QScopedPointer<AddAccountDialog> addAccountDialog(new AddAccountDialog(m_availableAccounts, parentWidget));
    addAccountDialog->setWindowModality(Qt::WindowModal);
    // TODO: setup some signal-slot connections for doing the actual work via the dialog
    Q_UNUSED(addAccountDialog->exec())
}

void AccountManager::raiseManageAccountsDialog()
{
    QNDEBUG(QStringLiteral("AccountManager::raiseManageAccountsDialog"));

    QWidget * parentWidget = qobject_cast<QWidget*>(parent());

    QScopedPointer<ManageAccountsDialog> manageAccountsDialog(new ManageAccountsDialog(m_availableAccounts, parentWidget));
    manageAccountsDialog->setWindowModality(Qt::WindowModal);
    // TODO: setup some signal-slot connections for doing the actual work via the dialog
    Q_UNUSED(manageAccountsDialog->exec())
}

void AccountManager::detectAvailableAccounts()
{
    QNDEBUG(QStringLiteral("AccountManager::detectAvailableAccounts"));

    m_availableAccounts.resize(0);

    QString appPersistenceStoragePath = applicationPersistentStoragePath();
    QDir appPersistenceStorageDir(appPersistenceStoragePath);

    QFileInfoList accountDirInfos = appPersistenceStorageDir.entryInfoList(QDir::AllDirs);
    int numPotentialAccountDirs = accountDirInfos.size();
    m_availableAccounts.reserve(numPotentialAccountDirs);

    for(int i = 0; i < numPotentialAccountDirs; ++i)
    {
        const QFileInfo & accountDirInfo = accountDirInfos[i];
        QDir accountDir = accountDirInfo.absoluteDir();
        QFileInfo accountBasicInfoFileInfo(accountDir.absolutePath() + QStringLiteral("/accountInfo.txt"));
        if (!accountBasicInfoFileInfo.exists()) {
            continue;
        }

        QString accountName = accountDir.dirName();
        bool isLocal = accountName.startsWith(QStringLiteral("local_"));
        if (isLocal) {
            accountName.remove(0, 6);
        }

        AvailableAccount availableAccount(accountName, accountDir.absolutePath(), isLocal);
        m_availableAccounts << availableAccount;
        QNDEBUG(QStringLiteral("Found available account: name = ") << accountName
                << QStringLiteral(", is local = ") << (isLocal ? QStringLiteral("true") : QStringLiteral("false"))
                << QStringLiteral(", dir ") << accountDir.absolutePath());

        int lastUnderlineIndex = accountName.lastIndexOf(QStringLiteral("_"));
        if (lastUnderlineIndex >= 0) {
            int numCharsToRemove = accountName.length() - lastUnderlineIndex;
            accountName.chop(numCharsToRemove);
        }
    }
}

QSharedPointer<Account> AccountManager::createDefaultAccount(QNLocalizedString & errorDescription)
{
    QNDEBUG(QStringLiteral("AccountManager::createDefaultAccount"));

    QString username = getCurrentUserName();
    if (Q_UNLIKELY(username.isEmpty())) {
        QNDEBUG(QStringLiteral("Couldn't get the current user's name, fallback to \"Default user\""));
        username = QStringLiteral("Default user");
    }

    QString appPersistenceStoragePath = applicationPersistentStoragePath();
    QString userPersistenceStoragePath = appPersistenceStoragePath + QStringLiteral("/local_") + username;
    QDir userPersistenceStorageDir(userPersistenceStoragePath);
    if (!userPersistenceStorageDir.exists())
    {
        bool created = userPersistenceStorageDir.mkpath(userPersistenceStoragePath);
        if (Q_UNLIKELY(!created)) {
            errorDescription = QT_TR_NOOP("can't create directory for the default account storage");
            QNWARNING(errorDescription);
            return QSharedPointer<Account>();
        }
    }

    QFile accountInfo(userPersistenceStoragePath + QStringLiteral("/accountInfo.txt"));

    bool open = accountInfo.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open))
    {
        errorDescription = QT_TR_NOOP("can't open the account info file for the default account for writing");
        QString errorString = accountInfo.errorString();
        if (!errorString.isEmpty()) {
            errorDescription += QStringLiteral(": ");
            errorDescription += errorString;
        }

        QNWARNING(errorDescription);
        return QSharedPointer<Account>();
    }

    QXmlStreamWriter writer(&accountInfo);

    writer.writeStartDocument();

    writer.writeStartElement(QStringLiteral("accountName"));
    writer.writeCharacters(username);
    writer.writeEndElement();

    writer.writeStartElement(QStringLiteral("accountType"));
    writer.writeCharacters(QStringLiteral("local"));
    writer.writeEndElement();

    writer.writeEndDocument();

    accountInfo.close();

    AvailableAccount availableAccount(username, userPersistenceStoragePath, /* local = */ true);
    m_availableAccounts << availableAccount;

    QSharedPointer<Account> result(new Account(username, Account::Type::Local));
    return result;
}

QSharedPointer<Account> AccountManager::lastUsedAccount() const
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

    // Now need to check whether such an account exists
    QString appPersistenceStoragePath = applicationPersistentStoragePath();
    QString accountDirName = (isLocal
                              ? (QStringLiteral("local_") + accountName)
                              : (accountName + QStringLiteral("_") + QString::number(id)));
    QFileInfo accountFileInfo(appPersistenceStoragePath + QStringLiteral("/") +
                              accountDirName + QStringLiteral("/accountInfo.txt"));
    if (accountFileInfo.exists()) {
        result = QSharedPointer<Account>(new Account(accountName,
                                                     (isLocal ? Account::Type::Local : Account::Type::Evernote),
                                                     id, evernoteAccountType));
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

    appSettings.endGroup();
}

AccountManager::AccountInitializationException::AccountInitializationException(const QNLocalizedString & message) :
    IQuentierException(message)
{}

const QString AccountManager::AccountInitializationException::exceptionDisplayName() const
{
    return QStringLiteral("AccountInitializationException");
}
