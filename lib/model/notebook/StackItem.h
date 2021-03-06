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

#ifndef QUENTIER_LIB_MODEL_NOTEBOOK_STACK_ITEM_H
#define QUENTIER_LIB_MODEL_NOTEBOOK_STACK_ITEM_H

#include "INotebookModelItem.h"

namespace quentier {

class StackItem : public INotebookModelItem
{
public:
    StackItem(QString name = {}) : m_name(std::move(name)) {}

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
    [[nodiscard]] Type type() const noexcept override
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

#endif // QUENTIER_LIB_MODEL_NOTEBOOK_STACK_ITEM_H
