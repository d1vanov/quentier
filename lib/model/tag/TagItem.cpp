/*
 * Copyright 2017-2021 Dmitry Ivanov
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

TagItem::TagItem(
    QString localId, QString guid, QString linkedNotebookGuid, QString name,
    QString parentLocalId, QString parentGuid) :
    m_localId(std::move(localId)),
    m_guid(std::move(guid)),
    m_linkedNotebookGuid(std::move(linkedNotebookGuid)),
    m_name(std::move(name)),
    m_parentLocalId(std::move(parentLocalId)),
    m_parentGuid(std::move(parentGuid))
{}

QTextStream & TagItem::print(QTextStream & strm) const
{
    strm << "Tag item: local id = " << m_localId << ", guid = " << m_guid
         << ", linked notebook guid = " << m_linkedNotebookGuid
         << ", name = " << m_name << ", parent local id = " << m_parentLocalId
         << ", is synchronizable = " << (m_isSynchronizable ? "true" : "false")
         << ", is dirty = " << (m_isDirty ? "true" : "false")
         << ", is favorited = " << (m_isFavorited ? "true" : "false")
         << ", note count = " << m_noteCount << ", parent: " << m_pParent
         << ", parent type: "
         << (m_pParent ? static_cast<int>(m_pParent->type()) : -1)
         << ", child count: " << m_children.size() << ";";
    return strm;
}

QDataStream & TagItem::serializeItemData(QDataStream & out) const
{
    out << m_localId;
    out << m_guid;
    return out;
}

QDataStream & TagItem::deserializeItemData(QDataStream & in)
{
    in >> m_localId;
    in >> m_guid;
    return in;
}

} // namespace quentier
