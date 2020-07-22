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

#include "SavedSearchModelItem.h"

namespace quentier {

SavedSearchModelItem::SavedSearchModelItem(
        QString localUid,
        QString guid,
        QString name,
        QString query,
        const bool isSynchronizable,
        const bool isDirty,
        const bool isFavorited) :
    m_localUid(std::move(localUid)),
    m_guid(std::move(guid)),
    m_name(std::move(name)),
    m_query(std::move(query)),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty),
    m_isFavorited(isFavorited)
{}

QTextStream & SavedSearchModelItem::print(QTextStream & strm) const
{
    strm << "Saved search model item: local uid = " << m_localUid
        << ", guid = " << m_guid
        << ", name = " << m_name << ", query = "
        << m_query << ", is synchronizable = "
        << (m_isSynchronizable ? "true" : "false")
        << ", is dirty = "
        << (m_isDirty ? "true" : "false")
        << ", is favorited = "
        << (m_isFavorited ? "true" : "false")
        << "\n";

    return strm;
}

} // namespace quentier
