/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include "Keychain.h"

#include <lib/utility/VersionInfo.h>

#include <quentier/logging/QuentierLogger.h>

#include <QDebug>
#include <QTextStream>

#include <string_view>

namespace quentier {

namespace {

constexpr std::string_view compositeKeychainName =
    "SyncCompositeKeychainStorage";

IKeychainServicePtr newDefaultKeychain()
{
    QNDEBUG("utility", "newDefaultKeychain");

#ifdef Q_OS_MAC
    // On macOS using the system keychain for sync tokens gets pretty annoying
    // as each read and write by default needs to be granted by the user, so
    // using obfuscating keychain by default
    QNDEBUG("utility", "Creating new migrating keychain service");
    return newMigratingKeychainService(
        newQtKeychainService(), newObfuscatingKeychainService());
#elif QUENTIER_PACKAGED_AS_APP_IMAGE
    QNDEBUG("utility", "Creating new composite keychain service");
    return newCompositeKeychainService(
        QString::fromUtf8(
            compositeKeychainName.data(), compositeKeychainName.size()),
        newQtKeychainService(), newObfuscatingKeychainService());
#else
    QNDEBUG("utility", "Creating new QtKeychain keychain service");
    return newQtKeychainService();
#endif
}

template <class T>
void printKeychainKind(const KeychainKind kind, T & t)
{
    switch (kind) {
    case KeychainKind::Default:
        t << "default";
        break;
    case KeychainKind::QtKeychain:
        t << "QtKeychain";
        break;
    case KeychainKind::ObfuscatingKeychain:
        t << "obfuscating";
        break;
    case KeychainKind::MigratingKeychain:
        t << "migrating";
        break;
    case KeychainKind::CompositeKeychain:
        t << "composite";
        break;
    default:
        t << "unknown (" << static_cast<qint64>(kind) << ")";
        break;
    }
}

} // namespace

IKeychainServicePtr newKeychain(KeychainKind kind)
{
    QNDEBUG("utility", "newKeychain: kind = " << kind);

    switch (kind) {
    case KeychainKind::Default:
        return newDefaultKeychain();
    case KeychainKind::QtKeychain:
        return newQtKeychainService();
    case KeychainKind::ObfuscatingKeychain:
        return newObfuscatingKeychainService();
    case KeychainKind::MigratingKeychain:
        return newMigratingKeychainService(
            newQtKeychainService(), newObfuscatingKeychainService());
    case KeychainKind::CompositeKeychain:
    default:
        return newCompositeKeychainService(
            QString::fromUtf8(
                compositeKeychainName.data(), compositeKeychainName.size()),
            newQtKeychainService(), newObfuscatingKeychainService());
    }
}

QDebug & operator<<(QDebug & dbg, const KeychainKind kind)
{
    printKeychainKind(kind, dbg);
    return dbg;
}

QTextStream & operator<<(QTextStream & strm, const KeychainKind kind)
{
    printKeychainKind(kind, strm);
    return strm;
}

} // namespace quentier
