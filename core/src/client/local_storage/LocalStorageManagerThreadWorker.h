#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_WORKER_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_WORKER_H

#include "LocalStorageManager.h"
#include "LocalStorageCacheManager.h"
#include <client/types/UserWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/SavedSearch.h>
#include <QObject>

namespace qute_note {

class QUTE_NOTE_EXPORT LocalStorageManagerThreadWorker: public QObject
{
    Q_OBJECT
public:
    explicit LocalStorageManagerThreadWorker(const QString & username,
                                             const qint32 userId,
                                             const bool startFromScratch,
                                             QObject * parent = nullptr);
    virtual ~LocalStorageManagerThreadWorker();
    
    void setUseCache(const bool useCache);

    const LocalStorageCacheManager * localStorageCacheManager() const;

Q_SIGNALS:
    // Generic failure signal
    void failure(QString errorDescription);

    // Sent when the initialization is complete
    void initialized();

    // User-related signals:
    void getUserCountComplete(int userCount);
    void getUserCountFailed(QString errorDescription);
    void switchUserComplete(qint32 userId);
    void switchUserFailed(qint32 userId, QString errorDescription);
    void addUserComplete(UserWrapper user);
    void addUserFailed(UserWrapper user, QString errorDescription);
    void updateUserComplete(UserWrapper user);
    void updateUserFailed(UserWrapper user, QString errorDecription);
    void findUserComplete(UserWrapper foundUser);
    void findUserFailed(UserWrapper user, QString errorDescription);
    void deleteUserComplete(UserWrapper user);
    void deleteUserFailed(UserWrapper user, QString errorDescription);
    void expungeUserComplete(UserWrapper user);
    void expungeUserFailed(UserWrapper user, QString errorDescription);

    // Notebook-related signals:
    void getNotebookCountComplete(int notebookCount);
    void getNotebookCountFailed(QString errorDescription);
    void addNotebookComplete(Notebook notebook);
    void addNotebookFailed(Notebook notebook, QString errorDescription);
    void updateNotebookComplete(Notebook notebook);
    void updateNotebookFailed(Notebook notebook, QString errorDescription);
    void findNotebookComplete(Notebook foundNotebook);
    void findNotebookFailed(Notebook notebook, QString errorDescription);
    void findDefaultNotebookComplete(Notebook foundNotebook);
    void findDefaultNotebookFailed(Notebook notebook, QString errorDescription);
    void findLastUsedNotebookComplete(Notebook foundNotebook);
    void findLastUsedNotebookFailed(Notebook notebook, QString errorDescription);
    void findDefaultOrLastUsedNotebookComplete(Notebook foundNotebook);
    void findDefaultOrLastUsedNotebookFailed(Notebook notebook, QString errorDescription);
    void listAllNotebooksComplete(QList<Notebook> foundNotebooks);
    void listAllNotebooksFailed(QString errorDescription);
    void listAllSharedNotebooksComplete(QList<SharedNotebookWrapper> foundSharedNotebooks);
    void listAllSharedNotebooksFailed(QString errorDescription);
    void listSharedNotebooksPerNotebookGuidComplete(QString notebookGuid, QList<SharedNotebookWrapper> foundSharedNotebooks);
    void listSharedNotebooksPerNotebookGuidFailed(QString notebookGuid, QString errorDescription);
    void expungeNotebookComplete(Notebook notebook);
    void expungeNotebookFailed(Notebook notebook, QString errorDescription);

