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

#include "NotebookItem.h"

namespace quentier {

NotebookItem::NotebookItem(const QString & localUid,
                           const QString & guid,
                           const QString & name,
                           const QString & stack,
                           const bool isSynchronizable,
                           const bool isUpdatable,
                           const bool nameIsUpdatable,
                           const bool isDirty,
                           const bool isDefault,
                           const bool isPublished,
                           const bool isLinkedNotebook,
                           const int numNotesPerNotebook) :
    m_localUid(localUid),
    m_guid(guid),
    m_name(name),
    m_stack(stack),
    m_isSynchronizable(isSynchronizable),
    m_isUpdatable(isUpdatable),
    m_nameIsUpdatable(nameIsUpdatable),
    m_isDirty(isDirty),
    m_isDefault(isDefault),
    m_isPublished(isPublished),
    m_isLinkedNotebook(isLinkedNotebook),
    m_numNotesPerNotebook(numNotesPerNotebook)
{}

NotebookItem::~NotebookItem()
{}

QTextStream & NotebookItem::print(QTextStream & strm) const
{
    strm << QStringLiteral("Notebook item: local uid = ") << m_localUid << QStringLiteral(", guid = ")
         << m_guid << QStringLiteral(", name = ") << m_name << QStringLiteral(", stack = ") << m_stack
         << QStringLiteral(", is synchronizable = ") << (m_isSynchronizable ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", is updatable = ") << (m_isUpdatable ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", name is updatable = ") << (m_nameIsUpdatable ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", is dirty = ") << (m_isDirty ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", is published = ") << (m_isPublished ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", is linked notebook = ") << (m_isLinkedNotebook ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", num notes per notebook = ") << m_numNotesPerNotebook;
    return strm;
}

} // namespace quentier
