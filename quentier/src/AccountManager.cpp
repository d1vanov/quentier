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
#include "dialogs/ManageAccountsDialog.h"
#include <quentier/utility/DesktopServices.h>
#include <quentier/logging/QuentierLogger.h>
#include <QXmlStreamWriter>

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

    // TODO: try to find within the app settings the last used account and pick it
    return Account("Default user", Account::Type::Local);
}

void AccountManager::raiseAddAccountDialog()
{
    QNDEBUG(QStringLiteral("AccountManager::raiseAddAccountDialog"));

    // TODO: implement
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

        int lastUnderlineIndex = accountName.lastIndexOf(QStringLiteral("_"));
        if (lastUnderlineIndex >= 0) {
            int numCharsToRemove = accountName.length() - lastUnderlineIndex;
            accountName.chop(numCharsToRemove);
        }
    }
}

QString AccountManager::createDefaultAccount()
{
    QNDEBUG(QStringLiteral("AccountManager::createDefaultAccount"));

    QString username = getCurrentUserName();

    QString appPersistenceStoragePath = applicationPersistentStoragePath();
    QString userPersistenceStoragePath = appPersistenceStoragePath + QStringLiteral("/local_") + username;
    QDir userPersistenceStorageDir(userPersistenceStoragePath);
    if (!userPersistenceStorageDir.exists())
    {
        bool created = userPersistenceStorageDir.mkpath(userPersistenceStoragePath);
        if (Q_UNLIKELY(!created)) {
            QNLocalizedString error = QT_TR_NOOP("Can't create directory for the default account storage");
            QNWARNING(error);
            emit notifyError(error);
            return QString();
        }
    }

    QFile accountInfo(userPersistenceStoragePath + QStringLiteral("/accountInfo.txt"));

    bool open = accountInfo.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open)) {
        QNLocalizedString error = QT_TR_NOOP("Can't open the account info file for the default account for writing");
        error += QStringLiteral(": ");
        error += accountInfo.errorString();
            QNWARNING(error);
            emit notifyError(error);
        return QString();
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

    return username;
}
