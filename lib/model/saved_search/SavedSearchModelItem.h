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

#ifndef QUENTIER_LIB_MODEL_SAVED_SEARCH_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_SAVED_SEARCH_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

namespace quentier {

class SavedSearchModelItem : public Printable
{
public:
    explicit SavedSearchModelItem(
        QString localUid = {}, QString guid = {}, QString name = {},
        QString query = {}, const bool isSynchronizable = false,
        const bool isDirty = false, const bool isFavorited = false);

    virtual ~SavedSearchModelItem() override = default;

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

    const QString & name() const
    {
        return m_name;
    }

    void setName(QString name)
    {
        m_name = std::move(name);
    }

    const QString & query() const
    {
        return m_query;
    }

    void setQuery(QString query)
    {
        m_query = std::move(query);
    }

    bool isSynchronizable() const
    {
        return m_isSynchronizable;
    }

    void setSynchronizable(const bool synchronizable)
    {
        m_isSynchronizable = synchronizable;
    }

    bool isDirty() const
    {
        return m_isDirty;
    }

    void setDirty(const bool dirty)
    {
        m_isDirty = dirty;
    }

    bool isFavorited() const
    {
        return m_isFavorited;
    }

    void setFavorited(const bool favorited)
    {
        m_isFavorited = favorited;
    }

public:

    QString nameUpper() const
    {
        return m_name.toUpper();
    }

public:
    virtual QTextStream & print(QTextStream & strm) const override;

private:
    QString m_localUid;
    QString m_guid;
    QString m_name;
    QString m_query;
    bool m_isSynchronizable;
    bool m_isDirty;
    bool m_isFavorited;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_SAVED_SEARCH_MODEL_ITEM_H
