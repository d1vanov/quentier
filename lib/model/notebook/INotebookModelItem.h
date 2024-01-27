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

class INotebookModelItem : public IModelItem<INotebookModelItem>
{
public:
    enum class Type
    {
        AllNotebooksRoot,
        InvisibleRoot,
        Notebook,
        LinkedNotebook,
        Stack
    };

    friend QDebug & operator<<(QDebug & dbg, Type type);
    friend QTextStream & operator<<(QTextStream & strm, Type type);

public:
    ~INotebookModelItem() override = default;

    [[nodiscard]] virtual Type type() const = 0;

    template <typename T>
    [[nodiscard]] T * cast();

    template <typename T>
    [[nodiscard]] const T * cast() const;

    friend QDataStream & operator<<(
        QDataStream & out, const INotebookModelItem & item);

    friend QDataStream & operator>>(
        QDataStream & in, INotebookModelItem & item);
};

} // namespace quentier
