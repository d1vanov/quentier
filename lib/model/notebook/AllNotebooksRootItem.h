/*
 * Copyright 2020-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_NOTEBOOK_ALL_NOTEBOOKS_ROOT_ITEM_H
#define QUENTIER_LIB_MODEL_NOTEBOOK_ALL_NOTEBOOKS_ROOT_ITEM_H

#include "INotebookModelItem.h"

namespace quentier {

class AllNotebooksRootItem final: public INotebookModelItem
{
public:
    [[nodiscard]] Type type() const noexcept override
    {
        return Type::AllNotebooksRoot;
    }

    QTextStream & print(QTextStream & strm) const override
    {
        strm << "AllNotebooksRootItem"
             << ", child count: " << m_children.size()
             << ", parent: " << m_pParent << ", parent type: "
             << (m_pParent ? static_cast<int>(m_pParent->type()) : -1);
        return strm;
    }

    QDataStream & serializeItemData(QDataStream & out) const override
    {
        return out;
    }

    QDataStream & deserializeItemData(QDataStream & in) override
    {
        return in;
    }
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTEBOOK_ALL_NOTEBOOKS_ROOT_ITEM_H
