/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include "NoteModelItem.h"

#include <quentier/utility/DateTime.h>

namespace quentier {

void NoteModelItem::addTagLocalId(const QString & tagLocalId)
{
    if (const int index = m_tagLocalIds.indexOf(tagLocalId); index >= 0) {
        return;
    }

    m_tagLocalIds << tagLocalId;
}

void NoteModelItem::removeTagLocalId(const QString & tagLocalId)
{
    const int index = m_tagLocalIds.indexOf(tagLocalId);
    if (index < 0) {
        return;
    }

    m_tagLocalIds.removeAt(index);
}

bool NoteModelItem::hasTagLocalId(const QString & tagLocalId) const noexcept
{
    return m_tagLocalIds.contains(tagLocalId);
}

int NoteModelItem::numTagLocalIds() const noexcept
{
    return m_tagLocalIds.size();
}

void NoteModelItem::addTagGuid(const QString & tagGuid)
{
    if (const int index = m_tagGuids.indexOf(tagGuid); index >= 0) {
        return;
    }

    m_tagGuids << tagGuid;
}

void NoteModelItem::removeTagGuid(const QString & tagGuid)
{
    const int index = m_tagGuids.indexOf(tagGuid);
    if (index < 0) {
        return;
    }

    m_tagGuids.removeAt(index);
}

bool NoteModelItem::hasTagGuid(const QString & tagGuid) const noexcept
{
    return m_tagGuids.contains(tagGuid);
}

int NoteModelItem::numTagGuids() const noexcept
{
    return m_tagGuids.size();
}

void NoteModelItem::addTagName(const QString & tagName)
{
    if (const int index = m_tagNameList.indexOf(tagName); index >= 0) {
        return;
    }

    m_tagNameList << tagName;
}

void NoteModelItem::removeTagName(const QString & tagName)
{
    int index = m_tagNameList.indexOf(tagName);
    if (index < 0) {
        return;
    }

    m_tagNameList.removeAt(index);
}

bool NoteModelItem::hasTagName(const QString & tagName) const noexcept
{
    return m_tagNameList.contains(tagName);
}

int NoteModelItem::numTagNames() const noexcept
{
    return m_tagNameList.size();
}

QTextStream & NoteModelItem::print(QTextStream & strm) const
{
    strm << "NoteModelItem: local id = " << m_localId << ", guid = " << m_guid
         << ", notebook local id = " << m_notebookLocalId
         << ", notebook guid = " << m_notebookGuid << ", title = " << m_title
         << ", preview text = " << m_previewText << ", thumbnail "
         << (m_thumbnailData.isEmpty() ? "null" : "not null")
         << ", notebook name = " << m_notebookName
         << ", tag local uids = " << m_tagLocalIds.join(QStringLiteral(", "))
         << ", tag guids = " << m_tagGuids.join(QStringLiteral(", "))
         << ", tag name list = " << m_tagNameList.join(QStringLiteral(", "))
         << ", creation timestamp = " << m_creationTimestamp << " ("
         << printableDateTimeFromTimestamp(m_creationTimestamp)
         << "), modification timestamp = " << m_modificationTimestamp << " ("
         << printableDateTimeFromTimestamp(m_modificationTimestamp)
         << "), deletion timestamp = " << m_deletionTimestamp << " ("
         << printableDateTimeFromTimestamp(m_deletionTimestamp)
         << "), size in bytes = " << m_sizeInBytes
         << ", is synchronizable = " << (m_isSynchronizable ? "true" : "false")
         << ", is dirty = " << (m_isDirty ? "true" : "false")
         << ", is favorited = " << (m_isFavorited ? "true" : "false")
         << ", is active = " << (m_isActive ? "true" : "false")
         << ", can update title = " << (m_canUpdateTitle ? "true" : "false")
         << ", can update content = " << (m_canUpdateContent ? "true" : "false")
         << ", can email = " << (m_canEmail ? "true" : "false")
         << ", can share = " << (m_canShare ? "true" : "false")
         << ", can share publicly = "
         << (m_canSharePublicly ? "true" : "false");

    return strm;
}

} // namespace quentier
