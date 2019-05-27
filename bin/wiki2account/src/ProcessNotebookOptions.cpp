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

#include "ProcessNotebookOptions.h"

#include <quentier/types/Notebook.h>

#include <QDebug>
#include <QTextStream>

#include <iostream>

namespace quentier {

bool processNotebookOptions(const CommandLineParser::CommandLineOptions & options,
                            QString & targetNotebookName,
                            quint32 & numNewNotebooks)
{
    auto targetNotebookNameIt = options.find(QStringLiteral("notebook"));
    auto numNewNotebooksIt = options.find(QStringLiteral("num-notebooks"));

    bool hasTargetNotebookName = (targetNotebookNameIt != options.end());
    bool hasNumNewNotebooks = (numNewNotebooksIt != options.end());
    if (hasTargetNotebookName && hasNumNewNotebooks) {
        qWarning() << "Either notebook or num-notebooks options should be "
            "supplied but not both of them";
        return false;
    }

    if (hasTargetNotebookName)
    {
        targetNotebookName = targetNotebookNameIt.value().toString().trimmed();

        ErrorString errorDescription;
        if (!Notebook::validateName(targetNotebookName, &errorDescription)) {
            qWarning() << "Invalid notebook name: "
                << errorDescription.nonLocalizedString();
            return false;
        }

        return true;
    }

    if (hasNumNewNotebooks)
    {
        bool conversionResult = false;
        quint32 numNotebooks = targetNotebookNameIt.value().toUInt(&conversionResult);
        if (!conversionResult) {
            qWarning() << "Failed to convert the number of notebooks to positive "
                "integer";
            return false;
        }

        numNewNotebooks = numNotebooks;
        return true;
    }

    QTextStream stdoutStrm(stdout);

    stdoutStrm << "Should notes downloaded from wiki be put into any particular "
        << "notebook or into several new notebooks?\n"
        << "1. Into new notebooks\n"
        << "2. Into a particular notebook\n"
        << "Enter the number corresponding to your choice\n"
        << "> ";
    stdoutStrm.flush();

    QTextStream stdinStrm(stdin);
    qint32 choice = -1;
    while(true)
    {
        QString line = stdinStrm.readLine();

        bool conversionResult = false;
        choice = line.toInt(&conversionResult);
        if (!conversionResult) {
            choice = -1;
            stdoutStrm << "Failed to parse the number, please try again\n"
                << "> ";
            stdoutStrm.flush();
            continue;
        }

        if ((choice != 1) && (choice != 2)) {
            choice = -1;
            stdoutStrm << "Please enter either 1 or 2\n> ";
            stdoutStrm.flush();
            continue;
        }

        break;
    }

    if (choice == 1)
    {
        stdoutStrm << "How many new notebooks should notes downloaded from "
            << "wiki go into?\n> ";
        stdoutStrm.flush();

        qint32 number = -1;
        while(true)
        {
            QString line = stdinStrm.readLine();

            bool conversionResult = false;
            number = line.toInt(&conversionResult);
            if (!conversionResult) {
                stdoutStrm << "Failed to parse the number, please try again\n"
                    << "> ";
                stdoutStrm.flush();
                continue;
            }

            if (number <= 0) {
                stdoutStrm << "Please enter positive number\n> ";
                stdoutStrm.flush();
                continue;
            }

            break;
        }

        numNewNotebooks = static_cast<quint32>(number);
        return true;
    }

    // If we got here, the chosen option was to use some particular notebook

    stdoutStrm << "Please enter the name of a new notebook\n> ";
    stdoutStrm.flush();

    QString line;
    while(true)
    {
        line = stdinStrm.readLine().trimmed();
        if (line.isEmpty()) {
            stdoutStrm << "Please enter non-empty notebook name\n> ";
            stdoutStrm.flush();
            continue;
        }

        ErrorString errorDescription;
        if (!Notebook::validateName(line, &errorDescription)) {
            stdoutStrm << "Entered notebook name is invalid: "
                << errorDescription.nonLocalizedString()
                << ", please try again\n> ";
            stdoutStrm.flush();
            continue;
        }

        break;
    }

    targetNotebookName = line;
    return true;
}

} // namespace quentier
