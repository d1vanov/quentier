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

#pragma once

#include "INotebookModelItem.h"

#include <bitset>
#include <cstddef>

namespace quentier {

class NotebookItem : public INotebookModelItem
{
public:
    NotebookItem(
        QString localId = {}, QString guid = {},
        QString linkedNotebookGuid = {}, QString name = {}, QString stack = {});

    ~NotebookItem() override = default;

public:
    [[nodiscard]] const QString & localId() const noexcept
    {
        return m_localId;
    }

    void setLocalId(QString localId)
    {
        m_localId = std::move(localId);
    }

    [[nodiscard]] const QString & guid() const noexcept
    {
        return m_guid;
    }

    void setGuid(QString guid)
    {
        m_guid = std::move(guid);
    }

    [[nodiscard]] const QString & linkedNotebookGuid() const noexcept
    {
        return m_linkedNotebookGuid;
    }

    void setLinkedNotebookGuid(QString linkedNotebookGuid)
    {
        m_linkedNotebookGuid = std::move(linkedNotebookGuid);
    }

    [[nodiscard]] const QString & name() const noexcept
    {
        return m_name;
    }

    void setName(QString name)
    {
        m_name = std::move(name);
    }

    [[nodiscard]] const QString & stack() const noexcept
    {
        return m_stack;
    }

    void setStack(QString stack)
    {
        m_stack = std::move(stack);
    }

    [[nodiscard]] bool isSynchronizable() const;
    void setSynchronizable(const bool synchronizable);

    [[nodiscard]] bool isUpdatable() const;
    void setUpdatable(const bool updatable);

    [[nodiscard]] bool nameIsUpdatable() const;
    void setNameIsUpdatable(const bool updatable);

    [[nodiscard]] bool isDirty() const;
    void setDirty(const bool dirty);

    [[nodiscard]] bool isDefault() const;
    void setDefault(const bool def);

    [[nodiscard]] bool isLastUsed() const;
    void setLastUsed(const bool lastUsed);

    [[nodiscard]] bool isPublished() const;
    void setPublished(const bool published);

    [[nodiscard]] bool isFavorited() const;
    void setFavorited(const bool favorited);

    [[nodiscard]] bool canCreateNotes() const;
    void setCanCreateNotes(const bool canCreateNotes);

    [[nodiscard]] bool canUpdateNotes() const;
    void setCanUpdateNotes(const bool canUpdateNotes);

    [[nodiscard]] int noteCount() const noexcept
    {
        return m_noteCount;
    }

    void setNoteCount(const int noteCount) noexcept
    {
        m_noteCount = noteCount;
    }

    [[nodiscard]] QString nameUpper() const
    {
        return m_name.toUpper();
    }

public: // INotebookModelItem
    [[nodiscard]] Type type() const noexcept override
    {
        return Type::Notebook;
    }

    QTextStream & print(QTextStream & strm) const override;

    QDataStream & serializeItemData(QDataStream & out) const override;
    QDataStream & deserializeItemData(QDataStream & in) override;

private:
    QString m_localId;
    QString m_guid;
    QString m_linkedNotebookGuid;

    QString m_name;
    QString m_stack;

    int m_noteCount = 0;

    // Using a bitset to save some space as compared to more straigforward
    // alternative of using booleans
    enum class Flags
    {
        IsSynchronizable = 0,
        IsUpdatable,
        IsNameUpdatable,
        IsDirty,
        IsDefault,
        IsLastUsed,
        IsPublished,
        IsFavorited,
        CanCreateNotes,
        CanUpdateNotes,
        Size
    };

    std::bitset<static_cast<std::size_t>(Flags::Size)> m_flags;
};

} // namespace quentier
