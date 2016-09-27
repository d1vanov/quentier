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

#ifndef LIB_QUENTIER_UTILITY_QUENTIER_APPLICATION_H
#define LIB_QUENTIER_UTILITY_QUENTIER_APPLICATION_H

#include <quentier/utility/Linkage.h>
#include <QApplication>

namespace quentier {

class QUENTIER_EXPORT QuentierApplication: public QApplication
{
    Q_OBJECT
public:
    QuentierApplication(int & argc, char * argv[]);
    virtual ~QuentierApplication();

    virtual bool notify(QObject * object, QEvent * event);
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_QUENTIER_APPLICATION_H
