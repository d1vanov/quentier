#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_WORKER_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_WORKER_H

#include "IAsyncLocalStorageManager.h"
#include "LocalStorageManager.h"
#include <QObject>

namespace qute_note {

class LocalStorageManagerThreadWorker: public QObject,
                                       public IAsyncLocalStorageManager
{
    Q_OBJECT
public:
    explicit LocalStorageManagerThreadWorker(const QString & username,
                                             const qint32 userId,
                                             const bool startFromScratch,
                                             QObject * parent = nullptr);
    virtual ~LocalStorageManagerThreadWorker();
    
Q_SIGNALS:
    // User-related signals:
    void getUserCountComplete(int userCount);
    void getUserCountFailed(QString errorDescription);
    void switchUserComplete(qint32 userId);
    void switchUserFailed(qint32 userId, QString errorDescription);
    void addUserComplete(QSharedPointer<UserWrapper> user);
    void addUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);
    void updateUserComplete(QSharedPointer<UserWrapper> user);
    void updateUserFailed(QSharedPointer<UserWrapper> user, QString errorDecription);
    void findUserComplete(QSharedPointer<UserWrapper> foundUser);
    void findUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);
    void deleteUserComplete(QSharedPointer<UserWrapper> user);
    void deleteUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);
    void expungeUserComplete(QSharedPointer<UserWrapper> user);
    void expungeUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);

    // Notebook-related signals:
    void getNotebookCountComplete(int notebookCount);
    void getNotebookCountFailed(QString errorDescription);
    void addNotebookComplete(QSharedPointer<Notebook> notebook);
    void addNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void updateNotebookComplete(QSharedPointer<Notebook> notebook);
    void updateNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void findNotebookComplete(QSharedPointer<Notebook> foundNotebook);
    void findNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void findDefaultNotebookComplete(QSharedPointer<Notebook> foundNotebook);
    void findDefaultNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void findLastUsedNotebookComplete(QSharedPointer<Notebook> foundNotebook);
    void findLastUsedNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void findDefaultOrLastUsedNotebookComplete(QSharedPointer<Notebook> foundNotebook);
    void findDefaultOrLastUsedNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void listAllNotebooksComplete(QList<Notebook> foundNotebooks);
    void listAllNotebooksFailed(QString errorDescription);
    void listAllSharedNotebooksComplete(QList<SharedNotebookWrapper> foundSharedNotebooks);
    void listAllSharedNotebooksFailed(QString errorDescription);
    void listSharedNotebooksPerNotebookGuidComplete(QString notebookGuid, QList<SharedNotebookWrapper> foundSharedNotebooks);
    void listSharedNotebooksPerNotebookGuidFailed(QString notebookGuid, QString errorDescription);
    void expungeNotebookComplete(QSharedPointer<Notebook> notebook);
    void expungeNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);

    // Linked notebook-related signals:
    void getLinkedNotebookCountComplete(int linkedNotebookCount);
    void getLinkedNotebookCountFailed(QString errorDescription);
    void addLinkedNotebookComplete(QSharedPointer<LinkedNotebook> linkedNotebook);
    void addLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription);
    void updateLinkedNotebookComplete(QSharedPointer<LinkedNotebook> linkedNotebook);
    void updateLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription);
    void findLinkedNotebookComplete(QSharedPointer<LinkedNotebook> foundLinkedNotebook);
    void findLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription);
    void listAllLinkedNotebooksComplete(QList<LinkedNotebook> foundLinkedNotebooks);
    void listAllLinkedNotebooksFailed(QString errorDescription);
    void expungeLinkedNotebookComplete(QSharedPointer<LinkedNotebook> linkedNotebook);
    void expungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription);

    // Note-related signals:
    void getNoteCountComplete(int noteCount);
    void getNoteCountFailed(QString errorDescription);
    void addNoteComplete(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void addNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription);
    void updateNoteComplete(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void updateNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription);
    void findNoteComplete(QSharedPointer<Note> foundNote);
    void findNoteFailed(QSharedPointer<Note> note, QString errorDescription);
    void listAllNotesPerNotebookComplete(QSharedPointer<Notebook> notebook, QList<Note> foundNotes);
    void listAllNotesPerNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void deleteNoteComplete(QSharedPointer<Note> note);
    void deleteNoteFailed(QSharedPointer<Note> note, QString errorDescription);
    void expungeNoteComplete(QSharedPointer<Note> note);
    void expungeNoteFailed(QSharedPointer<Note> note, QString errorDescription);

    // Tag-related signals:
    void getTagCountComplete(int tagCount);
    void getTagCountFailed(QString errorDescription);
    void addTagComplete(QSharedPointer<Tag> tag);
    void addTagFailed(QSharedPointer<Tag> tag, QString errorDescription);
    void updateTagComplete(QSharedPointer<Tag> tag);
    void updateTagFailed(QSharedPointer<Tag> tag, QString errorDescription);
    void linkTagWithNoteComplete(QSharedPointer<Tag> tag, QSharedPointer<Note> note);
    void linkTagWithNoteFailed(QSharedPointer<Tag> tag, QSharedPointer<Note> note, QString errorDescription);
    void findTagComplete(QSharedPointer<Tag> tag);
    void findTagFailed(QSharedPointer<Tag> tag, QString errorDescription);
    void listAllTagsPerNoteComplete(QList<Tag> foundTags, QSharedPointer<Note> note);
    void listAllTagsPerNoteFailed(QSharedPointer<Note> note, QString errorDescription);
    void listAllTagsComplete(QList<Tag> foundTags);
    void listAllTagsFailed(QString errorDescription);
    void deleteTagComplete(QSharedPointer<Tag> tag);
    void deleteTagFailed(QSharedPointer<Tag> tag, QString errorDescription);
    void expungeTagComplete(QSharedPointer<Tag> tag);
    void expungeTagFailed(QSharedPointer<Tag> tag, QString errorDescription);

    // Resource-related signals:
    void getResourceCountComplete(int resourceCount);
    void getResourceCountFailed(QString errorDescription);
    void addResourceComplete(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void addResourceFailed(QSharedPointer<IResource> resource, QSharedPointer<Note> note, QString errorDescription);
    void updateResourceComplete(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void updateResourceFailed(QSharedPointer<IResource> resource, QSharedPointer<Note> note, QString errorDescription);
    void findResourceComplete(QSharedPointer<IResource> resource, bool withBinaryData);
    void findResourceFailed(QSharedPointer<IResource> resource, bool withBinaryData, QString errorDescription);
    void expungeResourceComplete(QSharedPointer<IResource> resource);
    void expungeResourceFailed(QSharedPointer<IResource> resource, QString errorDescription);

    // Saved search-related signals:
    void getSavedSearchCountComplete(int savedSearchCount);
    void getSavedSearchCountFailed(QString errorDescription);
    void addSavedSearchComplete(QSharedPointer<SavedSearch> search);
    void addSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription);
    void updateSavedSearchComplete(QSharedPointer<SavedSearch> search);
    void updateSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription);
    void findSavedSearchComplete(QSharedPointer<SavedSearch> search);
    void findSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription);
    void listAllSavedSearchesComplete(QList<SavedSearch> foundSearches);
    void listAllSavedSearchesFailed(QString errorDescription);
    void expungeSavedSearchComplete(QSharedPointer<SavedSearch> search);
    void expungeSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription);
    
