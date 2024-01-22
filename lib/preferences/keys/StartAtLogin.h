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

namespace quentier::preferences::keys {

// Name of group within ApplicationSettings inside which preferences related
// to starting at login are stored
constexpr std::string_view startAtLoginGroup = "StartAutomaticallyAtLogin";

// Key for preference specifying whether Quentier should start automatically
// at login
constexpr std::string_view shouldStartAtLogin =
    "ShouldStartAutomaticallyAtLogin";

// Key for preference specifying how exactly Quentier should start at login:
// minimized, minimized to tray etc.
constexpr std::string_view startAtLoginOption =
    "StartAutomaticallyAtLoginOption";

} // namespace quentier::preferences::keys
