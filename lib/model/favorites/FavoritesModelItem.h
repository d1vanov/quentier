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

#ifndef QUENTIER_LIB_MODEL_FAVORITES_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_FAVORITES_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

namespace quentier {

class FavoritesModelItem final : public Printable
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
        Type type = Type::Unknown, QString localId = {},
        QString displayName = {}, int noteCount = 0);

    ~FavoritesModelItem() override;

    [[nodiscard]] Type type() const noexcept
    {
        return m_type;
    }

    void setType(const Type type)
    {
        m_type = type;
    }

    [[nodiscard]] const QString & localId() const noexcept
    {
        return m_localId;
    }

    void setLocalId(QString localUid)
    {
        m_localId = std::move(localUid);
    }

    [[nodiscard]] const QString & displayName() const noexcept
    {
        return m_displayName;
    }

    void setDisplayName(QString displayName)
    {
        m_displayName = std::move(displayName);
    }

    [[nodiscard]] int noteCount() const noexcept
    {
        return m_noteCount;
    }

    void setNoteCount(const int noteCount)
    {
        m_noteCount = noteCount;
    }

    QTextStream & print(QTextStream & strm) const override;

private:
    Type m_type;
    QString m_localId;
    QString m_displayName;
    int m_noteCount;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_FAVORITES_MODEL_ITEM_H
