#include "LocalStorageManagerTests.h"
#include <logging/QuteNoteLogger.h>
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/SavedSearch.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/ResourceAdapter.h>
#include <client/types/Note.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/UserWrapper.h>
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

    // ========= Check Find by name =============
    SavedSearch foundByNameSearch;
    foundByNameSearch.unsetLocalGuid();
    foundByNameSearch.setName(search.name());
    res = localStorageManager.FindSavedSearch(foundByNameSearch, errorDescription);
    if (!res) {
        return false;
    }

    if (search != foundByNameSearch) {
        errorDescription = "Added and found by name saved searches in local storage don't match";
        QNWARNING(errorDescription << ": SavedSearch added to LocalStorageManager: " << search
                  << "\nSaved search found by name in LocalStorageManager: " << foundByNameSearch);
        return false;
    }

    // ========= Check Update + Find =============
    SavedSearch modifiedSearch(search);
    modifiedSearch.setUpdateSequenceNumber(search.updateSequenceNumber() + 1);
    modifiedSearch.setName(search.name() + "_modified");
    modifiedSearch.setQuery(search.query() + "_modified");
    modifiedSearch.setShortcut(true);
    modifiedSearch.setDirty(true);

    QString localGuid = modifiedSearch.localGuid();
    modifiedSearch.unsetLocalGuid();

    res = localStorageManager.UpdateSavedSearch(modifiedSearch, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindSavedSearch(foundSearch, errorDescription);
    if (!res) {
        return false;
    }

    modifiedSearch.setLocalGuid(localGuid);
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

    // ========== Check Find by name ==========
    Tag foundByNameTag;
    foundByNameTag.unsetLocalGuid();
    foundByNameTag.setName(tag.name());
    res = localStorageManager.FindTag(foundByNameTag, errorDescription);
    if (!res) {
        return false;
    }

    if (tag != foundByNameTag) {
        errorDescription = "Tag found by name in local storage doesn't match the original tag";
        QNWARNING(errorDescription << ": Tag found by name: " << foundByNameTag << "\nOriginal tag: " << tag);
        return false;
    }

    // ========== Check Update + Find ==========
    Tag modifiedTag(tag);
    modifiedTag.setUpdateSequenceNumber(tag.updateSequenceNumber() + 1);
    modifiedTag.setLinkedNotebookGuid(QString());
    modifiedTag.setName(tag.name() + "_modified");
    modifiedTag.setShortcut(true);
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
    modifiedResource.setAlternateDataBody(resource.alternateDataBody() + "_modified");
    modifiedResource.setAlternateDataSize(modifiedResource.alternateDataBody().size());
    modifiedResource.setAlternateDataHash("Fake hash      5");

    qevercloud::ResourceAttributes & resourceAttributes = modifiedResource.resourceAttributes();

    resourceAttributes.sourceURL = "Modified source URL";
    resourceAttributes.timestamp += 1;
    resourceAttributes.latitude = 2.0;
    resourceAttributes.longitude = 2.0;
    resourceAttributes.altitude = 2.0;
    resourceAttributes.cameraMake = "Modified camera make";
    resourceAttributes.cameraModel = "Modified camera model";

    QString resourceLocalGuid = modifiedResource.localGuid();
    modifiedResource.unsetLocalGuid();

    res = localStorageManager.UpdateEnResource(modifiedResource, note, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindEnResource(foundResource, errorDescription,
                                             /* withBinaryData = */ true);
    if (!res) {
        return false;
    }

    modifiedResource.setLocalGuid(resourceLocalGuid);
    if (modifiedResource != foundResource) {
        errorDescription = "Updated and found in local storage resources don't match";
        QNWARNING(errorDescription << ": IResource updated in LocalStorageManager: " << modifiedResource
                  << "\nIResource found in LocalStorageManager: " << foundResource);
        return false;
    }

    // ========== Check Find without resource binary data =========
    foundResource.clear();
    foundResource.setGuid(resourceGuid);
    res = localStorageManager.FindEnResource(foundResource, errorDescription,
                                             /* withBinaryData = */ false);
    if (!res) {
        return false;
    }

    modifiedResource.setDataBody(QByteArray());
    modifiedResource.setRecognitionDataBody(QByteArray());
    modifiedResource.setAlternateDataBody(QByteArray());

    if (modifiedResource != foundResource) {
        errorDescription = "Updated and found in local storage resources without binary data don't match";
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
    modifiedNote.setCreationTimestamp(note.creationTimestamp() + 1);
    modifiedNote.setModificationTimestamp(note.modificationTimestamp() + 1);
    modifiedNote.setShortcut(true);

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

    qevercloud::ResourceAttributes & resourceAttributes = newResource.resourceAttributes();

    resourceAttributes.sourceURL = "Fake resource source URL";
    resourceAttributes.timestamp = 1;
    resourceAttributes.latitude = 0.0;
    resourceAttributes.longitude = 0.0;
    resourceAttributes.altitude = 0.0;
    resourceAttributes.cameraMake = "Fake resource camera make";
    resourceAttributes.cameraModel = "Fake resource camera model";

    resourceAttributes.applicationData = qevercloud::LazyMap();
    resourceAttributes.applicationData->keysOnly = QSet<QString>();
    resourceAttributes.applicationData->keysOnly->insert("key 1");
    resourceAttributes.applicationData->fullMap = QMap<QString, QString>();
    resourceAttributes.applicationData->fullMap.ref()["key 1 map"] = "value 1";

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

    foundResource.setNoteLocalGuid(QString());
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
    modifiedNote.setActive(false);
    modifiedNote.setDeletionTimestamp(1);
    foundNote.setActive(true);
    res = localStorageManager.DeleteNote(modifiedNote, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindNote(foundNote, errorDescription,
                                       /* withResourceBinaryData = */ true);
    if (!res) {
        return false;
    }

    if (!foundNote.hasActive() || foundNote.active()) {
        errorDescription = "Note which should have been marked non-active "
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

    // ========== Check Find by name ===========
    Notebook foundByNameNotebook;
    foundByNameNotebook.unsetLocalGuid();
    foundByNameNotebook.setName(notebook.name());
    res = localStorageManager.FindNotebook(foundByNameNotebook, errorDescription);
    if (!res) {
        return false;
    }

    if (notebook != foundByNameNotebook) {
        errorDescription = "Notebook found by name in local storage doesn't match the original notebook";
        QNWARNING(errorDescription << ": Notebook found by name: " << foundByNameNotebook
                  << "\nOriginal notebook: " << notebook);
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
    modifiedNotebook.setLinkedNotebookGuid(QString());
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
    modifiedNotebook.setShortcut(true);

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

bool TestSequentialUpdatesInLocalStorage(QString & errorDescription)
{
    // 1) ========== Create LocalStorageManager =============

    const bool startFromScratch = true;
    LocalStorageManager localStorageManager("LocalStorageManagerSequentialUpdatesTestFakeUser",
                                            0, startFromScratch);

    // 2) ========== Create User ============
    UserWrapper   user;
    user.setId(1);
    user.setUsername("checker");
    user.setEmail("mail@checker.com");
    user.setTimezone("Europe/Moscow");
    user.setPrivilegeLevel(qevercloud::PrivilegeLevel::NORMAL);
    user.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    user.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    user.setActive(true);

    qevercloud::UserAttributes userAttributes;
    userAttributes.defaultLocationName = "Default location";
    userAttributes.comments = "My comment";
    userAttributes.preferredLanguage = "English";

    userAttributes.viewedPromotions = QStringList();
    userAttributes.viewedPromotions.ref() << "Promotion #1" << "Promotion #2" << "Promotion #3";

    userAttributes.recentMailedAddresses = QStringList();
    userAttributes.recentMailedAddresses.ref() << "Recent mailed address #1"
                                               << "Recent mailed address #2"
                                               << "Recent mailed address #3";

    user.setUserAttributes(std::move(userAttributes));

    qevercloud::Accounting accounting;
    accounting.premiumOrderNumber = "Premium order number";
    accounting.premiumSubscriptionNumber = "Premium subscription number";
    accounting.updated = QDateTime::currentMSecsSinceEpoch();

    user.setAccounting(std::move(accounting));

    qevercloud::BusinessUserInfo businessUserInfo;
    businessUserInfo.businessName = "Business name";
    businessUserInfo.email = "Business email";

    user.setBusinessUserInfo(std::move(businessUserInfo));

    qevercloud::PremiumInfo premiumInfo;
    premiumInfo.sponsoredGroupName = "Sponsored group name";
    premiumInfo.canPurchaseUploadAllowance = true;
    premiumInfo.premiumExtendable = true;

    user.setPremiumInfo(std::move(premiumInfo));

    // 3) ============ Add user to local storage ==============
    bool res = localStorageManager.AddUser(user, errorDescription);
    if (!res) {
        return false;
    }

    // 4) ============ Create new user without all the supplementary data but with the same id
    //                 and update it in local storage ===================
    UserWrapper updatedUser;
    updatedUser.setId(1);
    updatedUser.setUsername("checker");
    updatedUser.setEmail("mail@checker.com");
    updatedUser.setPrivilegeLevel(qevercloud::PrivilegeLevel::NORMAL);
    updatedUser.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    updatedUser.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    updatedUser.setActive(true);

    res = localStorageManager.UpdateUser(updatedUser, errorDescription);
    if (!res) {
        return false;
    }

    // 5) =========== Find this user in local storage, check whether it has user attributes,
    //                accounting, business user info and premium info (it shouldn't) =========
    UserWrapper foundUser;
    foundUser.setId(1);

    res = localStorageManager.FindUser(foundUser, errorDescription);
    if (!res) {
        return false;
    }

    if (foundUser.hasUserAttributes()) {
        errorDescription = "Updated user found in local storage still has user attributes "
                           "while it shouldn't have them after the update";
        QNWARNING(errorDescription << ": initial user: " << user << "\nUpdated user: "
                  << updatedUser << "\nFound user: " << foundUser);
        return false;
    }

    if (foundUser.hasAccounting()) {
        errorDescription = "Updated user found in local storage still has accounting "
                           "while it shouldn't have it after the update";
        QNWARNING(errorDescription << ": initial user: " << user << "\nUpdated user: "
                  << updatedUser << "\nFound user: " << foundUser);
        return false;
    }

    if (foundUser.hasBusinessUserInfo()) {
        errorDescription = "Updated user found in local storage still has business user info "
                           "while it shouldn't have it after the update";
        QNWARNING(errorDescription << ": initial user: " << user << "\nUpdated user: "
                  << updatedUser << "\nFound user: " << foundUser);
        return false;
    }

    if (foundUser.hasPremiumInfo()) {
        errorDescription = "Updated user found in local storage still has premium info "
                           "while it shouldn't have it after the update";
        QNWARNING(errorDescription << ": initial user: " << user << "\nUpdated user: "
                  << updatedUser << "\nFound user: " << foundUser);
        return false;
    }

    // ============ 6) Create Notebook with restrictions and shared notebooks ==================
    Notebook notebook;
    notebook.setGuid("00000000-0000-0000-c000-000000000049");
    notebook.setUpdateSequenceNumber(1);
    notebook.setName("Fake notebook name");
    notebook.setCreationTimestamp(1);
    notebook.setModificationTimestamp(1);
    notebook.setDefaultNotebook(true);
    notebook.setLastUsed(false);
    notebook.setPublishingUri("Fake publishing uri");
    notebook.setPublishingOrder(1);
    notebook.setPublishingAscending(true);
    notebook.setPublishingPublicDescription("Fake public description");
    notebook.setPublished(true);
    notebook.setStack("Fake notebook stack");
    notebook.setBusinessNotebookDescription("Fake business notebook description");
    notebook.setBusinessNotebookPrivilegeLevel(1);
    notebook.setBusinessNotebookRecommended(true);
    // NotebookRestrictions
    notebook.setCanReadNotes(true);
    notebook.setCanCreateNotes(true);
    notebook.setCanUpdateNotes(true);
    notebook.setCanExpungeNotes(false);
    notebook.setCanShareNotes(true);
    notebook.setCanEmailNotes(false);
    notebook.setCanSendMessageToRecipients(true);
    notebook.setCanUpdateNotebook(true);
    notebook.setCanExpungeNotebook(false);
    notebook.setCanSetDefaultNotebook(true);
    notebook.setCanSetNotebookStack(false);
    notebook.setCanPublishToPublic(true);
    notebook.setCanPublishToBusinessLibrary(false);
    notebook.setCanCreateTags(true);
    notebook.setCanUpdateTags(true);
    notebook.setCanExpungeTags(false);
    notebook.setCanSetParentTag(true);
    notebook.setCanCreateSharedNotebooks(true);
    notebook.setCanCreateSharedNotebooks(true);
    notebook.setCanUpdateNotebook(true);
    notebook.setUpdateWhichSharedNotebookRestrictions(1);
    notebook.setExpungeWhichSharedNotebookRestrictions(1);

    SharedNotebookWrapper sharedNotebook;
    sharedNotebook.setId(1);
    sharedNotebook.setUserId(1);
    sharedNotebook.setNotebookGuid(notebook.guid());
    sharedNotebook.setEmail("Fake shared notebook email");
    sharedNotebook.setCreationTimestamp(1);
    sharedNotebook.setModificationTimestamp(1);
    sharedNotebook.setShareKey("Fake shared notebook share key");
    sharedNotebook.setUsername("Fake shared notebook username");
    sharedNotebook.setPrivilegeLevel(1);
    sharedNotebook.setAllowPreview(true);
    sharedNotebook.setReminderNotifyEmail(true);
    sharedNotebook.setReminderNotifyApp(false);

    notebook.addSharedNotebook(sharedNotebook);

    res = localStorageManager.AddNotebook(notebook, errorDescription);
    if (!res) {
        return false;
    }

    // 7) ============ Update notebook: remove restrictions and shared notebooks =========
    Notebook updatedNotebook;
    updatedNotebook.setLocalGuid(notebook.localGuid());
    updatedNotebook.setGuid(notebook.guid());
    updatedNotebook.setUpdateSequenceNumber(1);
    updatedNotebook.setName("Fake notebook name");
    updatedNotebook.setCreationTimestamp(1);
    updatedNotebook.setModificationTimestamp(1);
    updatedNotebook.setDefaultNotebook(true);
    updatedNotebook.setLastUsed(false);
    updatedNotebook.setPublishingUri("Fake publishing uri");
    updatedNotebook.setPublishingOrder(1);
    updatedNotebook.setPublishingAscending(true);
    updatedNotebook.setPublishingPublicDescription("Fake public description");
    updatedNotebook.setPublished(true);
    updatedNotebook.setStack("Fake notebook stack");
    updatedNotebook.setBusinessNotebookDescription("Fake business notebook description");
    updatedNotebook.setBusinessNotebookPrivilegeLevel(1);
    updatedNotebook.setBusinessNotebookRecommended(true);

    res = localStorageManager.UpdateNotebook(updatedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    // 8) ============= Find notebook, ensure it doesn't have neither restrictions nor shared notebooks

    Notebook foundNotebook;
    foundNotebook.setGuid(notebook.guid());

    res = localStorageManager.FindNotebook(foundNotebook, errorDescription);
    if (!res) {
        return false;
    }

    if (foundNotebook.hasSharedNotebooks()) {
        errorDescription = "Updated notebook found in local storage has shared notebooks "
                           "while it shouldn't have them";
        QNWARNING(errorDescription << ", original notebook: " << notebook
                  << "\nUpdated notebook: " << updatedNotebook << "\nFound notebook: "
                  << foundNotebook);
        return false;
    }

    if (foundNotebook.hasRestrictions()) {
        errorDescription = "Updated notebook found in local storage has restrictions "
                           "while it shouldn't have them";
        QNWARNING(errorDescription << ", original notebook: " << notebook
                  << "\nUpdated notebook: " << updatedNotebook << "\nFound notebook: "
                  << foundNotebook);
        return false;
    }

    // 9) ============== Create tag =================
    Tag tag;
    tag.setGuid("00000000-0000-0000-c000-000000000046");
    tag.setUpdateSequenceNumber(1);
    tag.setName("Fake tag name");

    res = localStorageManager.AddTag(tag, errorDescription);
    if (!res) {
        return false;
    }

    // 10) ============= Create note, add this tag to it along with some resource ===========
    Note note;
    note.setGuid("00000000-0000-0000-c000-000000000045");
    note.setUpdateSequenceNumber(1);
    note.setTitle("Fake note title");
    note.setContent("<en-note><h1>Hello, world</h1></en-note>");
    note.setCreationTimestamp(1);
    note.setModificationTimestamp(1);
    note.setActive(true);
    note.setNotebookGuid(notebook.guid());

    ResourceWrapper resource;
    resource.setGuid("00000000-0000-0000-c000-000000000044");
    resource.setUpdateSequenceNumber(1);
    resource.setNoteGuid(note.guid());
    resource.setDataBody("Fake resource data body");
    resource.setDataSize(resource.dataBody().size());
    resource.setDataHash("Fake hash      1");

    note.addResource(resource);
    note.addTagGuid(tag.guid());

    res = localStorageManager.AddNote(note, updatedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    // 11) ============ Update note, remove tag guid and resource ============
    Note updatedNote;
    updatedNote.setLocalGuid(note.localGuid());
    updatedNote.setGuid("00000000-0000-0000-c000-000000000045");
    updatedNote.setUpdateSequenceNumber(1);
    updatedNote.setTitle("Fake note title");
    updatedNote.setContent("<en-note><h1>Hello, world</h1></en-note>");
    updatedNote.setCreationTimestamp(1);
    updatedNote.setModificationTimestamp(1);
    updatedNote.setActive(true);
    updatedNote.setNotebookGuid(notebook.guid());

    res = localStorageManager.UpdateNote(updatedNote, updatedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    // 12) =========== Find updated note in local storage, ensure it doesn't have neither tag guids, nor resources
    Note foundNote;
    foundNote.setLocalGuid(updatedNote.localGuid());
    foundNote.setGuid(updatedNote.guid());

    res = localStorageManager.FindNote(foundNote, errorDescription);
    if (!res) {
        return false;
    }

    if (foundNote.hasTagGuids()) {
        errorDescription = "Updated note found in local storage has tag guids while it shouldn't have them";
        QNWARNING(errorDescription << ", original note: " << note << "\nUpdated note: "
                  << updatedNote << "\nFound note: " << foundNote);
        return false;
    }

    if (foundNote.hasResources()) {
        errorDescription = "Updated note found in local storage has resources while it shouldn't have them";
        QNWARNING(errorDescription << ", original note: " << note << "\nUpdated note: "
                  << updatedNote << "\nFound note: " << foundNote);
        return false;
    }

    // 13) ============== Add resource attributes to the resource and add resource to note =============
    qevercloud::ResourceAttributes & resourceAttributes = resource.resourceAttributes();
    resourceAttributes.applicationData = qevercloud::LazyMap();
    resourceAttributes.applicationData->keysOnly = QSet<QString>();
    resourceAttributes.applicationData->fullMap = QMap<QString, QString>();

    resourceAttributes.applicationData->keysOnly.ref() << "key_1" << "key_2" << "key_3";
    resourceAttributes.applicationData->fullMap.ref()["key_1"] = "value_1";
    resourceAttributes.applicationData->fullMap.ref()["key_2"] = "value_2";
    resourceAttributes.applicationData->fullMap.ref()["key_3"] = "value_3";

    updatedNote.addResource(resource);

    res = localStorageManager.UpdateNote(updatedNote, updatedNotebook, errorDescription);
    if (!res) {
        return res;
    }

    // 14) ================ Remove resource attributes from note's resource and update it again
    QList<ResourceWrapper> resources = updatedNote.resources();
    if (resources.empty()) {
        errorDescription = "Note returned empty list of resource adapters while it should have "
                           "contained at least one entry";
        QNWARNING(errorDescription << ", updated note: " << updatedNote);
        return false;
    }

    ResourceWrapper & resourceWrapper = resources[0];
    qevercloud::ResourceAttributes & underlyngResourceAttributes = resourceWrapper.resourceAttributes();
    underlyngResourceAttributes = qevercloud::ResourceAttributes();

    updatedNote.setResources(resources);

    res = localStorageManager.UpdateNote(updatedNote, updatedNotebook, errorDescription);
    if (!res) {
        return false;
    }

    // 15) ============= Find note in local storage again ===============
    res = localStorageManager.FindNote(foundNote, errorDescription);
    if (!res) {
        return false;
    }

    resources = foundNote.resources();
    if (resources.empty()) {
        errorDescription = "Note returned empty list of resource adapters while it should have "
                           "contained at least one entry";
        QNWARNING(errorDescription << ", found note: " << foundNote);
        return false;
    }

    ResourceWrapper & foundResourceWrapper = resources[0];
    qevercloud::ResourceAttributes & foundResourceAttributes = foundResourceWrapper.resourceAttributes();
    if (foundResourceAttributes.applicationData.isSet()) {
        errorDescription = "Resource from updated note has application data while it shouldn't have it";
        QNWARNING(errorDescription << ", found resource: " << foundResourceWrapper);
        return false;
    }

    return true;
}

}
}
