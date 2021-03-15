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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_ENEX_H
#define QUENTIER_LIB_PREFERENCES_KEYS_ENEX_H

namespace quentier::preferences::keys {

// Name of group within ApplicationSettings inside which preferences related
// to notes export and import to/from enex are stored
constexpr const char * enexExportImportGroup = "EnexExportImport";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the path to a folder into which some
// notes were exported as enex the last time
constexpr const char * lastExportNotesToEnexPath = "LastExportNotesToEnexPath";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing a boolean flag indicating whether
// the last time some notes were exported as enex they were exported with their
// tags or not
constexpr const char * lastExportNotesToEnexExportTags =
    "LastExportNotesToEnexExportTags";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the path to a folder from which some
// notes were imported from enex the last time
constexpr const char * lastEnexImportPath = "LastImportEnexPath";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the name of a notebook into which some
// notes were imported from enex the last time
constexpr const char * lastImportEnexNotebookName =
    "LastImportEnexNotebookName";

} // namespace quentier::preferences::keys

#endif // QUENTIER_LIB_PREFERENCES_KEYS_ENEX_H
