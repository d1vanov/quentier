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

#ifndef LIB_QUENTIER_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H
#define LIB_QUENTIER_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H

#include "NoteStoreDataElementData.h"
#include <quentier/utility/Macros.h>

namespace quentier {

class FavoritableDataElementData: public NoteStoreDataElementData
{
public:
    FavoritableDataElementData();
    virtual ~FavoritableDataElementData();

    FavoritableDataElementData(const FavoritableDataElementData & other);
    FavoritableDataElementData(FavoritableDataElementData && other);

    bool    m_isFavorited;

private:
    FavoritableDataElementData & operator=(const FavoritableDataElementData & other) Q_DECL_EQ_DELETE;
    FavoritableDataElementData & operator=(FavoritableDataElementData && other) Q_DECL_EQ_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H
