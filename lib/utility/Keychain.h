/*
 * Copyright 2020-2025 Dmitry Ivanov
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

#include <quentier/utility/IKeychainService.h>

class QDebug;
class QTextStream;

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
     * Create migrating keychain from QtKeychain to obfuscating keychain
     */
    MigratingKeychain,
    /**
     * Create composite keychain using QtKeychain as a primary keychain and
     * obfuscating keychain as a secondary keychain
     */
    CompositeKeychain
};

QDebug & operator<<(QDebug & dbg, KeychainKind kind);
QTextStream & operator<<(QTextStream & strm, KeychainKind kind);

[[nodiscard]] utility::IKeychainServicePtr newKeychain(
    KeychainKind kind = KeychainKind::Default);

} // namespace quentier
