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

#include <quentier/utility/EventLoopWithExitStatus.h>

namespace quentier {

EventLoopWithExitStatus::EventLoopWithExitStatus(QObject * parent) :
    QEventLoop(parent)
{
}

void EventLoopWithExitStatus::exitAsSuccess()
{
    QEventLoop::exit(ExitStatus::Success);
}

void EventLoopWithExitStatus::exitAsFailure()
{
    QEventLoop::exit(ExitStatus::Failure);
}

void EventLoopWithExitStatus::exitAsTimeout()
{
    QEventLoop::exit(ExitStatus::Timeout);
}

void EventLoopWithExitStatus::exitAsFailureWithError(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    QEventLoop::exit(ExitStatus::Failure);
}

} // namespace quentier
