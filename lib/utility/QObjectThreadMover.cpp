/*
 * Copyright 2019 Dmitry Ivanov
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
    ThreadMover * pThreadMover = new ThreadMover(object, targetThread);
    pThreadMover->moveToThread(object.thread());

    EventLoopWithExitStatus loop;
    QObject::connect(pThreadMover, QNSIGNAL(ThreadMover,finished),
                     &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
    QObject::connect(pThreadMover,
                     QNSIGNAL(ThreadMover,notifyError,ErrorString),
                     &loop,
                     QNSLOT(EventLoopWithExitStatus,
                            exitAsFailureWithErrorString,ErrorString));

    QTimer slotInvokingTimer;
    slotInvokingTimer.singleShot(0, pThreadMover, SLOT(start()));
    int result = loop.exec(QEventLoop::ExcludeUserInputEvents);

    pThreadMover->disconnect(&loop);
    pThreadMover->deleteLater();
    pThreadMover = nullptr;

    if (result == EventLoopWithExitStatus::ExitStatus::Success) {
        return true;
    }

    errorDescription = loop.errorDescription();
    return false;
}

} // namespace quentier

// #include "QObjectThreadMover.moc"
