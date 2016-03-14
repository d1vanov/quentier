#include "TagModelItem.h"

namespace qute_note {

TagModelItem::TagModelItem(const QString & localUid,
                           const QString & name,
                           const QString & parentLocalUid,
                           const bool isSynchronizable,
                           const bool isDirty) :
    m_localUid(localUid),
    m_name(name),
    m_parentLocalUid(parentLocalUid),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty)
{}

QTextStream & TagModelItem::Print(QTextStream & strm) const
{
    strm << "Tag model item: local uid = " << m_localUid << ", name = "
        << m_name << ", parent local uid = " << m_parentLocalUid
        << ", is synchronizable = " << (m_isSynchronizable ? "true" : "false")
        << ", is dirty = " << (m_isDirty ? "true" : "false") << "\n";
    return strm;
}

} // namespace qute_note
