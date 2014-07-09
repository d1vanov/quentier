#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_H

#include <QString>
#include <QSharedPointer>

namespace qevercloud {
QT_FORWARD_DECLARE_STRUCT(ResourceAttributes)
QT_FORWARD_DECLARE_STRUCT(NoteAttributes)
QT_FORWARD_DECLARE_STRUCT(UserAttributes)
QT_FORWARD_DECLARE_STRUCT(Accounting)
QT_FORWARD_DECLARE_STRUCT(PremiumInfo)
QT_FORWARD_DECLARE_STRUCT(BusinessUserInfo)
QT_FORWARD_DECLARE_STRUCT(SharedNotebook)
QT_FORWARD_DECLARE_STRUCT(NotebookRestrictions)
typedef int32_t UserID;
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ISharedNotebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebookWrapper)
QT_FORWARD_DECLARE_CLASS(SharedNotebookAdapter)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(SavedSearch)
QT_FORWARD_DECLARE_CLASS(IUser)
typedef qevercloud::UserID UserID;

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerPrivate)

class LocalStorageManager
{
public:
    /**
     * @brief LocalStorageManager - constructor. Accepts name and id of user
     * for which the LocalStorageManager instance is created and boolean parameter
     * defining whether any pre-existing database file for this user needs to be purged
     * @param username
     * @param userId
     * @param startFromScratch
     */
    LocalStorageManager(const QString & username, const UserID userId, const bool startFromScratch);
    virtual ~LocalStorageManager();

    /**
     * @brief SwitchUser - switches to another local database file associated with passed in
     * username and user id. If optional "startFromScratch" parameter is set to true (it is false
     * by default), the database file would be erased and only then - opened
     * @param username - name of user to which the local storage is switched
     * @param userId - id of user to which the local storage is switched
     * @param startFromScratch - optional, false by default; if true and database file
     * for this user existed previously, it is erased before open
     */
    void SwitchUser(const QString & username, const UserID userId, const bool startFromScratch = false);

    /**
     * @brief GetUserCount - returns the number of non-deleted users currently stored in local storage database
     * @param errorDescription - error description if the number of users could not be returned
     * @return either non-negative value with the number of users or -1 which means some error occured
     */
    int GetUserCount(QString & errorDescription) const;

    /**
     * @brief AddUser - adds passed in by const reference IUser subclass object
     * to the local storage database; basically the table with Users is only involved
     * in operations with Notebooks which have "contact" field set which in turn is
     * intended to use with business accounts.
     * @param user - user to be added to local storage database
     * @param errorDescription - error description if user could not be added
     * @return true if user was added successfully, false otherwise
     */
    bool AddUser(const IUser & user, QString & errorDescription);

    /**
     * @brief UpdateUser - updates passed in by const reference IUser subclass object
     * in the local storage database; basically the table with Users is only involved
     * in operations with Notebooks which have "contact" field set which in turn is
     * intended to use with business accounts.
     * @param user - user to be updated  in the local storage database
     * @param errorDescription - error description if user could not be added
     * @return true if user was updated successfully, false otherwise
     */
    bool UpdateUser(const IUser & user, QString & errorDescription);

    /**
     * @brief FindUser - attempts to find and fill the fields of passed in user object which must have
     * "id" field set as this value is the identifier of user objects in the local storage database.
     * @param user - user to be found. Must have "id" field set.
     * @param errorDescription - error description if user could not be found
     * @return true if user was found successfully, false otherwise
     */
    bool FindUser(IUser & user, QString & errorDescription) const;

    /**
     * @brief DeleteUser - either expunges the local user (i.e. deletes it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks
     * user as deleted otherwise. Evernote API doesn't allow thirdparty
     * applications to expunge users which have ever been synchronized
     * with remote data store at least once
     * @param user - user to be deleted
     * @param errorDescription - error description if user could not be deleted
     * @return true if user was deleted successfully, false otherwise
     */
    bool DeleteUser(const IUser & user, QString & errorDescription);

    /**
     * @brief ExpungeUser - permanently deletes local user i.e. user which
     * has not yet been synchronized with Evernote service. This method deletes
     * user from local storage completely, without the possibility to restore
     * @param user - user to be expunged
     * @param errorDescription - error description if user could not be expunged
     * @return true if user was expunged successfully, false otherwise
     */
    bool ExpungeUser(const IUser & user, QString & errorDescription);

