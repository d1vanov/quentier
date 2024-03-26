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

#include "CommandLineParser.h"

#include <lib/utility/HumanReadableVersionInfo.h>

#include <quentier/utility/Compat.h>

#include <QCommandLineParser>
#include <QDebug>
#include <QtGlobal>

#include <sstream>
#include <string>

namespace quentier {

namespace {

template <class T>
void printArgumentType(const CommandLineParser::ArgumentType type, T & t)
{
    using ArgumentType = CommandLineParser::ArgumentType;

    switch (type) {
    case ArgumentType::None:
        t << "None";
        break;
    case ArgumentType::String:
        t << "String";
        break;
    case ArgumentType::Bool:
        t << "Bool";
        break;
    case ArgumentType::Int:
        t << "Int";
        break;
    case ArgumentType::Double:
        t << "Double";
        break;
    default:
        t << "Unknown (" << static_cast<qint64>(type) << ")";
        break;
    }
}

} // namespace

CommandLineParser::CommandLineParser(
    int argc, char * argv[],
    const QHash<QString, OptionData> & availableCmdOptions)
{
    if (argc < 2) {
        return;
    }

    QCommandLineParser parser;

    parser.setApplicationDescription(QCoreApplication::translate(
        "CommandLineParser", "Cross-platform desktop Evernote client"));

    parser.addHelpOption();
    parser.addVersionOption();

    for (auto it = availableCmdOptions.constBegin(),
              end = availableCmdOptions.constEnd();
         it != end; ++it)
    {
        const QString & option = it.key();
        const OptionData & data = it.value();

        QStringList optionParts;
        if (!data.m_singleLetterKey.isNull()) {
            optionParts << data.m_singleLetterKey;
        }

        optionParts << option;

        QCommandLineOption opt{optionParts};

        if (data.m_type != ArgumentType::None) {
            if (!data.m_name.isEmpty()) {
                opt.setValueName(data.m_name);
            }
            else {
                opt.setValueName(QStringLiteral("arg"));
            }
        }

        if (!data.m_description.isEmpty()) {
            opt.setDescription(data.m_description);
        }

        parser.addOption(opt);
    }

    QStringList arguments;
    arguments.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        arguments << QString::fromLocal8Bit(argv[i]);
    }

    parser.process(arguments);

    auto optionNames = parser.optionNames();
    for (const auto & optionName: std::as_const(optionNames)) {
        const auto it = availableCmdOptions.find(optionName);
        if (Q_UNLIKELY(it == availableCmdOptions.end())) {
            continue;
        }

        const auto & optionData = it.value();
        QString value = parser.value(optionName);

        switch (optionData.m_type) {
        case ArgumentType::String:
            m_options[optionName] = value;
            break;
        case ArgumentType::Bool:
        {
            value = value.toLower();
            if ((value == QStringLiteral("yes")) ||
                (value == QStringLiteral("true")) ||
                (value == QStringLiteral("on")) ||
                (value == QStringLiteral("1")))
            {
                m_options[optionName] = true;
            }
            else {
                m_options[optionName] = false;
            }
            break;
        }
        case ArgumentType::Int:
        {
            bool conversionResult = false;
            const int valueInt = value.toInt(&conversionResult);
            if (!conversionResult) {
                m_errorDescription.setBase(QCoreApplication::translate(
                    "CommandLineParser",
                    "Failed to convert option value to int"));
                m_errorDescription.details() = optionName;
                m_errorDescription.details() += QStringLiteral("=");
                m_errorDescription.details() += value;
            }
            else {
                m_options[optionName] = valueInt;
            }
            break;
        }
        case ArgumentType::Double:
        {
            bool conversionResult = false;
            const double valueDouble = value.toDouble(&conversionResult);
            if (!conversionResult) {
                m_errorDescription.setBase(QCoreApplication::translate(
                    "CommandLineParser",
                    "Failed to convert option value to double"));
                m_errorDescription.details() = optionName;
                m_errorDescription.details() += QStringLiteral("=");
                m_errorDescription.details() += value;
            }
            else {
                m_options[optionName] = valueDouble;
            }
            break;
        }
        default:
            m_options[optionName] = QVariant();
            break;
        }

        if (hasError()) {
            break;
        }
    }
}

bool CommandLineParser::hasError() const
{
    return !m_errorDescription.isEmpty();
}

ErrorString CommandLineParser::errorDescription() const
{
    return m_errorDescription;
}

CommandLineParser::Options CommandLineParser::options() const
{
    return m_options;
}

////////////////////////////////////////////////////////////////////////////////

QDebug & operator<<(QDebug & dbg, const CommandLineParser::ArgumentType type)
{
    printArgumentType(type, dbg);
    return dbg;
}

QTextStream & operator<<(
    QTextStream & strm, const CommandLineParser::ArgumentType type)
{
    printArgumentType(type, strm);
    return strm;
}

} // namespace quentier
