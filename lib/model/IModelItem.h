/*
 * Copyright 2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_I_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_I_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

QT_FORWARD_DECLARE_CLASS(QDataStream)
QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

template <class TSubclass>
class IModelItem : public Printable
{
public:
    virtual ~IModelItem() = default;

    TSubclass * parent() const
    {
        return m_pParent;
    }

    void setParent(TSubclass * pParent)
    {
        m_pParent = pParent;

        auto * self = static_cast<TSubclass *>(this);

        if (m_pParent->rowForChild(self) < 0) {
            m_pParent->addChild(self);
        }
    }

    TSubclass * childAtRow(const int row) const
    {
        if ((row < 0) || (row >= m_children.size())) {
            return nullptr;
        }

        return m_children[row];
    }

    int rowForChild(const TSubclass * pChild) const
    {
        return m_children.indexOf(const_cast<TSubclass *>(pChild));
    }

    bool hasChildren() const
    {
        return !m_children.isEmpty();
    }

    QList<TSubclass *> children() const
    {
        return m_children;
    }

    int childrenCount() const
    {
        return m_children.size();
    }

    void insertChild(const int row, TSubclass * pItem)
    {
        if (Q_UNLIKELY(!pItem)) {
            return;
        }

        pItem->m_pParent = static_cast<TSubclass* >(this);
        m_children.insert(row, pItem);
    }

    void addChild(TSubclass * pItem)
    {
        insertChild(m_children.size(), pItem);
    }

    bool swapChildren(const int srcRow, const int dstRow)
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

    TSubclass * takeChild(const int row)
    {
        if ((row < 0) || (row >= m_children.size())) {
            return nullptr;
        }

        auto * pItem = m_children.takeAt(row);
        if (pItem) {
            pItem->m_pParent = nullptr;
        }

        return pItem;
    }

    template <typename Comparator>
    void sortChildren(Comparator comparator)
    {
        std::sort(m_children.begin(), m_children.end(), comparator);
    }

    virtual QDataStream & serializeItemData(QDataStream & out) const
    {
        return out;
    }

    virtual QDataStream & deserializeItemData(QDataStream & in)
    {
        return in;
    }

protected:
    TSubclass * m_pParent = nullptr;
    QList<TSubclass *> m_children;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_I_MODEL_ITEM_H
