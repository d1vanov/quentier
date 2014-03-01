#ifndef __QUTE_NOTE__CLIENT__TYPES__USER_ADAPTER_H
#define __QUTE_NOTE__CLIENT__TYPES__USER_ADAPTER_H

#include "IUser.h"

namespace qute_note {

/**
 * @brief The UserAdapter class uses reference to external evernote::edam::User
 * and adapts its interface to that of IUser
 */
class UserAdapter : public IUser
{
public:
    UserAdapter(evernote::edam::User & externalEnUser);
    UserAdapter(const evernote::edam::User & externalEnUser);
    UserAdapter(const UserAdapter & other);
    virtual ~UserAdapter() final override;

    virtual const evernote::edam::User & GetEnUser() const final override;
    virtual evernote::edam::User & GetEnUser() final override;

private:
    UserAdapter() = delete;
    UserAdapter & operator=(const UserAdapter & other) = delete;

    evernote::edam::User * m_pEnUser;
    bool m_isConst;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__USER_ADAPTER_H
