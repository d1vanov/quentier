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

#pragma once

#include "INotebookModelItem.h"

namespace quentier {

class AllNotebooksRootItem : public INotebookModelItem
{
public: // INotebookModelItem
    [[nodiscard]] Type type() const noexcept override
    {
        return Type::AllNotebooksRoot;
    }

    QTextStream & print(QTextStream & strm) const override
    {
        strm << "AllNotebooksRootItem"
             << ", child count: " << m_children.size()
             << ", parent: " << m_parent << ", parent type: "
             << (m_parent ? static_cast<int>(m_parent->type()) : -1);
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
