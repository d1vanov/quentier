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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_ACCOUNT_H
#define QUENTIER_LIB_PREFERENCES_KEYS_ACCOUNT_H

namespace quentier {
namespace preferences {
namespace keys {

// Name of group within ApplicationSettings inside which preferences related
// to accounts are stored
constexpr const char * accountGroup = "AccountSettings";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the name of the last account used within
// Quentier
constexpr const char * lastUsedAccountName = "LastUsedAccountName";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the type of the last account used within
// Quentier (i.e. local or Evernote account)
constexpr const char * lastUsedAccountType = "LastUsedAccountType";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the id of the last account used within
// Quentier
constexpr const char * lastUsedAccountId = "LastUsedAccountId";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the Evernote account type of the last
// account used within Quentier (i.e. Basic, Premium etc)
constexpr const char * lastUsedAccountEvernoteAccountType =
    "LastUsedAccountEvernoteAccountType";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the Evernote host address corresponding
// to the last account used within Quentier (i.e. Evernote production, Evernote
// sandbox or Yinxiang Biji)
constexpr const char * lastUsedAccountEvernoteHost =
    "LastUsedAccountEvernoteHost";

// Name of the environment variable which can be set to the name of the account
// which Quentier should use on startup
constexpr const char * startupAccountNameEnvVar = "QUENTIER_ACCOUNT_NAME";

// Name of the environment variable which can be set to the type of the account
// which Quentier should use on startup
constexpr const char * startupAccountTypeEnvVar = "QUENTIER_ACCOUNT_TYPE";

// Name of the environment variable which can be set to the id of the account
// which Quentier should use on startup
constexpr const char * startupAccountIdEnvVar = "QUENTIER_ACCOUNT_ID";

// Name of the environment variable which can be set to the Evernote account
// type of the account which Quentier should use on startup
constexpr const char * startupAccountEvernoteAccountTypeEnvVar =
    "QUENTIER_ACCOUNT_EVERNOTE_ACCOUNT_TYPE";

// Name of the environment variable containing the Evernote host corresponding
// to the account which Quentier should use on startup
constexpr const char * startupAccountEvernoteHostEnvVar =
    "QUENTIER_ACCOUNT_EVERNOTE_HOST";

} // namespace keys
} // namespace preferences
} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_KEYS_ACCOUNT_H
