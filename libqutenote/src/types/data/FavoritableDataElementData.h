#ifndef LIB_QUTE_NOTE_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H
#define LIB_QUTE_NOTE_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H

#include "NoteStoreDataElementData.h"
#include <qute_note/utility/Qt4Helper.h>

namespace qute_note {

class FavoritableDataElementData: public NoteStoreDataElementData
{
public:
    FavoritableDataElementData();
    virtual ~FavoritableDataElementData();

    FavoritableDataElementData(const FavoritableDataElementData & other);
    FavoritableDataElementData(FavoritableDataElementData && other);

    bool    m_isFavorited;

private:
    FavoritableDataElementData & operator=(const FavoritableDataElementData & other) Q_DECL_DELETE;
    FavoritableDataElementData & operator=(FavoritableDataElementData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H
