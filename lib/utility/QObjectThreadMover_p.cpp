/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#include "QObjectThreadMover_p.h"

#include <QThread>

namespace quentier {

ThreadMover::ThreadMover(
    QObject & object, QThread & targetThread, QObject * parent) :
    QObject{parent}, m_object{object}, m_targetThread{targetThread}
{}

void ThreadMover::start()
{
    auto * currentThread = QObject::thread();
    if (Q_UNLIKELY(!currentThread || !currentThread->isRunning())) {
        ErrorString error(
            QT_TR_NOOP("Can't move object to another thread: "
                       "current object's thread is not running"));

        Q_EMIT notifyError(error);
        return;
    }

    if (!m_targetThread.isRunning()) {
        ErrorString error(
            QT_TR_NOOP("Can't move object to another thread: "
                       "target thread is not running"));

        Q_EMIT notifyError(error);
        return;
    }

    m_object.moveToThread(&m_targetThread);
    Q_EMIT finished();
}

} // namespace quentier
