#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H

#include "Lists.h"
#include <tools/Linkage.h>
#include <QString>

namespace qute_note {


/**
 * @brief The IAsyncLocalStorageManager class defines the interfaces for signals and slots
 * used for asynchronous access to local storage database. Typically the interface is
 * slot like "onRequestToDoSmth" + a couple of "complete"/"failed" resulting signals
 * for each public method in LocalStorageManager.
 */
class QUTE_NOTE_EXPORT IAsyncLocalStorageManager
{
public:
    virtual ~IAsyncLocalStorageManager();

protected:
    IAsyncLocalStorageManager() = default;
    IAsyncLocalStorageManager(const IAsyncLocalStorageManager & other) = default;
    IAsyncLocalStorageManager(IAsyncLocalStorageManager && other);
    IAsyncLocalStorageManager & operator=(const IAsyncLocalStorageManager & other) = default;
    IAsyncLocalStorageManager & operator=(IAsyncLocalStorageManager && other);

    // Pure virtual prototypes for signals to be emitted from subclasses:

    // Prototypes for user-related signals:
    virtual void getUserCountComplete(int userCount) = 0;
    virtual void getUserCountFailed(QString errorDescription) = 0;

    virtual void switchUserComplete(qint32 userId) = 0;
    virtual void switchUserFailed(qint32 userId, QString errorDescription) = 0;

    virtual void addUserComplete(UserWrapper user) = 0;
    virtual void addUserFailed(UserWrapper user, QString errorDescription) = 0;

    virtual void updateUserComplete(UserWrapper user) = 0;
    virtual void updateUserFailed(UserWrapper user, QString errorDecription) = 0;

    virtual void findUserComplete(UserWrapper foundUser) = 0;
    virtual void findUserFailed(UserWrapper user, QString errorDescription) = 0;

    virtual void deleteUserComplete(UserWrapper user) = 0;
    virtual void deleteUserFailed(UserWrapper user, QString errorDescription) = 0;

    virtual void expungeUserComplete(UserWrapper user) = 0;
    virtual void expungeUserFailed(UserWrapper user, QString errorDescription) = 0;

    // Prototypes for notebook-related signals:
    virtual void getNotebookCountComplete(int notebookCount) = 0;
    virtual void getNotebookCountFailed(QString errorDescription) = 0;

    virtual void addNotebookComplete(Notebook notebook) = 0;
    virtual void addNotebookFailed(Notebook notebook, QString errorDescription) = 0;

    virtual void updateNotebookComplete(Notebook notebook) = 0;
    virtual void updateNotebookFailed(Notebook notebook, QString errorDescription) = 0;

    virtual void findNotebookComplete(Notebook foundNotebook) = 0;
    virtual void findNotebookFailed(Notebook notebook, QString errorDescription) = 0;

    virtual void findDefaultNotebookComplete(Notebook foundNotebook) = 0;
    virtual void findDefaultNotebookFailed(Notebook notebook, QString errorDescription) = 0;

    virtual void findLastUsedNotebookComplete(Notebook foundNotebook) = 0;
    virtual void findLastUsedNotebookFailed(Notebook notebook, QString errorDescription) = 0;

    virtual void findDefaultOrLastUsedNotebookComplete(Notebook foundNotebook) = 0;
    virtual void findDefaultOrLastUsedNotebookFailed(Notebook notebook, QString errorDescription) = 0;

    virtual void listAllNotebooksComplete(QList<Notebook> foundNotebooks) = 0;
    virtual void listAllNotebooksFailed(QString errorDescription) = 0;

    virtual void listAllSharedNotebooksComplete(QList<SharedNotebookWrapper> foundSharedNotebooks) = 0;
    virtual void listAllSharedNotebooksFailed(QString errorDescription) = 0;

    virtual void listSharedNotebooksPerNotebookGuidComplete(QString notebookGuid, QList<SharedNotebookWrapper> foundSharedNotebooks) = 0;
    virtual void listSharedNotebooksPerNotebookGuidFailed(QString notebookGuid, QString errorDescription) = 0;

    virtual void expungeNotebookComplete(Notebook notebook) = 0;
    virtual void expungeNotebookFailed(Notebook notebook, QString errorDescription) = 0;

    // Prototypes for linked notebook-related signals:
    virtual void getLinkedNotebookCountComplete(int linkedNotebookCount) = 0;
    virtual void getLinkedNotebookCountFailed(QString errorDescription) = 0;

    virtual void addLinkedNotebookComplete(LinkedNotebook linkedNotebook) = 0;
    virtual void addLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription) = 0;

