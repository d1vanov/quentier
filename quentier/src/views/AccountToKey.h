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

#ifndef QUENTIER_VIEWS_ACCOUNT_TO_KEY_H
#define QUENTIER_VIEWS_ACCOUNT_TO_KEY_H

#include <quentier/types/Account.h>

namespace quentier {

/**
 * @brief accountToKey creates a string from the given account object which can be used as a key for e.g.
 * storing some per-account application settings.
 *
 * The string would include the type of the account, the username and for Evernote accounts - the user id and Evernote host
 *
 * @return either non-empty string uniquely idenfitying the given account or empty string if the account object is empty
 */
QString accountToKey(const Account & account);

} // namespace quentier

#endif // QUENTIER_VIEWS_ACCOUNT_TO_KEY_H
