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

#ifndef QUENTIER_LIB_MODEL_SAVED_SEARCH_ALL_SAVED_SEARCHES_ROOT_ITEM_H
#define QUENTIER_LIB_MODEL_SAVED_SEARCH_ALL_SAVED_SEARCHES_ROOT_ITEM_H

#include "ISavedSearchModelItem.h"

namespace quentier {

class AllSavedSearchesRootItem final: public ISavedSearchModelItem
{
public:
    virtual Type type() const override
    {
        return Type::AllSavedSearchesRoot;
    }

    virtual QTextStream & print(QTextStream & strm) const override
    {
        strm << "AllSavedSearchesRootItem"
            << ", child count: " << m_children.size()
            << ", parent: " << m_pParent
            << ", parent type: "
            << (m_pParent ? static_cast<int>(m_pParent->type()) : -1);
        return strm;
    }
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_SAVED_SEARCH_ALL_SAVED_SEARCHES_ROOT_ITEM_H
