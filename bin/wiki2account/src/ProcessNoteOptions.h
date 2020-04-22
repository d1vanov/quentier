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

#ifndef QUENTIER_WIKI2ACCOUNT_PROCESS_NOTE_OPTIONS_H
#define QUENTIER_WIKI2ACCOUNT_PROCESS_NOTE_OPTIONS_H

#include <lib/initialization/CommandLineParser.h>

namespace quentier {

/**
 * Processes command line options related to how many notes the tool should
 * download from wiki: it should be either specified via --num-notes command
 * line options or the tool would ask the user to choose the number of notes
 *
 * @param options                   Command line options
 * @param numNotes                  Number of notes to download from wiki
 * @return                          True if note related command line options
 *                                  were processed successfully, false otherwise
 */
bool processNoteOptions(
    const CommandLineParser::Options & options, quint32 & numNotes);

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_PROCESS_NOTE_OPTIONS_H
