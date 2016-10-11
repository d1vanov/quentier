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

#include "ResourceData.h"

namespace quentier {

ResourceData::ResourceData() :
    NoteStoreDataElementData(),
    m_qecResource(),
    m_indexInNote(-1),
    m_noteLocalUid()
{}

ResourceData::ResourceData(const ResourceData & other) :
    NoteStoreDataElementData(other),
    m_qecResource(other.m_qecResource),
    m_indexInNote(other.m_indexInNote),
    m_noteLocalUid(other.m_noteLocalUid)
{}

ResourceData::ResourceData(ResourceData && other) :
    NoteStoreDataElementData(std::move(other)),
    m_qecResource(std::move(other.m_qecResource)),
    m_indexInNote(std::move(other.m_indexInNote)),
    m_noteLocalUid(std::move(other.m_noteLocalUid))
{}

ResourceData::ResourceData(const qevercloud::Resource & other) :
    NoteStoreDataElementData(),
    m_qecResource(other),
    m_indexInNote(-1),
    m_noteLocalUid()
{}

ResourceData::ResourceData(qevercloud::Resource && other) :
    NoteStoreDataElementData(),
    m_qecResource(std::move(other)),
    m_indexInNote(-1),
    m_noteLocalUid()
{}

ResourceData::~ResourceData()
{}

} // namespace quentier
