#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__USER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__USER_H

#include "../tools/TypeWithError.h"
#include "../tools/Printable.h"
#include <QScopedPointer>

namespace qute_note {

class UserPrivate;
class User final: public TypeWithError,
                  public Printable
{
public:
    User();
    virtual ~User() override;

    int32_t id() const;

    const QString username() const;

    const QString email() const;

    const QString name() const;

    const QString timezone() const;

    uint32_t privilegeLevel() const;

    time_t createdTimestamp() const;
    time_t updatedTimestamp() const;
    time_t deletedTimestamp() const;

    bool isActive() const;

    virtual QTextStream & Print(QTextStream & strm) const;

private:
    const QScopedPointer<UserPrivate> d_ptr;
    Q_DECLARE_PRIVATE(User)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__USER_H
