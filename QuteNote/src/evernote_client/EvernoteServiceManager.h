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
     * @brief login - attempts to authenticate to Evernote service
     * with provided username and password
     */
    void login(const std::string & username, const std::string & password);

    /**
     * @brief logout - quits from current Evernote service user account
     * or does nothing if there is no current user account
     */
    void logout();

    /**
     * @brief synchronize - attempts to synchronize to Evernote service; decides
     * on its own whether it should be full or incremental synchronization. However,
     * it is possible to force full synchronization by passing a boolean
     */
    void synchronize(const bool forceFullSync = false);

    /**
     * @brief findNote - attempts to find the note with specified guid in current Evernote account
     * @param noteId - guid of note to searched for
     * @param notePtr - shared pointer to found note; null if the note was not found
     * @param errorMessage - message explaining why the note was not found
     * @return true if note was found, false otherwise
     */
    bool findNote(const Guid & noteId, std::shared_ptr<Note> & notePtr,
                  const char *& errorMessage) const;

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
     * @brief findNotebook - attempts to find the notebook with specified Guid in current Evernote account
     * @param notebookGuid - guid of notebook to be searched for
     * @param notebookPtr - shared pointet to found notebook; null if the notebook was not found
     * @param errorMessage - mesage explaining why the notebook was not found
     * @return true if notebook was found, false otherwise
     */
    bool findNotebook(const Guid & notebookGuid, std::shared_ptr<Notebook> & notebookPtr,
                      const char *& errorMessage) const;

private:
    class EvernoteServiceManagerImpl;
    std::unique_ptr<EvernoteServiceManagerImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H
