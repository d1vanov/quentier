/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include "INotebookModelItem.h"

#include "AllNotebooksRootItem.h"
#include "InvisibleRootItem.h"
#include "LinkedNotebookRootItem.h"
#include "NotebookItem.h"
#include "StackItem.h"

#include <quentier/logging/QuentierLogger.h>

#include <QDataStream>
#include <QDebug>

namespace quentier {

void INotebookModelItem::setParent(INotebookModelItem * parent)
{
    m_pParent = parent;

    if (m_pParent->rowForChild(this) < 0) {
        m_pParent->addChild(this);
    }
}

INotebookModelItem * INotebookModelItem::childAtRow(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return nullptr;
    }

    return m_children[row];
}

int INotebookModelItem::rowForChild(const INotebookModelItem * pChild) const
{
    return m_children.indexOf(const_cast<INotebookModelItem*>(pChild));
}

void INotebookModelItem::insertChild(
    const int row, INotebookModelItem * pItem)
{
    if (Q_UNLIKELY(!pItem)) {
        return;
    }

    pItem->m_pParent = this;
    m_children.insert(row, pItem);
}

void INotebookModelItem::addChild(INotebookModelItem * pItem)
{
    if (Q_UNLIKELY(!pItem)) {
        return;
    }

    pItem->m_pParent = this;
    m_children.push_back(pItem);
}

bool INotebookModelItem::swapChildren(const int srcRow, const int dstRow)
{
    if ((srcRow < 0) || (srcRow >= m_children.size()) ||
        (dstRow < 0) || (dstRow >= m_children.size()))
    {
        return false;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    m_children.swapItemsAt(srcRow, dstRow);
#else
    m_children.swap(srcRow, dstRow);
#endif

    return true;
}

INotebookModelItem * INotebookModelItem::takeChild(const int row)
{
    if ((row < 0) || (row >= m_children.size())) {
        return nullptr;
    }

    INotebookModelItem * pItem = m_children.takeAt(row);
    if (pItem) {
        pItem->m_pParent = nullptr;
    }

    return pItem;
}

////////////////////////////////////////////////////////////////////////////////

QDebug & operator<<(QDebug & dbg, const INotebookModelItem::Type type)
{
    using Type = INotebookModelItem::Type;

    switch(type)
    {
    case Type::AllNotebooksRoot:
        dbg << "All notebooks root";
        break;
    case Type::InvisibleRoot:
        dbg << "Invisible root";
        break;
    case Type::Notebook:
        dbg << "Notebook";
        break;
    case Type::LinkedNotebook:
        dbg << "Linked notebook";
        break;
    case Type::Stack:
        dbg << "Stack";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(type) << ")";
        break;
    }

    return dbg;
}

QDataStream & operator<<(QDataStream & out, const INotebookModelItem & item)
{
    qint32 type = static_cast<qint32>(item.type());
    out << type;

    qulonglong parentItemPtr = reinterpret_cast<qulonglong>(item.m_pParent);
    out << parentItemPtr;

    qint32 numChildren = item.m_children.size();
    out << numChildren;

    for(qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = reinterpret_cast<qulonglong>(item.m_children[i]);
        out << childItemPtr;
    }

    return item.serializeItemData(out);
}

QDataStream & operator>>(QDataStream & in, INotebookModelItem & item)
{
    // NOTE: not reading the type here! It is assumed that client code
    // would read the type first in order to determine the type of the item
    // which needs to be created first and only then deserialized from data
    // stream

    qulonglong parent;
    in >> parent;
    item.m_pParent = reinterpret_cast<INotebookModelItem*>(parent);

    qint32 numChildren = 0;
    in >> numChildren;

    item.m_children.clear();
    item.m_children.reserve(numChildren);
    for(qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = 0;
        in >> childItemPtr;
        item.m_children
            << reinterpret_cast<INotebookModelItem*>(childItemPtr);
    }

    return item.deserializeItemData(in);
}

////////////////////////////////////////////////////////////////////////////////

#define DEFINE_CAST_IMPLEMENTATION(ItemType, ItemEnum)                         \
template <>                                                                    \
ItemType * INotebookModelItem::cast()                                          \
{                                                                              \
    if (type() == INotebookModelItem::Type::ItemEnum) {                        \
        return dynamic_cast<ItemType*>(this);                                  \
    }                                                                          \
    return nullptr;                                                            \
}                                                                              \
                                                                               \
template <>                                                                    \
const ItemType * INotebookModelItem::cast() const                              \
{                                                                              \
    if (type() == INotebookModelItem::Type::ItemEnum) {                        \
        return dynamic_cast<const ItemType*>(this);                            \
    }                                                                          \
    return nullptr;                                                            \
}                                                                              \
// DEFINE_CAST_IMPLEMENTATION

DEFINE_CAST_IMPLEMENTATION(AllNotebooksRootItem, AllNotebooksRoot)
DEFINE_CAST_IMPLEMENTATION(InvisibleRootItem, InvisibleRoot)
DEFINE_CAST_IMPLEMENTATION(LinkedNotebookRootItem, LinkedNotebook)
DEFINE_CAST_IMPLEMENTATION(NotebookItem, Notebook)
DEFINE_CAST_IMPLEMENTATION(StackItem, Stack)

#undef DEFINE_CAST_IMPLEMENTATION

} // namespace quentier