    /**
     * @brief GetNotebookCount returns the number of notebooks currently stored in local storage database
     * @param errorDescription - error description if the number of notebooks could not be returned
     * @return either non-negative value with the number of notebooks or -1 which means some error occured
     */
    int GetNotebookCount(QString & errorDescription) const;

    /**
     * @brief AddNotebook - adds passed in Notebook to the local storage database;
     * if Notebook has "remote" Evernote service's guid set, it is identified by this guid
     * in local storage database. Otherwise it is identified by its local guid.
     * @param notebook - notebook to be added to the local storage database
     * @param errorDescription - error description if notebook could not be added
     * @return true if notebook was added successfully, false otherwise
     */
    bool AddNotebook(const Notebook & notebook, QString & errorDescription);

    /**
     * @brief UpdateNotebook - updates passed in Notebook in the local storage database;
     * if Notebook has "remote" Evernote service's guid set, it is identified by this guid
     * in local storage database. Otherwise it is identified by its local guid.
     * @param notebook - notebook to be updated in the local storage database
     * @param errorDescription - error description if notebook could not be updated
     * @return true if notebook was updated successfully, false otherwise
     */
    bool UpdateNotebook(const Notebook & notebook, QString & errorDescription);

    /**
     * @brief FindNotebook - attempts to find and set all found fields for passed in
     * by reference Notebook object. If "remote" Evernote service's guid for Notebook is set,
     * it is used to identify the Notebook in local storage database. Otherwise it is
     * identified by its local guid.
     * @param notebook - notebook to be found. Must have either "remote" or local guid set
     * @param errorDescription - error description if notebook could not be found
     * @return true if notebook was found, false otherwise
     */
    bool FindNotebook(Notebook & notebook, QString & errorDescription) const;

    /**
     * @brief FindDefaultNotebook - attempts to find default notebook in the local storage database.
     * @param notebook - default notebook to be found
     * @param errorDescription - error description if default notebook could not be found
     * @return true if default notebook was found, false otherwise
     */
    bool FindDefaultNotebook(Notebook & notebook, QString & errorDescription) const;

    /**
     * @brief FindLastUsedNotebook - attempts to find last used notebook in the local storage database.
     * @param notebook - last used notebook to be found
     * @param errorDescription - error description if last used notebook could not be found
     * @return true if last used notebook was found, false otherwise
     */
    bool FindLastUsedNotebook(Notebook & notebook, QString & errorDescription) const;

    /**
     * @brief FindDefaultOrLastUsedNotebook - attempts to find either default or last used notebook
     * in the local storage database
     * @param notebook - default or last used notebook to be found
     * @param errorDescription - error description if default or last used notebook could not be found
     * @return true if default or last used notebook was found, false otherwise
     */
    bool FindDefaultOrLastUsedNotebook(Notebook & notebook, QString & errorDescription) const;

    /**
     * @brief ListAllNotebooks - attempts to list all notebooks within the account
     * @param errorDescription - error description if notebooks could not be listed;
     * if no error happens, this parameter is untouched
     * @return either list of all notebooks within the account or empty list in cases of
     * error or no notebooks presence within the account
     */
    QList<Notebook> ListAllNotebooks(QString & errorDescription) const;

    /**
     * @brief ListAllSharedNotebooks - attempts to list all shared notebooks within the account
     * @param errorDescription - error description if shared notebooks could not be listed;
     * if no error happens, this parameter is untouched
     * @return either list of all shared notebooks within the account or empty list in cases of
     * error or no shared notebooks presence within the account
     */
    QList<SharedNotebookWrapper> ListAllSharedNotebooks(QString & errorDescription) const;

