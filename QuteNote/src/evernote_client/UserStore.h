#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__USER_STORE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__USER_STORE_H

#include <memory>

namespace qute_note {

class UserStore
{
public:
    UserStore();

private:
    class UserStoreImpl;
    std::unique_ptr<UserStoreImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__USER_STORE_H
