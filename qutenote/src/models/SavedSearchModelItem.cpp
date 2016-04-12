#include "SavedSearchModelItem.h"

namespace qute_note {

SavedSearchModelItem::SavedSearchModelItem(const QString & localUid,
                                           const QString & name,
                                           const QString & query,
                                           const bool isSynchronizable,
                                           const bool isDirty) :
    m_localUid(localUid),
    m_name(name),
    m_query(query),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty)
{}

QTextStream & SavedSearchModelItem::print(QTextStream & strm) const
{
    strm << "Saved search model item: local uid = " << m_localUid
         << ", name = " << m_name << ", query = " << m_query
         << ", is synchronizable = " << (m_isSynchronizable ? "true" : "false")
         << ", is dirty = " << (m_isDirty ? "true" : "false") << "\n";
    return strm;
}

} // namespace qute_note
