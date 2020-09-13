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

#ifndef QUENTIER_LIB_MODEL_TAG_I_TAG_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_TAG_I_TAG_MODEL_ITEM_H

#include "../IModelItem.h"

namespace quentier {

class ITagModelItem : public IModelItem<ITagModelItem>
{
public:
    enum class Type
    {
        AllTagsRoot,
        InvisibleRoot,
        LinkedNotebook,
        Tag
    };

    friend QDebug & operator<<(QDebug & dbg, const Type type);

public:
    virtual ~ITagModelItem() = default;

    virtual Type type() const = 0;

    template <typename T>
    T * cast();

    template <typename T>
    const T * cast() const;

    friend QDataStream & operator<<(
        QDataStream & out, const ITagModelItem & item);

    friend QDataStream & operator>>(QDataStream & in, ITagModelItem & item);
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_I_TAG_MODEL_ITEM_H
