#include "NotebookModelItem.h"

namespace qute_note {

NotebookModelItem::NotebookModelItem(const Type::type type,
                                     const NotebookItem * notebookItem,
                                     const NotebookStackItem * notebookStackItem) :
    m_type(type),
    m_notebookItem(notebookItem),
    m_notebookStackItem(notebookStackItem)
{}

NotebookModelItem::~NotebookModelItem()
{
    delete m_notebookStackItem;
}

void NotebookModelItem::setParent(const NotebookModelItem * parent) const
{
    m_parent = parent;

    if (m_parent->rowForChild(this) < 0) {
        m_parent->addChild(this);
    }
}

const NotebookModelItem * NotebookModelItem::childAtRow(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    return m_children[row];
}

int NotebookModelItem::rowForChild(const NotebookModelItem * child) const
{
    return m_children.indexOf(child);
}

void NotebookModelItem::insertChild(const int row, const NotebookModelItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->m_parent = this;
    m_children.insert(row, item);
}

void NotebookModelItem::addChild(const NotebookModelItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->m_parent = this;
    m_children.push_back(item);
}

bool NotebookModelItem::swapChildren(const int sourceRow, const int destRow) const
{
    if ((sourceRow < 0) || (sourceRow >= m_children.size()) ||
        (destRow < 0) || (destRow >= m_children.size()))
    {
        return false;
    }

    m_children.swap(sourceRow, destRow);
    return true;
}

const NotebookModelItem * NotebookModelItem::takeChild(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    const NotebookModelItem * item = m_children.takeAt(row);
    if (item) {
        item->m_parent = Q_NULLPTR;
    }

    return item;
}

QTextStream & NotebookModelItem::Print(QTextStream & strm) const
{
    strm << "Notebook model item (" << (m_type == Type::Notebook ? "notebook" : "stack")
         << "): " << (m_type == Type::Notebook
                      ? (m_notebookItem ? m_notebookItem->ToQString() : QString("<null>"))
                      : (m_notebookStackItem ? m_notebookStackItem->ToQString() : QString("<null>")));

    strm << "Parent item: " << (m_parent ? m_parent->ToQString() : QString("<null>")) << "\n";

    if (m_children.isEmpty()) {
        return strm;
    }

    strm << "Children: \n";
    for(int i = 0, size = m_children.size(); i < size; ++i) {
        const NotebookModelItem * childItem = m_children[i];
        strm << "[" << i << "]: " << (childItem ? childItem->ToQString() : QString("<null>")) << "\n";
    }

    return strm;
}

} // namespace qute_note
