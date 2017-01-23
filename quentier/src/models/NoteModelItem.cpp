/*
 * Copyright 2016 Dmitry Ivanov
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
    m_thumbnail(),
    m_notebookName(),
    m_tagLocalUids(),
    m_tagGuids(),
    m_tagNameList(),
    m_creationTimestamp(0),
    m_modificationTimestamp(0),
    m_deletionTimestamp(0),
    m_sizeInBytes(0),
    m_isSynchronizable(false),
    m_isDirty(true),
    m_isFavorited(false),
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
    strm << QStringLiteral("NoteModelItem: local uid = ") << m_localUid << QStringLiteral(", guid = ") << m_guid
         << QStringLiteral(", notebook local uid = ") << m_notebookLocalUid << QStringLiteral(", notebook guid = ")
         << m_notebookGuid << QStringLiteral(", title = ") << m_title << QStringLiteral(", preview text = ")
         << m_previewText << QStringLiteral(", thumbnail ") << (m_thumbnail.isNull() ? QStringLiteral("null") : QStringLiteral("not null"))
         << QStringLiteral(", notebook name = ") << m_notebookName << QStringLiteral(", tag local uids = ")
         << m_tagLocalUids.join(QStringLiteral(", ")) << QStringLiteral(", tag guids = ") << m_tagGuids.join(QStringLiteral(", "))
         << QStringLiteral(", tag name list = ") << m_tagNameList.join(QStringLiteral(", ")) << QStringLiteral(", creation timestamp = ")
         << m_creationTimestamp << QStringLiteral(" (") << printableDateTimeFromTimestamp(m_creationTimestamp) << QStringLiteral(")")
         << QStringLiteral(", modification timestamp = ") << m_modificationTimestamp << QStringLiteral(" (")
         << printableDateTimeFromTimestamp(m_modificationTimestamp) << QStringLiteral(")") << QStringLiteral(", deletion timestamp = ")
         << m_deletionTimestamp << QStringLiteral(" (") << printableDateTimeFromTimestamp(m_deletionTimestamp) << QStringLiteral(")")
         << QStringLiteral(", size in bytes = ") << m_sizeInBytes << QStringLiteral(", is synchronizable = ")
         << (m_isSynchronizable ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", is dirty = ")
         << (m_isDirty ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", is favorited = ")
         << (m_isFavorited ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", can update title = ")
         << (m_canUpdateTitle ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", can update content = ")
         << (m_canUpdateContent ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", can email = ")
         << (m_canEmail ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", can share = ")
         << (m_canShare ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", can share publicly = ")
         << (m_canSharePublicly ? QStringLiteral("true") : QStringLiteral("false"));

    return strm;
}

} // namespace quentier
