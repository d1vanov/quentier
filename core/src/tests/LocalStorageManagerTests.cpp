#include "LocalStorageManagerTests.h"
#include <logging/QuteNoteLogger.h>
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/SavedSearch.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/Note.h>
#include <client/Utility.h>
#include <client/Serialization.h>

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
        errorDescription = QObject::tr("Added and found saved searches in local storage don't match");
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
        errorDescription = QObject::tr("Updated and found saved searches in local storage don't match");
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
        errorDescription = "Error: found saved search which should have been expunged from local storage";
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
        errorDescription = QObject::tr("Added and found linked noteboks in local storage don't match");
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
        errorDescription = QObject::tr("Updated and found linked notebooks in local storage don't match");
        QNWARNING(errorDescription << ": LinkedNotebook updated in LocalStorageManager: " << modifiedLinkedNotebook
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
        errorDescription = "Error: found linked notebook which should have been expunged from local storage";
        QNWARNING(errorDescription << ": LinkedNotebook expunged from LocalStorageManager: " << modifiedLinkedNotebook
                  << "\nLinkedNotebook found in LocalStorageManager: " << foundLinkedNotebook);
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

    const QString tagGuid = tag.guid();
    Tag foundTag;
    res = localStorageManager.FindTag(tagGuid, foundTag, errorDescription);
    if (!res) {
        return false;
    }

    if (tag != foundTag) {
        errorDescription = QObject::tr("Added and found tags in local storage tags don't match");
        QNWARNING(errorDescription << ": Tag added to LocalStorageManager: " << tag
                  << "\nTag found in LocalStorageManager: " << foundTag);
        return false;
    }

    // ========== Check Update + Find ==========
    Tag modifiedTag(tag);
    modifiedTag.setUpdateSequenceNumber(tag.updateSequenceNumber() + 1);
    modifiedTag.setName(tag.name() + "_modified");

    res = localStorageManager.UpdateTag(modifiedTag, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindTag(tagGuid, foundTag, errorDescription);
    if (!res) {
        return false;
    }

    if (modifiedTag != foundTag) {
        errorDescription = QObject::tr("Updated and found tags in local storage don't match");
        QNWARNING(errorDescription << ": Tag updated in LocalStorageManaged: " << modifiedTag
                  << "\nTag found in LocalStorageManager: " << foundTag);
        return false;
    }

    // ========== Check Delete + Find ==========
    modifiedTag.setDeleted(true);
    modifiedTag.setLocal(false);
    res = localStorageManager.DeleteTag(modifiedTag, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindTag(tagGuid, foundTag, errorDescription);
    if (!res) {
        return false;
    }

    if (!foundTag.isDeleted()) {
        errorDescription = QObject::tr("Tag which should have been marked as deleted one "
                                       "is not marked so in the result of FindTag");
        QNWARNING(errorDescription << ": deleted Tag which should have been found in LocalStorageManager: "
                  << modifiedTag << "\nTag which should have actually been found: "
                  << foundTag);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    modifiedTag.setLocal(true);
    res = localStorageManager.ExpungeTag(modifiedTag, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindTag(tagGuid, foundTag, errorDescription);
    if (res) {
        errorDescription = "Error: found tag which should have been exounged from local storage";
        QNWARNING(errorDescription << ": Tag expunged from LocalStorageManager: " << modifiedTag
                  << "\nTag found in LocalStorageManager: " << foundTag);
        return false;
    }

    return true;
}

bool TestResourceAddFindUpdateExpungeInLocalStorage(const IResource & resource,
                                                    LocalStorageManager & localStorageManager,
                                                    QString & errorDescription)
{
    if (!resource.checkParameters(errorDescription)) {
        QNWARNING("Found invalid IResource: " << resource);
        return false;
    }

    // ========== Check Add + Find ==========
    bool res = localStorageManager.AddResource(resource, errorDescription);
    if (!res) {
        return false;
    }

    const QString resourceGuid = resource.guid();
    ResourceWrapper foundResource;
    res = localStorageManager.FindResource(resourceGuid, foundResource, errorDescription,
                                           /* withBinaryData = */ true);
    if (!res) {
        return false;
    }

    if (resource != foundResource) {
        errorDescription = QObject::tr("Added and found in local storage resources don't match");
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

    evernote::edam::ResourceAttributes resourceAttributes = GetDeserializedResourceAttributes(modifiedResource.resourceAttributes());

    resourceAttributes.sourceURL = "Modified source URL";
    resourceAttributes.timestamp += 1;
    resourceAttributes.latitude = 2.0;
    resourceAttributes.longitude = 2.0;
    resourceAttributes.altitude = 2.0;
    resourceAttributes.cameraMake = "Modified camera make";
    resourceAttributes.cameraModel = "Modified camera model";

    QByteArray serializedResourceAttributes = GetSerializedResourceAttributes(resourceAttributes);

    modifiedResource.setResourceAttributes(serializedResourceAttributes);

    res = localStorageManager.UpdateResource(modifiedResource, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindResource(resourceGuid, foundResource, errorDescription,
                                           /* withBinaryData = */ true);
    if (!res) {
        return false;
    }

    if (modifiedResource != foundResource) {
        errorDescription = QObject::tr("Updated and found in local storage resources don't match");
        QNWARNING(errorDescription << ": IResource updated in LocalStorageManager: " << modifiedResource
                  << "\nIResource found in LocalStorageManager: " << foundResource);
        return false;
    }

    // ========== Check Expunge + Find (falure expected) ==========
    res = localStorageManager.ExpungeResource(modifiedResource, errorDescription);
    if (!res) {
        return false;
    }

    res = localStorageManager.FindResource(resourceGuid, foundResource, errorDescription);
    if (res) {
        errorDescription = "Error: found IResource which should have been expunged from LocalStorageManager";
        QNWARNING(errorDescription << ": IResource expunged from LocalStorageManager: " << modifiedResource
                  << "\nIResource found in LocalStorageManager: " << foundResource);
        return false;
    }

    return true;
}

bool TestNoteAddFindUpdateDeleteExpungeInLocalStorage(const Note & note,
                                                      LocalStorageManager & localStorageManager,
                                                      QString & errorDescription)
{
    if (!note.checkParameters(errorDescription)) {
        QNWARNING("Found invalid Note: " << note);
        return false;
    }

    // ========== Check Add + Find ==========
    bool res = localStorageManager.AddNote(note, errorDescription);
    if (!res) {
        return false;
    }

    const QString noteGuid = note.guid();
    const bool withResourceBinaryData = true;
    Note foundNote;
    res = localStorageManager.FindNote(noteGuid, foundNote, errorDescription,
                                       withResourceBinaryData);
    if (!res) {
        return false;
    }

    if (note != foundNote) {
        errorDescription = QObject::tr("Added and found notes in local storage don't match");
        QNWARNING(errorDescription << ": Note added to LocalStorageManager: " << note
                  << "\nNote found in LocalStorageManager: " << foundNote);
        return false;
    }

    // ========== Check Update + Find ==========
    // TODO: continue from here

    return true;
}

}
}
