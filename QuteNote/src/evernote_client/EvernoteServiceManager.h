#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H

#include <string>
#include <memory>

namespace qute_note {

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

private:
    class EvernoteServiceManagerImpl;
    std::unique_ptr<EvernoteServiceManagerImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_H
