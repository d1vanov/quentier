/*
 * Copyright 2017 Dmitry Ivanov
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

#include "TagItem.h"

namespace quentier {

TagItem::TagItem(const QString & localUid,
                 const QString & guid,
                 const QString & linkedNotebookGuid,
                 const QString & name,
                 const QString & parentLocalUid,
                 const QString & parentGuid,
                 const bool isSynchronizable,
                 const bool isDirty,
                 const bool isFavorited,
                 const int numNotesPerTag) :
    m_localUid(localUid),
    m_guid(guid),
    m_linkedNotebookGuid(linkedNotebookGuid),
    m_name(name),
    m_parentLocalUid(parentLocalUid),
    m_parentGuid(parentGuid),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty),
    m_isFavorited(isFavorited),
    m_numNotesPerTag(numNotesPerTag)
{}

TagItem::~TagItem()
{}

QTextStream & TagItem::print(QTextStream & strm) const
{
    strm << QStringLiteral("Tag item: local uid = ") << m_localUid << QStringLiteral(", guid = ") << m_guid
         << QStringLiteral(", linked notebook guid = ") << m_linkedNotebookGuid
         << QStringLiteral(", name = ") << m_name << QStringLiteral(", parent local uid = ") << m_parentLocalUid
         << QStringLiteral(", is synchronizable = ") << (m_isSynchronizable ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", is dirty = ") << (m_isDirty ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", is favorited = ") << (m_isFavorited ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", num notes per tag = ") << m_numNotesPerTag << QStringLiteral(";");
    return strm;
}

} // namespace quentier
