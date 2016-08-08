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

#ifndef LIB_QUENTIER_TYPES_DATA_NOTE_STORE_DATA_ELEMENT_DATA_H
#define LIB_QUENTIER_TYPES_DATA_NOTE_STORE_DATA_ELEMENT_DATA_H

#include "LocalStorageDataElementData.h"

namespace quentier {

class NoteStoreDataElementData : public LocalStorageDataElementData
{
public:
    NoteStoreDataElementData();
    virtual ~NoteStoreDataElementData();

    NoteStoreDataElementData(const NoteStoreDataElementData & other);
    NoteStoreDataElementData(NoteStoreDataElementData && other);

    bool    m_isDirty;
    bool    m_isLocal;

private:
    NoteStoreDataElementData & operator=(const NoteStoreDataElementData & other) Q_DECL_EQ_DELETE;
    NoteStoreDataElementData & operator=(NoteStoreDataElementData && other) Q_DECL_EQ_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_NOTE_STORE_DATA_ELEMENT_DATA_H
