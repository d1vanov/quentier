/*
 * Copyright 2016-2021 Dmitry Ivanov
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

FavoritesModelItem::FavoritesModelItem(
    const Type type, QString localId, QString displayName,
    const int noteCount) :
    m_type(type),
    m_localId(std::move(localId)), m_displayName(std::move(displayName)),
    m_noteCount(noteCount)
{}

QTextStream & FavoritesModelItem::print(QTextStream & strm) const
{
    strm << "Favorites model item: type = ";

    QString type;
    QDebug dbg(&type);
    dbg << m_type;

    strm << type << "; local id = " << m_localId
         << ", display name = " << m_displayName
         << ", note count = " << m_noteCount << ";";

    return strm;
}

QDebug & operator<<(QDebug & dbg, const FavoritesModelItem::Type type)
{
    using Type = FavoritesModelItem::Type;

    switch (type) {
    case Type::Notebook:
        dbg << "Notebook";
        break;
    case Type::Tag:
        dbg << "Tag";
        break;
    case Type::Note:
        dbg << "Note";
        break;
    case Type::SavedSearch:
        dbg << "Saved search";
        break;
    case Type::Unknown:
        dbg << "Unknown";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(type) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
