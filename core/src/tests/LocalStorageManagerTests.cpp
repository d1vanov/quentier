#include "LocalStorageManagerTests.h"
#include <logging/QuteNoteLogger.h>
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/SavedSearch.h>
#include <client/Utility.h>

namespace qute_note {
namespace test {

bool TestSavedSearchAddFindUpdateDeleteInLocalStorage(const SavedSearch & search,
                                                      LocalStorageManager & localStorageManager,
                                                      QString & errorDescription)
{
    if (!search.checkParameters(errorDescription)) {
        QNWARNING("Found invalid SavedSearch: " << search << ", error: " << errorDescription);
        return false;
    }

    // ======== Check Add + Find ============
    bool res = localStorageManager.AddSavedSearch(search, errorDescription);
    if (!res) {
        return false;
    }

    const QString searchGuid = search.guid();
    SavedSearch foundSearch;
    res = localStorageManager.FindSavedSearch(searchGuid, foundSearch, errorDescription);
    if (!res) {
        return false;
    }

    if (search != foundSearch) {
        errorDescription = QObject::tr("Added and found in local storage manager saved searches don't match");
        QNWARNING(errorDescription << ": SavedSearch added to LocalStorageManager: " << search
                  << "\nSavedSearch found in LocalStorageManager: " << foundSearch);
        return false;
    }

    // ========= Check Update + Find =============
    SavedSearch modifiedSearch(search);
    modifiedSearch.setUpdateSequenceNumber(search.updateSequenceNumber() + 1);
    modifiedSearch.setName(search.name() + "_modified");
    modifiedSearch.setQuery(search.query() + "_modified");

    res = localStorageManager.UpdateSavedSearch(modifiedSearch, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindSavedSearch(searchGuid, foundSearch, errorDescription);
    if (!res) {
        return false;
    }

    if (modifiedSearch != foundSearch) {
        errorDescription = QObject::tr("Updated and found in local storage manager saved searches don't match");
        QNWARNING(errorDescription << ": SavedSearch updated in LocalStorageManager: " << modifiedSearch
                  << "\nSavedSearch found in LocalStorageManager: " << foundSearch);
        return false;
    }

    // ============ Check Delete + Find (failure expected) ============
    res = localStorageManager.ExpungeSavedSearch(modifiedSearch, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindSavedSearch(searchGuid, foundSearch, errorDescription);
    if (res) {
        errorDescription = "Error: found SavedSearch which should have been expunged from LocalStorageManager";
        QNWARNING(errorDescription << ": SavedSearch expunged from LocalStorageManager: " << modifiedSearch
                  << "\nSavedSearch found in LocalStorageManager: " << foundSearch);
        return false;
    }

    return true;
}

}
}
