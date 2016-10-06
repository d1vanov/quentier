/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/utility/QuentierApplication.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/SysInfo.h>
#include <exception>

namespace quentier {

QuentierApplication::QuentierApplication(int & argc, char * argv[]) :
    QApplication(argc, argv)
{}

QuentierApplication::~QuentierApplication()
{}

bool QuentierApplication::notify(QObject * object, QEvent * event)
{
    try
    {
        return QApplication::notify(object, event);
    }
    catch(const std::exception & e)
    {
        QNCRITICAL(QStringLiteral("Caught unhandled properly exception: ") << e.what()
                   << QStringLiteral(", backtrace: ") << SysInfo::GetSingleton().GetStackTrace());
        return false;
    }
}

} // namespace quentier
