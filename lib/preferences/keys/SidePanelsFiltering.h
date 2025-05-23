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

#include <string_view>

namespace quentier::preferences::keys {

// Name of group within utility::ApplicationSettings inside which preferences
// related to notes filtering by selection in side panels are stored
constexpr std::string_view sidePanelsFilterBySelectionGroup =
    "SidePanelsFilterBySelection";

// Name of preference specifying whether selecting a notebook within a notebooks
// side panel should trigger filtering of notes by the selected notebook
constexpr std::string_view sidePanelsFilterBySelectedNotebook =
    "FilterBySelectedNotebook";

// Name of preference specifying whether selecting a tag within a tags side
// panel should trigger filtering of notes by the selected tag
constexpr std::string_view sidePanelsFilterBySelectedTag =
    "FilterBySelectedTag";

// Name of preference specifying whether selecting a saved search within
// a saved searches side panel should trigger filtering by the selected
// saved search
constexpr std::string_view sidePanelsFilterBySelectedSavedSearch =
    "FilterBySelectedSavedSearch";

// Name of preference specifying whether selecting a favorited item within
// a favorites side panel should trigger filtering by the selected favorited
// item
constexpr std::string_view sidePanelsFilterBySelectedFavoritedItems =
    "FilterBySelectedFavoritesItem";

} // namespace quentier::preferences::keys
