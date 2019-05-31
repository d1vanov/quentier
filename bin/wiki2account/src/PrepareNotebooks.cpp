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

#include "PrepareNotebooks.h"
#include "NotebookController.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QTimer>

// 10 minutes should be enough
#define PREPARE_NOTEBOOKS_TIMEOUT 600000

namespace quentier {

bool prepareNotebooks(const QString & targetNotebookName,
                      const quint32 numNewNotebooks,
                      LocalStorageManagerAsync & localStorageManagerAsync,
                      ErrorString & errorDescription)
{
    NotebookController controller(targetNotebookName, numNewNotebooks,
                                  localStorageManagerAsync);

    int status = -1;
    {
        QTimer timer;
        timer.setInterval(PREPARE_NOTEBOOKS_TIMEOUT);
        timer.setSingleShot(true);

        EventLoopWithExitStatus loop;
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout),
                         &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&controller, QNSIGNAL(NotebookController,finished),
                         &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&controller,
                         QNSIGNAL(NotebookController,failure,ErrorString),
                         &loop,
                         QNSLOT(EventLoopWithExitStatus,
                                exitAsFailureWithErrorString,ErrorString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &controller, SLOT(start()));

        status = loop.exec();
        errorDescription = loop.errorDescription();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Success) {
        errorDescription.clear();
        return true;
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        errorDescription.setBase(QT_TR_NOOP("Failed to prepare notebooks in due time"));
    }

    return false;
}

} // namespace quentier
