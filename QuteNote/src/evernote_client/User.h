#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__USER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__USER_H

#include "../tools/TypeWithError.h"
#include <memory>

namespace qute_note {

class User: public TypeWithError
{
public:
    User();

private:
    class UserImpl;
    std::unique_ptr<UserImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__USER_H
