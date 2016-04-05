#include "NotebookItem.h"

namespace qute_note {

NotebookItem::NotebookItem(const QString & localUid,
                           const QString & guid,
                           const QString & name,
                           const QString & stack,
                           const bool isSynchronizable,
                           const bool isUpdatable,
                           const bool isDirty,
                           const bool isDefault,
                           const bool isPublished,
                           const bool isLinkedNotebook) :
    m_localUid(localUid),
    m_guid(guid),
    m_name(name),
    m_stack(stack),
    m_isSynchronizable(isSynchronizable),
    m_isUpdatable(isUpdatable),
    m_isDirty(isDirty),
    m_isDefault(isDefault),
    m_isPublished(isPublished),
    m_isLinkedNotebook(isLinkedNotebook)
{}

NotebookItem::~NotebookItem()
{}

QTextStream & NotebookItem::Print(QTextStream & strm) const
{
    strm << "Notebook item: local uid = " << m_localUid << ", guid = "
         << m_guid << ", name = " << m_name << ", stack = " << m_stack
         << ", is synchronizable = " << (m_isSynchronizable ? "true" : "false")
         << ", is updatable = " << (m_isUpdatable ? "true" : "false")
         << ", is dirty = " << (m_isDirty ? "true" : "false")
         << ", is published = " << (m_isPublished ? "true" : "false")
         << ", is linked notebook = " << (m_isLinkedNotebook ? "true" : "false");
    return strm;
}

QDataStream & operator<<(QDataStream & out, const NotebookItem & item)
{
    out << item.m_localUid << item.m_guid << item.m_name << item.m_stack << item.m_isSynchronizable << item.m_isUpdatable
        << item.m_isDirty << item.m_isDefault << item.m_isPublished << item.m_isLinkedNotebook;
    return out;
}

QDataStream & operator>>(QDataStream & in, NotebookItem & item)
{
    in >> item.m_localUid >> item.m_guid >> item.m_name >> item.m_stack >> item.m_isSynchronizable >> item.m_isUpdatable
       >> item.m_isDirty >> item.m_isDefault >> item.m_isPublished >> item.m_isLinkedNotebook;
    return in;
}

} // namespace qute_note
