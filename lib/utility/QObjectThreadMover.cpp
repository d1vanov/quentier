/*
 * Copyright 2019-2025 Dmitry Ivanov
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

#include "QObjectThreadMover.h"
#include "QObjectThreadMover_p.h"

#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QThread>
#include <QTimer>

namespace quentier {

bool moveObjectToThread(
    QObject & object, QThread & targetThread, ErrorString & errorDescription)
{
    auto * threadMover = new ThreadMover(object, targetThread);
    threadMover->moveToThread(object.thread());

    utility::EventLoopWithExitStatus loop;

    QObject::connect(
        threadMover, &ThreadMover::finished, &loop,
        &utility::EventLoopWithExitStatus::exitAsSuccess);

    QObject::connect(
        threadMover, &ThreadMover::notifyError, &loop,
        &utility::EventLoopWithExitStatus::exitAsFailureWithErrorString);

    QTimer slotInvokingTimer;
    slotInvokingTimer.singleShot(0, threadMover, SLOT(start()));
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    auto status = loop.exitStatus();

    threadMover->disconnect(&loop);
    threadMover->deleteLater();
    threadMover = nullptr;

    if (status == utility::EventLoopWithExitStatus::ExitStatus::Success) {
        return true;
    }

    errorDescription = loop.errorDescription();
    return false;
}

} // namespace quentier
