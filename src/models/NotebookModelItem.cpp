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

#include "NotebookModelItem.h"

namespace quentier {

NotebookModelItem::NotebookModelItem(const Type::type type,
                                     const NotebookItem * notebookItem,
                                     const NotebookStackItem * notebookStackItem,
                                     const NotebookLinkedNotebookRootItem * notebookLinkedNotebookItem,
                                     const NotebookModelItem * parent) :
    m_type(type),
    m_pNotebookItem(notebookItem),
    m_pNotebookStackItem(notebookStackItem),
    m_pNotebookLinkedNotebookItem(notebookLinkedNotebookItem),
    m_pParent(Q_NULLPTR),
    m_children()
{
    if (parent) {
        setParent(parent);
    }
}

NotebookModelItem::~NotebookModelItem()
{}

void NotebookModelItem::setParent(const NotebookModelItem * parent) const
{
    m_pParent = parent;

    if (m_pParent->rowForChild(this) < 0) {
        m_pParent->addChild(this);
    }
}

const NotebookModelItem * NotebookModelItem::childAtRow(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    return m_children[row];
}

int NotebookModelItem::rowForChild(const NotebookModelItem * child) const
{
    return m_children.indexOf(child);
}

void NotebookModelItem::insertChild(const int row, const NotebookModelItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->m_pParent = this;
    m_children.insert(row, item);
}

void NotebookModelItem::addChild(const NotebookModelItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->m_pParent = this;
    m_children.push_back(item);
}

bool NotebookModelItem::swapChildren(const int sourceRow, const int destRow) const
{
    if ((sourceRow < 0) || (sourceRow >= m_children.size()) ||
        (destRow < 0) || (destRow >= m_children.size()))
    {
        return false;
    }

    m_children.swap(sourceRow, destRow);
    return true;
}

const NotebookModelItem * NotebookModelItem::takeChild(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    const NotebookModelItem * item = m_children.takeAt(row);
    if (item) {
        item->m_pParent = Q_NULLPTR;
    }

    return item;
}

QTextStream & NotebookModelItem::print(QTextStream & strm) const
{
    strm << QStringLiteral("Notebook model item (")
         << (m_type == Type::Notebook
             ? QStringLiteral("notebook")
             : (m_type == Type::Stack
                ? QStringLiteral("stack")
                : QStringLiteral("linked notebook")))
         << QStringLiteral("): ")
         << (m_type == Type::Notebook
             ? (m_pNotebookItem
                ? m_pNotebookItem->toString()
                : QStringLiteral("<null>"))
             : (m_type == Type::Stack
                ? (m_pNotebookStackItem
                   ? m_pNotebookStackItem->toString()
                   : QStringLiteral("<null>"))
                : (m_pNotebookLinkedNotebookItem
                   ? m_pNotebookLinkedNotebookItem->toString()
                   : QStringLiteral("<null>"))));

    strm << QStringLiteral("\nParent item: ");
    if (Q_UNLIKELY(!m_pParent))
    {
        strm << QStringLiteral("<null>\n");
    }
    else
    {
        if (m_pParent->type() == NotebookModelItem::Type::Stack) {
            strm << QStringLiteral("stack");
        }
        else if (m_pParent->type() == NotebookModelItem::Type::Notebook) {
            strm << QStringLiteral("notebook or fake root item");
        }
        else if (m_pParent->type() == NotebookModelItem::Type::LinkedNotebook) {
            strm << QStringLiteral("linked notebook root item");
        }
        else {
            strm << QStringLiteral("<unknown type>");
        }

        if (m_pParent->notebookItem()) {
            strm << QStringLiteral(", notebook local uid = ") << m_pParent->notebookItem()->localUid()
                 << QStringLiteral(", notebook name = ") << m_pParent->notebookItem()->name();
        }
        else if (m_pParent->notebookStackItem()) {
            strm << QStringLiteral(", stack name = ") << m_pParent->notebookStackItem()->name();
        }
        else if (m_pParent->notebookLinkedNotebookItem()) {
            strm << QStringLiteral(", linked notebook guid = ") << m_pParent->notebookLinkedNotebookItem()->linkedNotebookGuid()
                 << QStringLiteral(", owner username = ") << m_pParent->notebookLinkedNotebookItem()->username();
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

        const NotebookModelItem * childItem = m_children[i];
        if (Q_UNLIKELY(!childItem)) {
            strm << QStringLiteral("<null>");
            continue;
        }

        if (childItem->type() == NotebookModelItem::Type::Notebook) {
            strm << QStringLiteral("notebook");
        }
        else if (childItem->type() == NotebookModelItem::Type::Stack) {
            strm << QStringLiteral("stack");
        }
        else if (childItem->type() == NotebookModelItem::Type::LinkedNotebook) {
            strm << QStringLiteral("linked notebook root item");
        }
        else {
            strm << QStringLiteral("<unknown type>");
        }

        if (childItem->notebookItem()) {
            strm << QStringLiteral(", notebook local uid = ") << childItem->notebookItem()->localUid()
                 << QStringLiteral(", notebook name = ") << childItem->notebookItem()->name();
        }
        else if (childItem->notebookStackItem()) {
            strm << QStringLiteral(", stack = ") << childItem->notebookStackItem()->name();
        }
        else if (childItem->notebookLinkedNotebookItem()) {
            strm << QStringLiteral(", linked notebook guid = ") << childItem->notebookLinkedNotebookItem()->linkedNotebookGuid()
                 << QStringLiteral(", owner username = ") << childItem->notebookLinkedNotebookItem()->username();
        }

        strm << QStringLiteral("\n");
    }

    return strm;
}

QDataStream & operator<<(QDataStream & out, const NotebookModelItem & item)
{
    qint32 type = item.m_type;
    out << type;

    qulonglong notebookItemPtr = reinterpret_cast<qulonglong>(item.m_pNotebookItem);
    out << notebookItemPtr;

    qulonglong notebookStackItemPtr = reinterpret_cast<qulonglong>(item.m_pNotebookStackItem);
    out << notebookStackItemPtr;

    qulonglong notebookLinkedNotebookItemPtr = reinterpret_cast<qulonglong>(item.m_pNotebookLinkedNotebookItem);
    out << notebookLinkedNotebookItemPtr;

    qulonglong parentItemPtr = reinterpret_cast<qulonglong>(item.m_pParent);
    out << parentItemPtr;

    qint32 numChildren = item.m_children.size();
    out << numChildren;

    for(qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = reinterpret_cast<qulonglong>(item.m_children[i]);
        out << childItemPtr;
    }

    return out;
}

QDataStream & operator>>(QDataStream & in, NotebookModelItem & item)
{
    qint32 type = 0;
    in >> type;
    item.m_type = (type == NotebookModelItem::Type::Notebook ? NotebookModelItem::Type::Notebook : NotebookModelItem::Type::Stack);

    qulonglong notebookItemPtr = 0;
    in >> notebookItemPtr;
    item.m_pNotebookItem = reinterpret_cast<const NotebookItem*>(notebookItemPtr);

    qulonglong notebookStackItemPtr = 0;
    in >> notebookStackItemPtr;
    item.m_pNotebookStackItem = reinterpret_cast<const NotebookStackItem*>(notebookStackItemPtr);

    qulonglong notebookLinkedNotebookItemPtr = 0;
    in >> notebookLinkedNotebookItemPtr;
    item.m_pNotebookLinkedNotebookItem = reinterpret_cast<const NotebookLinkedNotebookRootItem*>(notebookLinkedNotebookItemPtr);

    qulonglong notebookParentItemPtr = 0;
    in >> notebookParentItemPtr;
    item.m_pParent = reinterpret_cast<const NotebookModelItem*>(notebookParentItemPtr);

    qint32 numChildren = 0;
    in >> numChildren;

    item.m_children.clear();
    item.m_children.reserve(numChildren);
    for(qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = 0;
        in >> childItemPtr;
        item.m_children << reinterpret_cast<const NotebookModelItem*>(childItemPtr);
    }

    return in;
}

} // namespace quentier
