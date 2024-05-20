/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include <quentier/utility/Printable.h>

#include <algorithm>
#include <limits>

class QDataStream;

namespace quentier {

template <class TSubclass>
class IModelItem : public Printable
{
public:
    ~IModelItem() override = default;

    [[nodiscard]] TSubclass * parent() const noexcept
    {
        return m_parent;
    }

    void setParent(TSubclass * parent)
    {
        m_parent = parent;
        auto * self = static_cast<TSubclass *>(this);
        if (m_parent->rowForChild(self) < 0) {
            m_parent->addChild(self);
        }
    }

    [[nodiscard]] TSubclass * childAtRow(const int row) const noexcept
    {
        if ((row < 0) || (row >= m_children.size())) {
            return nullptr;
        }

        return m_children[row];
    }

    [[nodiscard]] int rowForChild(const TSubclass * child) const
    {
        return m_children.indexOf(const_cast<TSubclass *>(child));
    }

    [[nodiscard]] bool hasChildren() const noexcept
    {
        return !m_children.isEmpty();
    }

    [[nodiscard]] QList<TSubclass *> children() const
    {
        return m_children;
    }

    [[nodiscard]] int childrenCount() const noexcept
    {
        // FIXME: switch to qsizetype after Qt5 support is discontinued
        Q_ASSERT(m_children.size() <= std::numeric_limits<int>::max());
        return static_cast<int>(m_children.size());
    }

    void insertChild(const int row, TSubclass * item)
    {
        if (Q_UNLIKELY(!item)) {
            return;
        }

        item->m_parent = static_cast<TSubclass *>(this);
        m_children.insert(row, item);
    }

    void addChild(TSubclass * item)
    {
        insertChild(m_children.size(), item);
    }

    [[nodiscard]] bool swapChildren(const int srcRow, const int dstRow)
    {
        if ((srcRow < 0) || (srcRow >= m_children.size()) || (dstRow < 0) ||
            (dstRow >= m_children.size()))
        {
            return false;
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        m_children.swapItemsAt(srcRow, dstRow);
#else
        m_children.swap(srcRow, dstRow);
#endif

        return true;
    }

    [[nodiscard]] TSubclass * takeChild(const int row)
    {
        if ((row < 0) || (row >= m_children.size())) {
            return nullptr;
        }

        auto * item = m_children.takeAt(row);
        if (item) {
            item->m_parent = nullptr;
        }

        return item;
    }

    template <typename Comparator>
    void sortChildren(Comparator comparator)
    {
        std::sort(m_children.begin(), m_children.end(), comparator);
    }

    [[nodiscard]] virtual QDataStream & serializeItemData(
        QDataStream & out) const
    {
        return out;
    }

    [[nodiscard]] virtual QDataStream & deserializeItemData(QDataStream & in)
    {
        return in;
    }

protected:
    TSubclass * m_parent = nullptr;
    QList<TSubclass *> m_children;
};

} // namespace quentier
