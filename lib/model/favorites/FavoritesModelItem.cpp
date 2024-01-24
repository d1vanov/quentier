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

#include "FavoritesModelItem.h"

namespace quentier {

namespace {

template <class T>
void printType(const FavoritesModelItem::Type type, T & t)
{
    using Type = FavoritesModelItem::Type;

    switch (type) {
    case Type::Notebook:
        t << "Notebook";
        break;
    case Type::Tag:
        t << "Tag";
        break;
    case Type::Note:
        t << "Note";
        break;
    case Type::SavedSearch:
        t << "Saved search";
        break;
    case Type::Unknown:
        t << "Unknown";
        break;
    default:
        t << "Unknown (" << static_cast<qint64>(type) << ")";
        break;
    }
}

} // namespace

FavoritesModelItem::FavoritesModelItem(
    const Type type, QString localId, QString displayName,
    const int noteCount) :
    m_type{type},
    m_localId{std::move(localId)}, m_displayName{std::move(displayName)},
    m_noteCount{noteCount}
{}

QTextStream & FavoritesModelItem::print(QTextStream & strm) const
{
    strm << "Favorites model item: type = " << m_type
         << "; local id = " << m_localId
         << ", display name = " << m_displayName
         << ", note count = " << m_noteCount << ";";
    return strm;
}

QDebug & operator<<(QDebug & dbg, const FavoritesModelItem::Type type)
{
    printType(type, dbg);
    return dbg;
}

QTextStream & operator<<(
    QTextStream & strm, const FavoritesModelItem::Type type)
{
    printType(type, strm);
    return strm;
}

} // namespace quentier
