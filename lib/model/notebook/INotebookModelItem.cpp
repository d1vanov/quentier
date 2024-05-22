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

#include "INotebookModelItem.h"

#include "AllNotebooksRootItem.h"
#include "InvisibleNotebookRootItem.h"
#include "LinkedNotebookRootItem.h"
#include "NotebookItem.h"
#include "StackItem.h"

#include <quentier/logging/QuentierLogger.h>

#include <QDataStream>
#include <QDebug>

#include <limits>

namespace quentier {

QDebug & operator<<(QDebug & dbg, const INotebookModelItem::Type type)
{
    using Type = INotebookModelItem::Type;

    switch (type) {
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

    qulonglong parentItemPtr = reinterpret_cast<qulonglong>(item.m_parent);
    out << parentItemPtr;

    Q_ASSERT(item.m_children.size() <= std::numeric_limits<int>::max());
    qint32 numChildren = static_cast<int>(item.m_children.size());
    out << numChildren;

    for (qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr =
            reinterpret_cast<qulonglong>(item.m_children[i]);
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
    item.m_parent = reinterpret_cast<INotebookModelItem *>(parent);

    qint32 numChildren = 0;
    in >> numChildren;

    item.m_children.clear();
    item.m_children.reserve(numChildren);
    for (qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = 0;
        in >> childItemPtr;
        item.m_children << reinterpret_cast<INotebookModelItem *>(childItemPtr);
    }

    return item.deserializeItemData(in);
}

////////////////////////////////////////////////////////////////////////////////

#define DEFINE_CAST_IMPLEMENTATION(ItemType, ItemEnum)                         \
    template <>                                                                \
    ItemType * INotebookModelItem::cast()                                      \
    {                                                                          \
        if (type() == INotebookModelItem::Type::ItemEnum) {                    \
            return dynamic_cast<ItemType *>(this);                             \
        }                                                                      \
        return nullptr;                                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const ItemType * INotebookModelItem::cast() const                          \
    {                                                                          \
        if (type() == INotebookModelItem::Type::ItemEnum) {                    \
            return dynamic_cast<const ItemType *>(this);                       \
        }                                                                      \
        return nullptr;                                                        \
    }                                                                          \
    // DEFINE_CAST_IMPLEMENTATION

DEFINE_CAST_IMPLEMENTATION(AllNotebooksRootItem, AllNotebooksRoot)
DEFINE_CAST_IMPLEMENTATION(InvisibleNotebookRootItem, InvisibleRoot)
DEFINE_CAST_IMPLEMENTATION(LinkedNotebookRootItem, LinkedNotebook)
DEFINE_CAST_IMPLEMENTATION(NotebookItem, Notebook)
DEFINE_CAST_IMPLEMENTATION(StackItem, Stack)

#undef DEFINE_CAST_IMPLEMENTATION

} // namespace quentier
