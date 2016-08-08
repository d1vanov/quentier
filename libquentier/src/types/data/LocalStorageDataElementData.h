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

#ifndef LIB_QUENTIER_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H
#define LIB_QUENTIER_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H

#include <quentier/utility/Qt4Helper.h>
#include <QUuid>
#include <QSharedData>

namespace quentier {

class LocalStorageDataElementData: public QSharedData
{
public:
    LocalStorageDataElementData();
    virtual ~LocalStorageDataElementData();

    LocalStorageDataElementData(const LocalStorageDataElementData & other);
    LocalStorageDataElementData(LocalStorageDataElementData && other);

    QUuid m_localUid;

private:
    LocalStorageDataElementData & operator=(const LocalStorageDataElementData & other) Q_DECL_EQ_DELETE;
    LocalStorageDataElementData & operator=(LocalStorageDataElementData && other) Q_DECL_EQ_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H
