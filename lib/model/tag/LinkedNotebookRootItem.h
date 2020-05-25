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

#ifndef QUENTIER_LIB_MODEL_TAG_LINKED_NOTEBOOK_ROOT_ITEM_H
#define QUENTIER_LIB_MODEL_TAG_LINKED_NOTEBOOK_ROOT_ITEM_H

#include "ITagModelItem.h"

namespace quentier {

class LinkedNotebookRootItem: public ITagModelItem
{
public:
    LinkedNotebookRootItem(
        QString username = {}, QString linkedNotebookGuid = {});

    virtual ~LinkedNotebookRootItem() override = default;

    const QString & username() const
    {
        return m_username;
    }

    void setUsername(QString username)
    {
        m_username = std::move(username);
    }

    const QString & linkedNotebookGuid() const
    {
        return m_linkedNotebookGuid;
    }

    void setLinkedNotebookGuid(QString linkedNotebookGuid)
    {
        m_linkedNotebookGuid = std::move(linkedNotebookGuid);
    }

public:
    virtual Type type() const override
    {
        return Type::LinkedNotebook;
    }

    virtual QTextStream & print(QTextStream & strm) const override;

    virtual QDataStream & serializeItemData(QDataStream & out) const override
    {
        out << m_linkedNotebookGuid;
        out << m_username;
        return out;
    }

    virtual QDataStream & deserializeItemData(QDataStream & in) override
    {
        in >> m_linkedNotebookGuid;
        in >> m_username;
        return in;
    }

private:
    QString     m_username;
    QString     m_linkedNotebookGuid;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_LINKED_NOTEBOOK_ROOT_ITEM_H
