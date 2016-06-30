#include "FavoritesModelItem.h"

namespace quentier {

FavoritesModelItem::FavoritesModelItem(const Type::type type,
                                       const QString & localUid,
                                       const QString & displayName,
                                       int numNotesTargeted) :
    m_type(type),
    m_localUid(localUid),
    m_displayName(displayName),
    m_numNotesTargeted(numNotesTargeted)
{}

QTextStream & FavoritesModelItem::print(QTextStream & strm) const
{
    strm << "Favorites model item: type = ";

    switch(m_type)
    {
    case Type::Notebook:
        strm << "Notebook";
        break;
    case Type::Tag:
        strm << "Tag";
        break;
    case Type::Note:
        strm << "Note";
        break;
    case Type::SavedSearch:
        strm << "Saved search";
        break;
    default:
        strm << "Unknown";
        break;
    }

    strm << "; local uid = " << m_localUid << ", display name = " << m_displayName
         << ", num notes targeted = " << m_numNotesTargeted << ";";

    return strm;
}

} // namespace quentier
