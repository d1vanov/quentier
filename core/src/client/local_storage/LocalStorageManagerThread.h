#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_THREAD_H

#include "IAsyncLocalStorageManager.h"
#include <client/types/IUser.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/IResource.h>
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
class LocalStorageManagerThread: public QThread,
                                 public IAsyncLocalStorageManager
{
    Q_OBJECT
public:
    explicit LocalStorageManagerThread(const QString & username, const qint32 userId,
                                       const bool startFromScratch, QObject * parent = nullptr);
    virtual ~LocalStorageManagerThread();

Q_SIGNALS:
    // User-related signals:
    void switchUserComplete(qint32 userId);
    void switchUserFailed(qint32 userId, QString errorDescription);
    void addUserComplete(QSharedPointer<IUser> user);
    void addUserFailed(QSharedPointer<IUser> user, QString errorDescription);
    void updateUserComplete(QSharedPointer<IUser> user);
    void updateUserFailed(QSharedPointer<IUser> user, QString errorDecription);
    void findUserComplete(QSharedPointer<IUser> foundUser);
    void findUserFailed(QSharedPointer<IUser> user, QString errorDescription);
    void deleteUserComplete(QSharedPointer<IUser> user);
    void deleteUserFailed(QSharedPointer<IUser> user, QString errorDescription);
    void expungeUserComplete(QSharedPointer<IUser> user);
    void expungeUserFailed(QSharedPointer<IUser> user, QString errorDescription);

    // Notebook-related signals:
    void addNotebookComplete(QSharedPointer<Notebook> notebook);
    void addNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void updateNotebookComplete(QSharedPointer<Notebook> notebook);
    void updateNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void findNotebookComplete(QSharedPointer<Notebook> foundNotebook);
    void findNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void listAllNotebooksComplete(QList<Notebook> foundNotebooks);
    void listAllNotebooksFailed(QString errorDescription);
    void listAllSharedNotebooksComplete(QList<SharedNotebookWrapper> foundSharedNotebooks);
    void listAllSharedNotebooksFailed(QString errorDescription);
    void listSharedNotebooksPerNotebookGuidComplete(QString notebookGuid, QList<SharedNotebookWrapper> foundSharedNotebooks);
    void listSharedNotebooksPerNotebookGuidFailed(QString notebookGuid, QString errorDescription);
    void expungeNotebookComplete(QSharedPointer<Notebook> notebook);
    void expungeNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);

    // Linked notebook-related signals:
    void addLinkedNotebookComplete(QSharedPointer<LinkedNotebook> linkedNotebook);
    void addLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription);
    void updateLinkedNotebookComplete(QSharedPointer<LinkedNotebook> linkedNotebook);
    void updateLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription);
    void findLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> foundLinkedNotebook);
    void findLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription);
    void listAllLinkedNotebooksComplete(QList<LinkedNotebook> foundLinkedNotebooks);
    void listAllLinkedNotebooksFailed(QString errorDescription);
    void expungeLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> linkedNotebook);
    void expungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription);

    // Note-related signals:
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
    void addResourceComplete(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void addResourceFailed(QSharedPointer<IResource> resource, QSharedPointer<Note> note, QString errorDescription);
    void updateResourceComplete(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void updateResourceFailed(QSharedPointer<IResource> resource, QSharedPointer<Note> note, QString errorDescription);
    void findResourceComplete(QSharedPointer<IResource> resource, bool withBinaryData);
    void findResourceFailed(QSharedPointer<IResource> resource, bool withBinaryData, QString errorDescription);
    void expungeResourceComplete(QSharedPointer<IResource> resource);
    void expungeResourceFailed(QSharedPointer<IResource> resource, QString errorDescription);

    // Saved search-related signals:
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

    // Signals for dealing with worker class:
    void switchUserRequest(QString username, qint32 userId, bool startFromScratch);
    void addUserRequest(QSharedPointer<IUser> user);
    void updateUserRequest(QSharedPointer<IUser> user);
    void findUserRequest(QSharedPointer<IUser> user);
    void deleteUserRequest(QSharedPointer<IUser> user);
    void expungeUserRequest(QSharedPointer<IUser> user);

    void addNotebookRequest(QSharedPointer<Notebook> notebook);
    void updateNotebookRequest(QSharedPointer<Notebook> notebook);
    void findNotebookRequest(QSharedPointer<Notebook> notebook);
    void listAllNotebooksRequest();
    void listAllSharedNotebooksRequest();
    void listSharedNotebooksPerNotebookGuidRequest(QString notebookGuid);
    void expungeNotebookRequest(QSharedPointer<Notebook> notebook);

    void addLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void updateLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void findLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void listAllLinkedNotebooksRequest();
    void expungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);

    void addNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void updateNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void findNoteRequest(QSharedPointer<Note> note, bool withResourceBinaryData);
    void listAllNotesPerNotebookRequest(QSharedPointer<Notebook> notebook, bool withResourceBinaryData);
    void deleteNoteRequest(QSharedPointer<Note> note);
    void expungeNoteRequest(QSharedPointer<Note> note);

    void addTagRequest(QSharedPointer<Tag> tag);
    void updateTagRequest(QSharedPointer<Tag> tag);
    void linkTagWithNoteRequest(QSharedPointer<Tag> tag, QSharedPointer<Note> note);
    void findTagRequest(QSharedPointer<Tag> tag);
    void listAllTagsPerNoteRequest(QSharedPointer<Note> note);
    void listAllTagsRequest();
    void deleteTagRequest(QSharedPointer<Tag> tag);
    void expungeTagRequest(QSharedPointer<Tag> tag);

    void addResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void updateResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void findResourceRequest(QSharedPointer<IResource> resource, bool withBinaryData);
    void expungeResourceRequest(QSharedPointer<IResource> resource);

    void addSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void updateSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void findSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void listAllSavedSearchesRequest();
    void expungeSavedSearchRequest(QSharedPointer<SavedSearch> search);

public Q_SLOTS:
    // User-related slots:
    void onSwitchUserRequest(QString username, qint32 userId, bool startFromScratch);
    void onAddUserRequest(QSharedPointer<IUser> user);
    void onUpdateUserRequest(QSharedPointer<IUser> user);
    void onFindUserRequest(QSharedPointer<IUser> user);
    void onDeleteUserRequest(QSharedPointer<IUser> user);
    void onExpungeUserRequest(QSharedPointer<IUser> user);

    // Notebook-related slots:
    void onAddNotebookRequest(QSharedPointer<Notebook> notebook);
    void onUpdateNotebookRequest(QSharedPointer<Notebook> notebook);
    void onFindNotebookRequest(QSharedPointer<Notebook> notebook);
    void onListAllNotebooksRequest();
    void onListAllSharedNotebooksRequest();
    void onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid);
    void onExpungeNotebookRequest(QSharedPointer<Notebook> notebook);

    // Linked notebook-related slots:
    void onAddLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void onUpdateLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void onFindLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);
    void onListAllLinkedNotebooksRequest();
    void onExpungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook);

    // Note-related slots:
    void onAddNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void onUpdateNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void onFindNoteRequest(QSharedPointer<Note> note, bool withResourceBinaryData);
    void onListAllNotesPerNotebookRequest(QSharedPointer<Notebook> notebook, bool withResourceBinaryData);
    void onDeleteNoteRequest(QSharedPointer<Note> note);
    void onExpungeNoteRequest(QSharedPointer<Note> note);

    // Tag-related slots:
    void onAddTagRequest(QSharedPointer<Tag> tag);
    void onUpdateTagRequest(QSharedPointer<Tag> tag);
    void onLinkTagWithNoteRequest(QSharedPointer<Tag> tag, QSharedPointer<Note> note);
    void onFindTagRequest(QSharedPointer<Tag> tag);
    void onListAllTagsPerNoteRequest(QSharedPointer<Note> note);
    void onListAllTagsRequest();
    void onDeleteTagRequest(QSharedPointer<Tag> tag);
    void onExpungeTagRequest(QSharedPointer<Tag> tag);

    // Resource-related slots:
    void onAddResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void onUpdateResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note);
    void onFindResourceRequest(QSharedPointer<IResource> resource, bool withBinaryData);
    void onExpungeResourceRequest(QSharedPointer<IResource> resource);

    // Saved search-related slots:
    void onAddSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void onUpdateSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void onFindSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void onListAllSavedSearchesRequest();
    void onExpungeSavedSearch(QSharedPointer<SavedSearch> search);

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
