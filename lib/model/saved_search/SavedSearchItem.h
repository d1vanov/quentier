/*
 * Copyright 2020-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_SAVED_SEARCH_SAVED_SEARCH_ITEM_H
#define QUENTIER_LIB_MODEL_SAVED_SEARCH_SAVED_SEARCH_ITEM_H

#include "ISavedSearchModelItem.h"

namespace quentier {

class SavedSearchItem final : public ISavedSearchModelItem
{
public:
    explicit SavedSearchItem(
        QString localId = {}, QString guid = {}, QString name = {},
        QString query = {}, bool isSynchronizable = false,
        bool isDirty = false, bool isFavorited = false);

    ~SavedSearchItem() override = default;

    [[nodiscard]] Type type() const noexcept override
    {
        return Type::SavedSearch;
    }

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

    [[nodiscard]] const QString & name() const noexcept
    {
        return m_name;
    }

    void setName(QString name)
    {
        m_name = std::move(name);
    }

    [[nodiscard]] const QString & query() const noexcept
    {
        return m_query;
    }

    void setQuery(QString query)
    {
        m_query = std::move(query);
    }

    [[nodiscard]] bool isSynchronizable() const noexcept
    {
        return m_isSynchronizable;
    }

    void setSynchronizable(const bool synchronizable)
    {
        m_isSynchronizable = synchronizable;
    }

    [[nodiscard]] bool isDirty() const noexcept
    {
        return m_isDirty;
    }

    void setDirty(const bool dirty)
    {
        m_isDirty = dirty;
    }

    [[nodiscard]] bool isFavorited() const
    {
        return m_isFavorited;
    }

    void setFavorited(const bool favorited)
    {
        m_isFavorited = favorited;
    }

public:
    [[nodiscard]] QString nameUpper() const
    {
        return m_name.toUpper();
    }

public:
    QTextStream & print(QTextStream & strm) const override;

private:
    QString m_localId;
    QString m_guid;
    QString m_name;
    QString m_query;
    bool m_isSynchronizable;
    bool m_isDirty;
    bool m_isFavorited;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_SAVED_SEARCH_SAVED_SEARCH_ITEM_H
