#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H

#include <tools/Linkage.h>
#include <QString>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IUser)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebookWrapper)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(SavedSearch)

/**
 * @brief The IAsyncLocalStorageManager class defines the interfaces for signals and slots
 * used for asynchronous access to local storage database. Typically the interface is
 * slot like "onRequestToDoSmth" + a couple of "complete"/"failed" resulting signals
 * for each public method in LocalStorageManager.
 */
class IAsyncLocalStorageManager
{
public:
    virtual ~IAsyncLocalStorageManager();

protected:
    IAsyncLocalStorageManager() = default;
    IAsyncLocalStorageManager(const IAsyncLocalStorageManager & other) = default;
    IAsyncLocalStorageManager(IAsyncLocalStorageManager && other) = default;
    IAsyncLocalStorageManager & operator=(const IAsyncLocalStorageManager & other) = default;
    IAsyncLocalStorageManager & operator=(IAsyncLocalStorageManager && other) = default;

    // Pure virtual prototypes for signals to be emitted from subclasses:

    // Prototypes for user-related signals:
    virtual void switchUserComplete(qint32 userId) = 0;
    virtual void switchUserFailed(qint32 userId, QString errorDescription) = 0;

    virtual void addUserComplete(QSharedPointer<IUser> user) = 0;
    virtual void addUserFailed(QSharedPointer<IUser> user, QString errorDescription) = 0;

    virtual void updateUserComplete(QSharedPointer<IUser> user) = 0;
    virtual void updateUserFailed(QSharedPointer<IUser> user, QString errorDecription) = 0;

    virtual void findUserComplete(QSharedPointer<IUser> foundUser) = 0;
    virtual void findUserFailed(QSharedPointer<IUser> user, QString errorDescription) = 0;

    virtual void deleteUserComplete(QSharedPointer<IUser> user) = 0;
    virtual void deleteUserFailed(QSharedPointer<IUser> user, QString errorDescription) = 0;

    virtual void expungeUserComplete(QSharedPointer<IUser> user) = 0;
    virtual void expungeUserFailed(QSharedPointer<IUser> user, QString errorDescription) = 0;

