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

#ifndef LIB_QUENTIER_UTILITY_EVENT_LOOP_WITH_EXIT_STATUS_H
#define LIB_QUENTIER_UTILITY_EVENT_LOOP_WITH_EXIT_STATUS_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/Linkage.h>
#include <QEventLoop>

namespace quentier {

class QUENTIER_EXPORT EventLoopWithExitStatus : public QEventLoop
{
    Q_OBJECT
public:
    explicit EventLoopWithExitStatus(QObject * parent = Q_NULLPTR);

    struct ExitStatus {
        enum type {
            Success,
            Failure,
            Timeout
        };
    };

public Q_SLOTS:
    void exitAsSuccess();
    void exitAsFailure();
    void exitAsFailureWithError(QString errorDescription);
    void exitAsTimeout();
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_EVENT_LOOP_WITH_EXIT_STATUS_H
