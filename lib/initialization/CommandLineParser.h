/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#include <quentier/types/ErrorString.h>
#include <quentier/utility/Macros.h>

#include <QHash>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

class CommandLineParser
{
public:
    enum class ArgumentType
    {
        None = 0,
        String,
        Bool,
        Int,
        Double
    };

    friend QDebug & operator<<(QDebug & dbg, const ArgumentType type);

    struct OptionData
    {
        QString m_name;
        QString m_description;
        QChar m_singleLetterKey;
        ArgumentType m_type = ArgumentType::None;
    };

    using Options = QHash<QString, QVariant>;

public:
    explicit CommandLineParser(
        int argc, char * argv[],
        const QHash<QString, OptionData> & availableCmdOptions);

    bool hasError() const;
    ErrorString errorDescription() const;

    Options options() const;

private:
    Q_DISABLE_COPY(CommandLineParser)

private:
    ErrorString m_errorDescription;
    Options m_options;
};

} // namespace quentier

#endif // QUENTIER_LIB_INITIALIZATION_COMMAND_LINE_PARSER_H
