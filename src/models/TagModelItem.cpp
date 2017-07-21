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

TagModelItem::TagModelItem(const Type::type type, const TagItem * pTagItem,
                           const TagLinkedNotebookRootItem * pTagLinkedNotebookRootItem,
                           TagModelItem * parent) :
    m_type(type),
    m_pTagItem(pTagItem),
    m_pTagLinkedNotebookRootItem(pTagLinkedNotebookRootItem)
    m_pParent(pParent),
    m_children()
{
    if (m_pParent) {
        m_pParent->addChild(this);
    }
}

TagModelItem::~TagModelItem()
{}

void TagModelItem::setParent(const TagModelItem * pParent) const
{
    m_pParent = pParent;

    int row = m_pParent->rowForChild(this);
    if (row < 0) {
        m_pParent->addChild(this);
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

void TagModelItem::insertChild(const int row, const TagModelItem * pItem) const
{
    if (Q_UNLIKELY(!pItem)) {
        return;
    }

    pItem->m_pParent = this;
    m_children.insert(row, pItem);
}

void TagModelItem::addChild(const TagModelItem * pItem) const
{
    if (Q_UNLIKELY(!pItem)) {
        return;
    }

    pItem->m_pParent = this;
    m_children.push_back(pItem);
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

    const TagModelItem * pItem = m_children.takeAt(row);
    if (pItem) {
        pItem->m_pParent = Q_NULLPTR;
    }

    return pItem;
}

QTextStream & TagModelItem::print(QTextStream & strm) const
{
    strm << QStringLiteral("Tag model item (")
         << ((m_type == Type::Tag)
             ? QStringLiteral("tag")
             : QStringLiteral("linked notebook"))
         << QStringLiteral("): ")
         << ((m_type == Type::Tag)
             ? (m_pTagItem
                ? m_pTagItem->toString()
                : QStringLiteral("<null>"))
             : (m_pTagLinkedNotebookRootItem
                ? m_pTagLinkedNotebookRootItem->toString()
                : QStringLiteral("<null>")));

    strm << QStringLiteral("\nParent item: ");
    if (Q_UNLIKELY(!m_pParent))
    {
        strm << QStringLiteral("<null>\n");
    }
    else
    {
        if (m_pParent->type() == Type::Tag) {
            strm << QStringLiteral("tag or fake root item");
        }
        else if (m_pParent->type() == Type::LinkedNotebook) {
            strm << QStringLiteral("linked notebook root item");
        }
        else {
            strm << QStringLiteral("<unknown type>");
        }

        if (m_pParent->tagItem()) {
            strm << QStringLiteral(", tag local uid = ") << m_pParent->tagItem()->localUid()
                 << QStringLiteral(", tag name = ") << m_pParent->tagItem()->name();
        }
        else if (m_pParent->tagLinkedNotebookItem()) {
            strm << QStringLiteral(", linked notebook guid = ") << m_pParent->tagLinkedNotebookItem()->linkedNotebookGuid()
                 << QStringLiteral(", linked notebook owner username = ") << m_pParent->tagLinkedNotebookItem()->username();
        }

        strm << QStringLiteral("\n");
    }

    if (m_children.isEmpty()) {
        return strm;
    }

    // TODO: print children stuff

    return strm;
}

// TODO: remake these
QDataStream & operator<<(QDataStream & out, const TagModelItem & item)
{
    out << item.m_localUid << item.m_guid << item.m_linkedNotebookGuid << item.m_name
        << item.m_parentLocalUid << item.m_parentGuid << item.m_isSynchronizable << item.m_isDirty
        << item.m_isFavorited << item.m_numNotesPerTag;

    return out;
}

QDataStream & operator>>(QDataStream & in, TagModelItem & item)
{
    in >> item.m_localUid >> item.m_guid >> item.m_linkedNotebookGuid >> item.m_name
       >> item.m_parentLocalUid >> item.m_parentGuid >> item.m_isSynchronizable >> item.m_isDirty
       >> item.m_isFavorited >> item.m_numNotesPerTag;

    return in;
}

} // namespace quentier
