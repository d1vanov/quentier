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
// related to note editor are stored
constexpr std::string_view noteEditorGroup = "NoteEditor";

// Name of preference specifying whether Quentier's note editor should allow
// to use only a limited set of fonts - those offered by Evernote's web editor.
// It might be handy to avoid notes display issues across different devices.
constexpr std::string_view noteEditorUseLimitedSetOfFonts =
    "UseLimitedSetOfFonts";

// Name of preference specifying whether Quentier should silently automatically
// remove empty notes which were open in the note editor but were closed before
// they were edited in any way
constexpr std::string_view removeEmptyUneditedNotes =
    "RemoveEmptyUneditedNotes";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the path to a folder into which some
// note exported as pdf was saved the last time
constexpr std::string_view lastExportNoteToPdfPath = "LastExportNoteToPdfPath";

// Name of preference specifying the timeout used by note editor to wait for
// a pause during editing to save the edits of the note into the local storage
constexpr std::string_view noteEditorConvertToNoteTimeout =
    "ConvertToNoteTimeout";

// Name of preference containing the timeout used by note editor to wait
// for the local storage to expunge the note (namely the empty and unedited
// note)
constexpr std::string_view noteEditorExpungeNoteTimeout = "ExpungeNoteTimeout";

// Name of preference specifying the default font color for the note editor
constexpr std::string_view noteEditorFontColor = "FontColor";

// Name of preference specifying the default background color for the note
// editor
constexpr std::string_view noteEditorBackgroundColor = "BackgroundColor";

// Name of preference specifying the default highlight color for the note
// editor
constexpr std::string_view noteEditorHighlightColor = "HighlightColor";

// Name of preference specifying the default highlighted text color for the note
// editor
constexpr std::string_view noteEditorHighlightedTextColor =
    "HighlightedTextColor";

} // namespace quentier::preferences::keys
