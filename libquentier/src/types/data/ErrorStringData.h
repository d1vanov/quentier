/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef LIB_QUENTIER_TYPES_DATA_ERROR_STRING_DATA_H
#define LIB_QUENTIER_TYPES_DATA_ERROR_STRING_DATA_H

#include <QStringList>
#include <QSharedData>

namespace quentier {

class ErrorStringData: public QSharedData
{
public:
    QString     m_base;
    QString     m_details;
    QStringList m_additionalBases;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_ERROR_STRING_DATA_H
