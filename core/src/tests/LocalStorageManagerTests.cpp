#include "LocalStorageManagerTests.h"
#include <logging/QuteNoteLogger.h>
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/SavedSearch.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/Note.h>
#include <client/types/Notebook.h>
#include <client/types/UserWrapper.h>
#include <client/types/QEverCloudHelpers.h>
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

    const QString searchGuid = search.localGuid();
    SavedSearch foundSearch;
    foundSearch.setLocalGuid(searchGuid);
    res = localStorageManager.FindSavedSearch(foundSearch, errorDescription);
    if (!res) {
        return false;
    }

    if (search != foundSearch) {
        errorDescription = "Added and found saved searches in local storage don't match";
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

    res = localStorageManager.FindSavedSearch(foundSearch, errorDescription);
    if (!res) {
        return false;
    }

    if (modifiedSearch != foundSearch) {
        errorDescription = "Updated and found saved searches in local storage don't match";
        QNWARNING(errorDescription << ": SavedSearch updated in LocalStorageManager: " << modifiedSearch
                  << "\nSavedSearch found in LocalStorageManager: " << foundSearch);
        return false;
    }

    // ========== Check GetSavedSearchCount to return 1 ============
    int count = localStorageManager.GetSavedSearchCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 1) {
        errorDescription = "GetSavedSearchCount returned result different from the expected one (1): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ============ Check Expunge + Find (failure expected) ============
    res = localStorageManager.ExpungeSavedSearch(modifiedSearch, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindSavedSearch(foundSearch, errorDescription);
    if (res) {
        errorDescription = "Error: found saved search which should have been expunged from local storage";
        QNWARNING(errorDescription << ": SavedSearch expunged from LocalStorageManager: " << modifiedSearch
                  << "\nSavedSearch found in LocalStorageManager: " << foundSearch);
        return false;
    }

    // ========== Check GetSavedSearchCount to return 0 ============
    count = localStorageManager.GetSavedSearchCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 0) {
        errorDescription = "GetSavedSearchCount returned result different from the expected one (0): ";
        errorDescription += QString::number(count);
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
    foundLinkedNotebook.setGuid(linkedNotebookGuid);
    res = localStorageManager.FindLinkedNotebook(foundLinkedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    if (linkedNotebook != foundLinkedNotebook) {
        errorDescription = "Added and found linked noteboks in local storage don't match";
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

    res = localStorageManager.FindLinkedNotebook(foundLinkedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    if (modifiedLinkedNotebook != foundLinkedNotebook) {
        errorDescription = "Updated and found linked notebooks in local storage don't match";
        QNWARNING(errorDescription << ": LinkedNotebook updated in LocalStorageManager: " << modifiedLinkedNotebook
                  << "\nLinkedNotebook found in LocalStorageManager: " << foundLinkedNotebook);
        return false;
    }

    // ========== Check GetLinkedNotebookCount to return 1 ============
    int count = localStorageManager.GetLinkedNotebookCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 1) {
        errorDescription = "GetLinkedNotebookCount returned result different from the expected one (1): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ============= Check Expunge + Find (failure expected) ============
    res = localStorageManager.ExpungeLinkedNotebook(modifiedLinkedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindLinkedNotebook(foundLinkedNotebook, errorDescription);
    if (res) {
        errorDescription = "Error: found linked notebook which should have been expunged from local storage";
        QNWARNING(errorDescription << ": LinkedNotebook expunged from LocalStorageManager: " << modifiedLinkedNotebook
                  << "\nLinkedNotebook found in LocalStorageManager: " << foundLinkedNotebook);
        return false;
    }

    // ========== Check GetLinkedNotebookCount to return 0 ============
    count = localStorageManager.GetLinkedNotebookCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 0) {
        errorDescription = "GetLinkedNotebookCount returned result different from the expected one (0): ";
        errorDescription += QString::number(count);
        return false;
    }

    return true;
}

bool TestTagAddFindUpdateExpungeInLocalStorage(const Tag & tag,
                                               LocalStorageManager & localStorageManager,
                                               QString & errorDescription)
{
    if (!tag.checkParameters(errorDescription)) {
        QNWARNING("Found invalid Tag: " << tag << ", error: " << errorDescription);
        return false;
    }

    // ========== Check Add + Find ==========
    bool res = localStorageManager.AddTag(tag, errorDescription);
    if (!res) {
        return false;
    }

    const QString localTagGuid = tag.localGuid();
    Tag foundTag;
    foundTag.setLocalGuid(localTagGuid);
    res = localStorageManager.FindTag(foundTag, errorDescription);
    if (!res) {
        return false;
    }

    if (tag != foundTag) {
        errorDescription = "Added and found tags in local storage tags don't match";
        QNWARNING(errorDescription << ": Tag added to LocalStorageManager: " << tag
                  << "\nTag found in LocalStorageManager: " << foundTag);
        return false;
    }

    // ========== Check Update + Find ==========
    Tag modifiedTag(tag);
    modifiedTag.setUpdateSequenceNumber(tag.updateSequenceNumber() + 1);
    modifiedTag.setName(tag.name() + "_modified");
    modifiedTag.unsetLocalGuid();

    res = localStorageManager.UpdateTag(modifiedTag, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindTag(foundTag, errorDescription);
    if (!res) {
        return false;
    }

    modifiedTag.setLocalGuid(localTagGuid);
    if (modifiedTag != foundTag) {
        errorDescription = "Updated and found tags in local storage don't match";
        QNWARNING(errorDescription << ": Tag updated in LocalStorageManaged: " << modifiedTag
                  << "\nTag found in LocalStorageManager: " << foundTag);
        return false;
    }

    // ========== GetTagCount to return 1 ============
    int count = localStorageManager.GetTagCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 1) {
        errorDescription = "GetTagCount returned result different from the expected one (1): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Delete + Find ==========
    modifiedTag.setDeleted(true);
    modifiedTag.setLocal(false);
    res = localStorageManager.DeleteTag(modifiedTag, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindTag(foundTag, errorDescription);
    if (!res) {
        return false;
    }

    if (!foundTag.isDeleted()) {
        errorDescription = "Tag which should have been marked as deleted one "
                           "is not marked so in the result of FindTag";
        QNWARNING(errorDescription << ": deleted Tag which should have been found in LocalStorageManager: "
                  << modifiedTag << "\nTag which should have actually been found: "
                  << foundTag);
        return false;
    }

    // ========== GetTagCount to return 0 ============
    count = localStorageManager.GetTagCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 0) {
        errorDescription = "GetTagCount returned result different from the expected one (0): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    modifiedTag.setLocal(true);
    res = localStorageManager.ExpungeTag(modifiedTag, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindTag(foundTag, errorDescription);
    if (res) {
        errorDescription = "Error: found tag which should have been exounged from local storage";
        QNWARNING(errorDescription << ": Tag expunged from LocalStorageManager: " << modifiedTag
                  << "\nTag found in LocalStorageManager: " << foundTag);
        return false;
    }

    return true;
}

bool TestResourceAddFindUpdateExpungeInLocalStorage(const IResource & resource, const Note & note,
                                                    LocalStorageManager & localStorageManager,
                                                    QString & errorDescription)
{
    if (!resource.checkParameters(errorDescription)) {
        QNWARNING("Found invalid IResource: " << resource);
        return false;
    }

    // ========== Check Add + Find ==========
    bool res = localStorageManager.AddEnResource(resource, note, errorDescription);
    if (!res) {
        return false;
    }

    const QString resourceGuid = resource.guid();
    ResourceWrapper foundResource;
    foundResource.setGuid(resourceGuid);
    res = localStorageManager.FindEnResource(foundResource, errorDescription,
                                             /* withBinaryData = */ true);
    if (!res) {
        return false;
    }

    if (resource != foundResource) {
        errorDescription = "Added and found in local storage resources don't match";
        QNWARNING(errorDescription << ": IResource added to LocalStorageManager: " << resource
                  << "\nIResource found in LocalStorageManager: " << foundResource);
        return false;
    }

    // ========== Check Update + Find ==========
    ResourceWrapper modifiedResource(resource);
    modifiedResource.setUpdateSequenceNumber(resource.updateSequenceNumber() + 1);
    modifiedResource.setDataBody(resource.dataBody() + "_modified");
    modifiedResource.setDataSize(modifiedResource.dataBody().size());
    modifiedResource.setDataHash("Fake hash      3");

    modifiedResource.setWidth(resource.width() + 1);
    modifiedResource.setHeight(resource.height() + 1);
    modifiedResource.setRecognitionDataBody(resource.recognitionDataBody() + "_modified");
    modifiedResource.setRecognitionDataSize(modifiedResource.recognitionDataBody().size());
    modifiedResource.setRecognitionDataHash("Fake hash      4");

    qevercloud::ResourceAttributes & resourceAttributes = modifiedResource.resourceAttributes();

    resourceAttributes.sourceURL = "Modified source URL";
    resourceAttributes.timestamp += 1;
    resourceAttributes.latitude = 2.0;
    resourceAttributes.longitude = 2.0;
    resourceAttributes.altitude = 2.0;
    resourceAttributes.cameraMake = "Modified camera make";
    resourceAttributes.cameraModel = "Modified camera model";

    res = localStorageManager.UpdateEnResource(modifiedResource, note, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindEnResource(foundResource, errorDescription,
                                             /* withBinaryData = */ true);
    if (!res) {
        return false;
    }

    if (modifiedResource != foundResource) {
        errorDescription = "Updated and found in local storage resources don't match";
        QNWARNING(errorDescription << ": IResource updated in LocalStorageManager: " << modifiedResource
                  << "\nIResource found in LocalStorageManager: " << foundResource);
        return false;
    }

    // ========== GetEnResourceCount to return 1 ============
    int count = localStorageManager.GetEnResourceCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 1) {
        errorDescription = "GetEnResourceCount returned result different from the expected one (1): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (falure expected) ==========
    res = localStorageManager.ExpungeEnResource(modifiedResource, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindEnResource(foundResource, errorDescription);
    if (res) {
        errorDescription = "Error: found IResource which should have been expunged from LocalStorageManager";
        QNWARNING(errorDescription << ": IResource expunged from LocalStorageManager: " << modifiedResource
                  << "\nIResource found in LocalStorageManager: " << foundResource);
        return false;
    }

    // ========== GetEnResourceCount to return 0 ============
    count = localStorageManager.GetEnResourceCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 0) {
        errorDescription = "GetEnResourceCount returned result different from the expected one (0): ";
        errorDescription += QString::number(count);
        return false;
    }

    return true;
}

bool TestNoteFindUpdateDeleteExpungeInLocalStorage(const Note & note, const Notebook & notebook,
                                                   LocalStorageManager & localStorageManager,
                                                   QString & errorDescription)
{
    if (!note.checkParameters(errorDescription)) {
        QNWARNING("Found invalid Note: " << note);
        return false;
    }

    // ========== Check Find ==========
    const QString initialResourceGuid = "00000000-0000-0000-c000-000000000049";
    ResourceWrapper foundResource;
    foundResource.setGuid(initialResourceGuid);
    bool res = localStorageManager.FindEnResource(foundResource, errorDescription,
                                                  /* withBinaryData = */ true);
    if (!res) {
        return false;
    }

    const QString noteGuid = note.guid();
    const bool withResourceBinaryData = true;
    Note foundNote;
    foundNote.setGuid(noteGuid);
    res = localStorageManager.FindNote(foundNote, errorDescription, withResourceBinaryData);
    if (!res) {
        return false;
    }

    // NOTE: foundNote was searched by guid and might have another local guid is the original note
    // doesn't have one. So use this workaround to ensure the comparison is good for everything
    // without local guid
    if (note.localGuid().isEmpty()) {
        foundNote.unsetLocalGuid();
    }

    if (note != foundNote) {
        errorDescription = "Added and found notes in local storage don't match";
        QNWARNING(errorDescription << ": Note added to LocalStorageManager: " << note
                  << "\nNote found in LocalStorageManager: " << foundNote);
        return false;
    }

    // ========== Check Update + Find ==========
    Note modifiedNote(note);
    modifiedNote.setUpdateSequenceNumber(note.updateSequenceNumber() + 1);
    modifiedNote.setTitle(note.title() + "_modified");
    modifiedNote.setContent(note.content() + "_modified");
    modifiedNote.setCreationTimestamp(note.creationTimestamp() + 1);
    modifiedNote.setModificationTimestamp(note.modificationTimestamp() + 1);

    qevercloud::NoteAttributes & noteAttributes = modifiedNote.noteAttributes();

    noteAttributes.subjectDate = 2;
    noteAttributes.latitude = 2.0;
    noteAttributes.longitude = 2.0;
    noteAttributes.altitude = 2.0;
    noteAttributes.author = "modified author";
    noteAttributes.source = "modified source";
    noteAttributes.sourceURL = "modified source URL";
    noteAttributes.sourceApplication = "modified source application";
    noteAttributes.shareDate = 2;

    Tag newTag;
    newTag.setGuid("00000000-0000-0000-c000-000000000050");
    newTag.setUpdateSequenceNumber(1);
    newTag.setName("Fake new tag name");

    res = localStorageManager.AddTag(newTag, errorDescription);
    if (!res) {
        QNWARNING("Can't add new tag to local storage manager: "
                  << errorDescription);
        return false;
    }

    modifiedNote.addTagGuid(newTag.guid());

    ResourceWrapper newResource;
    newResource.setGuid("00000000-0000-0000-c000-000000000051");
    newResource.setUpdateSequenceNumber(2);
    newResource.setFreeAccount(true);
    newResource.setNoteGuid(note.guid());
    newResource.setDataBody("Fake new resource data body");
    newResource.setDataSize(newResource.dataBody().size());
    newResource.setDataHash("Fake hash      3");
    newResource.setMime("text/plain");
    newResource.setWidth(2);
    newResource.setHeight(2);
    newResource.setRecognitionDataBody("Fake new resource recognition data body");
    newResource.setRecognitionDataSize(newResource.recognitionDataBody().size());
    newResource.setRecognitionDataHash("Fake hash      4");

    modifiedNote.addResource(newResource);

    modifiedNote.unsetLocalGuid();

    res = localStorageManager.UpdateNote(modifiedNote, notebook, errorDescription);
    if (!res) {
        return false;
    }

    foundResource = ResourceWrapper();
    foundResource.setGuid(newResource.guid());
    res = localStorageManager.FindEnResource(foundResource, errorDescription,
                                             /* withBinaryData = */ true);
    if (!res) {
        return false;
    }

    if (foundResource != newResource)
    {
        errorDescription = "Something is wrong with the new resource "
                           "which should have been added to local storage "
                           "along with note update: it is not equal to original resource";
        QNWARNING(errorDescription << ": original resource: " << newResource
                  << "\nfound resource: " << foundResource);
        return false;
    }

    res = localStorageManager.FindNote(foundNote, errorDescription,
                                       /* withResourceBinaryData = */ true);
    if (!res) {
        return false;
    }

    // NOTE: foundNote was searched by guid and might have another local guid is the original note
    // doesn't have one. So use this workaround to ensure the comparison is good for everything
    // without local guid
    if (modifiedNote.localGuid().isEmpty()) {
        foundNote.unsetLocalGuid();
    }

    if (modifiedNote != foundNote) {
        errorDescription = "Updated and found in local storage notes don't match";
        QNWARNING(errorDescription << ": Note updated in LocalStorageManager: " << modifiedNote
                  << "\nNote found in LocalStorageManager: " << foundNote);
        return false;
    }

    // ========== GetNoteCount to return 1 ============
    int count = localStorageManager.GetNoteCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 1) {
        errorDescription = "GetNoteCount returned result different from the expected one (1): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Delete + Find and check deleted flag ============
    modifiedNote.setLocal(false);
    modifiedNote.setDeleted(true);
    modifiedNote.setDeletionTimestamp(1);
    foundNote.setDeleted(false);
    res = localStorageManager.DeleteNote(modifiedNote, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindNote(foundNote, errorDescription,
                                       /* withResourceBinaryData = */ true);
    if (!res) {
        return false;
    }

    if (!foundNote.isDeleted()) {
        errorDescription = "Note which should have been marked deleted "
                           "is not marked so after LocalStorageManager::FindNote";
        QNWARNING(errorDescription << ": Note found in LocalStorageManager: " << foundNote);
        return false;
    }

    // ========== GetNoteCount to return 0 ============
    count = localStorageManager.GetNoteCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 0) {
        errorDescription = "GetNoteCount returned result different from the expected one (0): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    modifiedNote.setLocal(true);
    res = localStorageManager.ExpungeNote(modifiedNote, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindNote(foundNote, errorDescription,
                                       /* withResourceBinaryData = */ true);
    if (res) {
        errorDescription = "Error: found Note which should have been expunged "
                           "from LocalStorageManager";
        QNWARNING(errorDescription << ": Note expunged from LocalStorageManager: " << modifiedNote
                  << "\nNote found in LocalStorageManager: " << foundNote);
        return false;
    }

    // ========== Try to find resource belonging to expunged note (failure expected) ==========
    foundResource = ResourceWrapper();
    foundResource.setGuid(newResource.guid());
    res = localStorageManager.FindEnResource(foundResource, errorDescription,
                                             /* withBinaryData = */ true);
    if (res) {
        errorDescription = "Error: found Resource which should have been expunged "
                           "from LocalStorageManager along with Note owning it";
        QNWARNING(errorDescription << ": Note expunged from LocalStorageManager: " << modifiedNote
                  << "\nResource found in LocalStorageManager: " << foundResource);
        return false;
    }

    return true;
}

bool TestNotebookFindUpdateDeleteExpungeInLocalStorage(const Notebook & notebook,
                                                       LocalStorageManager & localStorageManager,
                                                       QString & errorDescription)
{
    if (!notebook.checkParameters(errorDescription)) {
        QNWARNING("Found invalid Notebook: " << notebook);
        return false;
    }

    // =========== Check Find ============
    const QString initialNoteGuid = "00000000-0000-0000-c000-000000000049";
    Note foundNote;
    foundNote.setGuid(initialNoteGuid);
    bool res = localStorageManager.FindNote(foundNote, errorDescription,
                                            /* withResourceBinaryData = */ true);
    if (!res) {
        return false;
    }

    Notebook foundNotebook;
    foundNotebook.setGuid(notebook.guid());
    res = localStorageManager.FindNotebook(foundNotebook, errorDescription);
    if (!res) {
        return false;
    }

    if (notebook != foundNotebook) {
        errorDescription = "Added and found notebooks in local storage don't match";
        QNWARNING(errorDescription << ": Notebook added to LocalStorageManager: " << notebook
                  << "\nNotebook found in LocalStorageManager: " << foundNotebook);
        return false;
    }

    // ========== Check FindDefaultNotebook =========
    Notebook defaultNotebook;
    res = localStorageManager.FindDefaultNotebook(defaultNotebook, errorDescription);
    if (!res) {
        return false;
    }

    // ========== Check FindLastUsedNotebook (failure expected) ==========
    Notebook lastUsedNotebook;
    res = localStorageManager.FindLastUsedNotebook(lastUsedNotebook, errorDescription);
    if (res) {
        errorDescription = "Found some last used notebook which shouldn't have been found";
        QNWARNING(errorDescription << ": " << lastUsedNotebook);
        return false;
    }

    // ========== Check FindDefaultOrLastUsedNotebook ===========
    Notebook defaultOrLastUsedNotebook;
    res = localStorageManager.FindDefaultOrLastUsedNotebook(defaultOrLastUsedNotebook,
                                                            errorDescription);
    if (!res) {
        return false;
    }

    if (defaultOrLastUsedNotebook != defaultNotebook) {
        errorDescription = "Found defaultOrLastUsed notebook which should be the same "
                           "as default notebook right now but it is not";
        QNWARNING(errorDescription << ". Default notebook: " << defaultNotebook
                  << ", defaultOrLastUsedNotebook: " << defaultOrLastUsedNotebook);
        return false;
    }

    // ========== Check Update + Find ==========
    Notebook modifiedNotebook(notebook);
    modifiedNotebook.setUpdateSequenceNumber(notebook.updateSequenceNumber() + 1);
    modifiedNotebook.setName(notebook.name() + "_modified");
    modifiedNotebook.setDefaultNotebook(false);
    modifiedNotebook.setLastUsed(true);
    modifiedNotebook.setModificationTimestamp(notebook.modificationTimestamp() + 1);
    modifiedNotebook.setPublishingUri(notebook.publishingUri() + "_modified");
    modifiedNotebook.setPublishingAscending(!notebook.isPublishingAscending());
    modifiedNotebook.setPublishingPublicDescription(notebook.publishingPublicDescription() + "_modified");
    modifiedNotebook.setStack(notebook.stack() + "_modified");
    modifiedNotebook.setBusinessNotebookDescription(notebook.businessNotebookDescription() + "_modified");
    modifiedNotebook.setBusinessNotebookRecommended(!notebook.isBusinessNotebookRecommended());
    modifiedNotebook.setCanExpungeNotes(false);
    modifiedNotebook.setCanEmailNotes(false);
    modifiedNotebook.setCanPublishToPublic(false);
    modifiedNotebook.unsetLocalGuid();

    res = localStorageManager.UpdateNotebook(modifiedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    foundNotebook = Notebook();
    foundNotebook.setGuid(modifiedNotebook.guid());
    res = localStorageManager.FindNotebook(foundNotebook, errorDescription);
    if (!res) {
        return false;
    }

    if (modifiedNotebook != foundNotebook) {
        errorDescription = "Updated and found notebooks in local storage don't match";
        QNWARNING(errorDescription << ": Notebook updated in LocalStorageManager: " << modifiedNotebook
                  << "\nNotebook found in LocalStorageManager: " << foundNotebook);
        return false;
    }

    // ========== Check FindDefaultNotebook (failure expected) =========
    defaultNotebook = Notebook();
    res = localStorageManager.FindDefaultNotebook(defaultNotebook, errorDescription);
    if (res) {
        errorDescription = "Found some default notebook which shouldn't have been found";
        QNWARNING(errorDescription << ": " << defaultNotebook);
        return false;
    }

    // ========== Check FindLastUsedNotebook  ==========
    lastUsedNotebook = Notebook();
    res = localStorageManager.FindLastUsedNotebook(lastUsedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    // ========== Check FindDefaultOrLastUsedNotebook ===========
    defaultOrLastUsedNotebook = Notebook();
    res = localStorageManager.FindDefaultOrLastUsedNotebook(defaultOrLastUsedNotebook,
                                                            errorDescription);
    if (!res) {
        return false;
    }

    if (defaultOrLastUsedNotebook != lastUsedNotebook) {
        errorDescription = "Found defaultOrLastUsed notebook which should be the same "
                           "as last used notebook right now but it is not";
        QNWARNING(errorDescription << ". Last used notebook: " << lastUsedNotebook
                  << ", defaultOrLastUsedNotebook: " << defaultOrLastUsedNotebook);
        return false;
    }

    // ========== Check GetNotebookCount to return 1 ============
    int count = localStorageManager.GetNotebookCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 1) {
        errorDescription = "GetNotebookCount returned result different from the expected one (1): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    modifiedNotebook.setLocal(true);
    res = localStorageManager.ExpungeNotebook(modifiedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindNotebook(foundNotebook, errorDescription);
    if (res) {
        errorDescription = "Error: found Notebook which should have been expunged "
                           "from LocalStorageManager";
        QNWARNING(errorDescription << ": Notebook expunged from LocalStorageManager: " << modifiedNotebook
                  << "\nNotebook found in LocalStorageManager: " << foundNotebook);
        return false;
    }

    // ========== Check GetNotebookCount to return 0 ============
    count = localStorageManager.GetNotebookCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 0) {
        errorDescription = "GetNotebookCount returned result different from the expected one (0): ";
        errorDescription += QString::number(count);
        return false;
    }

    return true;
}

bool TestUserAddFindUpdateDeleteExpungeInLocalStorage(const IUser & user, LocalStorageManager & localStorageManager,
                                                      QString & errorDescription)
{
    if (!user.checkParameters(errorDescription)) {
        QNWARNING("Found invalid IUser: " << user);
        return false;
    }

    // ========== Check Add + Find ==========
    bool res = localStorageManager.AddUser(user, errorDescription);
    if (!res) {
        return false;
    }

    const qint32 initialUserId = user.id();
    UserWrapper foundUser;
    foundUser.setId(initialUserId);
    res = localStorageManager.FindUser(foundUser, errorDescription);
    if (!res) {
        return false;
    }

    if (user != foundUser) {
        errorDescription = "Added and found users in local storage don't match";
        QNWARNING(errorDescription << ": IUser added to LocalStorageManager: " << user
                  << "\nIUser found in LocalStorageManager: " << foundUser);
        return false;
    }

    // ========== Check Update + Find ==========
    UserWrapper modifiedUser;
    modifiedUser.setId(user.id());
    modifiedUser.setUsername(user.username() + "_modified");
    modifiedUser.setEmail(user.email() + "_modified");
    modifiedUser.setName(user.name() + "_modified");
    modifiedUser.setTimezone(user.timezone() + "_modified");
    modifiedUser.setPrivilegeLevel(user.privilegeLevel());
    modifiedUser.setCreationTimestamp(user.creationTimestamp());
    modifiedUser.setModificationTimestamp(user.modificationTimestamp() + 1);
    modifiedUser.setActive(true);

    qevercloud::UserAttributes modifiedUserAttributes;
    modifiedUserAttributes = user.userAttributes();
    modifiedUserAttributes.defaultLocationName->append("_modified");
    modifiedUserAttributes.comments->append("_modified");
    modifiedUserAttributes.preferredCountry->append("_modified");
    modifiedUserAttributes.businessAddress->append("_modified");

    modifiedUser.setUserAttributes(std::move(modifiedUserAttributes));

    qevercloud::BusinessUserInfo modifiedBusinessUserInfo;
    modifiedBusinessUserInfo = user.businessUserInfo();
    modifiedBusinessUserInfo.businessName->append("_modified");
    modifiedBusinessUserInfo.email->append("_modified");

    modifiedUser.setBusinessUserInfo(std::move(modifiedBusinessUserInfo));

    qevercloud::Accounting modifiedAccounting;
    modifiedAccounting = user.accounting();
    modifiedAccounting.premiumOrderNumber->append("_modified");
    modifiedAccounting.premiumSubscriptionNumber->append("_modified");
    modifiedAccounting.updated += 1;

    modifiedUser.setAccounting(std::move(modifiedAccounting));

    qevercloud::PremiumInfo modifiedPremiumInfo;
    modifiedPremiumInfo = user.premiumInfo();
    modifiedPremiumInfo.sponsoredGroupName->append("_modified");
    modifiedPremiumInfo.canPurchaseUploadAllowance = !modifiedPremiumInfo.canPurchaseUploadAllowance;
    modifiedPremiumInfo.premiumExtendable = !modifiedPremiumInfo.premiumExtendable;

    modifiedUser.setPremiumInfo(std::move(modifiedPremiumInfo));

    res = localStorageManager.UpdateUser(modifiedUser, errorDescription);
    if (!res) {
        return false;
    }

    foundUser.clear();
    foundUser.setId(modifiedUser.id());
    res = localStorageManager.FindUser(foundUser, errorDescription);
    if (!res) {
        return false;
    }

    if (modifiedUser != foundUser) {
        errorDescription = "Updated and found users in local storage don't match";
        QNWARNING(errorDescription << ": IUser updated in LocalStorageManager: " << modifiedUser
                  << "\nIUser found in LocalStorageManager: " << foundUser);
        return false;
    }

    // ========== Check GetUserCount to return 1 ===========
    int count = localStorageManager.GetUserCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 1) {
        errorDescription = "GetUserCount returned value different from expected (1): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Delete + Find ==========
    modifiedUser.setDeletionTimestamp(5);
    modifiedUser.setLocal(false);

    res = localStorageManager.DeleteUser(modifiedUser, errorDescription);
    if (!res) {
        return false;
    }

    foundUser.clear();
    foundUser.setId(modifiedUser.id());
    res = localStorageManager.FindUser(foundUser, errorDescription);
    if (!res) {
        return false;
    }

    if (modifiedUser != foundUser) {
        errorDescription = "Deleted and found users in local storage manager don't match";
        QNWARNING(errorDescription << ": IUser marked deleted in LocalStorageManager: " << modifiedUser
                  << "\nIUser found in LocalStorageManager: " << foundUser);
        return false;
    }

    // ========== Check GetUserCount to return 0 (as it doesn't account for deleted resources) ===========
    count = localStorageManager.GetUserCount(errorDescription);
    if (count < 0) {
        return false;
    }
    else if (count != 0) {
        errorDescription = "GetUserCount returned value different from expected (0): ";
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    modifiedUser.setLocal(true);

    res = localStorageManager.ExpungeUser(modifiedUser, errorDescription);
    if (!res) {
        return false;
    }

    foundUser.clear();
    foundUser.setId(modifiedUser.id());
    res = localStorageManager.FindUser(foundUser, errorDescription);
    if (res) {
        errorDescription = "Error: found IUser which should have been expunged "
                           "from LocalStorageManager";
        QNWARNING(errorDescription << ": IUser expunged from LocalStorageManager: " << modifiedUser
                  << "\nIUser found in LocalStorageManager: " << foundUser);
        return false;
    }

    return true;
}

}
}
