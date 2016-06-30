#ifndef LIB_QUENTIER_TYPES_USER_WRAPPER_H
#define LIB_QUENTIER_TYPES_USER_WRAPPER_H

#include "IUser.h"
#include <QEverCloud.h>
#include <QSharedDataPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(UserWrapperData)

/**
 * @brief The UserWrapper class creates and manages its own instance of
 * qevercloud::User object
 */
class QUENTIER_EXPORT UserWrapper: public IUser
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

} // namespace quentier

Q_DECLARE_METATYPE(quentier::UserWrapper)

#endif // LIB_QUENTIER_TYPES_USER_WRAPPER_H
