#ifndef __LIB_QUTE_NOTE__TYPES__USER_WRAPPER_H
#define __LIB_QUTE_NOTE__TYPES__USER_WRAPPER_H

#include "IUser.h"
#include <QEverCloud.h>
#include <QSharedDataPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(UserWrapperData)

/**
 * @brief The UserWrapper class creates and manages its own instance of
 * qevercloud::User object
 */
class QUTE_NOTE_EXPORT UserWrapper: public IUser
{
public:
    UserWrapper();
    UserWrapper(const UserWrapper & other);
    UserWrapper(UserWrapper && other);
    UserWrapper & operator=(const UserWrapper & other);
    UserWrapper & operator=(UserWrapper && other);
    virtual ~UserWrapper();

private:
    virtual const qevercloud::User & GetEnUser() const Q_DECL_OVERRIDE;
    virtual qevercloud::User & GetEnUser() Q_DECL_OVERRIDE;

    QSharedDataPointer<UserWrapperData> d;
};

} // namespace qute_note

Q_DECLARE_METATYPE(qute_note::UserWrapper)

#endif // __LIB_QUTE_NOTE__TYPES__USER_WRAPPER_H
