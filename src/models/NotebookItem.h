/*
 * Copyright 20162-2019 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_NOTEBOOK_ITEM_H
#define QUENTIER_MODELS_NOTEBOOK_ITEM_H

#include <quentier/utility/Printable.h>
#include <bitset>

namespace quentier {

class NotebookItem: public Printable
{
public:
    explicit NotebookItem(const QString & localUid = QString(),
                          const QString & guid = QString(),
                          const QString & linkedNotebookGuid = QString(),
                          const QString & name = QString(),
                          const QString & stack = QString(),
                          const bool isSynchronizable = false,
                          const bool isUpdatable = false,
                          const bool nameIsUpdatable = false,
                          const bool isDirty = false,
                          const bool isDefault = false,
                          const bool isLastUsed = false,
                          const bool isPublished = false,
                          const bool isFavorited = false,
                          const bool canCreateNotes = true,
                          const bool canUpdateNotes = true,
                          const int numNotesPerNotebook = -1);
    ~NotebookItem();

    const QString & localUid() const { return m_localUid; }
    void setLocalUid(const QString & localUid) { m_localUid = localUid; }

    const QString & guid() const { return m_guid; }
    void setGuid(const QString & guid) { m_guid = guid; }

    const QString & linkedNotebookGuid() const { return m_linkedNotebookGuid; }
    void setLinkedNotebookGuid(const QString & linkedNotebookGuid)
    { m_linkedNotebookGuid = linkedNotebookGuid; }

    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    QString nameUpper() const { return m_name.toUpper(); }

    const QString & stack() const { return m_stack; }
    void setStack(const QString & stack) { m_stack = stack; }

    bool isSynchronizable() const;
    void setSynchronizable(const bool synchronizable);

    bool isUpdatable() const;
    void setUpdatable(const bool updatable);

    bool nameIsUpdatable() const;
    void setNameIsUpdatable(const bool updatable);

    bool isDirty() const;
    void setDirty(const bool dirty);

    bool isDefault() const;
    void setDefault(const bool flag);

    bool isLastUsed() const;
    void setLastUsed(const bool flag);

    bool isPublished() const;
    void setPublished(const bool flag);

    bool isFavorited() const;
    void setFavorited(const bool flag);

    bool canCreateNotes() const;
    void setCanCreateNotes(const bool flag);

    bool canUpdateNotes() const;
    void setCanUpdateNotes(const bool flag);

    int numNotesPerNotebook() const { return m_numNotesPerNotebook; }
    void setNumNotesPerNotebook(const int numNotesPerNotebook)
    { m_numNotesPerNotebook = numNotesPerNotebook; }

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString     m_localUid;
    QString     m_guid;
    QString     m_linkedNotebookGuid;
    QString     m_name;
    QString     m_stack;

    // Will use a bitset here to save some space from the more straigforward
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

    std::bitset<Flags::Size>    m_flags;

    int         m_numNotesPerNotebook;
};

} // namespace quentier

#endif // QUENTIER_MODELS_NOTEBOOK_ITEM_H
