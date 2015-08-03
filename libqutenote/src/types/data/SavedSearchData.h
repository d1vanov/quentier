#ifndef __LIB_QUTE_NOTE__TYPES__DATA__SAVED_SEARCH_DATA_H
#define __LIB_QUTE_NOTE__TYPES__DATA__SAVED_SEARCH_DATA_H

#include "DataElementWithShortcutData.h"
#include <QEverCloud.h>

namespace qute_note {

class SavedSearchData: public DataElementWithShortcutData
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

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__DATA__SAVED_SEARCH_DATA_H
