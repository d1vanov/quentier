#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__I_USER_STORE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__I_USER_STORE_H

#include <string>

namespace qute_note {

class IUserStore
{
public:
    /**
     * @brief getAuthenticationToken - attempts to provide a valid authentication token
     * @param authenticationToken - contains authentication token in case of success or
     * error description in case of failure
     * @return true if authentication token was provided successfully, false otherwise
     *
     * @note This method is not constant because it is more than a simple accessor:
     * for example, if authentication token has expired, this method attempts to refresh it
     * before returning.
     */
    virtual bool getAuthenticationToken(std::string & authenticationToken);

    /**
     * @brief getNoteStoreUrl - attempts to provide a valid URL for NoteStore
     * @param noteStoreUrl - contains path to NoteStore in case of success or
     * error description in case of failure
     * @return true if NoteStore URL was provided successfully, false otherwise
     *
     * @note This method is not constant because it is more than a simple accessor:
     * for example, it can attempt to refresh authentication token to receive new pack of
     * authentication result containing the NoteStore URL
     */
    virtual bool getNoteStoreUrl(std::string & noteStoreUrl);
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__I_USER_STORE_H
