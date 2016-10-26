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

#include "TagModelItem.h"

namespace quentier {

TagModelItem::TagModelItem(const QString & localUid,
                           const QString & guid,
                           const QString & linkedNotebookGuid,
                           const QString & name,
                           const QString & parentLocalUid,
                           const QString & parentGuid,
                           const bool isSynchronizable,
                           const bool isDirty,
                           TagModelItem * parent) :
    m_localUid(localUid),
    m_guid(guid),
    m_linkedNotebookGuid(linkedNotebookGuid),
    m_name(name),
    m_parentLocalUid(parentLocalUid),
    m_parentGuid(parentGuid),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty),
    m_parent(parent),
    m_children()
{
    if (m_parent) {
        m_parent->addChild(this);
    }
}

TagModelItem::~TagModelItem()
{}

void TagModelItem::setParent(const TagModelItem * parent) const
{
    m_parent = parent;

    int row = m_parent->rowForChild(this);
    if (row < 0) {
        m_parent->addChild(this);
    }
}

const TagModelItem * TagModelItem::childAtRow(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    return m_children[row];
}

int TagModelItem::rowForChild(const TagModelItem * child) const
{
    return m_children.indexOf(child);
}

void TagModelItem::insertChild(const int row, const TagModelItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->m_parent = this;
    m_children.insert(row, item);
}

void TagModelItem::addChild(const TagModelItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->m_parent = this;
    m_children.push_back(item);
}

bool TagModelItem::swapChildren(const int sourceRow, const int destRow) const
{
    if ((sourceRow < 0) || (sourceRow >= m_children.size()) ||
        (destRow < 0) || (destRow >= m_children.size()))
    {
        return false;
    }

    m_children.swap(sourceRow, destRow);
    return true;
}

const TagModelItem * TagModelItem::takeChild(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    const TagModelItem * item = m_children.takeAt(row);
    if (item) {
        item->m_parent = Q_NULLPTR;
    }

    return item;
}

QTextStream & TagModelItem::print(QTextStream & strm) const
{
    strm << QStringLiteral("Tag model item: local uid = ") << m_localUid << QStringLiteral(", guid = ") << m_guid
         << QStringLiteral(", linked notebook guid = ") << m_linkedNotebookGuid
         << QStringLiteral(", name = ") << m_name << QStringLiteral(", parent local uid = ") << m_parentLocalUid
         << QStringLiteral(", is synchronizable = ") << (m_isSynchronizable ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", is dirty = ") << (m_isDirty ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", parent = ") << (m_parent ? m_parent->m_localUid : QStringLiteral("<null>"))
         << QStringLiteral(", children: ");
    if (m_children.isEmpty()) {
        strm << QStringLiteral("<null> \n");
    }
    else {
        strm << QStringLiteral("\n");
        for(auto it = m_children.begin(), end = m_children.end(); it != end; ++it) {
            const TagModelItem * child = *it;
            strm << (child ? child->m_localUid : QStringLiteral("<null>")) << QStringLiteral("\n");
        }
        strm << QStringLiteral("\n");
    }

    return strm;
}

QDataStream & operator<<(QDataStream & out, const TagModelItem & item)
{
    out << item.m_localUid << item.m_guid << item.m_linkedNotebookGuid << item.m_name
        << item.m_parentLocalUid << item.m_parentGuid << item.m_isSynchronizable << item.m_isDirty;

    return out;
}

QDataStream & operator>>(QDataStream & in, TagModelItem & item)
{
    in >> item.m_localUid >> item.m_guid >> item.m_linkedNotebookGuid >> item.m_name
       >> item.m_parentLocalUid >> item.m_parentGuid >> item.m_isSynchronizable >> item.m_isDirty;

    return in;
}

} // namespace quentier
