#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H

#include <string>
#include <vector>
#include <memory>

namespace qute_note {

class User;
class Guid;
class Note;
class Tag;
class Notebook;
class LinkedNotebook;
class SharedNotebook;

class EvernoteServiceManager
{
public:
    EvernoteServiceManager();

    /**
     * @brief login - attempts to authenticate to Evernote service with provided username and password
     * @param username
     * @param password
     * @param errorMessage - message explaining why login to the service failed
     * @return true if login was successful, false otherwise
     */
    bool login(const std::string & username, const std::string & password,
               const char *& errorMessage);

    /**
     * @brief logout - quits from current Evernote service user account
     * or does nothing if there is no current user account
     */
    void logout();

    /**
     * @brief authenticateToSharedNote - attempts to authenticate to shared note
     * in another user's Evernote account.
     * @param guid - the guid of note to be authenticated to
     * @param noteKey - the share key necessary to access the note
     * @param errorMessage - message explaining why the authentication to shared note failed
     * @return true if authentication to shared note was successful, false otherwise
     */
    bool authenticateToSharedNote(const Guid & guid, const std::string & noteKey,
                                  const char *& errorMessage);

    /**
     * @brief authenticateToSharedNotebook - attempts to authenticate to shared notebook
     * @param sharedNotebookKey - share key identifier from the shared notebook
     * @param errorMessage - message explaining why the authentication to shared notebook failed
     * @return true if authenticaton to shared notebook was successful, false otherwise
     */
    bool authenticateToSharedNotebook(const std::string & sharedNotebookKey,
                                      const char *& errorMessage);

    /**
     * @brief synchronize - attempts to synchronize to Evernote service; decides
     * on its own whether it should be full or incremental synchronization. However,
     * it is possible to force full synchronization by passing a boolean flag = true to it
     * @param errorMessage - message explaining why the synchronization with the service failed
     * @param forceFullSync - when this parameter is true, full synchronization is performed anyway
     * @return true if synchronization was successful, false otherwise
     */
    bool synchronize(const char *& errorMessage, const bool forceFullSync = false);

    /**
     * @brief createNote - attempts to create a new note within current Evernote account
     * @param notebookId - guid of notebook in which the note should be created
     * @param note - created note; needs to have the desired fields filled with data.
     * Will have error set if note creation in the service would fail
     * @param errorMessage - message explaining why the note was not created
     * @return true if note was created, false otherwise
     */
    bool createNote(const Guid & notebookId, Note & note, const char *& errorMessage);

    /**
     * @brief updateNote - attempts to send the updates made to the note to the service
     * @param note - note to be updated
     * @param errorMessage - message explaining why the note was not updated
     * @return true if note was updated, false otherwise
     */
    bool updateNote(const Note & note, const char *& errorMessage);

    /**
     * @brief deleteNote - attempts to delete the node with the specified guid to the trash;
     * NOTE: Evernote service does not provide API to expunge notes completely from the account,
     * only official Evernote clients are capable of that.
     * @param noteId - guid of note to be deleted
     * @param errorMessage - message explaining why the note was not created
     * @return true if the note was deleted, false otherwise
     */
    bool deleteNote(const Guid & noteId, const char *& errorMessage);

    /**
     * @brief findNote - attempts to find the note with specified guid in current Evernote account
     * @param note - found note; must have at least guid; will have error set if no note would be found
     * @param errorMessage - message explaining why the note was not found
     * @return true if note was found, false otherwise
     */
    bool findNote(Note & note, const char *& errorMessage) const;

    /**
     * @brief copyNote - attempts to copy the note with specified guid into the notebook
     * with the specified guid
     * @param noteId - guid of note to be copied
     * @param targetNotebookId - guid of target notebook
     * @param errorMessage - message explaining why the note was not copied
     * @return true if note was copied, false otherwise
     */
    bool copyNote(const Guid & noteId, const Guid & targetNotebookId,
                  const char *& errorMessage);

    /**
     * @brief emailNote - attempts to send the note with specified guid to the specified email adress
     * @param noteId - guid of note to be emailed
     * @param emailAddress - email adress to send the note to
     * @param message - optional text message, may be empty
     * @param errorMessage - message explaining why the note was not emailed
     * @return true if note was emailed, false otherwise
     */
    bool emailNote(const Guid & noteId, const std::string & emailAddress,
                   const std::string & message, const char *& errorMessage);

    /**
     * @brief getNotesCountPerNotebook - attempts to provide the number of notes within specified notebook
     * @param notebookId - guid of notebook in which the number of notes is to be revealed
     * @param notesCount - the resulting number of notes
     * @param errorMessage - message explaining why the number of notes could not be provided
     * @return true if the number of notes was provided, false otherwise
     */
    bool getNotesCountPerNotebook(const Guid & notebookId, size_t & notesCount,
                                  const char *& errorMessage) const;

    /**
     * @brief getNotesCountPerTag - attempts to provide the number of notes marked with specified tag
     * @param tagId - guid of tag for which the number of notes is to be revealed
     * @param notesCount - resulting number of notes
     * @param errorMessage - message explaining why the number of notes could not be provided
     * @return true if the number of notes was provided, false otherwise
     */
    bool getNotesCountPerTag(const Guid & tagId, size_t & notesCount,
                             const char *& errorMessage) const;

