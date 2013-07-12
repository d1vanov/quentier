#ifndef __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H
#define __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H

#include <string>
#include "../tools/Singleton.h"

class EvernoteServiceManager
{
public:
    static EvernoteServiceManager & Instance();

//    /**
//     * @brief Authenticate - attempts to authenticate to Evernote service
//     * @param username
//     * @param password
//     * @param err_str - contains error message if authentication was not successful
//     * @return true if authentication was successful, false otherwise
//     */
//    bool Authenticate(const std::string & username,
//                      const std::string & password,
//                      const char *& err_str);

    friend class qutenote_tools::Singleton<EvernoteServiceManager>;

private:
    EvernoteServiceManager() {}  // standard constructor does nothing but exists as private method

    EvernoteServiceManager(const EvernoteServiceManager & other) = delete;
    EvernoteServiceManager & operator=(const EvernoteServiceManager & other) = delete;
};

#endif // __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H
