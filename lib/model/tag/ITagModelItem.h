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

#pragma once

#include <lib/model/common/IModelItem.h>

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

    friend QDebug & operator<<(QDebug & dbg, Type type);
    friend QTextStream & operator<<(QTextStream & strm, Type type);

public:
    ~ITagModelItem() override = default;

    [[nodiscard]] virtual Type type() const = 0;

    template <typename T>
    [[nodiscard]] T * cast();

    template <typename T>
    [[nodiscard]] const T * cast() const;

    friend QDataStream & operator<<(
        QDataStream & out, const ITagModelItem & item);

    friend QDataStream & operator>>(QDataStream & in, ITagModelItem & item);
};

} // namespace quentier
