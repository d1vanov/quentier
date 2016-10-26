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

#include "SavedSearchModelItem.h"

namespace quentier {

SavedSearchModelItem::SavedSearchModelItem(const QString & localUid,
                                           const QString & name,
                                           const QString & query,
                                           const bool isSynchronizable,
                                           const bool isDirty) :
    m_localUid(localUid),
    m_name(name),
    m_query(query),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty)
{}

QTextStream & SavedSearchModelItem::print(QTextStream & strm) const
{
    strm << QStringLiteral("Saved search model item: local uid = ") << m_localUid
         << QStringLiteral(", name = ") << m_name << QStringLiteral(", query = ") << m_query
         << QStringLiteral(", is synchronizable = ") << (m_isSynchronizable ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", is dirty = ") << (m_isDirty ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral("\n");
    return strm;
}

} // namespace quentier