public Q_SLOTS:
    // User-related slots:
    void onGetUserCountRequest();
    void onSwitchUserRequest(QString username, qint32 userId, bool startFromScratch);
    void onAddUserRequest(QSharedPointer<UserWrapper> user);
    void onUpdateUserRequest(QSharedPointer<UserWrapper> user);
    void onFindUserRequest(QSharedPointer<UserWrapper> user);
    void onDeleteUserRequest(QSharedPointer<UserWrapper> user);
    void onExpungeUserRequest(QSharedPointer<UserWrapper> user);

    // Notebook-related slots:
    void onGetNotebookCountRequest();
    void onAddNotebookRequest(QSharedPointer<Notebook> notebook);
    void onUpdateNotebookRequest(QSharedPointer<Notebook> notebook);
    void onFindNotebookRequest(QSharedPointer<Notebook> notebook);
    void onFindDefaultNotebookRequest(QSharedPointer<Notebook> notebook);
    void onFindLastUsedNotebookRequest(QSharedPointer<Notebook> notebook);
    void onFindDefaultOrLastUsedNotebookRequest(QSharedPointer<Notebook> notebook);
    void onListAllNotebooksRequest();
    void onListAllSharedNotebooksRequest();
    void onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid);
    void onExpungeNotebookRequest(QSharedPointer<Notebook> notebook);

    // Linked notebook-related slots:
    void onGetLinkedNotebookCountRequest();
    void onAddLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void onUpdateLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void onFindLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void onListAllLinkedNotebooksRequest();
    void onExpungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);

    // Note-related slots:
    void onGetNoteCountRequest();
    void onAddNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void onUpdateNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void onFindNoteRequest(QSharedPointer<Note> note, bool withResourceBinaryData);
    void onListAllNotesPerNotebookRequest(QSharedPointer<Notebook> notebook, bool withResourceBinaryData);
    void onDeleteNoteRequest(QSharedPointer<Note> note);
    void onExpungeNoteRequest(QSharedPointer<Note> note);

    // Tag-related slots:
    void onGetTagCountRequest();
    void onAddTagRequest(QSharedPointer<Tag> tag);
    void onUpdateTagRequest(QSharedPointer<Tag> tag);
    void onLinkTagWithNoteRequest(QSharedPointer<Tag> tag, QSharedPointer<Note> note);
    void onFindTagRequest(QSharedPointer<Tag> tag);
    void onListAllTagsPerNoteRequest(QSharedPointer<Note> note);
    void onListAllTagsRequest();
    void onDeleteTagRequest(QSharedPointer<Tag> tag);
    void onExpungeTagRequest(QSharedPointer<Tag> tag);

    // Resource-related slots:
    void onGetResourceCountRequest();
    void onAddResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void onUpdateResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void onFindResourceRequest(QSharedPointer<IResource> resource, bool withBinaryData);
    void onExpungeResourceRequest(QSharedPointer<IResource> resource);

    // Saved search-related slots:
    void onGetSavedSearchCountRequest();
    void onAddSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void onUpdateSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void onFindSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void onListAllSavedSearchesRequest();
    void onExpungeSavedSearch(QSharedPointer<SavedSearch> search);

private:
    LocalStorageManagerThreadWorker() = delete;
    LocalStorageManagerThreadWorker(const LocalStorageManagerThreadWorker & other) = delete;
    LocalStorageManagerThreadWorker(LocalStorageManagerThreadWorker && other) = delete;
    LocalStorageManagerThreadWorker & operator=(const LocalStorageManagerThreadWorker & other) = delete;
    LocalStorageManagerThreadWorker & operator=(LocalStorageManagerThreadWorker && other) = delete;

    LocalStorageManager   m_localStorageManager;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_WORKER_H
