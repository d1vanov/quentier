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

#ifndef QUENTIER_LIB_MODEL_NOTEBOOK_I_NOTEBOOK_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_NOTEBOOK_I_NOTEBOOK_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

QT_FORWARD_DECLARE_CLASS(QDataStream)
QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

class INotebookModelItem : public Printable
{
public:
    enum class Type
    {
        AllNotebooksRoot,
        InvisibleRoot,
        Notebook,
        LinkedNotebook,
        Stack
    };

    friend QDebug & operator<<(QDebug & dbg, const Type type);

public:
    virtual ~INotebookModelItem() = default;

    virtual Type type() const = 0;

    INotebookModelItem * parent() const
    {
        return m_pParent;
    }

    void setParent(INotebookModelItem * pParent);

    INotebookModelItem * childAtRow(const int row) const;

    int rowForChild(const INotebookModelItem * pChild) const;

    bool hasChildren() const
    {
        return !m_children.isEmpty();
    }

    QList<INotebookModelItem *> children() const
    {
        return m_children;
    }

    int childrenCount() const
    {
        return m_children.size();
    }

    void insertChild(const int row, INotebookModelItem * pItem);
    void addChild(INotebookModelItem * pItem);
    bool swapChildren(const int srcRow, const int dstRow);

    INotebookModelItem * takeChild(const int row);

    virtual QDataStream & serializeItemData(QDataStream & out) const = 0;
    virtual QDataStream & deserializeItemData(QDataStream & in) = 0;

    friend QDataStream & operator<<(
        QDataStream & out, const INotebookModelItem & item);
    friend QDataStream & operator>>(
        QDataStream & in, INotebookModelItem & item);

    template <typename T>
    T * cast();

    template <typename T>
    const T * cast() const;

protected:
    INotebookModelItem * m_pParent = nullptr;
    QList<INotebookModelItem *> m_children;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTEBOOK_I_NOTEBOOK_MODEL_ITEM_H
