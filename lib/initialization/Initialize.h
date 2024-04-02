/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include "CommandLineParser.h"

#include <optional>

namespace quentier {

class QuentierApplication;
class Account;

struct ParseCommandLineResult
{
    CommandLineParser::Options m_cmdOptions;
    ErrorString m_errorDescription;
};

/**
 * Setup common available command line options - help, version and some others
 * supposedly supported by many if not all binaries within quentier project
 */
void composeCommonAvailableCommandLineOptions(
    QHash<QString, CommandLineParser::OptionData> & availableCmdOptions);

void parseCommandLine(
    int argc, char * argv[],
    const QHash<QString, CommandLineParser::OptionData> & availableCmdOptions,
    ParseCommandLineResult & result);

/**
 * Processes "storageDir" command line option. This command line option is
 * special because if it is present, it changes the base path where app stores
 * nearly all of its persistent data. Therefore this command line option needs
 * to be processed separately from others very early during the app
 * initialization routine.
 *
 * @param options           Command line arguments being searched for
 *                          "storageDir"
 * @return                  True if no error was detected during the processing
 *                          of "storageDir" command line argument, false
 *                          otherwise
 */
[[nodiscard]] bool processStorageDirCommandLineOption(
    const CommandLineParser::Options & options);

/**
 * Processes "account" command line option, if it is present. The account being
 * parsed is the account which the app should use as the one loaded on startup
 *
 * @param options           Command line arguments being searched for "account"
 * @param startupAccount    Found account; if none is found or if no "account"
 *                          command line option is present, it would be
 *                          std::nullopt after the call
 * @return                  True if no error was detected during the processing
 *                          of "account" command line argument, false otherwise
 */
[[nodiscard]] bool processAccountCommandLineOption(
    const CommandLineParser::Options & options,
    std::optional<Account> & startupAccount);

/**
 * Processes "overrideSystemTrayAvailability" command line option, if it is
 * present.
 *
 * @param options           Command line arguments being searched for
 *                          "overrideSystemTrayAvailability"
 * @return                  True if no error was detected during the processing
 *                          of "overrideSystemTrayAvailability" command line
 *                          argument, false otherwise
 */
[[nodiscard]] bool processOverrideSystemTrayAvailabilityCommandLineOption(
    const CommandLineParser::Options & options);

/**
 * Initializes version string for QuentierApplication instance
 */
void initializeAppVersion(QuentierApplication & app);

/**
 * Initializes various things Quentier requires before actually launching
 * the app, including parsing of command line arguments
 *
 * @param app               Quentier app instance
 * @param cmdOptions        Command line arguments to be parsed
 * @return                  True if no error was detected during
 *                          the initialization, false otherwise
 */
[[nodiscard]] bool initialize(
    QuentierApplication & app, const CommandLineParser::Options & cmdOptions);

/**
 * @brief finalize          Finalizes various things to ensure Quentier quits
 *                          cleanly
 */
void finalize();

} // namespace quentier
