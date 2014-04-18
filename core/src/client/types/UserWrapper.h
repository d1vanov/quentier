#ifndef __QUTE_NOTE__CLIENT__TYPES__USER_WRAPPER_H
#define __QUTE_NOTE__CLIENT__TYPES__USER_WRAPPER_H

#include "IUser.h"
#include <Types_types.h>

namespace qute_note {

/**
 * @brief The UserWrapper class creates and manages its own instance of
 * evernote::edam::User object
 */
class UserWrapper final: public IUser
{
public:
    UserWrapper();
    UserWrapper(const UserWrapper & other);
    UserWrapper & operator=(const UserWrapper & other);
    virtual ~UserWrapper() final override;

private:
    virtual const evernote::edam::User & GetEnUser() const final override;
    virtual evernote::edam::User & GetEnUser() final override;

    evernote::edam::User m_enUser;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__USER_WRAPPER_H
