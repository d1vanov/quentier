/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/types/ErrorString.h>

#include <QHash>
#include <QString>

class QDebug;
class QTextStream;

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

    friend QDebug & operator<<(QDebug & dbg, ArgumentType type);
    friend QTextStream & operator<<(QTextStream & strm, ArgumentType type);

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

    [[nodiscard]] bool hasError() const;
    [[nodiscard]] ErrorString errorDescription() const;
    [[nodiscard]] Options options() const;

private:
    Q_DISABLE_COPY_MOVE(CommandLineParser)

private:
    ErrorString m_errorDescription;
    Options m_options;
};

} // namespace quentier
