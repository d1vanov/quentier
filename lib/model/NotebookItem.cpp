/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include <cstddef>

namespace quentier {

NotebookItem::NotebookItem(
        const QString & localUid,
        const QString & guid,
        const QString & linkedNotebookGuid,
        const QString & name,
        const QString & stack,
        const bool isSynchronizable,
        const bool isUpdatable,
        const bool nameIsUpdatable,
        const bool isDirty,
        const bool isDefault,
        const bool isLastUsed,
        const bool isPublished,
        const bool isFavorited,
        const bool canCreateNotes,
        const bool canUpdateNotes,
        const int numNotesPerNotebook) :
    m_localUid(localUid),
    m_guid(guid),
    m_linkedNotebookGuid(linkedNotebookGuid),
    m_name(name),
    m_stack(stack),
    m_flags(),
    m_numNotesPerNotebook(numNotesPerNotebook)
{
    setSynchronizable(isSynchronizable);
    setUpdatable(isUpdatable);
    setNameIsUpdatable(nameIsUpdatable);
    setDirty(isDirty);
    setDefault(isDefault);
    setLastUsed(isLastUsed);
    setPublished(isPublished);
    setFavorited(isFavorited);
    setCanCreateNotes(canCreateNotes);
    setCanUpdateNotes(canUpdateNotes);
}

NotebookItem::~NotebookItem()
{}

bool NotebookItem::isSynchronizable() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsSynchronizable));
}

void NotebookItem::setSynchronizable(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsSynchronizable), flag);
}

bool NotebookItem::isUpdatable() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsUpdatable));
}

void NotebookItem::setUpdatable(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsUpdatable), flag);
}

bool NotebookItem::nameIsUpdatable() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsNameUpdatable));
}

void NotebookItem::setNameIsUpdatable(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsNameUpdatable), flag);
}

bool NotebookItem::isDirty() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsDirty));
}

void NotebookItem::setDirty(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsDirty), flag);
}

bool NotebookItem::isDefault() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsDefault));
}

void NotebookItem::setDefault(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsDefault), flag);
}

bool NotebookItem::isLastUsed() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsLastUsed));
}

void NotebookItem::setLastUsed(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsLastUsed), flag);
}

bool NotebookItem::isPublished() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsPublished));
}

void NotebookItem::setPublished(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsPublished), flag);
}

bool NotebookItem::isFavorited() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::IsFavorited));
}

void NotebookItem::setFavorited(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::IsFavorited), flag);
}

bool NotebookItem::canCreateNotes() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::CanCreateNotes));
}

void NotebookItem::setCanCreateNotes(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::CanCreateNotes), flag);
}

bool NotebookItem::canUpdateNotes() const
{
    return m_flags.test(static_cast<std::size_t>(Flags::CanUpdateNotes));
}

void NotebookItem::setCanUpdateNotes(const bool flag)
{
    m_flags.set(static_cast<std::size_t>(Flags::CanUpdateNotes), flag);
}

QTextStream & NotebookItem::print(QTextStream & strm) const
{
    strm << "Notebook item: local uid = " << m_localUid
         << ", guid = " << m_guid
         << ", linked notebook guid = " << m_linkedNotebookGuid
         << ", name = " << m_name
         << ", stack = " << m_stack
         << ", is synchronizable = "
         << (isSynchronizable() ? "true" : "false")
         << ", is updatable = "
         << (isUpdatable() ? "true" : "false")
         << ", name is updatable = "
         << (nameIsUpdatable() ? "true" : "false")
         << ", is dirty = "
         << (isDirty() ? "true" : "false")
         << ", is default = "
         << (isDefault() ? "true" : "false")
         << ", is last used = "
         << (isLastUsed() ? "true" : "false")
         << ", is published = "
         << (isPublished() ? "true" : "false")
         << ", is favorited = "
         << (isFavorited() ? "true" : "false")
         << ", can create notes = "
         << (canCreateNotes() ? "true" : "false")
         << ", can update notes = "
         << (canUpdateNotes() ? "true" : "false")
         << ", num notes per notebook = " << m_numNotesPerNotebook;
    return strm;
}

} // namespace quentier
