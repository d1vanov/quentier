#include "TagModelItem.h"

namespace qute_note {

TagModelItem::TagModelItem(const QString & localUid,
                           const QString & name,
                           const QString & parentLocalUid,
                           const bool isSynchronizable,
                           const bool isDirty,
                           TagModelItem * parent) :
    m_localUid(localUid),
    m_name(name),
    m_parentLocalUid(parentLocalUid),
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

TagModelItem * TagModelItem::childAtRow(const int row) const
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    return m_children[row];
}

int TagModelItem::rowForChild(TagModelItem * child) const
{
    return m_children.indexOf(child);
}

void TagModelItem::insertChild(const int row, TagModelItem * item)
{
    if (!item) {
        return;
    }

    item->m_parent = this;
    m_children.insert(row, item);
}

void TagModelItem::addChild(TagModelItem * item)
{
    if (!item) {
        return;
    }

    item->m_parent = this;
    m_children.push_back(item);
}

bool TagModelItem::swapChildren(const int sourceRow, const int destRow)
{
    if ((sourceRow < 0) || (sourceRow >= m_children.size()) ||
        (destRow < 0) || (destRow >= m_children.size()))
    {
        return false;
    }

    m_children.swap(sourceRow, destRow);
    return true;
}

TagModelItem * TagModelItem::takeChild(const int row)
{
    if ((row < 0) || (row >= m_children.size())) {
        return Q_NULLPTR;
    }

    TagModelItem * item = m_children.takeAt(row);
    if (item) {
        item->m_parent = Q_NULLPTR;
    }

    return item;
}

QTextStream & TagModelItem::Print(QTextStream & strm) const
{
    strm << "Tag model item: local uid = " << m_localUid << ", name = "
        << m_name << ", parent local uid = " << m_parentLocalUid
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
            TagModelItem * child = *it;
            strm << (child ? child->m_localUid : QString("<null>")) << "\n";
        }
        strm << "\n";
    }

    return strm;
}

} // namespace qute_note
