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

#ifndef QUENTIER_LIB_MODEL_TAG_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_TAG_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

QT_FORWARD_DECLARE_CLASS(QDataStream)
QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

class ITagModelItem: public Printable
{
public:
    enum class Type
    {
        AllTagsRoot,
        InvisibleRoot,
        LinkedNotebook,
        Tag
    };

    friend QDebug & operator<<(QDebug & dbg, const Type type);

public:
    virtual ~ITagModelItem() = default;

    virtual Type type() const = 0;

    ITagModelItem * parent() const
    {
        return m_pParent;
    }

    void setParent(ITagModelItem * pParent);

    ITagModelItem * childAtRow(const int row) const;

    int rowForChild(const ITagModelItem * pChild) const;

    bool hasChildren() const
    {
        return !m_children.isEmpty();
    }

    QList<ITagModelItem*> children() const
    {
        return m_children;
    }

    int childrenCount() const
    {
        return m_children.size();
    }

    void insertChild(const int row, ITagModelItem * pItem);
    void addChild(ITagModelItem * pItem);
    bool swapChildren(const int srcRow, const int dstRow);

    ITagModelItem * takeChild(const int row);

    template <typename Comparator>
    void sortChildren(Comparator comparator) const
    {
        std::sort(m_children.begin(), m_children.end(), comparator);
    }

    virtual QDataStream & serializeItemData(QDataStream & out) const = 0;
    virtual QDataStream & deserializeItemData(QDataStream & in) = 0;

    friend QDataStream & operator<<(
        QDataStream & out, const ITagModelItem & item);

    friend QDataStream & operator>>(QDataStream & in, ITagModelItem & item);

    template <typename T>
    T * cast();

    template <typename T>
    const T * cast() const;

protected:
    ITagModelItem *         m_pParent;
    QList<ITagModelItem*>   m_children;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_MODEL_ITEM_H
