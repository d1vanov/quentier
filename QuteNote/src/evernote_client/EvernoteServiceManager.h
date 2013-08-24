#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H

#include <string>
#include <memory>

namespace qute_note {

class User;
class Guid;
class Note;
class Notebook;

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
     * @brief createNotebook - attempts to create a new notebook within current Evernote account
     * @param createdNotebook - created notebook; will have error set if the notebook would not be created
     * @param errorMessage - message explaining why the notebook was not created
     * @return true if the notebook was created, false otherwise
     */
    bool createNotebook(Notebook & createdNotebook, const char *& errorMessage);

    /**
     * @brief findNotebook - attempts to find the notebook with specified Guid in current Evernote account
     * @param notebook - found notebook; must have at least guid; will have error set if no notebook would be found
     * @param errorMessage - mesage explaining why the notebook was not found
     * @return true if notebook was found, false otherwise
     */
    bool findNotebook(Notebook & notebook, const char *& errorMessage) const;

private:
    class EvernoteServiceManagerImpl;
    std::unique_ptr<EvernoteServiceManagerImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H
