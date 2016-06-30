#ifndef LIB_QUENTIER_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H
#define LIB_QUENTIER_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H

#include "NoteStoreDataElementData.h"
#include <quentier/utility/Qt4Helper.h>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_FAVORITABLE_DATA_ELEMENT_DATA_H
