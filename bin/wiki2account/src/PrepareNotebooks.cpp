/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#include "PrepareNotebooks.h"

#include "NotebookController.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QTimer>

// 10 minutes should be enough
#define PREPARE_NOTEBOOKS_TIMEOUT 600000

namespace quentier {

QList<Notebook> prepareNotebooks(
    const QString & targetNotebookName, const quint32 numNewNotebooks,
    LocalStorageManagerAsync & localStorageManagerAsync,
    ErrorString & errorDescription)
{
    QList<Notebook> result;

    NotebookController controller(
        targetNotebookName, numNewNotebooks, localStorageManagerAsync);

    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(PREPARE_NOTEBOOKS_TIMEOUT);
        timer.setSingleShot(true);

        EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &controller, &NotebookController::finished, &loop,
            &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &controller, &NotebookController::failure, &loop,
            &EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &controller, SLOT(start()));

        Q_UNUSED(loop.exec())
        status = loop.exitStatus();
        errorDescription = loop.errorDescription();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Success) {
        errorDescription.clear();
        result = controller.newNotebooks();
        if (result.isEmpty()) {
            result << controller.targetNotebook();
        }
        return result;
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        errorDescription.setBase(
            QT_TR_NOOP("Failed to prepare notebooks in due time"));
    }

    return result;
}

} // namespace quentier
