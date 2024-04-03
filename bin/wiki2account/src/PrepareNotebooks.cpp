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

#include "PrepareNotebooks.h"

#include "NotebookController.h"

#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QTimer>

namespace quentier {

QList<qevercloud::Notebook> prepareNotebooks(
    const QString & targetNotebookName, const quint32 numNewNotebooks,
    local_storage::ILocalStoragePtr localStorage,
    ErrorString & errorDescription)
{
    QList<qevercloud::Notebook> result;

    NotebookController controller{
        targetNotebookName, numNewNotebooks, std::move(localStorage)};

    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        // 10 minutes in msec, should be enough
        constexpr int timeout = 600000;

        QTimer timer;
        timer.setInterval(timeout);
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
