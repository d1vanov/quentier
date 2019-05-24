/*
 * Copyright 2019 Dmitry Ivanov
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

#ifndef QUENTIER_WIKI2ACCOUNT_PROCESS_NOTEBOOK_OPTIONS_H
#define QUENTIER_WIKI2ACCOUNT_PROCESS_NOTEBOOK_OPTIONS_H

#include <lib/initialization/CommandLineParser.h>

namespace quentier {

/**
 * Processes command line options related to target notebooks for notes
 * downloaded from wiki: it should be either one target notebook or a number of
 * new notebooks to be created for wiki notes
 *
 * @param options                   Command line options
 * @param targetNotebookName        The name of notebook to which new notes
 *                                  downloaded from wiki should be put
 * @param numNewNotebooks           If target notebook name is not specified,
 *                                  this represents the number of new notebooks
 *                                  into which notes downloaded from wiki should
 *                                  be put
 * @return                          True if notebook related command line
 *                                  options were processed successfully, false
 *                                  otherwise
 */
bool processNoteboolOptions(const CommandLineParser::CommandLineOptions & options,
                            QString & targetNotebookName,
                            quint32 & numNewNotebooks);

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_PROCESS_NOTEBOOK_OPTIONS_H
