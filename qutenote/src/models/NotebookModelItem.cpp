#include "NotebookModelItem.h"

namespace qute_note {

NotebookModelItem::NotebookModelItem(const Type::type type,
                                     const NotebookItem * notebookItem,
                                     const NotebookStackItem * notebookStackItem,
                                     const NotebookModelItem * parent) :
    m_type(type),
    m_notebookItem(notebookItem),
    m_notebookStackItem(notebookStackItem),
    m_parent(Q_NULLPTR),
    m_children()
{
    if (parent) {
        setParent(parent);
    }
}

NotebookModelItem::~NotebookModelItem()
{}

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

QTextStream & NotebookModelItem::print(QTextStream & strm) const
{
    strm << "Notebook model item (" << (m_type == Type::Notebook ? "notebook" : "stack")
         << "): " << (m_type == Type::Notebook
                      ? (m_notebookItem ? m_notebookItem->toString() : QString("<null>"))
                      : (m_notebookStackItem ? m_notebookStackItem->toString() : QString("<null>")));

    strm << "Parent item: ";
    if (Q_UNLIKELY(!m_parent))
    {
        strm << "<null>";
    }
    else
    {
        if (m_parent->type() == NotebookModelItem::Type::Stack) {
            strm << "stack";
        }
        else if (m_parent->type() == NotebookModelItem::Type::Notebook) {
            strm << "notebook";
        }
        else {
            strm << "<unknown type>";
        }

        if (m_parent->notebookItem()) {
            strm << ", notebook local uid = " << m_parent->notebookItem()->localUid() << ", notebook name = "
                 << m_parent->notebookItem()->name();
        }
        else if (m_parent->notebookStackItem()) {
            strm << ", stack name = " << m_parent->notebookStackItem()->name();
        }

        strm << "\n";
    }

    if (m_children.isEmpty()) {
        return strm;
    }

    int numChildren = m_children.size();
    strm << "Num children: " << numChildren << "\n";

    for(int i = 0; i < numChildren; ++i)
    {
        strm << "Child[" << i << "]: ";

        const NotebookModelItem * childItem = m_children[i];
        if (Q_UNLIKELY(!childItem)) {
            strm << "<null>";
            continue;
        }

        if (childItem->type() == NotebookModelItem::Type::Notebook) {
            strm << "notebook";
        }
        else if (childItem->type() == NotebookModelItem::Type::Stack) {
            strm << "stack";
        }
        else {
            strm << "<unknown type>";
        }

        if (childItem->notebookItem()) {
            strm << ", notebook local uid = " << childItem->notebookItem()->localUid() << ", notebook name = "
                 << childItem->notebookItem()->name();
        }
        else if (childItem->notebookStackItem()) {
            strm << ", stack = " << childItem->notebookStackItem()->name();
        }

        strm << "\n";
    }

    return strm;
}

QDataStream & operator<<(QDataStream & out, const NotebookModelItem & item)
{
    qint32 type = item.m_type;
    out << type;

    qulonglong notebookItemPtr = *(reinterpret_cast<const qulonglong*>(item.m_notebookItem));
    out << notebookItemPtr;

    qulonglong notebookStackItemPtr = *(reinterpret_cast<const qulonglong*>(item.m_notebookStackItem));
    out << notebookStackItemPtr;

    qulonglong parentItemPtr = *(reinterpret_cast<const qulonglong*>(item.m_parent));
    out << parentItemPtr;

    qint32 numChildren = item.m_children.size();
    out << numChildren;

    for(qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr = *(reinterpret_cast<const qulonglong*>(item.m_children[i]));
        out << childItemPtr;
    }

    return out;
}

QDataStream & operator>>(QDataStream & in, NotebookModelItem & item)
{
    qint32 type;
    in >> type;
    item.m_type = (type == NotebookModelItem::Type::Notebook ? NotebookModelItem::Type::Notebook : NotebookModelItem::Type::Stack);

    qulonglong notebookItemPtr;
    in >> notebookItemPtr;
    item.m_notebookItem = reinterpret_cast<const NotebookItem*>(notebookItemPtr);

    qulonglong notebookStackItemPtr;
    in >> notebookStackItemPtr;
    item.m_notebookStackItem = reinterpret_cast<const NotebookStackItem*>(notebookStackItemPtr);

    qulonglong notebookParentItemPtr;
    in >> notebookParentItemPtr;
    item.m_parent = reinterpret_cast<const NotebookModelItem*>(notebookParentItemPtr);

    qint32 numChildren;
    in >> numChildren;

    item.m_children.clear();
    item.m_children.reserve(numChildren);
    for(qint32 i = 0; i < numChildren; ++i) {
        qulonglong childItemPtr;
        in >> childItemPtr;
        item.m_children << reinterpret_cast<const NotebookModelItem*>(childItemPtr);
    }

    return in;
}

} // namespace qute_note
