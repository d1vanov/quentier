/*
 * Copyright 2017-2020 Dmitry Ivanov
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

class TagItem: public ITagModelItem
{
public:
    explicit TagItem(
        QString localUid = {},
        QString guid = {},
        QString linkedNotebookGuid = {},
        QString name = {},
        QString parentLocalUid = {},
        QString parentGuid = {});

    virtual ~TagItem() override = default;

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

    const QString & parentGuid() const
    {
        return m_parentGuid;
    }

    void setParentGuid(QString parentGuid)
    {
        m_parentGuid = std::move(parentGuid);
    }

    const QString & parentLocalUid() const
    {
        return m_parentLocalUid;
    }

    void setParentLocalUid(QString parentLocalUid)
    {
        m_parentLocalUid = std::move(parentLocalUid);
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
        return Type::Tag;
    }

    virtual QTextStream & print(QTextStream & strm) const override;

    virtual QDataStream & serializeItemData(QDataStream & out) const override;
    virtual QDataStream & deserializeItemData(QDataStream & in) override;

private:
    QString     m_localUid;
    QString     m_guid;
    QString     m_linkedNotebookGuid;
    QString     m_name;
    QString     m_parentLocalUid;
    QString     m_parentGuid;

    bool        m_isSynchronizable = false;
    bool        m_isDirty = false;
    bool        m_isFavorited = false;
    int         m_noteCount = 0;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_ITEM_H