    // Linked notebook-related signals:
    void getLinkedNotebookCountComplete(int linkedNotebookCount);
    void getLinkedNotebookCountFailed(QString errorDescription);
    void addLinkedNotebookComplete(LinkedNotebook linkedNotebook);
    void addLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription);
    void updateLinkedNotebookComplete(LinkedNotebook linkedNotebook);
    void updateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription);
    void findLinkedNotebookComplete(LinkedNotebook foundLinkedNotebook);
    void findLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription);
    void listAllLinkedNotebooksComplete(QList<LinkedNotebook> foundLinkedNotebooks);
    void listAllLinkedNotebooksFailed(QString errorDescription);
    void expungeLinkedNotebookComplete(LinkedNotebook linkedNotebook);
    void expungeLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription);

    // Note-related signals:
    void getNoteCountComplete(int noteCount);
    void getNoteCountFailed(QString errorDescription);
    void addNoteComplete(Note note, Notebook notebook);
    void addNoteFailed(Note note, Notebook notebook, QString errorDescription);
    void updateNoteComplete(Note note, Notebook notebook);
    void updateNoteFailed(Note note, Notebook notebook, QString errorDescription);
    void findNoteComplete(Note foundNote, bool withResourceBinaryData);
    void findNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription);
    void listAllNotesPerNotebookComplete(Notebook notebook, bool withResourceBinaryData, QList<Note> foundNotes);
    void listAllNotesPerNotebookFailed(Notebook notebook, bool withResourceBinaryData, QString errorDescription);
    void deleteNoteComplete(Note note);
    void deleteNoteFailed(Note note, QString errorDescription);
    void expungeNoteComplete(Note note);
    void expungeNoteFailed(Note note, QString errorDescription);

    // Tag-related signals:
    void getTagCountComplete(int tagCount, QUuid requestId = QUuid());
    void getTagCountFailed(QString errorDescription, QUuid requestId = QUuid());
    void addTagComplete(Tag tag, QUuid requestId = QUuid());
    void addTagFailed(Tag tag, QString errorDescription, QUuid requestId = QUuid());
    void updateTagComplete(Tag tag, QUuid requestId = QUuid());
    void updateTagFailed(Tag tag, QString errorDescription, QUuid requestId = QUuid());
    void linkTagWithNoteComplete(Tag tag, Note note, QUuid requestId = QUuid());
    void linkTagWithNoteFailed(Tag tag, Note note, QString errorDescription, QUuid requestId = QUuid());
    void findTagComplete(Tag tag, QUuid requestId = QUuid());
    void findTagFailed(Tag tag, QString errorDescription, QUuid requestId = QUuid());
    void listAllTagsPerNoteComplete(QList<Tag> foundTags, Note note, QUuid requestId = QUuid());
    void listAllTagsPerNoteFailed(Note note, QString errorDescription, QUuid requestId = QUuid());
    void listAllTagsComplete(QList<Tag> foundTags);
    void listAllTagsFailed(QString errorDescription);
    void deleteTagComplete(Tag tag);
    void deleteTagFailed(Tag tag, QString errorDescription);
    void expungeTagComplete(Tag tag);
    void expungeTagFailed(Tag tag, QString errorDescription);

    // Resource-related signals:
    void getResourceCountComplete(int resourceCount);
    void getResourceCountFailed(QString errorDescription);
    void addResourceComplete(ResourceWrapper resource, Note note);
    void addResourceFailed(ResourceWrapper resource, Note note, QString errorDescription);
    void updateResourceComplete(ResourceWrapper resource, Note note);
    void updateResourceFailed(ResourceWrapper resource, Note note, QString errorDescription);
    void findResourceComplete(ResourceWrapper resource, bool withBinaryData);
    void findResourceFailed(ResourceWrapper resource, bool withBinaryData, QString errorDescription);
    void expungeResourceComplete(ResourceWrapper resource);
    void expungeResourceFailed(ResourceWrapper resource, QString errorDescription);

    // Saved search-related signals:
    void getSavedSearchCountComplete(int savedSearchCount);
    void getSavedSearchCountFailed(QString errorDescription);
    void addSavedSearchComplete(SavedSearch search);
    void addSavedSearchFailed(SavedSearch search, QString errorDescription);
    void updateSavedSearchComplete(SavedSearch search);
    void updateSavedSearchFailed(SavedSearch search, QString errorDescription);
    void findSavedSearchComplete(SavedSearch search);
    void findSavedSearchFailed(SavedSearch search, QString errorDescription);
    void listAllSavedSearchesComplete(QList<SavedSearch> foundSearches);
    void listAllSavedSearchesFailed(QString errorDescription);
    void expungeSavedSearchComplete(SavedSearch search);
    void expungeSavedSearchFailed(SavedSearch search, QString errorDescription);
    
