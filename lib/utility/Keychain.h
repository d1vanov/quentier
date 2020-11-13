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

#ifndef QUENTIER_LIB_UTILITY_KEYCHAIN_H
#define QUENTIER_LIB_UTILITY_KEYCHAIN_H

#include <quentier/utility/IKeychainService.h>

QT_FORWARD_DECLARE_CLASS(QDebug);

namespace quentier {

/**
 * Kind of keychain to create
 */
enum class KeychainKind
{

    /**
     * Create the keychain of default kind for current platform
     */
    Default,
    /**
     * Create keychain using QtKeychain as a backend
     */
    QtKeychain,
    /**
     * Create obfuscating keychain
     */
    ObfuscatingKeychain,
    /**
     * Create composite keychain using QtKeychain as a primary keychain and
     * obfuscating keychain as a secondary keychain
     */
    CompositeKeychain
};

QDebug & operator<<(QDebug & dbg, const KeychainKind kind);

IKeychainServicePtr newKeychain(
    KeychainKind kind = KeychainKind::Default,
    QObject * parent = nullptr);

} // namespace quentier

#endif // QUENTIER_LIB_UTILITY_KEYCHAIN_H
