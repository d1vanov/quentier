#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__USER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__USER_H

#include "../tools/TypeWithError.h"
#include "../tools/Printable.h"
#include <QScopedPointer>
#include <ctime>

namespace qute_note {

class UserPrivate;
class User final: public TypeWithError,
                  public Printable
{
public:
    User();
    User(const User & other);
    User(User && other);
    User & operator=(const User & other);
    User & operator=(User && other);
    virtual ~User() override;

    Q_PROPERTY(int32_t id READ id WRITE setId)
    Q_PROPERTY(QString username READ username WRITE setUsername)
    Q_PROPERTY(QString email READ email WRITE setEmail)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString timezone READ timezone WRITE setTimezone)
    Q_PROPERTY(uint32_t privilegeLevel READ privilegeLevel WRITE setPrivilegeLevel)
    Q_PROPERTY(time_t creationTimestamp READ creationTimestamp WRITE setCreationTimestamp)
    Q_PROPERTY(time_t modificationTimestamp READ modificationTimestamp WRITE setModificationTimestamp)
    Q_PROPERTY(time_t deletionTimestamp READ deletionTimestamp WRITE setDeletionTimestamp)
    Q_PROPERTY(bool isActive READ isActive WRITE setActive)

    int32_t id() const;
    void setId(const int32_t id);

    const QString username() const;
    void setUsername(const QString & username);

    const QString email() const;
    void setEmail(const QString & email);

    const QString name() const;
    void setName(const QString & name);

    const QString timezone() const;
    void setTimezone(const QString & timezone);

    // TODO: change to enum
    uint32_t privilegeLevel() const;
    void setPrivilegeLevel(const uint32_t privilegeLevel);

    time_t creationTimestamp() const;
    void setCreationTimestamp(const time_t creationTimestamp);

    time_t modificationTimestamp() const;
    void setModificationTimestamp(const time_t modificationTimestamp);

    time_t deletionTimestamp() const;
    void setDeletionTimestamp(const time_t deletionTimestamp);

    bool isActive() const;
    void setActive(const bool is_active);

    virtual QTextStream & Print(QTextStream & strm) const;

private:
    QScopedPointer<UserPrivate> d_ptr;
    Q_DECLARE_PRIVATE(User)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__USER_H
