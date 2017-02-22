/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/exception/ApplicationSettingsInitializationException.h>
#include <quentier/logging/QuentierLogger.h>
#include <QApplication>

namespace quentier {

QString defaultApplicationStoragePath()
{
    QString storagePath = applicationPersistentStoragePath();
    if (Q_UNLIKELY(storagePath.isEmpty())) {
        throw ApplicationSettingsInitializationException(ErrorString(QT_TRANSLATE_NOOP("", "Can't create ApplicationSettings instance: no persistent storage path")));
    }

    storagePath += QStringLiteral("/settings/");

    QString appName = QApplication::applicationName();
    if (!appName.isEmpty()) {
        storagePath += appName;
    }
    else {
        storagePath += QStringLiteral("config");
    }

    storagePath += QStringLiteral(".ini");
    return storagePath;
}

QString accountApplicationStoragePath(const Account & account, const QString & settingsName)
{
    QString storagePath = applicationPersistentStoragePath();
    if (Q_UNLIKELY(storagePath.isEmpty())) {
        throw ApplicationSettingsInitializationException(ErrorString(QT_TRANSLATE_NOOP("", "Can't create ApplicationSettings instance: no persistent storage path")));
    }

    QString accountName = account.name();
    if (Q_UNLIKELY(accountName.isEmpty())) {
        QNWARNING(QStringLiteral("Detected attempt to create ApplicationSettings for account with empty name"));
        throw ApplicationSettingsInitializationException(ErrorString(QT_TRANSLATE_NOOP("", "Can't create ApplicationSettings instance: the account name is empty")));
    }

    if (account.type() == Account::Type::Local)
    {
        storagePath += QStringLiteral("/LocalAccounts/");
        storagePath += accountName;
    }
    else
    {
        storagePath += QStringLiteral("/EvernoteAccounts/");
        storagePath += accountName;
        storagePath += QStringLiteral("_");
        storagePath += account.evernoteHost();
        storagePath += QStringLiteral("_");
        storagePath += QString::number(account.id());
    }

    storagePath += QStringLiteral("/settings/");

    if (!settingsName.isEmpty()) {
        storagePath += settingsName;
    }
    else {
        storagePath += QStringLiteral("config");
    }

    storagePath += QStringLiteral(".ini");
    return storagePath;
}

ApplicationSettings::ApplicationSettings() :
    QSettings(defaultApplicationStoragePath(), QSettings::IniFormat)
{}

ApplicationSettings::ApplicationSettings(const Account & account, const QString & settingsName) :
    QSettings(accountApplicationStoragePath(account, settingsName), QSettings::IniFormat)
{}

ApplicationSettings::~ApplicationSettings()
{}

QTextStream & ApplicationSettings::print(QTextStream & strm) const
{
    QStringList allStoredKeys = QSettings::allKeys();

    for(auto it = allStoredKeys.constBegin(), end = allStoredKeys.constEnd(); it != end; ++it) {
        QVariant value = QSettings::value(*it);
        strm << QStringLiteral("Key: ") << *it << QStringLiteral("; Value: ") << value.toString() << QStringLiteral("\n;");
    }

    return strm;
}

} // namespace quentier
