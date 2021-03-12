/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#include <QHash>
#include <QString>

class QDebug;

namespace quentier {

class CommandLineParser
{
public:
    enum class ArgumentType
    {
        None,
        String,
        Bool,
        Int,
        Double
    };

    friend QDebug & operator<<(QDebug & dbg, ArgumentType type);

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
        int argc, char * argv[], // NOLINT
        const QHash<QString, OptionData> & availableCmdOptions);

    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] ErrorString errorDescription() const;

    [[nodiscard]] Options options() const;

private:
    Q_DISABLE_COPY(CommandLineParser)

private:
    ErrorString m_errorDescription;
    Options m_options;
};

} // namespace quentier

#endif // QUENTIER_LIB_INITIALIZATION_COMMAND_LINE_PARSER_H
