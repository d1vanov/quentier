/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#include "ParseCommandLine.h"

#include <lib/initialization/Initialize.h>

#include <QCoreApplication>
#include <QHash>

namespace quentier {

void ParseCommandLine(int argc, char * argv[], ParseCommandLineResult & result)
{
    QHash<QString, CommandLineParser::OptionData> availableCmdOptions;
    composeCommonAvailableCommandLineOptions(availableCmdOptions);

    auto & overrideSystemTrayAvailabilityData =
        availableCmdOptions[QStringLiteral("overrideSystemTrayAvailability")];

    overrideSystemTrayAvailabilityData.m_type =
        CommandLineParser::ArgumentType::Bool;

    overrideSystemTrayAvailabilityData.m_name = QStringLiteral("on/off");

    overrideSystemTrayAvailabilityData.m_description =
        QCoreApplication::translate(
            "CommandLineParser",
            "override the availability of the system tray");

    auto & startMinimizedToTrayData =
        availableCmdOptions[QStringLiteral("startMinimizedToTray")];

    startMinimizedToTrayData.m_description = QCoreApplication::translate(
        "CommandLineParser", "start Quentier minimized to system tray");

    auto & startMinimizedData =
        availableCmdOptions[QStringLiteral("startMinimized")];

    startMinimizedData.m_description = QCoreApplication::translate(
        "CommandLineParser",
        "start Quentier with its main window minimized to the task bar");

    auto & logLevelData = availableCmdOptions[QStringLiteral("logLevel")];
    logLevelData.m_type = CommandLineParser::ArgumentType::String;
    logLevelData.m_name = QStringLiteral("logLevel");

    logLevelData.m_description = QCoreApplication::translate(
        "CommandLineParser",
        "start Quentier with specified log level: error, "
        "warning, info, debug or trace");

    parseCommandLine(argc, argv, availableCmdOptions, result);
}

} // namespace quentier
