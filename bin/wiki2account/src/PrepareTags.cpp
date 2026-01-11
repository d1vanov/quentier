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

#include "PrepareTags.h"
#include "TagController.h"

#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/utility/EventLoopWithExitStatus.h>

#include <QTimer>

namespace quentier {

QList<qevercloud::Tag> prepareTags(
    const quint32 minTagsPerNote, const quint32 maxTagsPerNote,
    local_storage::ILocalStoragePtr localStorage,
    ErrorString & errorDescription)
{
    QList<qevercloud::Tag> result;

    TagController controller{
        minTagsPerNote, maxTagsPerNote, std::move(localStorage)};

    auto status = utility::EventLoopWithExitStatus::ExitStatus::Failure;
    {
        // 10 minutes in msec, should be enough
        constexpr int timeout = 600000;

        QTimer timer;
        timer.setInterval(timeout);
        timer.setSingleShot(true);

        utility::EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &utility::EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &controller, &TagController::finished, &loop,
            &utility::EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &controller, &TagController::failure, &loop,
            &utility::EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &controller, SLOT(start()));

        Q_UNUSED(loop.exec())
        status = loop.exitStatus();
        errorDescription = loop.errorDescription();
    }

    if (status == utility::EventLoopWithExitStatus::ExitStatus::Success) {
        errorDescription.clear();
        result = controller.tags();
        return result;
    }

    if (status == utility::EventLoopWithExitStatus::ExitStatus::Timeout) {
        errorDescription.setBase(
            QT_TR_NOOP("Failed to prepare tags in due time"));
    }

    return result;
}

} // namespace quentier
