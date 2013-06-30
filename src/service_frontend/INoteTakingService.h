#ifndef __QUTE_NOTE__SERVICE_FRONTEND__INOTE_TAKING_SERVICE_H
#define __QUTE_NOTE__SERVICE_FRONTEND__INOTE_TAKING_SERVICE_H

class INoteTakingService
{
public:
    INoteTakingService();
    INoteTakingService(const INoteTakingService & other);
    INoteTakingService & operator=(const INoteTakingService & other);
    virtual ~INoteTakingService();

public:
    /**
     * Authentication info
     */
    class UserInfo;

    /**
     * Particular "folder" in the database
     */
    class NotebookInfo;

    /**
     * Note itself, its tags, resources and all other related data
     */
    class NoteInfo;

public:
    /**
     * @brief AddNote - adds new note to the database
     * @param userInfo - login, password and all other data related to authentication in the service
     * @param notebookInfo - information about the notebook - its name or id or anything like that
     * @param noteInfo - note itself, its tags, resources and all related data
     * @param err_str - error string if adding the note was not successful
     * @return true if the note was added successfully, false otherwise
     */
    bool AddNote(const UserInfo & userInfo, const NotebookInfo & notebookInfo,
                 const NoteInfo & noteInfo, const char *& err_str);

    /**
     * @brief DeleteNote - deletes specific note from the database
     * @param userInfo - login, password and all other data related to authentication in the service
     * @param notebookInfo - information about the notebook - its name or id or anything like that
     * @param noteInfo - note itself, its tags, resources and all related data
     * @param err_str - error string if deleting the note was not successful
     * @return true if the note was deleted successfully, false otherwise
     */
    bool DeleteNote(const UserInfo & userInfo, const NotebookInfo & notebookInfo,
                    const NoteInfo & noteInfo, const char *& err_str);
};

#endif // __QUTE_NOTE__SERVICE_FRONTEND__INOTE_TAKING_SERVICE_H
