/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include "INotebookModelItem.h"

namespace quentier {

class StackItem : public INotebookModelItem
{
public:
    explicit StackItem(QString name = {}) : m_name{std::move(name)} {}

    ~StackItem() override = default;

    [[nodiscard]] const QString & name() const noexcept
    {
        return m_name;
    }

    void setName(QString name)
    {
        m_name = std::move(name);
    }

public:
    [[nodiscard]] Type type() const override
    {
        return Type::Stack;
    }

    QTextStream & print(QTextStream & strm) const override;

    QDataStream & serializeItemData(QDataStream & out) const override
    {
        out << m_name;
        return out;
    }

    QDataStream & deserializeItemData(QDataStream & in) override
    {
        in >> m_name;
        return in;
    }

private:
    QString m_name;
};

} // namespace quentier
