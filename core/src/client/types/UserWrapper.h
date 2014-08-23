#ifndef __QUTE_NOTE__CLIENT__TYPES__USER_WRAPPER_H
#define __QUTE_NOTE__CLIENT__TYPES__USER_WRAPPER_H

#include "IUser.h"
#include <QEverCloud.h>

namespace qute_note {

/**
 * @brief The UserWrapper class creates and manages its own instance of
 * qevercloud::User object
 */
class QUTE_NOTE_EXPORT UserWrapper final: public IUser
{
public:
    UserWrapper() = default;
    UserWrapper(const UserWrapper & other) = default;
    UserWrapper(UserWrapper && other);
    UserWrapper & operator=(const UserWrapper & other) = default;
    UserWrapper & operator=(UserWrapper && other);

    virtual ~UserWrapper() final override;

private:
    virtual const qevercloud::User & GetEnUser() const final override;
    virtual qevercloud::User & GetEnUser() final override;

    qevercloud::User m_qecUser;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__USER_WRAPPER_H
