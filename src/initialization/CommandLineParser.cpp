/*
 * Copyright 2017-2019 Dmitry Ivanov
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
#include "../utility/HumanReadableVersionInfo.h"
#include <string>
#include <sstream>
#include <QtGlobal>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef QT_MOC_RUN
#include <boost/program_options.hpp>
#endif

// Overloaded function required in order to use boost::program_options along
// with QString
void validate(boost::any & value, const std::vector<std::string> & values,
              QString *, int)
{
    using namespace boost::program_options;

    // Make sure no previous assignment to 'value' was made.
    validators::check_first_occurrence(value);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    const std::string & str = validators::get_single_string(values);

    value = boost::any(QString::fromLocal8Bit(str.c_str()));
}

namespace quentier {

CommandLineParser::CommandLineParser(int argc, char * argv[]) :
    m_responseMessage(),
    m_shouldQuit(false),
    m_errorDescription(),
    m_parsedArgs()
{
    if (argc < 2) {
        return;
    }

    namespace po = boost::program_options;

    try
    {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "show help message")
            ("version,v", "show version info")
            ("storageDir", po::value<QString>(),
             "set directory with the app's persistence")
            ("account", po::value<QString>(),
             "set the account to use by default:\n"
             "local_<Name>\n"
             "evernote_<id>_<Name>\n"
             "evernotesandbox_<id>_<Name>\n"
             "yinxiangbiji_<id>_<Name>\n"
             "where <id> is user ID and <Name> is the account name")
            ("overrideSystemTrayAvailability", po::value<bool>(),
             "override the availability of the system tray\n(0 - override to "
             "false,\nany other value - override to true)")
            ("startMinimizedToTray", "start Quentier minimized to system tray")
            ("startMinimized",
             "start Quentier with its main window minimized to the task bar");

        po::variables_map varsMap;
        po::store(po::parse_command_line(argc, argv, desc), varsMap);
        po::notify(varsMap);

        if (varsMap.count("help")) {
            std::stringstream sstrm;
            desc.print(sstrm);
            m_responseMessage = QString::fromLocal8Bit(sstrm.str().c_str());
            m_shouldQuit = true;
            return;
        }

        if (varsMap.count("version"))
        {
            m_responseMessage =
                quentierVersion() + QStringLiteral(", build info: ") +
                quentierBuildInfo() + QStringLiteral("\nBuilt with Qt ") +
                QStringLiteral(QT_VERSION_STR) + QStringLiteral(", uses Qt ") +
                QString::fromUtf8(qVersion()) +
                QStringLiteral("\nBuilt with libquentier: ") +
                libquentierBuildTimeInfo() +
                QStringLiteral("\nUses libquentier: ") +
                libquentierRuntimeInfo() +
                QStringLiteral("\n");
            m_shouldQuit = true;
            return;
        }

        for(auto it = varsMap.begin(), end = varsMap.end(); it != end; ++it)
        {
            if ( (it->first == "help") ||
                 (it->first == "version") )
            {
                continue;
            }

            const auto & value = it->second.value();
            const std::type_info & valueType = value.type();
            QString key = QString::fromLocal8Bit(it->first.c_str());

            if (valueType == typeid(QString)) {
                m_parsedArgs[key] = QVariant(boost::any_cast<QString>(value));
            }
            else if (valueType == typeid(int)) {
                m_parsedArgs[key] = QVariant(boost::any_cast<int>(value));
            }
            else if (valueType == typeid(bool)) {
                m_parsedArgs[key] = QVariant(boost::any_cast<bool>(value));
            }
            else {
                m_parsedArgs[key] = QVariant();
            }
        }
    }
    catch(const po::error & error)
    {
        m_errorDescription.setBase(
            QT_TRANSLATE_NOOP("CommandLineParser",
                              "Error parsing the command line arguments"));
        m_errorDescription.details() = QString::fromLocal8Bit(error.what());
    }
}

QString CommandLineParser::responseMessage() const
{
    return m_responseMessage;
}

bool CommandLineParser::shouldQuit() const
{
    return m_shouldQuit;
}

bool CommandLineParser::hasError() const
{
    return !m_errorDescription.isEmpty();
}

ErrorString CommandLineParser::errorDescription() const
{
    return m_errorDescription;
}

CommandLineParser::CommandLineOptions CommandLineParser::options() const
{
    return m_parsedArgs;
}

} // namespace quentier
