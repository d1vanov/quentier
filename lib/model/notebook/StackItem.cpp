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

#include "StackItem.h"

namespace quentier {

QTextStream & StackItem::print(QTextStream & strm) const
{
    strm << "Notebook stack: " << m_name
         << ", child count: " << m_children.size() << ", parent: " << m_pParent
         << ", parent type: "
         << (m_pParent ? static_cast<int>(m_pParent->type()) : -1);
    return strm;
}

} // namespace quentier
