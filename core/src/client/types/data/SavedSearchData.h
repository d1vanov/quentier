#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SAVED_SEARCH_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SAVED_SEARCH_H

#include "DataElementWithShortcutData.h"
#include "SynchronizableDataElementData.h"
#include <QEverCloud.h>

namespace qute_note {

class SavedSearchData: public DataElementWithShortcutData,
                       public SynchronizableDataElementData
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

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SAVED_SEARCH_H
