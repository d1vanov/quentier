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

#pragma once

#include <string_view>

namespace quentier::preferences::keys::files {

// Names of different .ini files containing the values of preferences

constexpr std::string_view userInterface = "UserInterface";
constexpr std::string_view auxiliary = "Auxiliary";
constexpr std::string_view synchronization = "Synchronization";

} // namespace quentier::preferences::keys::files
