/*
 * Copyright 2016 Dmitry Ivanov
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

#include "FavoritesModelItem.h"

namespace quentier {

FavoritesModelItem::FavoritesModelItem(const Type::type type,
                                       const QString & localUid,
                                       const QString & displayName,
                                       int numNotesTargeted) :
    m_type(type),
    m_localUid(localUid),
    m_displayName(displayName),
    m_numNotesTargeted(numNotesTargeted)
{}

QTextStream & FavoritesModelItem::print(QTextStream & strm) const
{
    strm << "Favorites model item: type = ";

    switch(m_type)
    {
    case Type::Notebook:
        strm << "Notebook";
        break;
    case Type::Tag:
        strm << "Tag";
        break;
    case Type::Note:
        strm << "Note";
        break;
    case Type::SavedSearch:
        strm << "Saved search";
        break;
    default:
        strm << "Unknown";
        break;
    }

    strm << "; local uid = " << m_localUid << ", display name = " << m_displayName
         << ", num notes targeted = " << m_numNotesTargeted << ";";

    return strm;
}

} // namespace quentier
