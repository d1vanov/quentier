#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_H

#include "IAsyncLocalStorageManager.h"
#include <client/types/UserWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/SavedSearch.h>
#include <QThread>
#include <QScopedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

/**
 * @brief The LocalStorageManagerThread class encapsulates the idea of a thread
 * running processing of queries into the local storage database; it is designed to
 * re-send all coming in signals to @link LocalStorageManagerThreadWorker @endlink class
 * which performs the synchronous calls to @link LocalStorageManager @endlink and signals
 * back the results. The idea behind the design was taken from article "Asynchronous
 * database access with Qt 4.x", Linux journal, Jun 01, 2007, by Dave Berton. In short,
 * the idea is the following: "Utilize queued connections to communicate between
 * the application and the database threads. The queued connection provides all the advantages
 * for dealing with asynchronous database connections, but maintains a simple and familiar interface
 * using QObject::connect()".
 */
class QUTE_NOTE_EXPORT LocalStorageManagerThread: public QThread,
                                                  public IAsyncLocalStorageManager
{
    Q_OBJECT
public:
    explicit LocalStorageManagerThread(const QString & username, const qint32 userId,
                                       const bool startFromScratch, QObject * parent = nullptr);
    virtual ~LocalStorageManagerThread();

Q_SIGNALS:
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
    void getTagCountComplete(int tagCount);
    void getTagCountFailed(QString errorDescription);
    void addTagComplete(Tag tag);
    void addTagFailed(Tag tag, QString errorDescription);
    void updateTagComplete(Tag tag);
    void updateTagFailed(Tag tag, QString errorDescription);
    void linkTagWithNoteComplete(Tag tag, Note note);
    void linkTagWithNoteFailed(Tag tag, Note note, QString errorDescription);
    void findTagComplete(Tag tag);
    void findTagFailed(Tag tag, QString errorDescription);
    void listAllTagsPerNoteComplete(QList<Tag> foundTags, Note note);
    void listAllTagsPerNoteFailed(Note note, QString errorDescription);
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

    // Signals for dealing with worker class:
    void getUserCountRequest();
    void switchUserRequest(QString username, qint32 userId, bool startFromScratch);
    void addUserRequest(UserWrapper user);
    void updateUserRequest(UserWrapper user);
    void findUserRequest(UserWrapper user);
    void deleteUserRequest(UserWrapper user);
    void expungeUserRequest(UserWrapper user);

    void getNotebookCountRequest();
    void addNotebookRequest(Notebook notebook);
    void updateNotebookRequest(Notebook notebook);
    void findNotebookRequest(Notebook notebook);
    void findDefaultNotebookRequest(Notebook notebook);
    void findLastUsedNotebookRequest(Notebook notebook);
    void findDefaultOrLastUsedNotebookRequest(Notebook notebook);
    void listAllNotebooksRequest();
    void listAllSharedNotebooksRequest();
    void listSharedNotebooksPerNotebookGuidRequest(QString notebookGuid);
    void expungeNotebookRequest(Notebook notebook);

    void getLinkedNotebookCountRequest();
    void addLinkedNotebookRequest(LinkedNotebook linkedNotebook);
    void updateLinkedNotebookRequest(LinkedNotebook linkedNotebook);
    void findLinkedNotebookRequest(LinkedNotebook linkedNotebook);
    void listAllLinkedNotebooksRequest();
    void expungeLinkedNotebookRequest(LinkedNotebook linkedNotebook);

    void getNoteCountRequest();
    void addNoteRequest(Note note, Notebook notebook);
    void updateNoteRequest(Note note, Notebook notebook);
    void findNoteRequest(Note note, bool withResourceBinaryData);
    void listAllNotesPerNotebookRequest(Notebook notebook, bool withResourceBinaryData);
    void deleteNoteRequest(Note note);
    void expungeNoteRequest(Note note);

    void getTagCountRequest();
    void addTagRequest(Tag tag);
    void updateTagRequest(Tag tag);
    void linkTagWithNoteRequest(Tag tag, Note note);
    void findTagRequest(Tag tag);
    void listAllTagsPerNoteRequest(Note note);
    void listAllTagsRequest();
    void deleteTagRequest(Tag tag);
    void expungeTagRequest(Tag tag);

    void getResourceCountRequest();
    void addResourceRequest(ResourceWrapper resource, Note note);
    void updateResourceRequest(ResourceWrapper resource, Note note);
    void findResourceRequest(ResourceWrapper resource, bool withBinaryData);
    void expungeResourceRequest(ResourceWrapper resource);

    void getSavedSearchCountRequest();
    void addSavedSearchRequest(SavedSearch search);
    void updateSavedSearchRequest(SavedSearch search);
    void findSavedSearchRequest(SavedSearch search);
    void listAllSavedSearchesRequest();
    void expungeSavedSearchRequest(SavedSearch search);

public Q_SLOTS:
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
    void onGetTagCountRequest();
    void onAddTagRequest(Tag tag);
    void onUpdateTagRequest(Tag tag);
    void onLinkTagWithNoteRequest(Tag tag, Note note);
    void onFindTagRequest(Tag tag);
    void onListAllTagsPerNoteRequest(Note note);
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
    LocalStorageManagerThread() = delete;
    LocalStorageManagerThread(const LocalStorageManagerThread & other) = delete;
    LocalStorageManagerThread(LocalStorageManagerThread && other) = delete;
    LocalStorageManagerThread & operator=(const LocalStorageManagerThread & other) = delete;
    LocalStorageManagerThread & operator=(LocalStorageManagerThread && other) = delete;

    void createConnections();

    LocalStorageManagerThreadWorker * m_pWorker;
};

}

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_H
