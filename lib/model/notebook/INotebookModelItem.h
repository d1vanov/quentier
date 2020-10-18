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

#ifndef QUENTIER_LIB_MODEL_NOTEBOOK_I_NOTEBOOK_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_NOTEBOOK_I_NOTEBOOK_MODEL_ITEM_H

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

    friend QDebug & operator<<(QDebug & dbg, const Type type);

public:
    virtual ~INotebookModelItem() = default;

    virtual Type type() const = 0;

    template <typename T>
    T * cast();

    template <typename T>
    const T * cast() const;

    friend QDataStream & operator<<(
        QDataStream & out, const INotebookModelItem & item);

    friend QDataStream & operator>>(
        QDataStream & in, INotebookModelItem & item);
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTEBOOK_I_NOTEBOOK_MODEL_ITEM_H
