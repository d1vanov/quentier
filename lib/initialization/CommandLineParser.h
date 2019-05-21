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

#ifndef QUENTIER_LIB_INITIALIZATION_COMMAND_LINE_PARSER_H
#define QUENTIER_LIB_INITIALIZATION_COMMAND_LINE_PARSER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>

#include <QString>
#include <QHash>

namespace quentier {

class CommandLineParser
{
public:
    struct CommandLineArgumentType
    {
        enum type
        {
            None = 0,
            String,
            Bool,
            Int,
            Double
        };
    };

    struct CommandLineOptionData
    {
        CommandLineOptionData() :
            m_description(),
            m_singleLetterKey(),
            m_type(CommandLineArgumentType::None)
        {}

        QString m_description;
        QChar m_singleLetterKey;
        CommandLineArgumentType::type m_type;
    };

    explicit CommandLineParser(
        int argc, char * argv[],
        const QHash<QString, CommandLineOptionData> & availableCmdOptions);

    QString responseMessage() const;
    bool shouldQuit() const;

    bool hasError() const;
    ErrorString errorDescription() const;

    typedef QHash<QString, QVariant> CommandLineOptions;
    CommandLineOptions options() const;

private:
    Q_DISABLE_COPY(CommandLineParser)

private:
    QString             m_responseMessage;
    bool                m_shouldQuit;
    ErrorString         m_errorDescription;
    CommandLineOptions  m_parsedArgs;
};

} // namespace quentier

#endif // QUENTIER_LIB_INITIALIZATION_COMMAND_LINE_PARSER_H
