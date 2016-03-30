#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_STACK_ITEM_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_STACK_ITEM_H

#include "NotebookItem.h"
#include <qute_note/utility/Printable.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NotebookModelItem)

class NotebookStackItem: public Printable
{
public:
    NotebookStackItem(const QString & name = QString(),
                      const NotebookModelItem * parent = Q_NULLPTR) :
        m_name(name),
        m_parent(parent),
        m_notebooks()
    {}

    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    const NotebookModelItem * parent() const { return m_parent; }
    void setParent(const NotebookModelItem * parent) const;

    const NotebookItem * childAtRow(const int row) const;
    int rowForChild(const NotebookItem * child) const;

    bool hasChildren() const { return !m_notebooks.isEmpty(); }
    QList<const NotebookItem*> children() const { return m_notebooks; }
    int numChildren() const { return m_notebooks.size(); }

    void insertChild(const int row, const NotebookItem * item) const;
    void addChild(const NotebookItem * item) const;
    bool swapChildren(const int sourceRow, const int destRow) const;

    const NotebookItem * takeChild(const int row) const;

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString                 m_name;

    // NOTE: this is mutable in order to have the possibility to organize
    // the efficient storage of TagModelItems in boost::multi_index_container:
    // it doesn't allow the direct modification of its stored items,
    // however, these pointers to parent and children don't really affect
    // that container's indices
    mutable const NotebookModelItem *   m_parent;

    mutable QList<const NotebookItem*>  m_notebooks;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_STACK_ITEM_H