    virtual void updateLinkedNotebookComplete(LinkedNotebook linkedNotebook) = 0;
    virtual void updateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription) = 0;

    virtual void findLinkedNotebookComplete(LinkedNotebook foundLinkedNotebook) = 0;
    virtual void findLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription) = 0;

    virtual void listAllLinkedNotebooksComplete(QList<LinkedNotebook> foundLinkedNotebooks) = 0;
    virtual void listAllLinkedNotebooksFailed(QString errorDescription) = 0;

    virtual void expungeLinkedNotebookComplete(LinkedNotebook linkedNotebook) = 0;
    virtual void expungeLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription) = 0;

    // Prototypes for note-related signals:
    virtual void getNoteCountComplete(int noteCount) = 0;
    virtual void getNoteCountFailed(QString errorDescription) = 0;

    virtual void addNoteComplete(Note note, Notebook notebook) = 0;
    virtual void addNoteFailed(Note note, Notebook notebook, QString errorDescription) = 0;

    virtual void updateNoteComplete(Note note, Notebook notebook) = 0;
    virtual void updateNoteFailed(Note note, Notebook notebook, QString errorDescription) = 0;

    virtual void findNoteComplete(Note foundNote, bool withResourceBinaryData) = 0;
    virtual void findNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription) = 0;

    virtual void listAllNotesPerNotebookComplete(Notebook notebook, bool withResourceBinaryData, QList<Note> foundNotes) = 0;
    virtual void listAllNotesPerNotebookFailed(Notebook notebook, bool withResourceBinaryData, QString errorDescription) = 0;

    virtual void deleteNoteComplete(Note note) = 0;
    virtual void deleteNoteFailed(Note note, QString errorDescription) = 0;

    virtual void expungeNoteComplete(Note note) = 0;
    virtual void expungeNoteFailed(Note note, QString errorDescription) = 0;

    // Prototypes for tag-related signals:
    virtual void getTagCountComplete(int tagCount) = 0;
    virtual void getTagCountFailed(QString errorDescription) = 0;

    virtual void addTagComplete(Tag tag) = 0;
    virtual void addTagFailed(Tag tag, QString errorDescription) = 0;

    virtual void updateTagComplete(Tag tag) = 0;
    virtual void updateTagFailed(Tag tag, QString errorDescription) = 0;

    virtual void linkTagWithNoteComplete(Tag tag, Note note) = 0;
    virtual void linkTagWithNoteFailed(Tag tag, Note note, QString errorDescription) = 0;

    virtual void findTagComplete(Tag tag) = 0;
    virtual void findTagFailed(Tag tag, QString errorDescription) = 0;

    virtual void listAllTagsPerNoteComplete(QList<Tag> foundTags, Note note) = 0;
    virtual void listAllTagsPerNoteFailed(Note note, QString errorDescription) = 0;

    virtual void listAllTagsComplete(QList<Tag> foundTags) = 0;
    virtual void listAllTagsFailed(QString errorDescription) = 0;

    virtual void deleteTagComplete(Tag tag) = 0;
    virtual void deleteTagFailed(Tag tag, QString errorDescription) = 0;

    virtual void expungeTagComplete(Tag tag) = 0;
    virtual void expungeTagFailed(Tag tag, QString errorDescription) = 0;

    // Prototypes for resource-related signals
    virtual void getResourceCountComplete(int resourceCount) = 0;
    virtual void getResourceCountFailed(QString errorDescription) = 0;

    virtual void addResourceComplete(ResourceWrapper resource, Note note) = 0;
    virtual void addResourceFailed(ResourceWrapper resource, Note note,
                                   QString errorDescription) = 0;

    virtual void updateResourceComplete(ResourceWrapper resource, Note note) = 0;
    virtual void updateResourceFailed(ResourceWrapper resource, Note note,
                                      QString errorDescription) = 0;

    virtual void findResourceComplete(ResourceWrapper resource, bool withBinaryData) = 0;
    virtual void findResourceFailed(ResourceWrapper resource, bool withBinaryData,
                                    QString errorDescription) = 0;

    virtual void expungeResourceComplete(ResourceWrapper resource) = 0;
    virtual void expungeResourceFailed(ResourceWrapper resource,
                                       QString errorDescription) = 0;

    // Prototypes for saved search-related signals:
    virtual void getSavedSearchCountComplete(int savedSearchCount) = 0;
    virtual void getSavedSearchCountFailed(QString errorDescription) = 0;

    virtual void addSavedSearchComplete(SavedSearch search) = 0;
    virtual void addSavedSearchFailed(SavedSearch search, QString errorDescription) = 0;

    virtual void updateSavedSearchComplete(SavedSearch search) = 0;
    virtual void updateSavedSearchFailed(SavedSearch search, QString errorDescription) = 0;

    virtual void findSavedSearchComplete(SavedSearch search) = 0;
    virtual void findSavedSearchFailed(SavedSearch search, QString errorDescription) = 0;

    virtual void listAllSavedSearchesComplete(QList<SavedSearch> foundSearches) = 0;
    virtual void listAllSavedSearchesFailed(QString errorDescription) = 0;

    virtual void expungeSavedSearchComplete(SavedSearch search) = 0;
    virtual void expungeSavedSearchFailed(SavedSearch search,
                                          QString errorDescription) = 0;

    // Pure virtual prototypes for slots to be invoked:

    // Pure virtual prototypes for user-related slots:
    virtual void onGetUserCountRequest() = 0;
    virtual void onSwitchUserRequest(QString username, qint32 userId, bool startFromScratch) = 0;
    virtual void onAddUserRequest(UserWrapper user) = 0;
    virtual void onUpdateUserRequest(UserWrapper user) = 0;
    virtual void onFindUserRequest(UserWrapper user) = 0;
    virtual void onDeleteUserRequest(UserWrapper user) = 0;
    virtual void onExpungeUserRequest(UserWrapper user) = 0;

    // Pure virtual prototypes for notebook-related slots:
    virtual void onGetNotebookCountRequest() = 0;
    virtual void onAddNotebookRequest(Notebook notebook) = 0;
    virtual void onUpdateNotebookRequest(Notebook notebook) = 0;
    virtual void onFindNotebookRequest(Notebook notebook) = 0;
    virtual void onFindDefaultNotebookRequest(Notebook notebook) = 0;
    virtual void onFindLastUsedNotebookRequest(Notebook notebook) = 0;
    virtual void onFindDefaultOrLastUsedNotebookRequest(Notebook notebook) = 0;
    virtual void onListAllNotebooksRequest() = 0;
    virtual void onListAllSharedNotebooksRequest() = 0;
    virtual void onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid) = 0;
    virtual void onExpungeNotebookRequest(Notebook notebook) = 0;

    // Pure virtual prototypes for linked notebook-related slots:
    virtual void onGetLinkedNotebookCountRequest() = 0;
    virtual void onAddLinkedNotebookRequest(LinkedNotebook linkedNotebook) = 0;
    virtual void onUpdateLinkedNotebookRequest(LinkedNotebook linkedNotebook) = 0;
    virtual void onFindLinkedNotebookRequest(LinkedNotebook linkedNotebook) = 0;
    virtual void onListAllLinkedNotebooksRequest() = 0;
    virtual void onExpungeLinkedNotebookRequest(LinkedNotebook linkedNotebook) = 0;

    // Pure virtual prototypes for note-related slots:
    virtual void onGetNoteCountRequest() = 0;
    virtual void onAddNoteRequest(Note note, Notebook notebook) = 0;
    virtual void onUpdateNoteRequest(Note note, Notebook notebook) = 0;
    virtual void onFindNoteRequest(Note note, bool withResourceBinaryData) = 0;
    virtual void onListAllNotesPerNotebookRequest(Notebook notebook, bool withResourceBinaryData) = 0;
    virtual void onDeleteNoteRequest(Note note) = 0;
    virtual void onExpungeNoteRequest(Note note) = 0;

    // Pure virtual prototypes for tag-related slots:
    virtual void onGetTagCountRequest() = 0;
    virtual void onAddTagRequest(Tag tag) = 0;
    virtual void onUpdateTagRequest(Tag tag) = 0;
    virtual void onLinkTagWithNoteRequest(Tag tag, Note note) = 0;
    virtual void onFindTagRequest(Tag tag) = 0;
    virtual void onListAllTagsPerNoteRequest(Note note) = 0;
    virtual void onListAllTagsRequest() = 0;
    virtual void onDeleteTagRequest(Tag tag) = 0;
    virtual void onExpungeTagRequest(Tag tag) = 0;

    // Pure virtual prototypes for resource-related slots:
    virtual void onGetResourceCountRequest() = 0;
    virtual void onAddResourceRequest(ResourceWrapper resource, Note note) = 0;
    virtual void onUpdateResourceRequest(ResourceWrapper resource, Note note) = 0;
    virtual void onFindResourceRequest(ResourceWrapper resource, bool withBinaryData) = 0;
    virtual void onExpungeResourceRequest(ResourceWrapper resource) = 0;

    // Pure virtual prototypes for saved search-related methods:
    virtual void onGetSavedSearchCountRequest() = 0;
    virtual void onAddSavedSearchRequest(SavedSearch search) = 0;
    virtual void onUpdateSavedSearchRequest(SavedSearch search) = 0;
    virtual void onFindSavedSearchRequest(SavedSearch search) = 0;
    virtual void onListAllSavedSearchesRequest() = 0;
    virtual void onExpungeSavedSearch(SavedSearch search) = 0;
};

}

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H