    /**
     * @brief getTags - provides a vector containing all tags within current account
     * @param tags - resulting vector of tags
     * @param errorMessage - message explaining why the vector of tags could not be provided
     * @return true if the vector of tags was provided successfully, false otherwise
     */
    bool getTags(std::vector<Tag> & tags, const char *& errorMessage) const;

    /**
     * @brief getTagsPerNotebook - provides a vector containing all tags assotiated
     * with notes from the notebook with specified guid
     * @param tags - resulting vector of tags
     * @param notebookId - guid of notebook to search the tags for
     * @param errorMessage - message explaining why the vector of tags per notebook could not be provided
     * @return true if the vector of tags was provided successfully, false otherwise
     */
    bool getTagsPerNotebook(std::vector<Tag> & tags, const Guid & notebookId,
                            const char *& errorMessage) const;

    /**
     * @brief getTagsPerNote - provides a vector of tags associated with note
     * with provided guid. Performs double search in the database: first the names
     * of tags associated with the notes are obtained, then the guids are found
     * @param tags - resulting vector of tags
     * @param noteId - guid of note to search for the associated tags
     * @param errorMessage - message explaining why the vector of tags per note could not be provided
     * @return true if vector of tags was provided successfully, false otherwise
     */
    bool getTagsPerNote(std::vector<Tag> & tags, const Guid & noteId,
                        const char *& errorMessage) const;

    /**
     * @brief createTag - attempts to create a new tag in current user's account
     * @param tagToCreate - tag with set name and optionally with non-empty parentGuid.
     * If the name of tag to create is empty, the method will reject the request
     * @param errorMessage - message explaining why the tag was not created
     * @return true if tag was successfully created, false otherwise
     */
    bool createTag(const Tag & tagToCreate, const char *& errorMessage);

    /**
     * @brief updateTag - attempts to update the tag with specified name and parent guid
     * @param tagToUpdate - tag object which should contain name and parent guid
     * which will be updated
     * @param errorMessage - message explaining why the tag was not updated
     * @return true if tag was updated successfully, false otherwise
     */
    bool updateTag(const Tag & tagToUpdate, const char *& errorMessage);

    /**
     * @brief deleteTag - attempts to delete the tag with provided guid from all notes
     * within current user's account
     * @param tagGuid - guid of tag to be untagged from all notes
     * @param errorMessage - message explaining why untag was not done successfully
     * @return true if untag was done successfully, false otherwise
     */
    bool untagAll(const Guid & tagGuid, const char *& errorMessage);

    /**
     * @brief findTag - attempts to find the tag with specified guid in current user's account
     * @param tagGuid - guid of tag to find
     * @param tag - resulting found tag object
     * @param errorMessage - message explaining why tag was not found
     * @return true if tag was found, false otherwise
     */
    bool findTag(const Guid & tagGuid, Tag & tag, const char *& errorMessage);

    /**
     * @brief createNotebook - attempts to create a new notebook within current Evernote account
     * @param createdNotebook - created notebook; will have error set if the notebook would not be created
     * @param errorMessage - message explaining why the notebook was not created
     * @return true if the notebook was created, false otherwise
     */
    bool createNotebook(Notebook & createdNotebook, const char *& errorMessage);

    /**
     * @brief findNotebook - attempts to find the notebook with specified guid in current Evernote account
     * @param notebook - found notebook; must have at least guid; will have error set if no notebook would be found
     * @param errorMessage - mesage explaining why the notebook was not found
     * @return true if notebook was found, false otherwise
     */
    bool findNotebook(Notebook & notebook, const char *& errorMessage) const;

    /**
     * @brief getDefaultNotebook - attempts to return the default notebook in which the new notes
     * should be stored if no other notebook was specified
     * @param defaultNotebook - resulting default notebook; will have error set if no notebook would be found
     * @param errorMessage - message explaining why the default notebook was not found
     * @return true if default notebook was found, false otherwise
     */
    bool getDefaultNotebook(Notebook & defaultNotebook, const char *& errorMessage) const;

    /**
     * @brief createLinkedNotebook - attempts to create a linked notebook
     * @param notebook - before the call this object should contain the name of
     * the linked notebook and either a username uri or a shard id and share key must be set
     * @param errorMessage - message explaining why the linked notebook was not created
     * @return true if linked notebook was created, false otherwise
     */
    bool createLinkedNotebook(LinkedNotebook & notebook, const char *& errorMessage) const;

    /**
     * @brief createSharedNotebook - attempts to create a shared notebook
     * @param notebook - before the call this object should contain the email adress of
     * share recipient, guid and notebook permissions (including "allowPreview" property)
     * @param errorMessage - message explaining why the shared notebook was not created
     * @return true if shared notebook was created, false otherwise
     */
    bool createSharedNotebook(SharedNotebook & notebook, const char *& errorMessage) const;

private:
    class EvernoteServiceManagerImpl;
    std::unique_ptr<EvernoteServiceManagerImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H
