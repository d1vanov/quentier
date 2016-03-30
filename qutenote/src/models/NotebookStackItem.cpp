#include "NotebookStackItem.h"
#include "NotebookModelItem.h"

namespace qute_note {

void NotebookStackItem::setParent(const NotebookModelItem * parent) const
{
    m_parent = parent;

    for(auto it = m_notebooks.begin(), end = m_notebooks.end(); it != end; ++it) {
        (*it)->setParent(parent);
    }
}

const NotebookItem * NotebookStackItem::childAtRow(const int row) const
{
    if ((row < 0) || (row >= m_notebooks.size())) {
        return Q_NULLPTR;
    }

    return m_notebooks[row];
}

int NotebookStackItem::rowForChild(const NotebookItem * item) const
{
    return m_notebooks.indexOf(item);
}

void NotebookStackItem::insertChild(const int row, const NotebookItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->setParent(m_parent);
    m_notebooks.insert(row, item);
}

void NotebookStackItem::addChild(const NotebookItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->setParent(m_parent);
    m_notebooks.push_back(item);
}

bool NotebookStackItem::swapChildren(const int sourceRow, const int destRow) const
{
    if ((sourceRow < 0) || (sourceRow >= m_notebooks.size()) ||
        (destRow < 0) || (destRow >= m_notebooks.size()))
    {
        return false;
    }

    m_notebooks.swap(sourceRow, destRow);
    return true;
}

const NotebookItem * NotebookStackItem::takeChild(const int row) const
{
    if ((row < 0) || (row >= m_notebooks.size())) {
        return Q_NULLPTR;
    }

    const NotebookItem * item = m_notebooks.takeAt(row);
    if (item) {
        item->setParent(Q_NULLPTR);
    }

    return item;
}

QTextStream & NotebookStackItem::Print(QTextStream & strm) const
{
    strm << "Notebook stack: " << m_name;
    return strm;
}

} // namespace qute_note
