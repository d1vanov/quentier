#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_ITEM_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_ITEM_H

#include "NotebookItem.h"
#include "NotebookStackItem.h"

namespace qute_note {

class NotebookModelItem: public Printable
{
public:
    struct Type
    {
        enum type {
            Stack = 0,
            Notebook
        };
    };

    NotebookModelItem(const Type::type type = Type::Notebook,
                      const NotebookItem * notebookItem = Q_NULLPTR,
                      const NotebookStackItem * notebookStackItem = Q_NULLPTR,
                      const NotebookModelItem * parent = Q_NULLPTR);
    ~NotebookModelItem();

    Type::type type() const { return m_type; }
    void setType(const Type::type type) { m_type = type; }

    const NotebookItem * notebookItem() const { return m_notebookItem; }
    void setNotebookItem(const NotebookItem * notebookItem) { m_notebookItem = notebookItem; }

    const NotebookStackItem * notebookStackItem() const { return m_notebookStackItem; }
    void setNotebookStackItem(const NotebookStackItem * notebookStackItem) { m_notebookStackItem = notebookStackItem; }

    const NotebookModelItem * parent() const { return m_parent; }
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

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    Type::type                  m_type;
    const NotebookItem *        m_notebookItem;
    const NotebookStackItem *   m_notebookStackItem;

    // NOTE: this is mutable in order to have the possibility to organize
    // the efficient storage of TagModelItems in boost::multi_index_container:
    // it doesn't allow the direct modification of its stored items,
    // however, these pointers to parent and children don't really affect
    // that container's indices
    mutable const NotebookModelItem *   m_parent;

    mutable QList<const NotebookModelItem*>  m_children;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_ITEM_H
