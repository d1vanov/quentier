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

#include "NotebookItem.h"

namespace quentier {

NotebookItem::NotebookItem(
    QString localId, QString guid, QString linkedNotebookGuid, QString name,
    QString stack) :
    m_localId(std::move(localId)),
    m_guid(std::move(guid)),
    m_linkedNotebookGuid(std::move(linkedNotebookGuid)),
    m_name(std::move(name)), m_stack(std::move(stack))
{
    setCanCreateNotes(true);
    setCanUpdateNotes(true);
}

bool NotebookItem::isSynchronizable() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsSynchronizable));
}

void NotebookItem::setSynchronizable(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsSynchronizable), flag);
}

bool NotebookItem::isUpdatable() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsUpdatable));
}

void NotebookItem::setUpdatable(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsUpdatable), flag);
}

bool NotebookItem::nameIsUpdatable() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsNameUpdatable));
}

void NotebookItem::setNameIsUpdatable(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsNameUpdatable), flag);
}

bool NotebookItem::isDirty() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsDirty));
}

void NotebookItem::setDirty(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsDirty), flag);
}

bool NotebookItem::isDefault() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsDefault));
}

void NotebookItem::setDefault(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsDefault), flag);
}

bool NotebookItem::isLastUsed() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsLastUsed));
}

void NotebookItem::setLastUsed(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsLastUsed), flag);
}

bool NotebookItem::isPublished() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsPublished));
}

void NotebookItem::setPublished(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsPublished), flag);
}

bool NotebookItem::isFavorited() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsFavorited));
}

void NotebookItem::setFavorited(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsFavorited), flag);
}

bool NotebookItem::canCreateNotes() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::CanCreateNotes));
}

void NotebookItem::setCanCreateNotes(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::CanCreateNotes), flag);
}

bool NotebookItem::canUpdateNotes() const noexcept
{
    return m_flags.test(static_cast<std::size_t>(Flags::CanUpdateNotes));
}

void NotebookItem::setCanUpdateNotes(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::CanUpdateNotes), flag);
}

QTextStream & NotebookItem::print(QTextStream & strm) const
{
    strm << "Notebook item: local uid = " << m_localId << ", guid = " << m_guid
         << ", linked notebook guid = " << m_linkedNotebookGuid
         << ", name = " << m_name << ", stack = " << m_stack
         << ", is synchronizable = " << (isSynchronizable() ? "true" : "false")
         << ", is updatable = " << (isUpdatable() ? "true" : "false")
         << ", name is updatable = " << (nameIsUpdatable() ? "true" : "false")
         << ", is dirty = " << (isDirty() ? "true" : "false")
         << ", is default = " << (isDefault() ? "true" : "false")
         << ", is last used = " << (isLastUsed() ? "true" : "false")
         << ", is published = " << (isPublished() ? "true" : "false")
         << ", is favorited = " << (isFavorited() ? "true" : "false")
         << ", can create notes = " << (canCreateNotes() ? "true" : "false")
         << ", can update notes = " << (canUpdateNotes() ? "true" : "false")
         << ", num notes per notebook = " << m_noteCount
         << ", parent: " << m_pParent << ", parent type: "
         << (m_pParent ? static_cast<int>(m_pParent->type()) : -1);
    return strm;
}

QDataStream & NotebookItem::serializeItemData(QDataStream & out) const
{
    out << m_localId;
    return out;
}

QDataStream & NotebookItem::deserializeItemData(QDataStream & in)
{
    in >> m_localId;
    return in;
}

} // namespace quentier
