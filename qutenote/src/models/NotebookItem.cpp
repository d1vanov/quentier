#include "NotebookItem.h"
#include "NotebookModelItem.h"

namespace qute_note {

NotebookItem::NotebookItem(const QString & localUid,
                           const QString & guid,
                           const QString & name,
                           const bool isSynchronizable,
                           const bool isDirty,
                           const NotebookModelItem * parent) :
    m_localUid(localUid),
    m_guid(guid),
    m_name(name),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty),
    m_parent(parent)
{
    // TODO: if have parent, add child to it
}

NotebookItem::~NotebookItem()
{}

void NotebookItem::setParent(const NotebookModelItem * parent) const
{
    m_parent = parent;

    // TODO: add this item as a child to the parent
}

QTextStream & NotebookItem::Print(QTextStream & strm) const
{
    strm << "Notebook item: local uid = " << m_localUid << ", guid = "
         << m_guid << ", name = " << m_name << ", is synchronizable = "
         << (m_isSynchronizable ? "true" : "false") << ", is dirty = "
         << (m_isDirty ? "true" : "false");
    // TODO: print parent info
    return strm;
}

QDataStream & operator<<(QDataStream & out, const NotebookItem & item)
{
    out << item.m_localUid << item.m_guid << item.m_name << item.m_isSynchronizable << item.m_isDirty;
    return out;
}

QDataStream & operator>>(QDataStream & in, NotebookItem & item)
{
    in >> item.m_localUid >> item.m_guid >> item.m_name >> item.m_isSynchronizable >> item.m_isDirty;
    return in;
}

} // namespace qute_note
