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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_APPEARANCE_H
#define QUENTIER_LIB_PREFERENCES_KEYS_APPEARANCE_H

namespace quentier {
namespace preferences {
namespace keys {

// Name of group within ApplicationSettings inside which preferences related
// to appearance are stored
constexpr const char * appearanceGroup = "LookAndFeel";

// Name of preference specifying the name of the icon theme which should be used
// by Quentier
constexpr const char * iconTheme = "IconTheme";

// Name of preference specifying whether Quentier should display thumbnails
// for notes containing images
constexpr const char * showNoteThumbnails = "ShowNoteThumbnails";

// Name of preference specifying the local uids of notes for which Quentier
// should not display thumbnails (the user can manually choose to not do it for
// particular notes)
constexpr const char * notesWithHiddenThumbnails = "HideNoteThumbnailsFor";

// Max number of notes for which thumbnails should not be displayed
constexpr int maxNotesWithHiddenThumbnails = 100;

// Name of preference specifying whether Quentier should not use native menu bar
// (could be useful to workaround some platform/DE specific quirks with panels)
constexpr const char * disableNativeMenuBar = "DisableNativeMenuBar";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing a boolean flag specifying whether Quentier
// has once displayed "Welcome to Quentier" dialog which is typically displayed
// once when the user launches the app for the very first time
constexpr const char * onceDisplayedWelcomeDialog =
    "OnceDisplayedGreeterScreen";

} // keys
} // preferences
} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_KEYS_APPEARANCE_H
