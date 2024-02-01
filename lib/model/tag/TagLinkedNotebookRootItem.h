/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#pragma once

#include "ITagModelItem.h"

namespace quentier {

class TagLinkedNotebookRootItem : public ITagModelItem
{
public:
    TagLinkedNotebookRootItem(
        QString username = {}, QString linkedNotebookGuid = {});

    ~TagLinkedNotebookRootItem() override = default;

    [[nodiscard]] const QString & username() const noexcept
    {
        return m_username;
    }

    void setUsername(QString username)
    {
        m_username = std::move(username);
    }

    [[nodiscard]] const QString & linkedNotebookGuid() const noexcept
    {
        return m_linkedNotebookGuid;
    }

    void setLinkedNotebookGuid(QString linkedNotebookGuid)
    {
        m_linkedNotebookGuid = std::move(linkedNotebookGuid);
    }

public:
    [[nodiscard]] Type type() const override
    {
        return Type::LinkedNotebook;
    }

    QDataStream & serializeItemData(QDataStream & out) const override
    {
        out << m_linkedNotebookGuid;
        out << m_username;
        return out;
    }

    QDataStream & deserializeItemData(QDataStream & in) override
    {
        in >> m_linkedNotebookGuid;
        in >> m_username;
        return in;
    }

    QTextStream & print(QTextStream & strm) const override;

private:
    QString m_username;
    QString m_linkedNotebookGuid;
};

} // namespace quentier
