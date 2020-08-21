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

#ifndef QUENTIER_LIB_MODEL_NOTEBOOK_ITEM_H
#define QUENTIER_LIB_MODEL_NOTEBOOK_ITEM_H

#include "INotebookModelItem.h"

#include <bitset>

namespace quentier {

class NotebookItem : public INotebookModelItem
{
public:
    NotebookItem(
        QString localUid = {}, QString guid = {},
        QString linkedNotebookGuid = {}, QString name = {}, QString stack = {});

    virtual ~NotebookItem() override = default;

public:
    const QString & localUid() const
    {
        return m_localUid;
    }

    void setLocalUid(QString localUid)
    {
        m_localUid = std::move(localUid);
    }

    const QString & guid() const
    {
        return m_guid;
    }

    void setGuid(QString guid)
    {
        m_guid = std::move(guid);
    }

    const QString & linkedNotebookGuid() const
    {
        return m_linkedNotebookGuid;
    }

    void setLinkedNotebookGuid(QString linkedNotebookGuid)
    {
        m_linkedNotebookGuid = std::move(linkedNotebookGuid);
    }

    const QString & name() const
    {
        return m_name;
    }

    void setName(QString name)
    {
        m_name = std::move(name);
    }

    const QString & stack() const
    {
        return m_stack;
    }

    void setStack(QString stack)
    {
        m_stack = std::move(stack);
    }

    bool isSynchronizable() const;
    void setSynchronizable(const bool synchronizable);

    bool isUpdatable() const;
    void setUpdatable(const bool updatable);

    bool nameIsUpdatable() const;
    void setNameIsUpdatable(const bool updatable);

    bool isDirty() const;
    void setDirty(const bool dirty);

    bool isDefault() const;
    void setDefault(const bool def);

    bool isLastUsed() const;
    void setLastUsed(const bool lastUsed);

    bool isPublished() const;
    void setPublished(const bool published);

    bool isFavorited() const;
    void setFavorited(const bool favorited);

    bool canCreateNotes() const;
    void setCanCreateNotes(const bool canCreateNotes);

    bool canUpdateNotes() const;
    void setCanUpdateNotes(const bool canUpdateNotes);

    int noteCount() const
    {
        return m_noteCount;
    }

    void setNoteCount(const int noteCount)
    {
        m_noteCount = noteCount;
    }

public:
    QString nameUpper() const
    {
        return m_name.toUpper();
    }

public:
    virtual Type type() const override
    {
        return Type::Notebook;
    }

    virtual QTextStream & print(QTextStream & strm) const override;

    virtual QDataStream & serializeItemData(QDataStream & out) const override;
    virtual QDataStream & deserializeItemData(QDataStream & in) override;

private:
    QString m_localUid;
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
