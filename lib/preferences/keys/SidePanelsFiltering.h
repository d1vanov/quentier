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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_SIDE_PANELS_FILTERING_H
#define QUENTIER_LIB_PREFERENCES_KEYS_SIDE_PANELS_FILTERING_H

namespace quentier {
namespace preferences {
namespace keys {

// Name of group within ApplicationSettings inside which preferences related
// to notes filtering by selection in side panels are stored
constexpr const char * sidePanelsFilterBySelectionGroup =
    "SidePanelsFilterBySelection";

// Name of preference specifying whether selecting a notebook within a notebooks
// side panel should trigger filtering of notes by the selected notebook
constexpr const char * sidePanelsFilterBySelectedNotebook =
    "FilterBySelectedNotebook";

// Name of preference specifying whether selecting a tag within a tags side
// panel should trigger filtering of notes by the selected tag
constexpr const char * sidePanelsFilterBySelectedTag =
    "FilterBySelectedTag";

// Name of preference specifying whether selecting a saved search within
// a saved searches side panel should trigger filtering by the selected
// saved search
constexpr const char * sidePanelsFilterBySelectedSavedSearch =
    "FilterBySelectedSavedSearch";

// Name of preference specifying whether selecting a favorited item within
// a favorites side panel should trigger filtering by the selected favorited
// item
constexpr const char * sidePanelsFilterBySelectedFavoritedItems =
    "FilterBySelectedFavoritesItem";

} // keys
} // preferences
} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_KEYS_SIDE_PANELS_FILTERING_H
