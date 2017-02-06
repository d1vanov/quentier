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

#include "TagData.h"
#include <quentier/types/Tag.h>
#include <quentier/utility/Utility.h>

namespace quentier {

TagData::TagData() :
    FavoritableDataElementData(),
    m_qecTag(),
    m_linkedNotebookGuid(),
    m_parentLocalUid()
{}

TagData::TagData(const TagData & other) :
    FavoritableDataElementData(other),
    m_qecTag(other.m_qecTag),
    m_linkedNotebookGuid(other.m_linkedNotebookGuid),
    m_parentLocalUid(other.m_parentLocalUid)
{}

TagData::TagData(TagData && other) :
    FavoritableDataElementData(std::move(other)),
    m_qecTag(std::move(other.m_qecTag)),
    m_linkedNotebookGuid(std::move(other.m_linkedNotebookGuid)),
    m_parentLocalUid(std::move(other.m_parentLocalUid))
{}

TagData::TagData(const qevercloud::Tag & other) :
    FavoritableDataElementData(),
    m_qecTag(other),
    m_linkedNotebookGuid(),
    m_parentLocalUid()
{}

TagData::~TagData()
{}

void TagData::clear()
{
    m_qecTag = qevercloud::Tag();
    m_linkedNotebookGuid.clear();
}

bool TagData::checkParameters(ErrorString & errorDescription) const
{
    if (m_qecTag.guid.isSet() && !checkGuid(m_qecTag.guid.ref())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Tag's guid is invalid");
        errorDescription.details() = m_qecTag.guid;
        return false;
    }

    if (m_linkedNotebookGuid.isSet() && !checkGuid(m_linkedNotebookGuid.ref())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Tag's linked notebook guid is invalid");
        errorDescription.details() = m_linkedNotebookGuid;
        return false;
    }

    if (m_qecTag.name.isSet() && !Tag::validateName(m_qecTag.name.ref(), &errorDescription)) {
        return false;
    }

    if (m_qecTag.updateSequenceNum.isSet() && !checkUpdateSequenceNumber(m_qecTag.updateSequenceNum)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Tag's update sequence number is invalid");
        errorDescription.details() = QString::number(m_qecTag.updateSequenceNum);
        return false;
    }

    if (m_qecTag.parentGuid.isSet() && !checkGuid(m_qecTag.parentGuid.ref())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Tag's parent guid is invalid");
        errorDescription.details() = m_qecTag.parentGuid;
        return false;
    }

    return true;
}

bool TagData::operator==(const TagData & other) const
{
    return (m_qecTag == other.m_qecTag) &&
           (m_isDirty == other.m_isDirty) &&
           (m_isLocal == other.m_isLocal) &&
           (m_isFavorited == other.m_isFavorited) &&
           m_linkedNotebookGuid.isEqual(other.m_linkedNotebookGuid) &&
           m_parentLocalUid.isEqual(other.m_parentLocalUid);
}

bool TagData::operator!=(const TagData & other) const
{
    return !(*this == other);
}

} // namespace
