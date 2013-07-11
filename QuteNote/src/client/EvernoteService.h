#ifndef __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_H
#define __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_H

#include <string>

// NOTE: should be a singleton really, need to think about it
class EvernoteService
{
public:
    EvernoteService();

    /**
     * @brief Authenticate - attempts to authenticate to Evernote service
     * @param username
     * @param password
     * @param err_str - contains error message if authentication was not successful
     * @return true if authentication was successful, false otherwise
     */
    bool Authenticate(const std::string & username,
                      const std::string & password,
                      const char *& err_str);
};

#endif // __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_H