public Q_SLOTS:
    void init();

    // User-related slots:
    void onGetUserCountRequest();
    void onSwitchUserRequest(QString username, qint32 userId, bool startFromScratch);
    void onAddUserRequest(UserWrapper user);
    void onUpdateUserRequest(UserWrapper user);
    void onFindUserRequest(UserWrapper user);
    void onDeleteUserRequest(UserWrapper user);
    void onExpungeUserRequest(UserWrapper user);

    // Notebook-related slots:
    void onGetNotebookCountRequest();
    void onAddNotebookRequest(Notebook notebook);
    void onUpdateNotebookRequest(Notebook notebook);
    void onFindNotebookRequest(Notebook notebook);
    void onFindDefaultNotebookRequest(Notebook notebook);
    void onFindLastUsedNotebookRequest(Notebook notebook);
    void onFindDefaultOrLastUsedNotebookRequest(Notebook notebook);
    void onListAllNotebooksRequest();
    void onListAllSharedNotebooksRequest();
    void onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid);
    void onExpungeNotebookRequest(Notebook notebook);

    // Linked notebook-related slots:
    void onGetLinkedNotebookCountRequest();
    void onAddLinkedNotebookRequest(LinkedNotebook linkedNotebook);
    void onUpdateLinkedNotebookRequest(LinkedNotebook linkedNotebook);
    void onFindLinkedNotebookRequest(LinkedNotebook linkedNotebook);
    void onListAllLinkedNotebooksRequest();
    void onExpungeLinkedNotebookRequest(LinkedNotebook linkedNotebook);

    // Note-related slots:
    void onGetNoteCountRequest();
    void onAddNoteRequest(Note note, Notebook notebook);
    void onUpdateNoteRequest(Note note, Notebook notebook);
    void onFindNoteRequest(Note note, bool withResourceBinaryData);
    void onListAllNotesPerNotebookRequest(Notebook notebook, bool withResourceBinaryData);
    void onDeleteNoteRequest(Note note);
    void onExpungeNoteRequest(Note note);

    // Tag-related slots:
    void onGetTagCountRequest(QUuid requestId);
    void onAddTagRequest(Tag tag, QUuid requestId);
    void onUpdateTagRequest(Tag tag, QUuid requestId);
    void onLinkTagWithNoteRequest(Tag tag, Note note, QUuid requestId);
    void onFindTagRequest(Tag tag, QUuid requestId);
    void onListAllTagsPerNoteRequest(Note note, QUuid requestId);
    void onListAllTagsRequest();
    void onDeleteTagRequest(Tag tag);
    void onExpungeTagRequest(Tag tag);

    // Resource-related slots:
    void onGetResourceCountRequest();
    void onAddResourceRequest(ResourceWrapper resource, Note note);
    void onUpdateResourceRequest(ResourceWrapper resource, Note note);
    void onFindResourceRequest(ResourceWrapper resource, bool withBinaryData);
    void onExpungeResourceRequest(ResourceWrapper resource);

    // Saved search-related slots:
    void onGetSavedSearchCountRequest();
    void onAddSavedSearchRequest(SavedSearch search);
    void onUpdateSavedSearchRequest(SavedSearch search);
    void onFindSavedSearchRequest(SavedSearch search);
    void onListAllSavedSearchesRequest();
    void onExpungeSavedSearch(SavedSearch search);

private:
    LocalStorageManagerThreadWorker() Q_DECL_DELETE;
    LocalStorageManagerThreadWorker(const LocalStorageManagerThreadWorker & other) Q_DECL_DELETE;
    LocalStorageManagerThreadWorker(LocalStorageManagerThreadWorker && other) Q_DECL_DELETE;
    LocalStorageManagerThreadWorker & operator=(const LocalStorageManagerThreadWorker & other) Q_DECL_DELETE;
    LocalStorageManagerThreadWorker & operator=(LocalStorageManagerThreadWorker && other) Q_DECL_DELETE;

    QString                     m_username;
    qint32                      m_userId;
    bool                        m_startFromScratch;
    LocalStorageManager *       m_pLocalStorageManager;
    bool                        m_useCache;
    LocalStorageCacheManager *  m_pLocalStorageCacheManager;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_WORKER_H
