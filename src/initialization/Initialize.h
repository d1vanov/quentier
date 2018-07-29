/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef QUENTIER_INITIALIZATION_INITIALIZE_H
#define QUENTIER_INITIALIZATION_INITIALIZE_H

#include "CommandLineParser.h"

namespace quentier {

QT_FORWARD_DECLARE_CLASS(QuentierApplication)

class ParseCommandLineResult
{
public:
    ParseCommandLineResult() :
        m_shouldQuit(false),
        m_responseMessage(),
        m_errorDescription(),
        m_cmdOptions()
    {}

    bool            m_shouldQuit;
    QString         m_responseMessage;
    ErrorString     m_errorDescription;
    CommandLineParser::CommandLineOptions   m_cmdOptions;
};

void parseCommandLine(int argc, char *argv[], ParseCommandLineResult & result);

int initialize(QuentierApplication & app, const CommandLineParser::CommandLineOptions & cmdOptions);

/**
 * Checks and processes "storageDir" command line options. This is special because it changes
 * the base path there app is searching for other things. Therefore it *must* be processed
 * very early. Other options may need be checked later.
 *
 * @param options Parsed command line arguments.
 * @return 0=OK, anything else is error code
 */
int processStorageDirCommandLineOption(const CommandLineParser::CommandLineOptions & options);

/**
 * Checks and processes rest of command line options. See processStorageDirCommandLineOption().
 *
 * @param options Parsed command line arguments.
 * @return 0=OK, anything else is error code
 */
int processCommandLineOptions(const CommandLineParser::CommandLineOptions & options);

} // namespace quentier

#endif // QUENTIER_INITIALIZATION_INITIALIZE_H
