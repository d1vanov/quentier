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

#ifndef QUENTIER_LIB_MODEL_FAVORITES_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_FAVORITES_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

namespace quentier {

class FavoritesModelItem: public Printable
{
public:
    struct Type
    {
        enum type
        {
            Notebook = 0,
            Tag,
            Note,
            SavedSearch,
            Unknown
        };
    };

public:
    explicit FavoritesModelItem(
        const Type::type type = Type::Unknown,
        const QString & localUid = QString(),
        const QString & displayName = QString(),
        int numNotesTargeted = 0);

    Type::type type() const
    {
        return m_type;
    }

    void setType(const Type::type type)
    {
        m_type = type;
    }

    const QString & localUid() const
    {
        return m_localUid;
    }

    void setLocalUid(const QString & localUid)
    {
        m_localUid = localUid;
    }

    const QString & displayName() const
    {
        return m_displayName;
    }

    void setDisplayName(const QString & displayName)
    {
        m_displayName = displayName;
    }

    int numNotesTargeted() const
    {
        return m_numNotesTargeted;
    }

    void setNumNotesTargeted(const int numNotesTargeted)
    {
        m_numNotesTargeted = numNotesTargeted;
    }

    virtual QTextStream & print(QTextStream & strm) const override;

private:
    Type::type      m_type;
    QString         m_localUid;
    QString         m_displayName;
    int             m_numNotesTargeted;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_FAVORITES_MODEL_ITEM_H
