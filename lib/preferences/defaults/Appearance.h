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

#ifndef QUENTIER_LIB_PREFERENCES_DEFAULTS_APPEARANCE_H
#define QUENTIER_LIB_PREFERENCES_DEFAULTS_APPEARANCE_H

namespace quentier {
namespace preferences {
namespace defaults {

// Will display note thumbnails by default
constexpr bool showNoteThumbnails = true;

// Determine whether native menu bar should be disabled by default
[[nodiscard]] bool disableNativeMenuBar();

} // namespace defaults
} // namespace preferences
} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_DEFAULTS_APPEARANCE_H
