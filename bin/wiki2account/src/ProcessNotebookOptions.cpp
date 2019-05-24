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

#include <iostream>

namespace quentier {

bool processNoteboolOptions(const CommandLineParser::CommandLineOptions & options,
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

    // TODO: implement further
    return false;
}

} // namespace quentier
