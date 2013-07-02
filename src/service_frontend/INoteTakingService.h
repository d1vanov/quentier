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

    /**
     * Class for search content - search within note, withon notebook or any other
     * data somehow related to search
     */
    class SearchContent;

public:
    /**
     * @brief AddNote - adds new note to the database
     * @param userInfo - login, password and all other data related to authentication in the service
     * @param notebookInfo - information about the notebook or notebooks - name or id or anything else
     * @param noteInfo - note itself, its tags, resources and all related data
     * @param err_str - error string if adding the note was not successful
     * @return true if the note was added successfully, false otherwise
     */
    bool AddNote(const UserInfo & userInfo, const NotebookInfo & notebookInfo,
                 const NoteInfo & noteInfo, const char *& err_str);

    /**
     * @brief DeleteNote - deletes specific note from the database
     * @param userInfo - login, password and all other data related to authentication in the service
     * @param notebookInfo - information about the notebook or notebooks - name or id or anything else
     * @param noteInfo - note itself, its tags, resources and all related data
     * @param err_str - error string if deleting the note was not successful
     * @return true if the note was deleted successfully, false otherwise
     */
    bool DeleteNote(const UserInfo & userInfo, const NotebookInfo & notebookInfo,
                    const NoteInfo & noteInfo, const char *& err_str);

    /**
     * @brief SearchForNote - non-const version; searches for specific note in specific notebook or
     * specific notebooks or even all notebooks (it is specified in @param notebookInfo)
     * @param userInfo - login, password and all other data related to authentication in the service
     * @param notebookInfo - information about the notebook or notebooks - name or id or anything else
     * @param searchContent - any information related to current search, filter
     * by note name, by notebook name, by text piece or anything else.
     * @param pNoteInfo - resulting pointer to found note; nullptr if the search was not successful
     * @param err_str - error string; not touched if the search was successful, otherwise
     * contains explanation why the rearch failed
     * @return true if the note was found, false otherwise
     */
    bool SearchForNote(const UserInfo & userInfo, const NotebookInfo & notebookInfo,
                       const SearchContent & searchContent, NoteInfo * pNoteInfo,
                       const char *& err_str);

    /**
     * @brief SearchFotNote - const version; searches for specific note in specific notebook or
     * specific notebooks or even all notebooks (it is specified in @param notebookInfo)
     * @param userInfo - login, password and all other data related to authentication in the service
     * @param notebookInfo - information about the notebook or notebooks - name or id or anything else
     * @param searchContent - any information related to current search, filter
     * by note name, by notebook name, by text piece or anything else.
     * @param pNoteInfo - resulting pointer to found note; nullptr if the search was not successful
     * @param err_str - error string; not touched if the search was successful, otherwise
     * contains explanation why the rearch failed
     * @return true if the note was found, false otherwise
     */
    bool SearchFotNote(const UserInfo & UserInfo, const NotebookInfo & notebookInfo,
                       const SearchContent & searchContent, const NoteInfo * pNoteInfo,
                       const char *& err_str) const;
};

#endif // __QUTE_NOTE__SERVICE_FRONTEND__INOTE_TAKING_SERVICE_H
