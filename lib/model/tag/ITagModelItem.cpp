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

#include "ITagModelItem.h"

#include "AllTagsRootItem.h"
#include "InvisibleTagRootItem.h"
#include "TagItem.h"
#include "TagLinkedNotebookRootItem.h"

#include <QDataStream>

namespace quentier {

QDebug & operator<<(QDebug & dbg, const ITagModelItem::Type type)
{
    using Type = ITagModelItem::Type;

    switch (type) {
    case Type::AllTagsRoot:
        dbg << "All tags root";
        break;
    case Type::InvisibleRoot:
        dbg << "Invisible root";
        break;
    case Type::LinkedNotebook:
        dbg << "Linked notebook";
        break;
    case Type::Tag:
        dbg << "Tag";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(type) << ")";
        break;
    }

    return dbg;
}

QDataStream & operator<<(QDataStream & out, const ITagModelItem & item)
{
    qint32 type = static_cast<qint32>(item.type());
    out << type;

    qulonglong parentItemPtr = reinterpret_cast<qulonglong>(item.parent());
    out << parentItemPtr;

    qint32 numChildren = item.children().size();
    out << numChildren;

    for (qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr =
            reinterpret_cast<qulonglong>(item.childAtRow(i));
        out << childItemPtr;
    }

    return item.serializeItemData(out);
}

QDataStream & operator>>(QDataStream & in, ITagModelItem & item)
{
    // NOTE: not reading the type here! It is assumed that client code
    // would read the type first in order to determine the type of the item
    // which needs to be created first and only then deserialized from data
    // stream

    qulonglong parentItemPtr = 0;
    in >> parentItemPtr;
    item.m_pParent = reinterpret_cast<ITagModelItem *>(parentItemPtr);

    qint32 numChildren = 0;
    in >> numChildren;

    item.m_children.clear();
    item.m_children.reserve(numChildren);
    for (qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = 0;
        in >> childItemPtr;
        item.m_children << reinterpret_cast<ITagModelItem *>(childItemPtr);
    }

    return item.deserializeItemData(in);
}

////////////////////////////////////////////////////////////////////////////////

#define DEFINE_CAST_IMPLEMENTATION(ItemType, ItemEnum)                         \
    template <>                                                                \
    ItemType * ITagModelItem::cast()                                           \
    {                                                                          \
        if (type() == ITagModelItem::Type::ItemEnum) {                         \
            return dynamic_cast<ItemType *>(this);                             \
        }                                                                      \
        return nullptr;                                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const ItemType * ITagModelItem::cast() const                               \
    {                                                                          \
        if (type() == ITagModelItem::Type::ItemEnum) {                         \
            return dynamic_cast<const ItemType *>(this);                       \
        }                                                                      \
        return nullptr;                                                        \
    }

DEFINE_CAST_IMPLEMENTATION(AllTagsRootItem, AllTagsRoot)
DEFINE_CAST_IMPLEMENTATION(InvisibleTagRootItem, InvisibleRoot)
DEFINE_CAST_IMPLEMENTATION(TagLinkedNotebookRootItem, LinkedNotebook)
DEFINE_CAST_IMPLEMENTATION(TagItem, Tag)

#undef DEFINE_CAST_IMPLEMENTATION

} // namespace quentier
