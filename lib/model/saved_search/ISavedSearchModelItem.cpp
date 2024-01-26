/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include "ISavedSearchModelItem.h"

#include "AllSavedSearchesRootItem.h"
#include "InvisibleSavedSearchRootItem.h"
#include "SavedSearchItem.h"

namespace quentier {

namespace {

template <class T>
void printSavedSearchModelItemType(
    const ISavedSearchModelItem::Type type, T & t)
{
    using Type = ISavedSearchModelItem::Type;

    switch (type) {
    case Type::AllSavedSearchesRoot:
        t << "All saved searches root";
        break;
    case Type::InvisibleRoot:
        t << "Invisible root";
        break;
    case Type::SavedSearch:
        t << "Saved search";
        break;
    default:
        t << "Unknown (" << static_cast<qint64>(type) << ")";
        break;
    }
}

} // namespace

QDebug & operator<<(QDebug & dbg, const ISavedSearchModelItem::Type type)
{
    printSavedSearchModelItemType(type, dbg);
    return dbg;
}

QTextStream & operator<<(
    QTextStream & strm, const ISavedSearchModelItem::Type type)
{
    printSavedSearchModelItemType(type, strm);
    return strm;
}

#define DEFINE_CAST_IMPLEMENTATION(ItemType, ItemEnum)                         \
    template <>                                                                \
    ItemType * ISavedSearchModelItem::cast()                                   \
    {                                                                          \
        if (type() == ISavedSearchModelItem::Type::ItemEnum) {                 \
            return dynamic_cast<ItemType *>(this);                             \
        }                                                                      \
        return nullptr;                                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const ItemType * ISavedSearchModelItem::cast() const                       \
    {                                                                          \
        if (type() == ISavedSearchModelItem::Type::ItemEnum) {                 \
            return dynamic_cast<const ItemType *>(this);                       \
        }                                                                      \
        return nullptr;                                                        \
    }

DEFINE_CAST_IMPLEMENTATION(AllSavedSearchesRootItem, AllSavedSearchesRoot)
DEFINE_CAST_IMPLEMENTATION(InvisibleSavedSearchRootItem, InvisibleRoot)
DEFINE_CAST_IMPLEMENTATION(SavedSearchItem, SavedSearch)

#undef DEFINE_CAST_IMPLEMENTATION

} // namespace quentier
