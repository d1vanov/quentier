/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include <quentier/utility/Utility.h>

namespace quentier {

NoteModelItem::NoteModelItem() :
    m_localUid(),
    m_guid(),
    m_notebookLocalUid(),
    m_notebookGuid(),
    m_title(),
    m_previewText(),
    m_thumbnailData(),
    m_notebookName(),
    m_tagLocalUids(),
    m_tagGuids(),
    m_tagNameList(),
    m_creationTimestamp(-1),
    m_modificationTimestamp(-1),
    m_deletionTimestamp(-1),
    m_sizeInBytes(0),
    m_isSynchronizable(false),
    m_isDirty(true),
    m_isFavorited(false),
    m_isActive(true),
    m_hasResources(false),
    m_canUpdateTitle(true),
    m_canUpdateContent(true),
    m_canEmail(true),
    m_canShare(true),
    m_canSharePublicly(true)
{}

NoteModelItem::~NoteModelItem()
{}

void NoteModelItem::addTagLocalUid(const QString & tagLocalUid)
{
    int index = m_tagLocalUids.indexOf(tagLocalUid);
    if (index >= 0) {
        return;
    }

    m_tagLocalUids << tagLocalUid;
}

void NoteModelItem::removeTagLocalUid(const QString & tagLocalUid)
{
    int index = m_tagLocalUids.indexOf(tagLocalUid);
    if (index < 0) {
        return;
    }

    m_tagLocalUids.removeAt(index);
}

bool NoteModelItem::hasTagLocalUid(const QString & tagLocalUid) const
{
    return m_tagLocalUids.contains(tagLocalUid);
}

int NoteModelItem::numTagLocalUids() const
{
    return m_tagLocalUids.size();
}

void NoteModelItem::addTagGuid(const QString & tagGuid)
{
    int index = m_tagGuids.indexOf(tagGuid);
    if (index >= 0) {
        return;
    }

    m_tagGuids << tagGuid;
}

void NoteModelItem::removeTagGuid(const QString & tagGuid)
{
    int index = m_tagGuids.indexOf(tagGuid);
    if (index < 0) {
        return;
    }

    m_tagGuids.removeAt(index);
}

bool NoteModelItem::hasTagGuid(const QString & tagGuid) const
{
    return m_tagGuids.contains(tagGuid);
}

int NoteModelItem::numTagGuids() const
{
    return m_tagGuids.size();
}

void NoteModelItem::addTagName(const QString & tagName)
{
    int index = m_tagNameList.indexOf(tagName);
    if (index >= 0) {
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

bool NoteModelItem::hasTagName(const QString & tagName) const
{
    return m_tagNameList.contains(tagName);
}

int NoteModelItem::numTagNames() const
{
    return m_tagNameList.size();
}

QTextStream & NoteModelItem::print(QTextStream & strm) const
{
    strm << "NoteModelItem: local uid = " << m_localUid
         << ", guid = " << m_guid
         << ", notebook local uid = " << m_notebookLocalUid
         << ", notebook guid = " << m_notebookGuid
         << ", title = " << m_title
         << ", preview text = " << m_previewText
         << ", thumbnail "
         << (m_thumbnailData.isEmpty() ? "null" : "not null")
         << ", notebook name = " << m_notebookName
         << ", tag local uids = "
         << m_tagLocalUids.join(QStringLiteral(", "))
         << ", tag guids = "
         << m_tagGuids.join(QStringLiteral(", "))
         << ", tag name list = "
         << m_tagNameList.join(QStringLiteral(", "))
         << ", creation timestamp = " << m_creationTimestamp
         << " ("
         << printableDateTimeFromTimestamp(m_creationTimestamp)
         << "), modification timestamp = "
         << m_modificationTimestamp << " ("
         << printableDateTimeFromTimestamp(m_modificationTimestamp)
         << "), deletion timestamp = "
         << m_deletionTimestamp << " ("
         << printableDateTimeFromTimestamp(m_deletionTimestamp)
         << "), size in bytes = " << m_sizeInBytes
         << ", is synchronizable = "
         << (m_isSynchronizable
             ? "true"
             : "false")
         << ", is dirty = "
         << (m_isDirty ? "true" : "false")
         << ", is favorited = "
         << (m_isFavorited ? "true" : "false")
         << ", is active = "
         << (m_isActive ? "true" : "false")
         << ", can update title = "
         << (m_canUpdateTitle ? "true" : "false")
         << ", can update content = "
         << (m_canUpdateContent ? "true" : "false")
         << ", can email = "
         << (m_canEmail ? "true" : "false")
         << ", can share = "
         << (m_canShare ? "true" : "false")
         << ", can share publicly = "
         << (m_canSharePublicly ? "true" : "false");

    return strm;
}

} // namespace quentier
