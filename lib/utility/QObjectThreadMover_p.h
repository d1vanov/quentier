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

#ifndef QUENTIER_LIB_UTILITY_QOBJECT_THREAD_MOVER_PRIVATE_H
#define QUENTIER_LIB_UTILITY_QOBJECT_THREAD_MOVER_PRIVATE_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>

#include <QObject>

namespace quentier {

class ThreadMover: public QObject
{
    Q_OBJECT
public:
    explicit ThreadMover(QObject & object, QThread & targetThread,
                         QObject * parent = Q_NULLPTR);

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void finished();
    void notifyError(ErrorString errorDescription);

private:
    QObject &   m_object;
    QThread &   m_targetThread;
};

} // namespace quentier

#endif // QUENTIER_LIB_UTILITY_QOBJECT_THREAD_MOVER_PRIVATE_H
