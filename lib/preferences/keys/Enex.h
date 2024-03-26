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
// to notes export and import to/from enex are stored
constexpr std::string_view enexExportImportGroup = "EnexExportImport";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the path to a folder into which some
// notes were exported as enex the last time
constexpr std::string_view lastExportNotesToEnexPath =
    "LastExportNotesToEnexPath";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing a boolean flag indicating whether
// the last time some notes were exported as enex they were exported with their
// tags or not
constexpr std::string_view lastExportNotesToEnexExportTags =
    "LastExportNotesToEnexExportTags";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the path to a folder from which some
// notes were imported from enex the last time
constexpr std::string_view lastEnexImportPath = "LastImportEnexPath";

// Name of technical preference (not really a preference but a value stored
// alongside preferences) containing the name of a notebook into which some
// notes were imported from enex the last time
constexpr std::string_view lastImportEnexNotebookName =
    "LastImportEnexNotebookName";

} // namespace quentier::preferences::keys
