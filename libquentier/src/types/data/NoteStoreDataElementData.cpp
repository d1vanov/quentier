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

#include "NoteStoreDataElementData.h"

namespace quentier {

NoteStoreDataElementData::NoteStoreDataElementData() :
    LocalStorageDataElementData(),
    m_isDirty(true),
    m_isLocal(false)
{}

NoteStoreDataElementData::~NoteStoreDataElementData()
{}

NoteStoreDataElementData::NoteStoreDataElementData(const NoteStoreDataElementData & other) :
    LocalStorageDataElementData(other),
    m_isDirty(other.m_isDirty),
    m_isLocal(other.m_isLocal)
{}

NoteStoreDataElementData::NoteStoreDataElementData(NoteStoreDataElementData && other) :
    LocalStorageDataElementData(std::move(other)),
    m_isDirty(std::move(other.m_isDirty)),
    m_isLocal(std::move(other.m_isLocal))
{}

} // namespace quentier
