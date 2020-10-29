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

#ifndef QUENTIER_LIB_PREFERENCES_DEFAULTS_NOTE_EDITOR_H
#define QUENTIER_LIB_PREFERENCES_DEFAULTS_NOTE_EDITOR_H

namespace quentier {
namespace preferences {
namespace defaults {

// Will remove empty unedited notes after closing them in the note editor
// by default
constexpr bool removeEmptyUneditedNotes = true;

// The default period of inactivity in milliseconds after which the note
// editor saves fresh text edits into a note
constexpr int convertToNoteTimeout = 500;

// The default period of waiting in milliseconds by the note editor for
// expunging the empty unedited note from the local storage
constexpr int expungeNoteTimeout = 500;

} // namespace defaults
} // namespace preferences
} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_DEFAULTS_NOTE_EDITOR_H
