/*
 * Copyright 2020-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_FILES_H
#define QUENTIER_LIB_PREFERENCES_KEYS_FILES_H

namespace quentier::preferences::keys::files {

// Names of different .ini files containing the values of preferences

constexpr const char * userInterface = "UserInterface";
constexpr const char * auxiliary = "Auxiliary";
constexpr const char * synchronization = "Synchronization";

} // namespace quentier::preferences::keys::files

#endif // QUENTIER_LIB_PREFERENCES_KEYS_FILES_H
