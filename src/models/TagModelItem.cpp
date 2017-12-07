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
                           TagModelItem * pParent) :
    m_type(type),
    m_pTagItem(pTagItem),
    m_pTagLinkedNotebookRootItem(pTagLinkedNotebookRootItem),
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

    int numChildren = m_children.size();
    strm << QStringLiteral("Num children: ") << numChildren << QStringLiteral("\n");

    for(int i = 0; i < numChildren; ++i)
    {
        strm << QStringLiteral("Child[") << i << QStringLiteral("]: ");

        const TagModelItem * pChildItem = m_children[i];
        if (Q_UNLIKELY(!pChildItem)) {
            strm << QStringLiteral("<null>");
            continue;
        }

        if (pChildItem->type() == TagModelItem::Type::Tag) {
            strm << QStringLiteral("tag");
        }
        else if (pChildItem->type() == TagModelItem::Type::LinkedNotebook) {
            strm << QStringLiteral("linked notebook root item");
        }
        else {
            strm << QStringLiteral("<unknown type>");
        }

        if (pChildItem->tagItem()) {
            strm << QStringLiteral(", tag local uid = ") << pChildItem->tagItem()->localUid()
                 << QStringLiteral(", tag guid = ") << pChildItem->tagItem()->guid()
                 << QStringLiteral(", tag name = ") << pChildItem->tagItem()->name();
        }
        else if (pChildItem->tagLinkedNotebookItem()) {
            strm << QStringLiteral(", linked notebook guid = ") << pChildItem->tagLinkedNotebookItem()->linkedNotebookGuid()
                 << QStringLiteral(", owner username = ") << pChildItem->tagLinkedNotebookItem()->username();
        }

        strm << QStringLiteral("\n");
    }

    return strm;
}

QDataStream & operator<<(QDataStream & out, const TagModelItem & modelItem)
{
    qint8 type = modelItem.type();
    out << type;

    qulonglong tagItemPtr = reinterpret_cast<qulonglong>(modelItem.tagItem());
    out << tagItemPtr;

    qulonglong tagLinkedNotebookItemPtr = reinterpret_cast<qulonglong>(modelItem.tagLinkedNotebookItem());
    out << tagLinkedNotebookItemPtr;

    qulonglong parentItemPtr = reinterpret_cast<qulonglong>(modelItem.parent());
    out << parentItemPtr;

    qint32 numChildren = modelItem.children().size();
    out << numChildren;

    for(qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = reinterpret_cast<qulonglong>(modelItem.childAtRow(i));
        out << childItemPtr;
    }

    return out;
}

QDataStream & operator>>(QDataStream & in, TagModelItem & modelItem)
{
    qint8 type = 0;
    in >> type;
    modelItem.m_type = static_cast<TagModelItem::Type::type>(type);

    qulonglong tagItemPtr = 0;
    in >> tagItemPtr;
    modelItem.m_pTagItem = reinterpret_cast<const TagItem*>(tagItemPtr);

    qulonglong tagLinkedNotebookItemPtr = 0;
    in >> tagLinkedNotebookItemPtr;
    modelItem.m_pTagLinkedNotebookRootItem = reinterpret_cast<const TagLinkedNotebookRootItem*>(tagLinkedNotebookItemPtr);

    qulonglong parentItemPtr = 0;
    in >> parentItemPtr;
    modelItem.m_pParent = reinterpret_cast<const TagModelItem*>(parentItemPtr);

    qint32 numChildren = 0;
    in >> numChildren;

    modelItem.m_children.clear();
    modelItem.m_children.reserve(numChildren);
    for(qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = 0;
        in >> childItemPtr;
        modelItem.m_children << reinterpret_cast<const TagModelItem*>(childItemPtr);
    }

    return in;
}

} // namespace quentier
