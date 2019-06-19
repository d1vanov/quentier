/*
 * Copyright 2019 Dmitry Ivanov
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

#ifndef QUENTIER_WIKI2ACCOUNT_PREPARE_LOCAL_STORAGE_MANAGER_H
#define QUENTIER_WIKI2ACCOUNT_PREPARE_LOCAL_STORAGE_MANAGER_H

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QThread)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)
QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(ErrorString)

LocalStorageManagerAsync * prepareLocalStorageManager(const Account & account,
                                                      QThread & localStorageThread,
                                                      ErrorString & errorDescription);

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_PREPARE_LOCAL_STORAGE_MANAGER_H
