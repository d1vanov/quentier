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

namespace quentier::preferences::defaults {

// Will filter by selected notebooks by default
constexpr bool filterBySelectedNotebooks = true;

// Will filter by selected tags by default
constexpr bool filterBySelectedTags = true;

// Will filter by selected saved search by default
constexpr bool filterBySelectedSavedSearch = true;

// Will filter by selected favorited items by default
constexpr bool filterBySelectedFavoritedItems = true;

} // namespace quentier::preferences::defaults
