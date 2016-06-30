#ifndef LIB_QUENTIER_TYPES_DATA_SAVED_SEARCH_DATA_H
#define LIB_QUENTIER_TYPES_DATA_SAVED_SEARCH_DATA_H

#include "FavoritableDataElementData.h"
#include <QEverCloud.h>

namespace quentier {

class SavedSearchData: public FavoritableDataElementData
{
public:
    SavedSearchData();
    SavedSearchData(const SavedSearchData & other);
    SavedSearchData(SavedSearchData && other);
    SavedSearchData(const qevercloud::SavedSearch & other);
    SavedSearchData(qevercloud::SavedSearch && other);
    virtual ~SavedSearchData();

    void clear();
    bool checkParameters(QString & errorDescription) const;

    bool operator==(const SavedSearchData & other) const;
    bool operator!=(const SavedSearchData & other) const;

    qevercloud::SavedSearch     m_qecSearch;

private:
    SavedSearchData & operator=(const SavedSearchData & other) Q_DECL_DELETE;
    SavedSearchData & operator=(SavedSearchData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_SAVED_SEARCH_DATA_H
