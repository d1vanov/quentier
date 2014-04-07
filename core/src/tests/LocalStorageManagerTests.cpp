#include "LocalStorageManagerTests.h"
#include <logging/QuteNoteLogger.h>
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/SavedSearch.h>
#include <client/types/LinkedNotebook.h>
#include <client/Utility.h>

namespace qute_note {
namespace test {

bool TestSavedSearchAddFindUpdateExpungeInLocalStorage(const SavedSearch & search,
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

bool TestLinkedNotebookAddFindUpdateExpungeInLocalStorage(const LinkedNotebook & linkedNotebook,
                                                          LocalStorageManager & localStorageManager,
                                                          QString & errorDescription)
{
    if (!linkedNotebook.checkParameters(errorDescription)) {
        QNWARNING("Found invalid LinkedNotebook: " << linkedNotebook << ", error: " << errorDescription);
        return false;
    }

    // ========== Check Add + Find ===========
    bool res = localStorageManager.AddLinkedNotebook(linkedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    const QString linkedNotebookGuid = linkedNotebook.guid();
    LinkedNotebook foundLinkedNotebook;
    res = localStorageManager.FindLinkedNotebook(linkedNotebookGuid, foundLinkedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    if (linkedNotebook != foundLinkedNotebook) {
        errorDescription = QObject::tr("Added and found in local storage manager linked notebooks don't match");
        QNWARNING(errorDescription << ": LinkedNotebook added to LocalStorageManager: " << linkedNotebook
                  << "\nLinkedNotebook found in LocalStorageManager: " << foundLinkedNotebook);
        return false;
    }

    // =========== Check Update + Find ===========
    LinkedNotebook modifiedLinkedNotebook(linkedNotebook);
    modifiedLinkedNotebook.setUpdateSequenceNumber(linkedNotebook.updateSequenceNumber() + 1);
    modifiedLinkedNotebook.setShareName(linkedNotebook.shareName() + "_modified");
    modifiedLinkedNotebook.setUsername(linkedNotebook.username() + "_modified");
    modifiedLinkedNotebook.setShardId(linkedNotebook.shardId() + "_modified");
    modifiedLinkedNotebook.setShareKey(linkedNotebook.shareKey() + "_modified");
    modifiedLinkedNotebook.setUri(linkedNotebook.uri() + "_modified");
    modifiedLinkedNotebook.setNoteStoreUrl(linkedNotebook.noteStoreUrl() + "_modified");
    modifiedLinkedNotebook.setWebApiUrlPrefix(linkedNotebook.webApiUrlPrefix() + "_modified");
    modifiedLinkedNotebook.setStack(linkedNotebook.stack() + "_modified");
    modifiedLinkedNotebook.setBusinessId(linkedNotebook.businessId() + 1);

    res = localStorageManager.UpdateLinkedNotebook(modifiedLinkedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindLinkedNotebook(linkedNotebookGuid, foundLinkedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    if (modifiedLinkedNotebook != foundLinkedNotebook) {
        errorDescription = QObject::tr("Updated and found in local storage manager linked notebooks don't match");
        QNWARNING(errorDescription <<": LinkedNotebook updated in LocalStorageManager: " << modifiedLinkedNotebook
                  << "\nLinkedNotebook found in LocalStorageManager: " << foundLinkedNotebook);
        return false;
    }

    // ============= Check Delete + Find (failure expected) ============
    res = localStorageManager.ExpungeLinkedNotebook(modifiedLinkedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindLinkedNotebook(linkedNotebookGuid, foundLinkedNotebook, errorDescription);
    if (res) {
        errorDescription = "Error: found LinkedNotebook which should have been expunged from LocalStorageManager";
        QNWARNING(errorDescription << ": LinkedNotebook expunged from LocalStorageManager: " << modifiedLinkedNotebook
                  << "\nLinkedNotebook found in LocalStorageManager: " << foundLinkedNotebook);
        return false;
    }

    return true;
}

}
}
