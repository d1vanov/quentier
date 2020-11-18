/*
 * Copyright 2020 Dmitry Ivanov
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

namespace quentier {

namespace {

constexpr const char * compositeKeychainName = "SyncCompositeKeychainStorage";

IKeychainServicePtr newDefaultKeychain(QObject * parent)
{
    QNDEBUG("utility", "newDefaultKeychain");

#ifdef Q_OS_MAC
    // On macOS using the system keychain for sync tokens gets pretty annoying
    // as each read and write by default needs to be granted by the user, so
    // using obfuscating keychain by default
    QNDEBUG("utility", "Creating new migrating keychain service");
    return newMigratingKeychainService(
        newQtKeychainService(parent), newObfuscatingKeychainService(parent));
#elif QUENTIER_PACKAGED_AS_APP_IMAGE
    QNDEBUG("utility", "Creating new composite keychain service");
    return newCompositeKeychainService(
        QString::fromUtf8(compositeKeychainName), newQtKeychainService(parent),
        newObfuscatingKeychainService(parent), parent);
#else
    QNDEBUG("utility", "Creating new QtKeychain keychain service");
    return newQtKeychainService(parent);
#endif
}

} // namespace

IKeychainServicePtr newKeychain(KeychainKind kind, QObject * parent)
{
    QNDEBUG("utility", "newKeychain: kind = " << kind);

    switch (kind) {
    case KeychainKind::Default:
        return newDefaultKeychain(parent);
    case KeychainKind::QtKeychain:
        return newQtKeychainService(parent);
    case KeychainKind::ObfuscatingKeychain:
        return newObfuscatingKeychainService(parent);
    case KeychainKind::MigratingKeychain:
        return newMigratingKeychainService(
            newQtKeychainService(parent),
            newObfuscatingKeychainService(parent));
    case KeychainKind::CompositeKeychain:
    default:
        return newCompositeKeychainService(
            QString::fromUtf8(compositeKeychainName),
            newQtKeychainService(parent), newObfuscatingKeychainService(parent),
            parent);
    }
}

QDebug & operator<<(QDebug & dbg, const KeychainKind kind)
{
    switch (kind) {
    case KeychainKind::Default:
        dbg << "default";
        break;
    case KeychainKind::QtKeychain:
        dbg << "QtKeychain";
        break;
    case KeychainKind::ObfuscatingKeychain:
        dbg << "obfuscating";
        break;
    case KeychainKind::MigratingKeychain:
        dbg << "migrating";
        break;
    case KeychainKind::CompositeKeychain:
        dbg << "composite";
        break;
    default:
        dbg << "unknown (" << static_cast<qint64>(kind) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
