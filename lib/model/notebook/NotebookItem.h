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

#ifndef QUENTIER_LIB_MODEL_NOTEBOOK_ITEM_H
#define QUENTIER_LIB_MODEL_NOTEBOOK_ITEM_H

#include "INotebookModelItem.h"

#include <bitset>

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

    [[nodiscard]] bool isSynchronizable() const noexcept;
    void setSynchronizable(bool synchronizable);

    [[nodiscard]] bool isUpdatable() const noexcept;
    void setUpdatable(bool updatable);

    [[nodiscard]] bool nameIsUpdatable() const noexcept;
    void setNameIsUpdatable(bool updatable);

    [[nodiscard]] bool isDirty() const noexcept;
    void setDirty(bool dirty);

    [[nodiscard]] bool isDefault() const noexcept;
    void setDefault(bool def);

    [[nodiscard]] bool isLastUsed() const noexcept;
    void setLastUsed(bool lastUsed);

    [[nodiscard]] bool isPublished() const noexcept;
    void setPublished(bool published);

    [[nodiscard]] bool isFavorited() const noexcept;
    void setFavorited(bool favorited);

    [[nodiscard]] bool canCreateNotes() const noexcept;
    void setCanCreateNotes(bool canCreateNotes);

    [[nodiscard]] bool canUpdateNotes() const noexcept;
    void setCanUpdateNotes(bool canUpdateNotes);

    [[nodiscard]] int noteCount() const noexcept
    {
        return m_noteCount;
    }

    void setNoteCount(int noteCount) noexcept
    {
        m_noteCount = noteCount;
    }

public:
    [[nodiscard]] QString nameUpper() const
    {
        return m_name.toUpper();
    }

public:
    Type type() const noexcept override
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
    struct Flags
    {
        enum type
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
    };

    std::bitset<Flags::Size> m_flags;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTEBOOK_ITEM_H
