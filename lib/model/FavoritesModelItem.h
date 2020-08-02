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

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

class FavoritesModelItem: public Printable
{
public:
    enum class Type
    {
        Notebook = 0,
        Tag,
        Note,
        SavedSearch,
        Unknown
    };

    friend QDebug & operator<<(QDebug & dbg, const Type type);

public:
    explicit FavoritesModelItem(
        const Type type = Type::Unknown,
        QString localUid = {},
        QString displayName = {},
        const int noteCount = 0);

    Type type() const
    {
        return m_type;
    }

    void setType(const Type type)
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

    int noteCount() const
    {
        return m_noteCount;
    }

    void setNoteCount(const int noteCount)
    {
        m_noteCount = noteCount;
    }

    virtual QTextStream & print(QTextStream & strm) const override;

private:
    Type            m_type;
    QString         m_localUid;
    QString         m_displayName;
    int             m_noteCount;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_FAVORITES_MODEL_ITEM_H
