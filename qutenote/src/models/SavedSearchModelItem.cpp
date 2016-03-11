#include "SavedSearchModelItem.h"

namespace qute_note {

SavedSearchModelItem::SavedSearchModelItem(const QString & localUid,
                                           const QString & name,
                                           const QString & query) :
    m_localUid(localUid),
    m_name(name),
    m_query(query)
{}

bool SavedSearchModelItem::operator<(const SavedSearchModelItem & other) const
{
    return (m_name.localeAwareCompare(other.m_name) < 0);
}

QTextStream & SavedSearchModelItem::Print(QTextStream & strm) const
{
    strm << "Saved search model item: local uid = " << m_localUid
         << ", name = " << m_name << ", query = " << m_query << "\n";
    return strm;
}

} // namespace qute_note
