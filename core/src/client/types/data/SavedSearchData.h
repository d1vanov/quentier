#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SAVED_SEARCH_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SAVED_SEARCH_H

#include "LocalStorageDataElementData.h"
#include <QEverCloud.h>
#include <QSharedData>

namespace qute_note {

class SavedSearchData : public LocalStorageDataElementData,
                        public QSharedData
{
public:
    SavedSearchData();
    SavedSearchData(const SavedSearchData & other);
    SavedSearchData(SavedSearchData && other);
    SavedSearchData(const qevercloud::SavedSearch & other);
    SavedSearchData(qevercloud::SavedSearch && other);
    SavedSearchData & operator=(const SavedSearchData & other);
    SavedSearchData & operator=(SavedSearchData && other);
    SavedSearchData & operator=(const qevercloud::SavedSearch & other);
    virtual ~SavedSearchData();

    void clear();
    bool checkParameters(QString & errorDescription) const;

    bool operator==(const SavedSearchData & other) const;
    bool operator!=(const SavedSearchData & other) const;

    qevercloud::SavedSearch     m_qecSearch;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SAVED_SEARCH_H
