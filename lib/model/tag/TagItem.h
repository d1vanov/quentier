/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_TAG_ITEM_H
#define QUENTIER_LIB_MODEL_TAG_ITEM_H

#include "ITagModelItem.h"

namespace quentier {

class TagItem : public ITagModelItem
{
public:
    explicit TagItem(
        QString localId = {}, QString guid = {},
        QString linkedNotebookGuid = {}, QString name = {},
        QString parentLocalId = {}, QString parentGuid = {});

    ~TagItem() override = default;

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

    [[nodiscard]] const QString & parentGuid() const noexcept
    {
        return m_parentGuid;
    }

    void setParentGuid(QString parentGuid)
    {
        m_parentGuid = std::move(parentGuid);
    }

    [[nodiscard]] const QString & parentLocalId() const noexcept
    {
        return m_parentLocalId;
    }

    void setParentLocalId(QString parentLocalId)
    {
        m_parentLocalId = std::move(parentLocalId);
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

    [[nodiscard]] bool isFavorited() const noexcept
    {
        return m_isFavorited;
    }

    void setFavorited(const bool favorited)
    {
        m_isFavorited = favorited;
    }

    [[nodiscard]] int noteCount() const noexcept
    {
        return m_noteCount;
    }

    void setNoteCount(const int noteCount)
    {
        m_noteCount = noteCount;
    }

public:
    [[nodiscard]] QString nameUpper() const
    {
        return m_name.toUpper();
    }

public:
    [[nodiscard]] Type type() const noexcept override
    {
        return Type::Tag;
    }

    QTextStream & print(QTextStream & strm) const override;

    QDataStream & serializeItemData(QDataStream & out) const override;
    QDataStream & deserializeItemData(QDataStream & in) override;

private:
    QString m_localId;
    QString m_guid;
    QString m_linkedNotebookGuid;
    QString m_name;
    QString m_parentLocalId;
    QString m_parentGuid;

    bool m_isSynchronizable = false;
    bool m_isDirty = false;
    bool m_isFavorited = false;
    int m_noteCount = 0;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_ITEM_H
