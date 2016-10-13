/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LocalStorageManagerTests.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/SavedSearch.h>
#include <quentier/types/LinkedNotebook.h>
#include <quentier/types/Tag.h>
#include <quentier/types/Resource.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/SharedNotebook.h>
#include <quentier/types/User.h>
#include <quentier/utility/Utility.h>

namespace quentier {
namespace test {

bool TestSavedSearchAddFindUpdateExpungeInLocalStorage(SavedSearch & search,
                                                       LocalStorageManager & localStorageManager,
                                                       QString & errorDescription)
{
    QNLocalizedString errorMessage;

    if (!search.checkParameters(errorMessage)) {
        errorDescription = errorMessage.nonLocalizedString();
        QNWARNING(QStringLiteral("Found invalid SavedSearch: ") << search << QStringLiteral(", error: ") << errorDescription);
        return false;
    }

    // ======== Check Add + Find ============
    bool res = localStorageManager.addSavedSearch(search, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    const QString searchGuid = search.localUid();
    SavedSearch foundSearch;
    foundSearch.setLocalUid(searchGuid);
    res = localStorageManager.findSavedSearch(foundSearch, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (search != foundSearch) {
        errorDescription = QStringLiteral("Added and found saved searches in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": SavedSearch added to LocalStorageManager: ") << search
                  << QStringLiteral("\nSavedSearch found in LocalStorageManager: ") << foundSearch);
        return false;
    }

    // ========= Check Find by name =============
    SavedSearch foundByNameSearch;
    foundByNameSearch.unsetLocalUid();
    foundByNameSearch.setName(search.name());
    res = localStorageManager.findSavedSearch(foundByNameSearch, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (search != foundByNameSearch) {
        errorDescription = QStringLiteral("Added and found by name saved searches in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": SavedSearch added to LocalStorageManager: ") << search
                  << QStringLiteral("\nSaved search found by name in LocalStorageManager: ") << foundByNameSearch);
        return false;
    }

    // ========= Check Update + Find =============
    SavedSearch modifiedSearch(search);
    modifiedSearch.setUpdateSequenceNumber(search.updateSequenceNumber() + 1);
    modifiedSearch.setName(search.name() + QStringLiteral("_modified"));
    modifiedSearch.setQuery(search.query() + QStringLiteral("_modified"));
    modifiedSearch.setFavorited(true);
    modifiedSearch.setDirty(true);

    QString localUid = modifiedSearch.localUid();
    modifiedSearch.unsetLocalUid();

    res = localStorageManager.updateSavedSearch(modifiedSearch, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findSavedSearch(foundSearch, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    modifiedSearch.setLocalUid(localUid);
    if (modifiedSearch != foundSearch) {
        errorDescription = QStringLiteral("Updated and found saved searches in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": SavedSearch updated in LocalStorageManager: ") << modifiedSearch
                  << QStringLiteral("\nSavedSearch found in LocalStorageManager: ") << foundSearch);
        return false;
    }

    // ========== Check savedSearchCount to return 1 ============
    int count = localStorageManager.savedSearchCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("GetSavedSearchCount returned result different from the expected one (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ============ Check Expunge + Find (failure expected) ============
    res = localStorageManager.expungeSavedSearch(modifiedSearch, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findSavedSearch(foundSearch, errorMessage);
    if (res) {
        errorDescription = QStringLiteral("Error: found saved search which should have been expunged from local storage");
        QNWARNING(errorDescription << QStringLiteral(": SavedSearch expunged from LocalStorageManager: ") << modifiedSearch
                  << QStringLiteral("\nSavedSearch found in LocalStorageManager: ") << foundSearch);
        return false;
    }

    // ========== Check savedSearchCount to return 0 ============
    count = localStorageManager.savedSearchCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 0) {
        errorDescription = QStringLiteral("savedSearchCount returned result different from the expected one (0): ");
        errorDescription += QString::number(count);
        return false;
    }

    return true;
}

bool TestLinkedNotebookAddFindUpdateExpungeInLocalStorage(const LinkedNotebook & linkedNotebook,
                                                          LocalStorageManager & localStorageManager,
                                                          QString & errorDescription)
{
    QNLocalizedString errorMessage;

    if (!linkedNotebook.checkParameters(errorMessage)) {
        errorDescription = errorMessage.nonLocalizedString();
        QNWARNING(QStringLiteral("Found invalid LinkedNotebook: ") << linkedNotebook << QStringLiteral(", error: ") << errorDescription);
        return false;
    }

    // ========== Check Add + Find ===========
    bool res = localStorageManager.addLinkedNotebook(linkedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    const QString linkedNotebookGuid = linkedNotebook.guid();
    LinkedNotebook foundLinkedNotebook;
    foundLinkedNotebook.setGuid(linkedNotebookGuid);
    res = localStorageManager.findLinkedNotebook(foundLinkedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (linkedNotebook != foundLinkedNotebook) {
        errorDescription = QStringLiteral("Added and found linked noteboks in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": LinkedNotebook added to LocalStorageManager: ") << linkedNotebook
                  << QStringLiteral("\nLinkedNotebook found in LocalStorageManager: ") << foundLinkedNotebook);
        return false;
    }

    // =========== Check Update + Find ===========
    LinkedNotebook modifiedLinkedNotebook(linkedNotebook);
    modifiedLinkedNotebook.setUpdateSequenceNumber(linkedNotebook.updateSequenceNumber() + 1);
    modifiedLinkedNotebook.setShareName(linkedNotebook.shareName() + QStringLiteral("_modified"));
    modifiedLinkedNotebook.setUsername(linkedNotebook.username() + QStringLiteral("_modified"));
    modifiedLinkedNotebook.setShardId(linkedNotebook.shardId() + QStringLiteral("_modified"));
    modifiedLinkedNotebook.setSharedNotebookGlobalId(linkedNotebook.sharedNotebookGlobalId() + QStringLiteral("_modified"));
    modifiedLinkedNotebook.setUri(linkedNotebook.uri() + QStringLiteral("_modified"));
    modifiedLinkedNotebook.setNoteStoreUrl(linkedNotebook.noteStoreUrl() + QStringLiteral("_modified"));
    modifiedLinkedNotebook.setWebApiUrlPrefix(linkedNotebook.webApiUrlPrefix() + QStringLiteral("_modified"));
    modifiedLinkedNotebook.setStack(linkedNotebook.stack() + QStringLiteral("_modified"));
    modifiedLinkedNotebook.setBusinessId(linkedNotebook.businessId() + 1);

    res = localStorageManager.updateLinkedNotebook(modifiedLinkedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findLinkedNotebook(foundLinkedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (modifiedLinkedNotebook != foundLinkedNotebook) {
        errorDescription = QStringLiteral("Updated and found linked notebooks in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": LinkedNotebook updated in LocalStorageManager: ") << modifiedLinkedNotebook
                  << QStringLiteral("\nLinkedNotebook found in LocalStorageManager: ") << foundLinkedNotebook);
        return false;
    }

    // ========== Check linkedNotebookCount to return 1 ============
    int count = localStorageManager.linkedNotebookCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("linkedNotebookCount returned result different from the expected one (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ============= Check Expunge + Find (failure expected) ============
    res = localStorageManager.expungeLinkedNotebook(modifiedLinkedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findLinkedNotebook(foundLinkedNotebook, errorMessage);
    if (res) {
        errorDescription = QStringLiteral("Error: found linked notebook which should have been expunged from local storage");
        QNWARNING(errorDescription << QStringLiteral(": LinkedNotebook expunged from LocalStorageManager: ") << modifiedLinkedNotebook
                  << QStringLiteral("\nLinkedNotebook found in LocalStorageManager: ") << foundLinkedNotebook);
        return false;
    }

    // ========== Check linkedNotebookCount to return 0 ============
    count = localStorageManager.linkedNotebookCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 0) {
        errorDescription = QStringLiteral("GetLinkedNotebookCount returned result different from the expected one (0): ");
        errorDescription += QString::number(count);
        return false;
    }

    return true;
}

bool TestTagAddFindUpdateExpungeInLocalStorage(Tag & tag,
                                               LocalStorageManager & localStorageManager,
                                               QString & errorDescription)
{
    QNLocalizedString errorMessage;

    if (!tag.checkParameters(errorMessage)) {
        errorDescription = errorMessage.nonLocalizedString();
        QNWARNING(QStringLiteral("Found invalid Tag: ") << tag << QStringLiteral(", error: ") << errorDescription);
        return false;
    }

    // ========== Check Add + Find ==========
    bool res = localStorageManager.addTag(tag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    const QString localTagGuid = tag.localUid();
    Tag foundTag;
    foundTag.setLocalUid(localTagGuid);
    if (tag.hasLinkedNotebookGuid()) {
        foundTag.setLinkedNotebookGuid(tag.linkedNotebookGuid());
    }

    res = localStorageManager.findTag(foundTag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (tag != foundTag) {
        errorDescription = QStringLiteral("Added and found tags in local storage tags don't match");
        QNWARNING(errorDescription << QStringLiteral(": Tag added to LocalStorageManager: ") << tag
                  << QStringLiteral("\nTag found in LocalStorageManager: ") << foundTag);
        return false;
    }

    // ========== Check Find by name ==========
    Tag foundByNameTag;
    foundByNameTag.unsetLocalUid();
    foundByNameTag.setName(tag.name());
    if (tag.hasLinkedNotebookGuid()) {
        foundByNameTag.setLinkedNotebookGuid(tag.linkedNotebookGuid());
    }

    res = localStorageManager.findTag(foundByNameTag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (tag != foundByNameTag) {
        errorDescription = QStringLiteral("Tag found by name in local storage doesn't match the original tag");
        QNWARNING(errorDescription << QStringLiteral(": Tag found by name: ") << foundByNameTag << QStringLiteral("\nOriginal tag: ") << tag);
        return false;
    }

    // ========== Check Update + Find ==========
    Tag modifiedTag(tag);
    modifiedTag.setUpdateSequenceNumber(tag.updateSequenceNumber() + 1);
    modifiedTag.setLinkedNotebookGuid(QStringLiteral(""));
    modifiedTag.setName(tag.name() + QStringLiteral("_modified"));
    modifiedTag.setFavorited(true);
    modifiedTag.unsetLocalUid();

    res = localStorageManager.updateTag(modifiedTag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (!modifiedTag.hasLinkedNotebookGuid()) {
        foundTag.setLinkedNotebookGuid(QStringLiteral(""));
    }

    res = localStorageManager.findTag(foundTag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    modifiedTag.setLocalUid(localTagGuid);
    if (modifiedTag != foundTag) {
        errorDescription = QStringLiteral("Updated and found tags in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": Tag updated in LocalStorageManaged: ") << modifiedTag
                  << QStringLiteral("\nTag found in LocalStorageManager: ") << foundTag);
        return false;
    }

    // ========== tagCount to return 1 ============
    int count = localStorageManager.tagCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("tagCount returned result different from the expected one (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Add another tag referencing the first tag as its parent =========
    Tag newTag;
    newTag.setName(QStringLiteral("New tag"));
    newTag.setParentGuid(tag.guid());
    newTag.setParentLocalUid(tag.localUid());

    res = localStorageManager.addTag(newTag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    Tag foundNewTag;
    foundNewTag.setLocalUid(newTag.localUid());
    res = localStorageManager.findTag(foundNewTag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (newTag != foundNewTag) {
        errorDescription = QStringLiteral("Second added tag and its found copy from the local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": the second tag added to LocalStorageManager: ") << newTag
                  << QStringLiteral("\nTag found in LocalStorageManager: ") << foundNewTag);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    res = localStorageManager.expungeTag(modifiedTag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findTag(foundTag, errorMessage);
    if (res) {
        errorDescription = QStringLiteral("Error: found tag which should have been exounged from local storage");
        QNWARNING(errorDescription << QStringLiteral(": Tag expunged from LocalStorageManager: ") << modifiedTag
                  << QStringLiteral("\nTag found in LocalStorageManager: ") << foundTag);
        return false;
    }

    return true;
}

bool TestResourceAddFindUpdateExpungeInLocalStorage(Resource & resource, LocalStorageManager & localStorageManager,
                                                    QString & errorDescription)
{
    QNLocalizedString errorMessage;

    if (!resource.checkParameters(errorMessage)) {
        errorDescription = errorMessage.nonLocalizedString();
        QNWARNING(QStringLiteral("Found invalid Resource: ") << resource);
        return false;
    }

    // ========== Check Add + Find ==========
    bool res = localStorageManager.addEnResource(resource, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    const QString resourceGuid = resource.guid();
    Resource foundResource;
    foundResource.setGuid(resourceGuid);
    res = localStorageManager.findEnResource(foundResource, errorMessage,
                                             /* withBinaryData = */ true);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (resource != foundResource) {
        errorDescription = QStringLiteral("Added and found in local storage resources don't match");
        QNWARNING(errorDescription << QStringLiteral(": Resource added to LocalStorageManager: ") << resource
                  << QStringLiteral("\nIResource found in LocalStorageManager: ") << foundResource);
        return false;
    }

    // ========== Check Update + Find ==========
    Resource modifiedResource(resource);
    modifiedResource.setUpdateSequenceNumber(resource.updateSequenceNumber() + 1);
    modifiedResource.setDataBody(resource.dataBody() + QByteArray("_modified"));
    modifiedResource.setDataSize(modifiedResource.dataBody().size());
    modifiedResource.setDataHash(QByteArray("Fake hash      3"));

    modifiedResource.setWidth(resource.width() + 1);
    modifiedResource.setHeight(resource.height() + 1);
    modifiedResource.setRecognitionDataBody(QByteArray("<recoIndex docType=\"picture\" objType=\"image\" objID=\"fc83e58282d8059be17debabb69be900\" "
                                                       "engineVersion=\"5.5.22.7\" recoType=\"service\" lang=\"en\" objWidth=\"2398\" objHeight=\"1798\"> "
                                                       "<item x=\"437\" y=\"589\" w=\"1415\" h=\"190\">"
                                                       "<t w=\"87\">OVER ?</t>"
                                                       "<t w=\"83\">AVER NOTE</t>"
                                                       "<t w=\"82\">PVERNOTE</t>"
                                                       "<t w=\"71\">QVER NaTE</t>"
                                                       "<t w=\"67\">LVER nine</t>"
                                                       "<t w=\"67\">KVER none</t>"
                                                       "<t w=\"66\">JVER not</t>"
                                                       "<t w=\"62\">jver NOTE</t>"
                                                       "<t w=\"62\">hven NOTE</t>"
                                                       "<t w=\"61\">eVER nose</t>"
                                                       "<t w=\"50\">pV£RNoTE</t>"
                                                       "</item>"
                                                       "<item x=\"1840\" y=\"1475\" w=\"14\" h=\"12\">"
                                                       "<t w=\"11\">et</t>"
                                                       "<t w=\"10\">TQ</t>"
                                                       "</item>"
                                                       "</recoIndex>"));
    modifiedResource.setRecognitionDataSize(modifiedResource.recognitionDataBody().size());
    modifiedResource.setRecognitionDataHash(QByteArray("Fake hash      4"));
    modifiedResource.setAlternateDataBody(resource.alternateDataBody() + QByteArray("_modified"));
    modifiedResource.setAlternateDataSize(modifiedResource.alternateDataBody().size());
    modifiedResource.setAlternateDataHash(QByteArray("Fake hash      5"));

    qevercloud::ResourceAttributes & resourceAttributes = modifiedResource.resourceAttributes();

    resourceAttributes.sourceURL = QStringLiteral("Modified source URL");
    resourceAttributes.timestamp += 1;
    resourceAttributes.latitude = 2.0;
    resourceAttributes.longitude = 2.0;
    resourceAttributes.altitude = 2.0;
    resourceAttributes.cameraMake = QStringLiteral("Modified camera make");
    resourceAttributes.cameraModel = QStringLiteral("Modified camera model");

    modifiedResource.unsetLocalUid();

    res = localStorageManager.updateEnResource(modifiedResource, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findEnResource(foundResource, errorMessage,
                                             /* withBinaryData = */ true);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (modifiedResource != foundResource) {
        errorDescription = QStringLiteral("Updated and found in local storage resources don't match");
        QNWARNING(errorDescription << QStringLiteral(": Resource updated in LocalStorageManager: ") << modifiedResource
                  << QStringLiteral("\nIResource found in LocalStorageManager: ") << foundResource);
        return false;
    }

    // ========== Check Find without resource binary data =========
    foundResource.clear();
    foundResource.setGuid(resourceGuid);
    res = localStorageManager.findEnResource(foundResource, errorMessage,
                                             /* withBinaryData = */ false);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    modifiedResource.setDataBody(QByteArray());
    modifiedResource.setRecognitionDataBody(QByteArray());
    modifiedResource.setAlternateDataBody(QByteArray());

    if (modifiedResource != foundResource) {
        errorDescription = QStringLiteral("Updated and found in local storage resources without binary data don't match");
        QNWARNING(errorDescription << QStringLiteral(": Resource updated in LocalStorageManager: ") << modifiedResource
                  << QStringLiteral("\nIResource found in LocalStorageManager: ") << foundResource);
        return false;
    }


    // ========== enResourceCount to return 1 ============
    int count = localStorageManager.enResourceCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("enResourceCount returned result different from the expected one (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (falure expected) ==========
    res = localStorageManager.expungeEnResource(modifiedResource, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findEnResource(foundResource, errorMessage);
    if (res) {
        errorDescription = QStringLiteral("Error: found Resource which should have been expunged from LocalStorageManager");
        QNWARNING(errorDescription << QStringLiteral(": Resource expunged from LocalStorageManager: ") << modifiedResource
                  << QStringLiteral("\nIResource found in LocalStorageManager: ") << foundResource);
        return false;
    }

    // ========== enResourceCount to return 0 ============
    count = localStorageManager.enResourceCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 0) {
        errorDescription = QStringLiteral("enResourceCount returned result different from the expected one (0): ");
        errorDescription += QString::number(count);
        return false;
    }

    return true;
}

bool TestNoteFindUpdateDeleteExpungeInLocalStorage(Note & note, const Notebook & notebook,
                                                   LocalStorageManager & localStorageManager,
                                                   QString & errorDescription)
{
    QNLocalizedString errorMessage;

    if (!note.checkParameters(errorMessage)) {
        errorDescription = errorMessage.nonLocalizedString();
        QNWARNING(QStringLiteral("Found invalid Note: ") << note);
        return false;
    }

    // ========== Check Find ==========
    const QString initialResourceGuid = QStringLiteral("00000000-0000-0000-c000-000000000049");
    Resource foundResource;
    foundResource.setGuid(initialResourceGuid);
    bool res = localStorageManager.findEnResource(foundResource, errorMessage,
                                                  /* withBinaryData = */ true);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    const QString noteGuid = note.guid();
    const bool withResourceBinaryData = true;
    Note foundNote;
    foundNote.setGuid(noteGuid);
    res = localStorageManager.findNote(foundNote, errorMessage, withResourceBinaryData);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // NOTE: foundNote was searched by guid and might have another local uid is the original note
    // doesn't have one. So use this workaround to ensure the comparison is good for everything
    // without local uid
    if (note.localUid().isEmpty()) {
        foundNote.unsetLocalUid();
    }

    if (note != foundNote) {
        errorDescription = QStringLiteral("Added and found notes in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": Note added to LocalStorageManager: ") << note
                  << QStringLiteral("\nNote found in LocalStorageManager: ") << foundNote);
        return false;
    }

    // ========== Check Update + Find ==========
    Note modifiedNote(note);
    modifiedNote.setUpdateSequenceNumber(note.updateSequenceNumber() + 1);
    modifiedNote.setTitle(note.title() + QStringLiteral("_modified"));
    modifiedNote.setCreationTimestamp(note.creationTimestamp() + 1);
    modifiedNote.setModificationTimestamp(note.modificationTimestamp() + 1);
    modifiedNote.setFavorited(true);

    qevercloud::NoteAttributes & noteAttributes = modifiedNote.noteAttributes();

    noteAttributes.subjectDate = 2;
    noteAttributes.latitude = 2.0;
    noteAttributes.longitude = 2.0;
    noteAttributes.altitude = 2.0;
    noteAttributes.author = QStringLiteral("modified author");
    noteAttributes.source = QStringLiteral("modified source");
    noteAttributes.sourceURL = QStringLiteral("modified source URL");
    noteAttributes.sourceApplication = QStringLiteral("modified source application");
    noteAttributes.shareDate = 2;

    Tag newTag;
    newTag.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000050"));
    newTag.setUpdateSequenceNumber(1);
    newTag.setName(QStringLiteral("Fake new tag name"));

    res = localStorageManager.addTag(newTag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        QNWARNING(QStringLiteral("Can't add new tag to local storage manager: ") << errorDescription);
        return false;
    }

    modifiedNote.addTagGuid(newTag.guid());
    modifiedNote.addTagLocalUid(newTag.localUid());

    Resource newResource;
    newResource.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000051"));
    newResource.setUpdateSequenceNumber(2);
    newResource.setNoteGuid(note.guid());
    newResource.setDataBody(QByteArray("Fake new resource data body"));
    newResource.setDataSize(newResource.dataBody().size());
    newResource.setDataHash(QByteArray("Fake hash      3"));
    newResource.setMime(QStringLiteral("text/plain"));
    newResource.setWidth(2);
    newResource.setHeight(2);
    newResource.setRecognitionDataBody(QByteArray("<recoIndex docType=\"picture\" objType=\"image\" objID=\"fc83e58282d8059be17debabb69be900\" "
                                                  "engineVersion=\"5.5.22.7\" recoType=\"service\" lang=\"en\" objWidth=\"2398\" objHeight=\"1798\"> "
                                                  "<item x=\"437\" y=\"589\" w=\"1415\" h=\"190\">"
                                                  "<t w=\"87\">OVER ?</t>"
                                                  "<t w=\"83\">AVER NOTE</t>"
                                                  "<t w=\"82\">PVERNOTE</t>"
                                                  "<t w=\"71\">QVER NaTE</t>"
                                                  "<t w=\"67\">LVER nine</t>"
                                                  "<t w=\"67\">KVER none</t>"
                                                  "<t w=\"66\">JVER not</t>"
                                                  "<t w=\"62\">jver NOTE</t>"
                                                  "<t w=\"62\">hven NOTE</t>"
                                                  "<t w=\"61\">eVER nose</t>"
                                                  "<t w=\"50\">pV£RNoTE</t>"
                                                  "</item>"
                                                  "<item x=\"1840\" y=\"1475\" w=\"14\" h=\"12\">"
                                                  "<t w=\"11\">et</t>"
                                                  "<t w=\"10\">TQ</t>"
                                                  "</item>"
                                                  "</recoIndex>"));
    newResource.setRecognitionDataSize(newResource.recognitionDataBody().size());
    newResource.setRecognitionDataHash(QByteArray("Fake hash      4"));

    qevercloud::ResourceAttributes & resourceAttributes = newResource.resourceAttributes();

    resourceAttributes.sourceURL = QStringLiteral("Fake resource source URL");
    resourceAttributes.timestamp = 1;
    resourceAttributes.latitude = 0.0;
    resourceAttributes.longitude = 0.0;
    resourceAttributes.altitude = 0.0;
    resourceAttributes.cameraMake = QStringLiteral("Fake resource camera make");
    resourceAttributes.cameraModel = QStringLiteral("Fake resource camera model");

    resourceAttributes.applicationData = qevercloud::LazyMap();

    resourceAttributes.applicationData->keysOnly = QSet<QString>();
    auto & keysOnly = resourceAttributes.applicationData->keysOnly.ref();
    keysOnly.reserve(1);
    keysOnly.insert(QStringLiteral("key 1"));

    resourceAttributes.applicationData->fullMap = QMap<QString, QString>();
    auto & fullMap = resourceAttributes.applicationData->fullMap.ref();
    fullMap[QStringLiteral("key 1 map")] = QStringLiteral("value 1");

    modifiedNote.addResource(newResource);

    modifiedNote.unsetLocalUid();
    modifiedNote.setNotebookLocalUid(notebook.localUid());

    res = localStorageManager.updateNote(modifiedNote, /* update resources = */ true,
                                         /* update tags = */ true, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    foundResource = Resource();
    foundResource.setGuid(newResource.guid());
    res = localStorageManager.findEnResource(foundResource, errorMessage,
                                             /* withBinaryData = */ true);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    foundResource.setNoteLocalUid(QString());
    if (foundResource != newResource)
    {
        errorDescription = QStringLiteral("Something is wrong with the new resource "
                                          "which should have been added to local storage "
                                          "along with note update: it is not equal to original resource");
        QNWARNING(errorDescription << QStringLiteral(": original resource: ") << newResource
                  << QStringLiteral("\nfound resource: ") << foundResource);
        return false;
    }

    res = localStorageManager.findNote(foundNote, errorMessage,
                                       /* withResourceBinaryData = */ true);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // NOTE: foundNote was searched by guid and might have another local uid is the original note
    // doesn't have one. So use this workaround to ensure the comparison is good for everything
    // without local uid
    if (modifiedNote.localUid().isEmpty()) {
        foundNote.unsetLocalUid();
    }

    if (modifiedNote != foundNote) {
        errorDescription = QStringLiteral("Updated and found in local storage notes don't match");
        QNWARNING(errorDescription << QStringLiteral(": Note updated in LocalStorageManager: ") << modifiedNote
                  << QStringLiteral("\nNote found in LocalStorageManager: ") << foundNote);
        return false;
    }

    // ========== noteCount to return 1 ============
    int count = localStorageManager.noteCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("noteCount returned result different from the expected one (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== noteCountPerNotebook to return 1 ===========
    count = localStorageManager.noteCountPerNotebook(notebook, errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("noteCountPerNotebook returned result different from the expected one (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== noteCountPerTag to return 1 ==========
    count = localStorageManager.noteCountPerTag(newTag, errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("noteCountPerTag returned result different from the expected one (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Delete + Find and check deleted flag ============
    modifiedNote.setActive(false);
    modifiedNote.setDeletionTimestamp(1);
    foundNote.setActive(true);
    res = localStorageManager.updateNote(modifiedNote, /* update resources = */ false,
                                         /* update tags = */ false, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findNote(foundNote, errorMessage,
                                       /* withResourceBinaryData = */ true);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (!foundNote.hasActive() || foundNote.active()) {
        errorDescription = QStringLiteral("Note which should have been marked non-active "
                                          "is not marked so after LocalStorageManager::FindNote");
        QNWARNING(errorDescription << QStringLiteral(": Note found in LocalStorageManager: ") << foundNote);
        return false;
    }

    // ========== noteCount to return 0 ============
    count = localStorageManager.noteCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 0) {
        errorDescription = QStringLiteral("noteCount returned result different from the expected one (0): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    res = localStorageManager.expungeNote(modifiedNote, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findNote(foundNote, errorMessage,
                                       /* withResourceBinaryData = */ true);
    if (res) {
        errorDescription = QStringLiteral("Error: found Note which should have been expunged "
                                          "from LocalStorageManager");
        QNWARNING(errorDescription << QStringLiteral(": Note expunged from LocalStorageManager: ") << modifiedNote
                  << QStringLiteral("\nNote found in LocalStorageManager: ") << foundNote);
        return false;
    }

    // ========== Try to find resource belonging to expunged note (failure expected) ==========
    foundResource = Resource();
    foundResource.setGuid(newResource.guid());
    res = localStorageManager.findEnResource(foundResource, errorMessage,
                                             /* withBinaryData = */ true);
    if (res) {
        errorDescription = QStringLiteral("Error: found Resource which should have been expunged "
                                          "from LocalStorageManager along with Note owning it");
        QNWARNING(errorDescription << QStringLiteral(": Note expunged from LocalStorageManager: ") << modifiedNote
                  << QStringLiteral("\nResource found in LocalStorageManager: ") << foundResource);
        return false;
    }

    return true;
}

bool TestNotebookFindUpdateDeleteExpungeInLocalStorage(Notebook & notebook,
                                                       LocalStorageManager & localStorageManager,
                                                       QString & errorDescription)
{
    QNLocalizedString errorMessage;

    if (!notebook.checkParameters(errorMessage)) {
        errorDescription = errorMessage.nonLocalizedString();
        QNWARNING(QStringLiteral("Found invalid Notebook: ") << notebook);
        return false;
    }

    // =========== Check Find ============
    const QString initialNoteGuid = QStringLiteral("00000000-0000-0000-c000-000000000049");
    Note foundNote;
    foundNote.setGuid(initialNoteGuid);
    bool res = localStorageManager.findNote(foundNote, errorMessage,
                                            /* withResourceBinaryData = */ true);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    Notebook foundNotebook;
    foundNotebook.setGuid(notebook.guid());
    if (notebook.hasLinkedNotebookGuid()) {
        foundNotebook.setLinkedNotebookGuid(notebook.linkedNotebookGuid());
    }

    res = localStorageManager.findNotebook(foundNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (notebook != foundNotebook) {
        errorDescription = QStringLiteral("Added and found notebooks in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": Notebook added to LocalStorageManager: ") << notebook
                  << QStringLiteral("\nNotebook found in LocalStorageManager: ") << foundNotebook);
        return false;
    }

    // ========== Check Find by name ===========
    Notebook foundByNameNotebook;
    foundByNameNotebook.unsetLocalUid();
    foundByNameNotebook.setName(notebook.name());
    if (notebook.hasLinkedNotebookGuid()) {
        foundByNameNotebook.setLinkedNotebookGuid(notebook.linkedNotebookGuid());
    }

    res = localStorageManager.findNotebook(foundByNameNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (notebook != foundByNameNotebook) {
        errorDescription = QStringLiteral("Notebook found by name in local storage doesn't match the original notebook");
        QNWARNING(errorDescription << QStringLiteral(": Notebook found by name: ") << foundByNameNotebook
                  << QStringLiteral("\nOriginal notebook: ") << notebook);
        return false;
    }

    // ========== Check FindDefaultNotebook =========
    Notebook defaultNotebook;
    res = localStorageManager.findDefaultNotebook(defaultNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // ========== Check FindLastUsedNotebook (failure expected) ==========
    Notebook lastUsedNotebook;
    res = localStorageManager.findLastUsedNotebook(lastUsedNotebook, errorMessage);
    if (res) {
        errorDescription = QStringLiteral("Found some last used notebook which shouldn't have been found");
        QNWARNING(errorDescription << QStringLiteral(": ") << lastUsedNotebook);
        return false;
    }

    // ========== Check FindDefaultOrLastUsedNotebook ===========
    Notebook defaultOrLastUsedNotebook;
    res = localStorageManager.findDefaultOrLastUsedNotebook(defaultOrLastUsedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (defaultOrLastUsedNotebook != defaultNotebook) {
        errorDescription = QStringLiteral("Found defaultOrLastUsed notebook which should be the same "
                                          "as default notebook right now but it is not");
        QNWARNING(errorDescription << QStringLiteral(". Default notebook: ") << defaultNotebook
                  << QStringLiteral(", defaultOrLastUsedNotebook: ") << defaultOrLastUsedNotebook);
        return false;
    }

    // ========== Check Update + Find ==========
    Notebook modifiedNotebook(notebook);
    modifiedNotebook.setUpdateSequenceNumber(notebook.updateSequenceNumber() + 1);
    modifiedNotebook.setLinkedNotebookGuid(QStringLiteral(""));
    modifiedNotebook.setName(notebook.name() + QStringLiteral("_modified"));
    modifiedNotebook.setDefaultNotebook(false);
    modifiedNotebook.setLastUsed(true);
    modifiedNotebook.setModificationTimestamp(notebook.modificationTimestamp() + 1);
    modifiedNotebook.setPublishingUri(notebook.publishingUri() + QStringLiteral("_modified"));
    modifiedNotebook.setPublishingAscending(!notebook.isPublishingAscending());
    modifiedNotebook.setPublishingPublicDescription(notebook.publishingPublicDescription() + QStringLiteral("_modified"));
    modifiedNotebook.setStack(notebook.stack() + QStringLiteral("_modified"));
    modifiedNotebook.setBusinessNotebookDescription(notebook.businessNotebookDescription() + QStringLiteral("_modified"));
    modifiedNotebook.setBusinessNotebookRecommended(!notebook.isBusinessNotebookRecommended());
    modifiedNotebook.setCanExpungeNotes(false);
    modifiedNotebook.setCanEmailNotes(false);
    modifiedNotebook.setCanPublishToPublic(false);
    modifiedNotebook.setFavorited(true);

    res = localStorageManager.updateNotebook(modifiedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    foundNotebook = Notebook();
    foundNotebook.setGuid(modifiedNotebook.guid());
    if (modifiedNotebook.hasLinkedNotebookGuid()) {
        foundNotebook.setLinkedNotebookGuid(modifiedNotebook.linkedNotebookGuid());
    }

    res = localStorageManager.findNotebook(foundNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (modifiedNotebook != foundNotebook) {
        errorDescription = QStringLiteral("Updated and found notebooks in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": Notebook updated in LocalStorageManager: ") << modifiedNotebook
                  << QStringLiteral("\nNotebook found in LocalStorageManager: ") << foundNotebook);
        return false;
    }

    // ========== Check FindDefaultNotebook (failure expected) =========
    defaultNotebook = Notebook();
    res = localStorageManager.findDefaultNotebook(defaultNotebook, errorMessage);
    if (res) {
        errorDescription = QStringLiteral("Found some default notebook which shouldn't have been found");
        QNWARNING(errorDescription << QStringLiteral(": ") << defaultNotebook);
        return false;
    }

    // ========== Check FindLastUsedNotebook  ==========
    lastUsedNotebook = Notebook();
    res = localStorageManager.findLastUsedNotebook(lastUsedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // ========== Check FindDefaultOrLastUsedNotebook ===========
    defaultOrLastUsedNotebook = Notebook();
    res = localStorageManager.findDefaultOrLastUsedNotebook(defaultOrLastUsedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (defaultOrLastUsedNotebook != lastUsedNotebook) {
        errorDescription = QStringLiteral("Found defaultOrLastUsed notebook which should be the same "
                                          "as last used notebook right now but it is not");
        QNWARNING(errorDescription << QStringLiteral(". Last used notebook: ") << lastUsedNotebook
                  << QStringLiteral(", defaultOrLastUsedNotebook: ") << defaultOrLastUsedNotebook);
        return false;
    }

    // ========== Check notebookCount to return 1 ============
    int count = localStorageManager.notebookCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("notebookCount returned result different from the expected one (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    res = localStorageManager.expungeNotebook(modifiedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    res = localStorageManager.findNotebook(foundNotebook, errorMessage);
    if (res) {
        errorDescription = QStringLiteral("Error: found Notebook which should have been expunged from LocalStorageManager");
        QNWARNING(errorDescription << QStringLiteral(": Notebook expunged from LocalStorageManager: ") << modifiedNotebook
                  << QStringLiteral("\nNotebook found in LocalStorageManager: ") << foundNotebook);
        return false;
    }

    // ========== Check notebookCount to return 0 ============
    count = localStorageManager.notebookCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 0) {
        errorDescription = QStringLiteral("notebookCount returned result different from the expected one (0): ");
        errorDescription += QString::number(count);
        return false;
    }

    return true;
}

bool TestUserAddFindUpdateDeleteExpungeInLocalStorage(const User & user, LocalStorageManager & localStorageManager,
                                                      QString & errorDescription)
{
    QNLocalizedString errorMessage;

    if (!user.checkParameters(errorMessage)) {
        errorDescription = errorMessage.nonLocalizedString();
        QNWARNING(QStringLiteral("Found invalid User: ") << user);
        return false;
    }

    // ========== Check Add + Find ==========
    bool res = localStorageManager.addUser(user, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    const qint32 initialUserId = user.id();
    User foundUser;
    foundUser.setId(initialUserId);
    res = localStorageManager.findUser(foundUser, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (user != foundUser) {
        errorDescription = QStringLiteral("Added and found users in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": User added to LocalStorageManager: ") << user
                  << QStringLiteral("\nIUser found in LocalStorageManager: ") << foundUser);
        return false;
    }

    // ========== Check Update + Find ==========
    User modifiedUser;
    modifiedUser.setId(user.id());
    modifiedUser.setUsername(user.username() + QStringLiteral("_modified"));
    modifiedUser.setEmail(user.email() + QStringLiteral("_modified"));
    modifiedUser.setName(user.name() + QStringLiteral("_modified"));
    modifiedUser.setTimezone(user.timezone() + QStringLiteral("_modified"));
    modifiedUser.setPrivilegeLevel(user.privilegeLevel());
    modifiedUser.setCreationTimestamp(user.creationTimestamp());
    modifiedUser.setModificationTimestamp(user.modificationTimestamp() + 1);
    modifiedUser.setActive(true);

    qevercloud::UserAttributes modifiedUserAttributes;
    modifiedUserAttributes = user.userAttributes();
    modifiedUserAttributes.defaultLocationName->append(QStringLiteral("_modified"));
    modifiedUserAttributes.comments->append(QStringLiteral("_modified"));
    modifiedUserAttributes.preferredCountry->append(QStringLiteral("_modified"));
    modifiedUserAttributes.businessAddress->append(QStringLiteral("_modified"));

    modifiedUser.setUserAttributes(std::move(modifiedUserAttributes));

    qevercloud::BusinessUserInfo modifiedBusinessUserInfo;
    modifiedBusinessUserInfo = user.businessUserInfo();
    modifiedBusinessUserInfo.businessName->append(QStringLiteral("_modified"));
    modifiedBusinessUserInfo.email->append(QStringLiteral("_modified"));

    modifiedUser.setBusinessUserInfo(std::move(modifiedBusinessUserInfo));

    qevercloud::Accounting modifiedAccounting;
    modifiedAccounting = user.accounting();
    modifiedAccounting.premiumOrderNumber->append(QStringLiteral("_modified"));
    modifiedAccounting.premiumSubscriptionNumber->append(QStringLiteral("_modified"));
    modifiedAccounting.updated += 1;

    modifiedUser.setAccounting(std::move(modifiedAccounting));

    qevercloud::AccountLimits modifiedAccountLimits;
    modifiedAccountLimits = user.accountLimits();
    modifiedAccountLimits.noteTagCountMax = 2;
    modifiedAccountLimits.userLinkedNotebookMax = 2;
    modifiedAccountLimits.userNotebookCountMax = 2;

    modifiedUser.setAccountLimits(std::move(modifiedAccountLimits));

    res = localStorageManager.updateUser(modifiedUser, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    foundUser.clear();
    foundUser.setId(modifiedUser.id());
    res = localStorageManager.findUser(foundUser, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (modifiedUser != foundUser) {
        errorDescription = QStringLiteral("Updated and found users in local storage don't match");
        QNWARNING(errorDescription << QStringLiteral(": User updated in LocalStorageManager: ") << modifiedUser
                  << QStringLiteral("\nIUser found in LocalStorageManager: ") << foundUser);
        return false;
    }

    // ========== Check userCount to return 1 ===========
    int count = localStorageManager.userCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 1) {
        errorDescription = QStringLiteral("userCount returned value different from expected (1): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Delete + Find ==========
    modifiedUser.setDeletionTimestamp(5);

    res = localStorageManager.deleteUser(modifiedUser, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    foundUser.clear();
    foundUser.setId(modifiedUser.id());
    res = localStorageManager.findUser(foundUser, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (modifiedUser != foundUser) {
        errorDescription = QStringLiteral("Deleted and found users in local storage manager don't match");
        QNWARNING(errorDescription << QStringLiteral(": User marked deleted in LocalStorageManager: ") << modifiedUser
                  << QStringLiteral("\nIUser found in LocalStorageManager: ") << foundUser);
        return false;
    }

    // ========== Check userCount to return 0 (as it doesn't account for deleted resources) ===========
    count = localStorageManager.userCount(errorMessage);
    if (count < 0) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }
    else if (count != 0) {
        errorDescription = QStringLiteral("userCount returned value different from expected (0): ");
        errorDescription += QString::number(count);
        return false;
    }

    // ========== Check Expunge + Find (failure expected) ==========
    res = localStorageManager.expungeUser(modifiedUser, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    foundUser.clear();
    foundUser.setId(modifiedUser.id());
    res = localStorageManager.findUser(foundUser, errorMessage);
    if (res) {
        errorDescription = QStringLiteral("Error: found User which should have been expunged from LocalStorageManager");
        QNWARNING(errorDescription << QStringLiteral(": User expunged from LocalStorageManager: ") << modifiedUser
                  << QStringLiteral("\nIUser found in LocalStorageManager: ") << foundUser);
        return false;
    }

    return true;
}

bool TestSequentialUpdatesInLocalStorage(QString & errorDescription)
{
    // 1) ========== Create LocalStorageManager =============

    const bool startFromScratch = true;
    const bool overrideLock = false;
    LocalStorageManager localStorageManager(QStringLiteral("LocalStorageManagerSequentialUpdatesTestFakeUser"),
                                            0, startFromScratch, overrideLock);

    // 2) ========== Create User ============
    User   user;
    user.setId(1);
    user.setUsername(QStringLiteral("checker"));
    user.setEmail(QStringLiteral("mail@checker.com"));
    user.setTimezone(QStringLiteral("Europe/Moscow"));
    user.setPrivilegeLevel(qevercloud::PrivilegeLevel::NORMAL);
    user.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    user.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    user.setActive(true);

    qevercloud::UserAttributes userAttributes;
    userAttributes.defaultLocationName = QStringLiteral("Default location");
    userAttributes.comments = QStringLiteral("My comment");
    userAttributes.preferredLanguage = QStringLiteral("English");

    userAttributes.viewedPromotions = QStringList();
    userAttributes.viewedPromotions.ref() << QStringLiteral("Promotion #1")
                                          << QStringLiteral("Promotion #2")
                                          << QStringLiteral("Promotion #3");

    userAttributes.recentMailedAddresses = QStringList();
    userAttributes.recentMailedAddresses.ref() << QStringLiteral("Recent mailed address #1")
                                               << QStringLiteral("Recent mailed address #2")
                                               << QStringLiteral("Recent mailed address #3");

    user.setUserAttributes(std::move(userAttributes));

    qevercloud::Accounting accounting;
    accounting.premiumOrderNumber = QStringLiteral("Premium order number");
    accounting.premiumSubscriptionNumber = QStringLiteral("Premium subscription number");
    accounting.updated = QDateTime::currentMSecsSinceEpoch();

    user.setAccounting(std::move(accounting));

    qevercloud::BusinessUserInfo businessUserInfo;
    businessUserInfo.businessName = QStringLiteral("Business name");
    businessUserInfo.email = QStringLiteral("Business email");

    user.setBusinessUserInfo(std::move(businessUserInfo));

    qevercloud::AccountLimits accountLimits;
    accountLimits.noteResourceCountMax = 20;
    accountLimits.userNoteCountMax = 200;
    accountLimits.userSavedSearchesMax = 100;

    user.setAccountLimits(std::move(accountLimits));

    QNLocalizedString errorMessage;

    // 3) ============ Add user to local storage ==============
    bool res = localStorageManager.addUser(user, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // 4) ============ Create new user without all the supplementary data but with the same id
    //                 and update it in local storage ===================
    User updatedUser;
    updatedUser.setId(1);
    updatedUser.setUsername(QStringLiteral("checker"));
    updatedUser.setEmail(QStringLiteral("mail@checker.com"));
    updatedUser.setPrivilegeLevel(qevercloud::PrivilegeLevel::NORMAL);
    updatedUser.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    updatedUser.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    updatedUser.setActive(true);

    res = localStorageManager.updateUser(updatedUser, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // 5) =========== Find this user in local storage, check whether it has user attributes,
    //                accounting, business user info and premium info (it shouldn't) =========
    User foundUser;
    foundUser.setId(1);

    res = localStorageManager.findUser(foundUser, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (foundUser.hasUserAttributes()) {
        errorDescription = QStringLiteral("Updated user found in local storage still has user attributes "
                                          "while it shouldn't have them after the update");
        QNWARNING(errorDescription << QStringLiteral(": initial user: ") << user << QStringLiteral("\nUpdated user: ")
                  << updatedUser << QStringLiteral("\nFound user: ") << foundUser);
        return false;
    }

    if (foundUser.hasAccounting()) {
        errorDescription = QStringLiteral("Updated user found in local storage still has accounting "
                                          "while it shouldn't have it after the update");
        QNWARNING(errorDescription << QStringLiteral(": initial user: ") << user << QStringLiteral("\nUpdated user: ")
                  << updatedUser << QStringLiteral("\nFound user: ") << foundUser);
        return false;
    }

    if (foundUser.hasBusinessUserInfo()) {
        errorDescription = QStringLiteral("Updated user found in local storage still has business user info "
                                          "while it shouldn't have it after the update");
        QNWARNING(errorDescription << QStringLiteral(": initial user: ") << user << QStringLiteral("\nUpdated user: ")
                  << updatedUser << QStringLiteral("\nFound user: ") << foundUser);
        return false;
    }

    if (foundUser.hasAccountLimits()) {
        errorDescription = QStringLiteral("Updated user found in local storage still has account limits "
                                          "while it shouldn't have them after the update");
        QNWARNING(errorDescription << QStringLiteral(": initial user: ") << user << QStringLiteral("\nUpdated user: ")
                  << updatedUser << QStringLiteral("\nFound user: ") << foundUser);
        return false;
    }

    // ============ 6) Create Notebook with restrictions and shared notebooks ==================
    Notebook notebook;
    notebook.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000049"));
    notebook.setUpdateSequenceNumber(1);
    notebook.setName(QStringLiteral("Fake notebook name"));
    notebook.setCreationTimestamp(1);
    notebook.setModificationTimestamp(1);
    notebook.setDefaultNotebook(true);
    notebook.setLastUsed(false);
    notebook.setPublishingUri(QStringLiteral("Fake publishing uri"));
    notebook.setPublishingOrder(1);
    notebook.setPublishingAscending(true);
    notebook.setPublishingPublicDescription(QStringLiteral("Fake public description"));
    notebook.setPublished(true);
    notebook.setStack(QStringLiteral("Fake notebook stack"));
    notebook.setBusinessNotebookDescription(QStringLiteral("Fake business notebook description"));
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

    SharedNotebook sharedNotebook;
    sharedNotebook.setId(1);
    sharedNotebook.setUserId(1);
    sharedNotebook.setNotebookGuid(notebook.guid());
    sharedNotebook.setEmail(QStringLiteral("Fake shared notebook email"));
    sharedNotebook.setCreationTimestamp(1);
    sharedNotebook.setModificationTimestamp(1);
    sharedNotebook.setGlobalId(QStringLiteral("Fake shared notebook global id"));
    sharedNotebook.setUsername(QStringLiteral("Fake shared notebook username"));
    sharedNotebook.setPrivilegeLevel(1);
    sharedNotebook.setReminderNotifyEmail(true);
    sharedNotebook.setReminderNotifyApp(false);

    notebook.addSharedNotebook(sharedNotebook);

    res = localStorageManager.addNotebook(notebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // 7) ============ Update notebook: remove restrictions and shared notebooks =========
    Notebook updatedNotebook;
    updatedNotebook.setLocalUid(notebook.localUid());
    updatedNotebook.setGuid(notebook.guid());
    updatedNotebook.setUpdateSequenceNumber(1);
    updatedNotebook.setName(QStringLiteral("Fake notebook name"));
    updatedNotebook.setCreationTimestamp(1);
    updatedNotebook.setModificationTimestamp(1);
    updatedNotebook.setDefaultNotebook(true);
    updatedNotebook.setLastUsed(false);
    updatedNotebook.setPublishingUri(QStringLiteral("Fake publishing uri"));
    updatedNotebook.setPublishingOrder(1);
    updatedNotebook.setPublishingAscending(true);
    updatedNotebook.setPublishingPublicDescription(QStringLiteral("Fake public description"));
    updatedNotebook.setPublished(true);
    updatedNotebook.setStack(QStringLiteral("Fake notebook stack"));
    updatedNotebook.setBusinessNotebookDescription(QStringLiteral("Fake business notebook description"));
    updatedNotebook.setBusinessNotebookPrivilegeLevel(1);
    updatedNotebook.setBusinessNotebookRecommended(true);

    res = localStorageManager.updateNotebook(updatedNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // 8) ============= Find notebook, ensure it doesn't have neither restrictions nor shared notebooks

    Notebook foundNotebook;
    foundNotebook.setGuid(notebook.guid());

    res = localStorageManager.findNotebook(foundNotebook, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (foundNotebook.hasSharedNotebooks()) {
        errorDescription = QStringLiteral("Updated notebook found in local storage has shared notebooks "
                                          "while it shouldn't have them");
        QNWARNING(errorDescription << QStringLiteral(", original notebook: ") << notebook
                  << QStringLiteral("\nUpdated notebook: ") << updatedNotebook << QStringLiteral("\nFound notebook: ")
                  << foundNotebook);
        return false;
    }

    if (foundNotebook.hasRestrictions()) {
        errorDescription = QStringLiteral("Updated notebook found in local storage has restrictions "
                                          "while it shouldn't have them");
        QNWARNING(errorDescription << QStringLiteral(", original notebook: ") << notebook
                  << QStringLiteral("\nUpdated notebook: ") << updatedNotebook << QStringLiteral("\nFound notebook: ")
                  << foundNotebook);
        return false;
    }

    // 9) ============== Create tag =================
    Tag tag;
    tag.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000046"));
    tag.setUpdateSequenceNumber(1);
    tag.setName(QStringLiteral("Fake tag name"));

    res = localStorageManager.addTag(tag, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // 10) ============= Create note, add this tag to it along with some resource ===========
    Note note;
    note.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000045"));
    note.setUpdateSequenceNumber(1);
    note.setTitle(QStringLiteral("Fake note title"));
    note.setContent(QStringLiteral("<en-note><h1>Hello, world</h1></en-note>"));
    note.setCreationTimestamp(1);
    note.setModificationTimestamp(1);
    note.setActive(true);
    note.setNotebookGuid(notebook.guid());

    Resource resource;
    resource.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000044"));
    resource.setUpdateSequenceNumber(1);
    resource.setNoteGuid(note.guid());
    resource.setDataBody(QByteArray("Fake resource data body"));
    resource.setDataSize(resource.dataBody().size());
    resource.setDataHash(QByteArray("Fake hash      1"));

    note.addResource(resource);
    note.addTagGuid(tag.guid());
    note.setNotebookLocalUid(updatedNotebook.localUid());

    res = localStorageManager.addNote(note, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // 11) ============ Update note, remove tag guid and resource ============
    Note updatedNote;
    updatedNote.setLocalUid(note.localUid());
    updatedNote.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000045"));
    updatedNote.setUpdateSequenceNumber(1);
    updatedNote.setTitle(QStringLiteral("Fake note title"));
    updatedNote.setContent(QStringLiteral("<en-note><h1>Hello, world</h1></en-note>"));
    updatedNote.setCreationTimestamp(1);
    updatedNote.setModificationTimestamp(1);
    updatedNote.setActive(true);
    updatedNote.setNotebookGuid(notebook.guid());
    updatedNote.setNotebookLocalUid(notebook.localUid());

    res = localStorageManager.updateNote(updatedNote, /* update resources = */ true,
                                         /* update tags = */ true, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // 12) =========== Find updated note in local storage, ensure it doesn't have neither tag guids, nor resources
    Note foundNote;
    foundNote.setLocalUid(updatedNote.localUid());
    foundNote.setGuid(updatedNote.guid());

    res = localStorageManager.findNote(foundNote, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    if (foundNote.hasTagGuids()) {
        errorDescription = QStringLiteral("Updated note found in local storage has tag guids while it shouldn't have them");
        QNWARNING(errorDescription << QStringLiteral(", original note: ") << note << QStringLiteral("\nUpdated note: ")
                  << updatedNote << QStringLiteral("\nFound note: ") << foundNote);
        return false;
    }

    if (foundNote.hasResources()) {
        errorDescription = QStringLiteral("Updated note found in local storage has resources while it shouldn't have them");
        QNWARNING(errorDescription << QStringLiteral(", original note: ") << note << QStringLiteral("\nUpdated note: ")
                  << updatedNote << QStringLiteral("\nFound note: ") << foundNote);
        return false;
    }

    // 13) ============== Add resource attributes to the resource and add resource to note =============
    qevercloud::ResourceAttributes & resourceAttributes = resource.resourceAttributes();
    resourceAttributes.applicationData = qevercloud::LazyMap();
    resourceAttributes.applicationData->keysOnly = QSet<QString>();
    resourceAttributes.applicationData->fullMap = QMap<QString, QString>();

    resourceAttributes.applicationData->keysOnly.ref() << QStringLiteral("key_1") << QStringLiteral("key_2") << QStringLiteral("key_3");
    resourceAttributes.applicationData->fullMap.ref()[QStringLiteral("key_1")] = QStringLiteral("value_1");
    resourceAttributes.applicationData->fullMap.ref()[QStringLiteral("key_2")] = QStringLiteral("value_2");
    resourceAttributes.applicationData->fullMap.ref()[QStringLiteral("key_3")] = QStringLiteral("value_3");

    updatedNote.addResource(resource);

    res = localStorageManager.updateNote(updatedNote, /* update resources = */ true,
                                         /* update tags = */ true, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return res;
    }

    // 14) ================ Remove resource attributes from note's resource and update it again
    QList<Resource> resources = updatedNote.resources();
    if (resources.empty()) {
        errorDescription = QStringLiteral("Note returned empty list of resource adapters while it should have "
                                          "contained at least one entry");
        QNWARNING(errorDescription << QStringLiteral(", updated note: ") << updatedNote);
        return false;
    }

    Resource & resourceWrapper = resources[0];
    qevercloud::ResourceAttributes & underlyngResourceAttributes = resourceWrapper.resourceAttributes();
    underlyngResourceAttributes = qevercloud::ResourceAttributes();

    updatedNote.setResources(resources);

    res = localStorageManager.updateNote(updatedNote, /* update resources = */ true,
                                         /* update tags = */ true, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    // 15) ============= Find note in local storage again ===============
    res = localStorageManager.findNote(foundNote, errorMessage);
    if (!res) {
        errorDescription = errorMessage.nonLocalizedString();
        return false;
    }

    resources = foundNote.resources();
    if (resources.empty()) {
        errorDescription = QStringLiteral("Note returned empty list of resource adapters while it should have "
                                          "contained at least one entry");
        QNWARNING(errorDescription << ", found note: " << foundNote);
        return false;
    }

    Resource & foundResourceWrapper = resources[0];
    qevercloud::ResourceAttributes & foundResourceAttributes = foundResourceWrapper.resourceAttributes();
    if (foundResourceAttributes.applicationData.isSet()) {
        errorDescription = QStringLiteral("Resource from updated note has application data while it shouldn't have it");
        QNWARNING(errorDescription << QStringLiteral(", found resource: ") << foundResourceWrapper);
        return false;
    }

    return true;
}

} // namespace test
} // namespace quentier
