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
// related to appearance are stored
constexpr std::string_view appearanceGroup = "LookAndFeel";

// Name of preference specifying the name of the icon theme which should be used
// by Quentier
constexpr std::string_view iconTheme = "IconTheme";

// Name of preference specifying whether Quentier should display thumbnails
// for notes containing images
constexpr std::string_view showNoteThumbnails = "ShowNoteThumbnails";

// Name of preference specifying the local ids of notes for which Quentier
// should not display thumbnails (the user can manually choose to not do it for
// particular notes)
constexpr std::string_view notesWithHiddenThumbnails = "HideNoteThumbnailsFor";

// Max number of notes for which thumbnails should not be displayed
constexpr int maxNotesWithHiddenThumbnails = 100;

// Name of preference specifying whether Quentier should not use native menu bar
// (could be useful to workaround some platform/DE specific quirks with panels)
constexpr std::string_view disableNativeMenuBar = "DisableNativeMenuBar";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing a boolean flag specifying whether Quentier
// has once displayed "Welcome to Quentier" dialog which is typically displayed
// once when the user launches the app for the very first time
constexpr std::string_view onceDisplayedWelcomeDialog =
    "OnceDisplayedGreeterScreen";

} // namespace quentier::preferences::keys
