/*
 * Copyright 2020 Dmitry Ivanov
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
#include "InvisibleRootItem.h"
#include "SavedSearchItem.h"

namespace quentier {

QDebug & operator<<(QDebug & dbg, const ISavedSearchModelItem::Type type)
{
    using Type = ISavedSearchModelItem::Type;

    switch (type) {
    case Type::AllSavedSearchesRoot:
        dbg << "All saved searches root";
        break;
    case Type::InvisibleRoot:
        dbg << "Invisible root";
        break;
    case Type::SavedSearch:
        dbg << "Saved search";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(type) << ")";
        break;
    }

    return dbg;
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
DEFINE_CAST_IMPLEMENTATION(InvisibleRootItem, InvisibleRoot)
DEFINE_CAST_IMPLEMENTATION(SavedSearchItem, SavedSearch)

#undef DEFINE_CAST_IMPLEMENTATION

} // namespace quentier
