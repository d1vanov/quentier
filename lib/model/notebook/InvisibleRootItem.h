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

#ifndef QUENTIER_LIB_MODEL_NOTEBOOK_INVISIBLE_ROOT_ITEM_H
#define QUENTIER_LIB_MODEL_NOTEBOOK_INVISIBLE_ROOT_ITEM_H

#include "INotebookModelItem.h"

namespace quentier {

class InvisibleRootItem: public INotebookModelItem
{
public:
    virtual Type type() const override
    {
        return Type::InvisibleRoot;
    }

    virtual QTextStream & print(QTextStream & strm) const override
    {
        strm << "InsivibleRootItem";
        return strm;
    }

    virtual QDataStream & serializeItemData(QDataStream & out) const override
    {
        return out;
    }

    virtual QDataStream & deserializeItemData(QDataStream & in) override
    {
        return in;
    }
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTEBOOK_INVISIBLE_ROOT_ITEM_H