    // Prototypes for notebook-related signals:
    virtual void addNotebookComplete(QSharedPointer<Notebook> notebook) = 0;
    virtual void addNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription) = 0;

    virtual void updateNotebookComplete(QSharedPointer<Notebook> notebook) = 0;
    virtual void updateNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription) = 0;

    virtual void findNotebookComplete(QSharedPointer<Notebook> foundNotebook) = 0;
    virtual void findNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription) = 0;

    virtual void listAllNotebooksComplete(QList<Notebook> foundNotebooks) = 0;
    virtual void listAllNotebooksFailed(QString errorDescription) = 0;

    virtual void listAllSharedNotebooksComplete(QList<SharedNotebookWrapper> foundSharedNotebooks) = 0;
    virtual void listAllSharedNotebooksFailed(QString errorDescription) = 0;

    virtual void listSharedNotebooksPerNotebookGuidComplete(QString notebookGuid, QList<SharedNotebookWrapper> foundSharedNotebooks) = 0;
    virtual void listSharedNotebooksPerNotebookGuidFailed(QString notebookGuid, QString errorDescription) = 0;

    virtual void expungeNotebookComplete(QSharedPointer<Notebook> notebook) = 0;
    virtual void expungeNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription) = 0;

    // Prototypes for linked notebook-related signals:
    virtual void addLinkedNotebookComplete(QSharedPointer<LinkedNotebook> linkedNotebook) = 0;
    virtual void addLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription) = 0;

    virtual void updateLinkedNotebookComplete(QSharedPointer<LinkedNotebook> linkedNotebook) = 0;
    virtual void updateLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription) = 0;

    virtual void findLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> foundLinkedNotebook) = 0;
    virtual void findLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription) = 0;

    virtual void listAllLinkedNotebooksComplete(QList<LinkedNotebook> foundLinkedNotebooks) = 0;
    virtual void listAllLinkedNotebooksFailed(QString errorDescription) = 0;

    virtual void expungeLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> linkedNotebook) = 0;
    virtual void expungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook> linkedNotebook, QString errorDescription) = 0;

    // Prototypes for note-related signals:
    virtual void addNoteComplete(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook) = 0;
    virtual void addNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription) = 0;

    virtual void updateNoteComplete(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook) = 0;
    virtual void updateNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription) = 0;

    virtual void findNoteComplete(QSharedPointer<Note> foundNote) = 0;
    virtual void findNoteFailed(QSharedPointer<Note> note, QString errorDescription) = 0;

    virtual void listAllNotesPerNotebookComplete(QSharedPointer<Notebook> notebook, QList<Note> foundNotes) = 0;
    virtual void listAllNotesPerNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription) = 0;

    virtual void deleteNoteComplete(QSharedPointer<Note> note) = 0;
    virtual void deleteNoteFailed(QSharedPointer<Note> note, QString errorDescription) = 0;

    virtual void expungeNoteComplete(QSharedPointer<Note> note) = 0;
    virtual void expungeNoteFailed(QSharedPointer<Note> note, QString errorDescription) = 0;

    // Prototypes for tag-related signals:
    virtual void addTagComplete(QSharedPointer<Tag> tag) = 0;
    virtual void addTagFailed(QSharedPointer<Tag> tag, QString errorDescription) = 0;

    virtual void updateTagComplete(QSharedPointer<Tag> tag) = 0;
    virtual void updateTagFailed(QSharedPointer<Tag> tag, QString errorDescription) = 0;

    virtual void linkTagWithNoteComplete(QSharedPointer<Tag> tag, QSharedPointer<Note> note) = 0;
    virtual void linkTagWithNoteFailed(QSharedPointer<Tag> tag, QSharedPointer<Note> note, QString errorDescription) = 0;

    virtual void findTagComplete(QSharedPointer<Tag> tag) = 0;
    virtual void findTagFailed(QSharedPointer<Tag> tag, QString errorDescription) = 0;

    virtual void listAllTagsPerNoteComplete(QList<Tag> foundTags, QSharedPointer<Note> note) = 0;
    virtual void listAllTagsPerNoteFailed(QSharedPointer<Note> note, QString errorDescription) = 0;

    virtual void listAllTagsComplete(QList<Tag> foundTags) = 0;
    virtual void listAllTagsFailed(QString errorDescription) = 0;

    virtual void deleteTagComplete(QSharedPointer<Tag> tag) = 0;
    virtual void deleteTagFailed(QSharedPointer<Tag> tag, QString errorDescription) = 0;

    virtual void expungeTagComplete(QSharedPointer<Tag> tag) = 0;
    virtual void expungeTagFailed(QSharedPointer<Tag> tag, QString errorDescription) = 0;

    // Prototypes for resource-related signals
    virtual void addResourceComplete(QSharedPointer<IResource> resource, QSharedPointer<Note> note) = 0;
    virtual void addResourceFailed(QSharedPointer<IResource> resource, QSharedPointer<Note> note,
                                   QString errorDescription) = 0;

    virtual void updateResourceComplete(QSharedPointer<IResource> resource, QSharedPointer<Note> note) = 0;
    virtual void updateResourceFailed(QSharedPointer<IResource> resource, QSharedPointer<Note> note,
                                      QString errorDescription) = 0;

    virtual void findResourceComplete(QSharedPointer<IResource> resource, bool withBinaryData) = 0;
    virtual void findResourceFailed(QSharedPointer<IResource> resource, bool withBinaryData,
                                    QString errorDescription) = 0;

    virtual void expungeResourceComplete(QSharedPointer<IResource> resource) = 0;
    virtual void expungeResourceFailed(QSharedPointer<IResource> resource,
                                       QString errorDescription) = 0;

    // Prototypes for saved search-related signals:
    virtual void addSavedSearchComplete(QSharedPointer<SavedSearch> search) = 0;
    virtual void addSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription) = 0;

    virtual void updateSavedSearchComplete(QSharedPointer<SavedSearch> search) = 0;
    virtual void updateSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription) = 0;

    virtual void findSavedSearchComplete(QSharedPointer<SavedSearch> search) = 0;
    virtual void findSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription) = 0;

    virtual void listAllSavedSearchesComplete(QList<SavedSearch> foundSearches) = 0;
    virtual void listAllSavedSearchesFailed(QString errorDescription) = 0;

    virtual void expungeSavedSearchComplete(QSharedPointer<SavedSearch> search) = 0;
    virtual void expungeSavedSearchFailed(QSharedPointer<SavedSearch> search,
                                          QString errorDescription) = 0;

    // Pure virtual prototypes for slots to be invoked:

    // Pure virtual prototypes for user-related slots:
    virtual void onSwitchUserRequest(QString username, qint32 userId, bool startFromScratch) = 0;
    virtual void onAddUserRequest(QSharedPointer<IUser> user) = 0;
    virtual void onUpdateUserRequest(QSharedPointer<IUser> user) = 0;
    virtual void onFindUserRequest(QSharedPointer<IUser> user) = 0;
    virtual void onDeleteUserRequest(QSharedPointer<IUser> user) = 0;
    virtual void onExpungeUserRequest(QSharedPointer<IUser> user) = 0;

    // Pure virtual prototypes for notebook-related slots:
    virtual void onAddNotebookRequest(QSharedPointer<Notebook> notebook) = 0;
    virtual void onUpdateNotebookRequest(QSharedPointer<Notebook> notebook) = 0;
    virtual void onFindNotebookRequest(QSharedPointer<Notebook> notebook) = 0;
    virtual void onListAllNotebooksRequest() = 0;
    virtual void onListAllSharedNotebooksRequest() = 0;
    virtual void onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid) = 0;
    virtual void onExpungeNotebookRequest(QSharedPointer<Notebook> notebook) = 0;

    // Pure virtual prototypes for linked notebook-related slots:
    virtual void onAddLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook) = 0;
    virtual void onUpdateLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook) = 0;
    virtual void onFindLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook) = 0;
    virtual void onListAllLinkedNotebooksRequest() = 0;
    virtual void onExpungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook) = 0;

    // Pure virtual prototypes for note-related slots:
    virtual void onAddNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook) = 0;
    virtual void onUpdateNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook) = 0;
    virtual void onFindNoteRequest(QSharedPointer<Note> note, bool withResourceBinaryData) = 0;
    virtual void onListAllNotesPerNotebookRequest(QSharedPointer<Notebook> notebook,
                                                  bool withResourceBinaryData) = 0;
    virtual void onDeleteNoteRequest(QSharedPointer<Note> note) = 0;
    virtual void onExpungeNoteRequest(QSharedPointer<Note> note) = 0;

    // Pure virtual prototypes for tag-related slots:
    virtual void onAddTagRequest(QSharedPointer<Tag> tag) = 0;
    virtual void onUpdateTagRequest(QSharedPointer<Tag> tag) = 0;
    virtual void onLinkTagWithNoteRequest(QSharedPointer<Tag> tag, QSharedPointer<Note> note) = 0;
    virtual void onFindTagRequest(QSharedPointer<Tag> tag) = 0;
    virtual void onListAllTagsPerNoteRequest(QSharedPointer<Note> note) = 0;
    virtual void onListAllTagsRequest() = 0;
    virtual void onDeleteTagRequest(QSharedPointer<Tag> tag) = 0;
    virtual void onExpungeTagRequest(QSharedPointer<Tag> tag) = 0;

    // Pure virtual prototypes for resource-related slots:
    virtual void onAddResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note) = 0;
    virtual void onUpdateResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note) = 0;
    virtual void onFindResourceRequest(QSharedPointer<IResource> resource, bool withBinaryData) = 0;
    virtual void onExpungeResourceRequest(QSharedPointer<IResource> resource) = 0;

    /*
    // Pure virtual prototypes for saved search-related methods:
    virtual void onAddSavedSearchRequest(QSharedPointer<SavedSearch> search) = 0;
    virtual void onUpdateSavedSearchRequest(QSharedPointer<SavedSearch> search) = 0;
    virtual void onFindSavedSearch(QSharedPointer<SavedSearch> search) = 0;
    virtual void onListAllSavedSearchesRequest() = 0;
    virtual void onExpungeSavedSearch(QSharedPointer<SavedSearch> search) = 0;
    */
};

}

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H