    /**
     * @brief ListSharedNotebooksPerNotebookGuid - attempts to list all shared notebooks
     * per given notebook's remote guid. It is important, guid here is the remote one,
     * the one used by Evernote service, not the local guid!
     * @param notebookGuid - remote Evernote service's guid of notebook for which
     * shared notebooks are requested
     * @param errorDescription - errir description if shared notebooks per notebook guid
     * could not be listed; if no error happens, this parameter is untouched
     * @return either list of shared notebooks per notebook guid or empy list
     * in case of error of no shared notebooks presence per given notebook guid
     */
    QList<SharedNotebookWrapper> ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                    QString & errorDescription) const;

    /**
     * @brief ExpungeNotebook - deletes specified notebook from local storage.
     * Evernote API doesn't allow to delete notebooks from remote storage, it can
     * only be done by official desktop client or web GUI. So this method should be called
     * only during the synchronization with remote database, when some notebook is found to be
     * deleted via either official desktop client or web GUI
     * @param notebook - notebook to be expunged. Must have either "remote" or local guid set
     * @param errorDescription - error description if notebook could not be expunged
     * @return true if notebook was expunged successfully, false otherwise
     */
    bool ExpungeNotebook(const Notebook & notebook, QString & errorDescription);

    /**
     * @brief GetLinkedNotebookCount - returns the number of linked notebooks stored in the local storage database
     * @param errorDescription - error description if the number of linked notebooks count not be returned
     * @return either non-negative number of linked notebooks or -1 if some error has occured
     */
    int GetLinkedNotebookCount(QString & errorDescription) const;

    /**
     * @brief AddLinkedNotebook - adds passed in LinkedNotebook to the local storage database;
     * LinkedNotebook must have "remote" Evernote service's guid set. It is not possible
     * to add a linked notebook in offline mode so it doesn't make sense for LinkedNotebook
     * objects to not have guid.
     * @param linkedNotebook - LinkedNotebook to be added to the local storage database
     * @param errorDescription - error description if linked notebook could not be added
     * @return true if linked notebook was added successfully, false otherwise
     */
    bool AddLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);

    /**
     * @brief UpdateLinkedNotebook - updates passd in LinkedNotebook in the local storage database;
     * LinkedNotebook must have "remote" Evernote service's guid set.
     * @param linkedNotebook - LinkedNotebook to be updated in the local storage database
     * @param errorDescription - error description if linked notebook could not be updated
     * @return true if linked notebook was updated successfully, false otherwise
     */
    bool UpdateLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);

    /**
     * @brief FindLinkedNotebook - attempts to find and set all found fields for passed in
     * by reference LinkedNotebook object. For LinkedNotebook local guid doesn't mean anything
     * because it can only be considered valid if it has "remote" Evernote service's guid set.
     * So this passed in LinkedNotebook object must have guid set to identify
     * the linked notebook in the local storage database.
     * @param linkedNotebook - linked notebook to be found. Must have "remote" guid set
     * @param errorDescription - error description if linked notebook could not be found
     * @return true if linked notebook was found, false otherwise
     */
    bool FindLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const;

    /**
     * @brief ListAllLinkedNotebooks - attempts to list all linked notebooks within the account
     * @param errorDescription - error description if linked notebooks could not be listed,
     * otherwise this parameter is untouched
     * @return either list of all linked notebooks or empty list in case of error or
     * no linked notebooks presence within the account
     */
    QList<LinkedNotebook> ListAllLinkedNotebooks(QString & errorDescription) const;

    /**
     * @brief ExpungeLinkedNotebook - deletes specified linked notebook from local storage.
     * Evernote API doesn't allow to delete linked notebooks from remote storage, it can
     * only be done by official desktop client or web GUI. So this method should be called
     * only during the synchronization with remote database, when some linked notebook
     * is found to be deleted via either official desktop client or web GUI
     * @param linkedNotebook - linked notebook to be expunged. Must have "remote" guid set
     * @param errorDescription - error description if linked notebook could not be expunged
     * @return true if linked notebook was expunged successfully, false otherwise
     */
    bool ExpungeLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);

    /**
     * @brief GetNoteCount returns the number of non-deleted notes currently stored in local storage database
     * @param errorDescription - error description if the number of notes could not be returned
     * @return either non-negative value with the number of notes or -1 which means some error occured
     */
    int GetNoteCount(QString & errorDescription) const;

    /**
     * @brief AddNote - adds passed in Note to the local storage database.
     * @param note - note to be passed to local storage database
     * @param notebook - notebook for which the note must be added. It is needed
     * because note only keeps "remote" notebook's guid inside itself but local storage
     * references note - notebook relations via both local and "remote" guids if the latter one is set.
     * The presence of this input parameter in the method ensures it would be possible
     * to create offline notebooks and notes from the local storage perspective.
     * Also, the notebook may prohibit the creation of notes in which case the error
     * would be returned
     * @param errorDescription - error description if note could not be added
     * @return true if note was added successfully, false otherwise
     */
    bool AddNote(const Note & note, const Notebook & notebook, QString & errorDescription);

    /**
     * @brief UpdateNote - updates passed in Note in the local storage database
     * @param note - note to be updated in the local storage database
     * @param notebook - notebook in which the note must be updated. It is needed
     * because note only keeps "remote" notebook's guid inside itself but local storage
     * references note - notebook relations via both local and "remote" guids if the latter one is set.
     * The presence of this input parameter in the method ensures it would be possible
     * to create and update offline notebooks and notes from the local storage perspective.
     * Also, the notebook may prohibit the update of notes in which case the error
     * would be returned
     * @param errorDescription - error description if note could not be updated
     * @return true if note was updated successfully, false otherwise
     */
    bool UpdateNote(const Note & note, const Notebook & notebook, QString & errorDescription);

    /**
     * @brief FindNote - attempts to find note in the local storage database
     * @param note - note to be found in the local storage database. Must have either
     * local or "remote" Evernote service's guid set
     * @param errorDescription - error description if note could not be found
     * @param withResourceBinaryData - optional boolean parameter defining whether found note
     * should be filled with all the contents of its attached resources. By default this parameter is true
     * which means the whole contents of all resources would be filled. If it's false,
     * dataBody, recognitionBody or alternateDataBody won't be present within the found note's
     * resources
     * @return true if note was found, false otherwise
     */
    bool FindNote(Note & note, QString & errorDescription,
                  const bool withResourceBinaryData = true) const;

    /**
     * @brief ListAllNotesPerNotebook - attempts to list all notes per given notebook
     * @param notebook - notebook for which list of notes is requested. If it has
     * "remote" Evernote service's guid set, it would be used to identify the notebook
     * in the local storage database, otherwise its local guid would be used
     * @param errorDescription - error description in case notes could not be listed
     * @param withResourceBinaryData - optional boolean parameter defining whether found notes
     * should be filled with all the contents of their respective attached resources.
     * By default this parameter is true which means the whole contents of all resources
     * would be filled. If it's false, dataBody, recognitionBody or alternateDataBody
     * won't be present within each found note's resources
     * @return either list of notes per notebook or empty list in case of error or
     * no notes presence in the given notebook
     */
    QList<Note> ListAllNotesPerNotebook(const Notebook & notebook, QString & errorDescription,
                                        const bool withResourceBinaryData = true) const;


    /**
     * @brief DeleteNote - either expunges the local note (i.e. deletes it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks
     * the note as deleted otherwise. Evernote API doesn't allow thirdparty
     * applications to expunge notes which have ever been synchronized
     * with remote data store at least once
     * @param note - note to be deleted
     * @param errorDescription - error description if note could not be deleted
     * @return true if note was deleted successfully, false otherwise
     */
    bool DeleteNote(const Note & note, QString & errorDescription);

    /**
     * @brief ExpungeNote - permanently deletes local notes i.e. notes which
     * have not yet been synchronized with Evernote service. This method deletes
     * the note from local storage completely, without the possibility to restore it
     * @param note - note to be expunged
     * @param errorDescription - error description if note could not be expunged
     * @return true if note was expunged successfully, false otherwise
     */
    bool ExpungeNote(const Note & note, QString & errorDescription);

    /**
     * @brief GetTagCount returns the number of non-deleted tags currently stored in local storage database
     * @param errorDescription - error description if the number of tags could not be returned
     * @return either non-negative value with the number of tags or -1 which means some error occured
     */
    int GetTagCount(QString & errorDescription) const;

    /**
     * @brief AddTag - adds passed in Tag to the local storage database. If tag has
     * "remote" Evernote service's guid set, it is identified in the database by this guid.
     * Otherwise it is identified by local guid.
     * @param tag - tag to be added to the local storage
     * @param errorDescription - error description if Tag could not be added
     * @return true if Tag was added successfully, false otherwise
     */
    bool AddTag(const Tag & tag, QString & errorDescription);

    /**
     * @brief UpdateTag - updates passed in Tag in the local storage database. If tag has
     * "remote" Evernote service's guid set, it is identified in the database by this guid.
     * Otherwise it is identified by local guid.
     * @param tag - Tag filled with values to be updated in the local storage database
     * @param errorDescription - error description if Tag could not be updated
     * @return true if Tag was updated successfully, false otherwise
     */
    bool UpdateTag(const Tag & tag, QString & errorDescription);

    /**
     * @brief LinkTagWithNote - attempts to link the given tag to the given note
     * in the local storage database. Note that "note" parameter is not altered
     * as a result of this method. Both note and tag can have or not have "remote"
     * Evernote service's guid set, in case they are not set only their local guids
     * would be linked. This way makes it possible to create both offline notes and tags
     * and link tags to notes. During the synchronization procedure one needs to
     * carefully review the linkage of tags and notes via local guids to ensure
     * no such any connection is forgotten to be re-expressed in terms of "remote" guids.
     * @param tag - tag to be linked with note
     * @param note - note to be linked with tag
     * @param errorDescription - error description if tag could not be linked with note
     * @return true if tag was linked to note successfully, false otherwise
     */
    bool LinkTagWithNote(const Tag & tag, const Note & note, QString & errorDescription);

    /**
     * @brief FindTag - attempts to find and fill the fields of passed in tag object.
     * If it would have "remote" Evernote service's guid set, it would be used to identify
     * the tag in the local storage database. Otherwise the local guid would be used.
     * @param tag - tag to be found in the local storage database
     * @param errorDescription - error description in case tag could not be found
     * @return true if tag was found, false otherwise
     */
    bool FindTag(Tag & tag, QString & errorDescription) const;

    /**
     * @brief ListAllTagsPerNote - lists all tags per given note
     * @param note - note for which the list of tags is requested. If it has "remote"
     * Evernote service's guid set, it is used to identify the note in the local storage database.
     * Otherwise its local guid is used for that.
     * @param errorDescription - error description if tags were not listed successfully.
     * In such case the returned list of tags would be empty and error description won't be empty.
     * However, if, for example, the list of tags is empty and error description is empty too,
     * it means the provided note does not have any tags assigned to it.
     * @return the list of found tags per note
     */
    QList<Tag> ListAllTagsPerNote(const Note & note, QString & errorDescription) const;

    /**
     * @brief ListAllTags - lists all tags within current user's account
     * @param errorDescription - error description if tags were not listed successfully.
     * In such case the returned list of tags would be empty and error description won't be empty.
     * However, if, for example, the list of tags is empty and error description is empty too,
     * it means the current account does not have any tags created.
     * @return the list of found tags within the account
     */
    QList<Tag> ListAllTags(QString & errorDescription) const;

    /**
     * @brief DeleteTag - either expunges the local tag (i.e. deletes it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks the tag
     * as deleted otherwise. Evernote API doesn't allow thirdparty applications
     * to expunge tags which have ever been synchronized with remote data store at least once
     * @param tag - tag to be deleted
     * @param errorDescription - error description if tag could not be deleted
     * @return true if tag was deleted successfully, false otherwise
     */
    bool DeleteTag(const Tag & tag, QString & errorDescription);

    /**
     * @brief ExpungeTag - permanently deletes local tags i.e. tags which have not yet been
     * sychronized with Evernote service. This method deletes the tag from local storage
     * completely, without the possibility to restore it
     * @param tag - tag to be expunged
     * @param errorDescription - error description if tag could not be expunged
     * @return true if tag was expunged successfully, false otherwise
     */
    bool ExpungeTag(const Tag & tag, QString & errorDescription);

    /**
     * @brief GetEnResourceCount returns the number of resources currently stored in local storage database
     * @param errorDescription - error description if the number of resources could not be returned
     * @return either non-negative value with the number of resources or -1 which means some error occured
     */
    int GetEnResourceCount(QString & errorDescription) const;

    /**
     * @brief AddEnResource - adds passed in resource to the local storage database
     * @param resource - resource to be added to the database
     * @param note - note for which the resource is added. If note doesn't have
     * "remote" Evernote service's guid set, the resource can be linked to note
     * by its local guid (but only in the database, not in IResource subclass object).
     * @param errorDescription - error description if resource could not be added
     * @return true if resource was added successfully, false otherwise
     */
    bool AddEnResource(const IResource & resource, const Note & note, QString & errorDescription);

    /**
     * @brief UpdateEnResource - updates passed in resource in the local storage database
     * @param resource - resource to be updated
     * @param note - note for which the resource is updated. If note doesn't have
     * "remote" Evernote service's guid set, the resource can be linked to note
     * by its local guid (but only in the database, not in IResource subclass object).
     * @param errorDescription - error description if resource could not be updated
     * @return true if resource was updated successfully, false otherwise
     */
    bool UpdateEnResource(const IResource & resource, const Note & note, QString & errorDescription);

    /**
     * @brief FindEnResource - attempts to find resource in the local storage database
     * @param resource - resource to be found in the local storage database. If it has
     * "remote" Evernote service's guid set, this guid is used to identify the resource
     * in the local storage database. Otherwise resource's local guid is used
     * @param errorDescription - error description if resource could not be found
     * @param withBinaryData - optional parameter defining whether found resource should have
     * dataBody recognitionBody, alternateDataBody filled with actual binary data.
     * By default this parameter is true.
     * @return true if resource was found successfully, false otherwise
     */
    bool FindEnResource(IResource & resource, QString & errorDescription, const bool withBinaryData = true) const;

    // NOTE: there is no 'DeleteEnResource' method for a reason: resources are deleted automatically
    // in remote storage so there's no need to mark some resource as deleted for synchronization procedure.

    /**
     * @brief ExpungeResource - permanently deletes resource from local storage completely,
     * without the possibility to restore it
     * @param resource - resource to be expunged
     * @param errorDescription - error description if resource could not be expunged
     * @return true if resource was expunged successfully, false otherwise
     */
    bool ExpungeEnResource(const IResource & resource, QString & errorDescription);

    /**
     * @brief GetSavedSearchCount returns the number of saved seacrhes currently stored in local storage database
     * @param errorDescription - error description if the number of saved seacrhes could not be returned
     * @return either non-negative value with the number of saved seacrhes or -1 which means some error occured
     */
    int GetSavedSearchCount(QString & errorDescription) const;

    /**
     * @brief AddSavedSearch - adds passed in SavedSearch to the local storage database;
     * If search has "remote" Evernote service's guid set, it is identified in the database
     * by this guid. Otherwise it is identified by local guid.
     * @param search - SavedSearch to be added to the local storage
     * @param errorDescription - error description if SavedSearch could not be added
     * @return true if SavedSearch was added successfully, false otherwise
     */
    bool AddSavedSearch(const SavedSearch & search, QString & errorDescription);

    /**
     * @brief UpdateSavedSearch - updates passed in SavedSearch in th local storage database.
     * If search has "remote" Evernote service's guid set, it is identified in the database
     * by this guid. Otherwise it is identified by local guid.
     * @param search - SavedSearch filled with values to be updated in the local storage database
     * @param errorDescription - error description if SavedSearch could not be updated
     * @return true if SavedSearch was updated successfully, false otherwise
     */
    bool UpdateSavedSearch(const SavedSearch & search, QString & errorDescription);

    /**
     * @brief FindSavedSearch - attempts to find SavedSearch in the local storage database
     * @param search - SavedSearch to be found in the local storage database. If it
     * would have "remote" Evernote service's guid set, it would be used to identify
     * the search in the local storage database. Otherwise its local guid would be used.
     * @param errorDescription - error description if SavedSearch could not be found
     * @return true if SavedSearch was found, false otherwise
     */
    bool FindSavedSearch(SavedSearch & search, QString & errorDescription) const;

    /**
     * @brief ListAllSavedSearches - lists all saved searches within the account
     * @param errorDescription - error description if all saved searches could not be listed;
     * otherwise this parameter is untouched
     * @return either the list of all saved searches within the account or empty list
     * in case of error or if there are no saved searches within the account
     */
    QList<SavedSearch> ListAllSavedSearches(QString & errorDescription) const;

    // NOTE: there is no 'DeleteSearch' method for a reason: saved searches are deleted automatically
    // in remote storage so there's no need to mark some saved search as deleted for synchronization procedure.

    /**
     * @brief ExpungeSavedSearch - permanently deletes saved search from local storage completely,
     * without the possibility to restore it
     * @param search - saved search to be expunged
     * @param errorDescription - error description if saved search could not be expunged
     * @return true if saved search was expunged succesfully, false otherwise
     */
    bool ExpungeSavedSearch(const SavedSearch & search, QString & errorDescription);

private:
    LocalStorageManager() = delete;
    LocalStorageManager(const LocalStorageManager & other) = delete;
    LocalStorageManager & operator=(const LocalStorageManager & other) = delete;

    LocalStorageManagerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(LocalStorageManager)
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_H
