/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include "TagItem.h"
#include "TagLinkedNotebookRootItem.h"

#include <QDataStream>
#include <QtAlgorithms>

namespace quentier {

class TagModelItem: public Printable
{
public:
    struct Type
    {
        enum type {
            Tag = 0,
            LinkedNotebook
        };
    };

    explicit TagModelItem(
        const Type::type type = Type::Tag,
        const TagItem * pTagItem = Q_NULLPTR,
        const TagLinkedNotebookRootItem * pTagLinkedNotebookRootItem = Q_NULLPTR,
        TagModelItem * parent = Q_NULLPTR);

    virtual ~TagModelItem();

    Type::type type() const { return m_type; }
    void setType(const Type::type type) { m_type = type; }

    const TagItem * tagItem() const { return m_pTagItem; }
    void setTagItem(const TagItem * pTagItem) { m_pTagItem = pTagItem; }

    const TagLinkedNotebookRootItem * tagLinkedNotebookItem() const
    { return m_pTagLinkedNotebookRootItem; }

    void setTagLinkedNotebookItem(
        const TagLinkedNotebookRootItem * pTagLinkedNotebookRootItem)
    {
        m_pTagLinkedNotebookRootItem = pTagLinkedNotebookRootItem;
    }

    const TagModelItem * parent() const { return m_pParent; }
    void setParent(const TagModelItem * parent) const;

    const TagModelItem * childAtRow(const int row) const;
    int rowForChild(const TagModelItem * child) const;

    bool hasChildren() const { return !m_children.isEmpty(); }
    QList<const TagModelItem*> children() const { return m_children; }
    int numChildren() const { return m_children.size(); }

    void insertChild(const int row, const TagModelItem * item) const;
    void addChild(const TagModelItem * item) const;
    bool swapChildren(const int sourceRow, const int destRow) const;
    const TagModelItem * takeChild(const int row) const;

    template <typename Comparator>
    void sortChildren(Comparator comparator) const
    {
        qSort(m_children.begin(), m_children.end(), comparator);
    }

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    friend QDataStream & operator<<(QDataStream & out, const TagModelItem & item);
    friend QDataStream & operator>>(QDataStream & in, TagModelItem & item);

private:
    Type::type                          m_type;
    const TagItem *                     m_pTagItem;
    const TagLinkedNotebookRootItem *   m_pTagLinkedNotebookRootItem;

    // NOTE: these are mutable in order to have the possibility to organize
    // the efficient storage of TagModelItems in boost::multi_index_container:
    // it doesn't allow the direct modification of its stored items,
    // however, these pointers to parent and children don't really affect
    // that container's indices
    mutable const TagModelItem *          m_pParent;
    mutable QList<const TagModelItem*>    m_children;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_MODEL_ITEM_H
