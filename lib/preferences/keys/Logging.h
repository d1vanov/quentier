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
// to logging are stored
constexpr std::string_view loggingGroup = "LoggingSettings";

// Name of preference corresponding to min log level which Quentier should use
constexpr std::string_view minLogLevel = "MinLogLevel";

// Name of preference corresponding to current log filter by component preset
constexpr std::string_view loggingFilterByComponentPreset =
    "FilterByComponentPreset";

// Name of preference corresponding to current log filter by component regex
constexpr std::string_view loggingFilterByComponentRegex =
    "FilterByComponentRegex";

// Name of preference specifying whether a separate internal log for Quentier's
// log viewer widget should be enabled (the widget uses a separate log so that
// its own logs don't interfere with the logs it's displaying)
constexpr std::string_view enableLogViewerInternalLogs =
    "EnableLogViewerInternalLogs";

} // namespace quentier::preferences::keys
