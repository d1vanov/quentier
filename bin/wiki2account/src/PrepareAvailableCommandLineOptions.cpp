/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#include "PrepareAvailableCommandLineOptions.h"

namespace quentier {

void prepareAvailableCommandLineOptions(
    QHash<QString, CommandLineParser::OptionData> & options)
{
    composeCommonAvailableCommandLineOptions(options);

    auto & newAccountData = options[QStringLiteral("new-account")];

    newAccountData.m_description = QStringLiteral(
        "add notes created from wiki articles to a new local account");

    auto & notebookData = options[QStringLiteral("notebook")];

    notebookData.m_description = QStringLiteral(
        "name of the notebook into which notes created from wiki "
        "articles should be put; incompatible with --num-notebooks");

    auto & numNewNotebooksData = options[QStringLiteral("num-notebooks")];

    numNewNotebooksData.m_description = QStringLiteral(
        "number of new notebooks into which notes created from "
        "wiki articles should be put; incompatible with --notebook");

    auto & numNotesData = options[QStringLiteral("num-notes")];

    numNotesData.m_description = QStringLiteral(
        "number of notes which should be created from wiki articles");

    auto & minTagsPerNote = options[QStringLiteral("min-tags-per-note")];

    minTagsPerNote.m_description = QStringLiteral(
        "min number of new tags to be assigned to notes created "
        "from wiki articles; by default 0");

    auto & maxTagsPerNote = options[QStringLiteral("max-tags-per-note")];

    maxTagsPerNote.m_description = QStringLiteral(
        "max number of new tags to be assigned to notes created "
        "from wiki articles; by default 0");
}

} // namespace quentier
