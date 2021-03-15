/*
 * Copyright 2019-2021 Dmitry Ivanov
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
    auto * pThreadMover = new ThreadMover(object, targetThread);
    pThreadMover->moveToThread(object.thread());

    EventLoopWithExitStatus loop;

    QObject::connect(
        pThreadMover, &ThreadMover::finished, &loop,
        &EventLoopWithExitStatus::exitAsSuccess);

    QObject::connect(
        pThreadMover, &ThreadMover::notifyError, &loop,
        &EventLoopWithExitStatus::exitAsFailureWithErrorString);

    QTimer::singleShot(0, pThreadMover, SLOT(start()));
    Q_UNUSED(loop.exec(QEventLoop::ExcludeUserInputEvents))
    auto status = loop.exitStatus();

    pThreadMover->disconnect(&loop);
    pThreadMover->deleteLater();
    pThreadMover = nullptr;

    if (status == EventLoopWithExitStatus::ExitStatus::Success) {
        return true;
    }

    errorDescription = loop.errorDescription();
    return false;
}

} // namespace quentier
