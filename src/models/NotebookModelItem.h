/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_NOTEBOOK_MODEL_ITEM_H
#define QUENTIER_MODELS_NOTEBOOK_MODEL_ITEM_H

#include "NotebookItem.h"
#include "NotebookStackItem.h"
#include "NotebookLinkedNotebookRootItem.h"
#include <QDataStream>

namespace quentier {

class NotebookModelItem: public Printable
{
public:
    struct Type
    {
        enum type {
            Stack = 0,
            Notebook,
            LinkedNotebook
        };
    };

    NotebookModelItem(const Type::type type = Type::Notebook,
                      const NotebookItem * notebookItem = Q_NULLPTR,
                      const NotebookStackItem * notebookStackItem = Q_NULLPTR,
                      const NotebookLinkedNotebookRootItem * notebookLinkedNotebookItem = Q_NULLPTR,
                      const NotebookModelItem * parent = Q_NULLPTR);
    virtual ~NotebookModelItem();

    Type::type type() const { return m_type; }
    void setType(const Type::type type) { m_type = type; }

    const NotebookItem * notebookItem() const { return m_pNotebookItem; }
    void setNotebookItem(const NotebookItem * notebookItem) { m_pNotebookItem = notebookItem; }

    const NotebookStackItem * notebookStackItem() const { return m_pNotebookStackItem; }
    void setNotebookStackItem(const NotebookStackItem * notebookStackItem) { m_pNotebookStackItem = notebookStackItem; }

    const NotebookLinkedNotebookRootItem * notebookLinkedNotebookItem() const { return m_pNotebookLinkedNotebookItem; }
    void setNotebookLinkedNotebookItem(const NotebookLinkedNotebookRootItem * notebookLinkedNotebookItem)
    { m_pNotebookLinkedNotebookItem = notebookLinkedNotebookItem; }

    const NotebookModelItem * parent() const { return m_pParent; }
    void setParent(const NotebookModelItem * parent) const;

    const NotebookModelItem * childAtRow(const int row) const;
    int rowForChild(const NotebookModelItem * child) const;

    bool hasChildren() const { return !m_children.isEmpty(); }
    QList<const NotebookModelItem*> children() const { return m_children; }
    int numChildren() const { return m_children.size(); }

    void insertChild(const int row, const NotebookModelItem * item) const;
    void addChild(const NotebookModelItem * item) const;
    bool swapChildren(const int sourceRow, const int destRow) const;

    const NotebookModelItem * takeChild(const int row) const;

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    friend QDataStream & operator<<(QDataStream & out, const NotebookModelItem & item);
    friend QDataStream & operator>>(QDataStream & in, NotebookModelItem & item);

private:
    Type::type                  m_type;
    const NotebookItem *        m_pNotebookItem;
    const NotebookStackItem *   m_pNotebookStackItem;
    const NotebookLinkedNotebookRootItem *  m_pNotebookLinkedNotebookItem;

    // NOTE: this is mutable in order to have the possibility to organize
    // the efficient storage of TagModelItems in boost::multi_index_container:
    // it doesn't allow the direct modification of its stored items,
    // however, these pointers to parent and children don't really affect
    // that container's indices
    mutable const NotebookModelItem *   m_pParent;

    mutable QList<const NotebookModelItem*>  m_children;
};

} // namespace quentier

#endif // QUENTIER_MODELS_NOTEBOOK_MODEL_ITEM_H
