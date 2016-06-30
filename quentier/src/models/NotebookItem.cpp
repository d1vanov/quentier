#include "NotebookItem.h"

namespace quentier {

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

QTextStream & NotebookItem::print(QTextStream & strm) const
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

} // namespace quentier
