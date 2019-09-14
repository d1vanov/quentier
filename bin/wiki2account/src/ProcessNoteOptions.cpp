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

#include "ProcessNoteOptions.h"

#include <QDebug>
#include <QTextStream>

#include <iostream>

namespace quentier {

bool processNoteOptions(
    const CommandLineParser::CommandLineOptions & options, quint32 & numNotes)
{
    auto numNotesIt = options.find(QStringLiteral("num-notes"));
    if (numNotesIt != options.end())
    {
        bool conversionResult = false;
        quint32 num = numNotesIt.value().toUInt(&conversionResult);
        if (!conversionResult) {
            qWarning() << "Failed to parse the number of notes to download";
            return false;
        }

        numNotes = num;
        return true;
    }

    QTextStream stdoutStrm(stdout);
    stdoutStrm << "Enter the number of notes to download from wiki\n> ";
    stdoutStrm.flush();

    QTextStream stdinStrm(stdin);
    while(true)
    {
        QString line = stdinStrm.readLine();

        bool conversionResult = false;
        quint32 num = line.toUInt(&conversionResult);
        if (!conversionResult) {
            stdoutStrm << "Failed to parse the number of notes to download, "
                "please try again\n> ";
            stdoutStrm.flush();
            continue;
        }

        numNotes = num;
        break;
    }

    return true;
}

} // namespace quentier
