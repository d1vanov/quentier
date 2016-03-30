#include "TagModelItem.h"

namespace qute_note {

TagModelItem::TagModelItem(const QString & localUid,
                           const QString & guid,
                           const QString & linkedNotebookGuid,
                           const QString & name,
                           const QString & parentLocalUid,
                           const QString & parentGuid,
                           const bool isSynchronizable,
                           const bool isDirty,
                           TagModelItem * parent) :
    m_localUid(localUid),
    m_guid(guid),
    m_linkedNotebookGuid(linkedNotebookGuid),
    m_name(name),
    m_parentLocalUid(parentLocalUid),
    m_parentGuid(parentGuid),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty),
    m_parent(parent),
    m_children()
{
    if (m_parent) {
        m_parent->addChild(this);
    }
}

TagModelItem::~TagModelItem()
{}

void TagModelItem::setParent(const TagModelItem * parent) const
{
    m_parent = parent;

    int row = m_parent->rowForChild(this);
    if (row < 0) {
        m_parent->addChild(this);
    }
}

const TagModelItem * TagModelItem::childAtRow(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    return m_children[row];
}

int TagModelItem::rowForChild(const TagModelItem * child) const
{
    return m_children.indexOf(child);
}

void TagModelItem::insertChild(const int row, const TagModelItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->m_parent = this;
    m_children.insert(row, item);
}

void TagModelItem::addChild(const TagModelItem * item) const
{
    if (Q_UNLIKELY(!item)) {
        return;
    }

    item->m_parent = this;
    m_children.push_back(item);
}

bool TagModelItem::swapChildren(const int sourceRow, const int destRow) const
{
    if ((sourceRow < 0) || (sourceRow >= m_children.size()) ||
        (destRow < 0) || (destRow >= m_children.size()))
    {
        return false;
    }

    m_children.swap(sourceRow, destRow);
    return true;
}

const TagModelItem * TagModelItem::takeChild(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    const TagModelItem * item = m_children.takeAt(row);
    if (item) {
        item->m_parent = Q_NULLPTR;
    }

    return item;
}

QTextStream & TagModelItem::Print(QTextStream & strm) const
{
    strm << "Tag model item: local uid = " << m_localUid << ", guid = " << m_guid
         << ", linked notebook guid = " << m_linkedNotebookGuid
         << ", name = " << m_name << ", parent local uid = " << m_parentLocalUid
         << ", is synchronizable = " << (m_isSynchronizable ? "true" : "false")
         << ", is dirty = " << (m_isDirty ? "true" : "false")
         << ", parent = " << (m_parent ? m_parent->m_localUid : QString("<null>"))
         << ", children: ";
    if (m_children.isEmpty()) {
        strm << "<null> \n";
    }
    else {
        strm << "\n";
        for(auto it = m_children.begin(), end = m_children.end(); it != end; ++it) {
            const TagModelItem * child = *it;
            strm << (child ? child->m_localUid : QString("<null>")) << "\n";
        }
        strm << "\n";
    }

    return strm;
}

QDataStream & operator<<(QDataStream & out, const TagModelItem & item)
{
    out << item.m_localUid << item.m_guid << item.m_linkedNotebookGuid << item.m_name
        << item.m_parentLocalUid << item.m_parentGuid << item.m_isSynchronizable << item.m_isDirty;

    return out;
}

QDataStream & operator>>(QDataStream & in, TagModelItem & item)
{
    in >> item.m_localUid >> item.m_guid >> item.m_linkedNotebookGuid >> item.m_name
       >> item.m_parentLocalUid >> item.m_parentGuid >> item.m_isSynchronizable >> item.m_isDirty;

    return in;
}

} // namespace qute_note
