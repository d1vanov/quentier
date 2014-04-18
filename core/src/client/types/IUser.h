#ifndef __QUTE_NOTE__CLIENT__TYPES__IUSER_H
#define __QUTE_NOTE__CLIENT__TYPES__IUSER_H

#include <tools/Printable.h>
#include <QtGlobal>

namespace evernote {
namespace edam {
QT_FORWARD_DECLARE_CLASS(User)
}
}

namespace qute_note {

class QUTE_NOTE_EXPORT IUser: public Printable
{
public:
    IUser();
    virtual ~IUser();

    bool operator==(const IUser & other) const;
    bool operator!=(const IUser & other) const;

    void clear();

    bool isDirty() const;
    void setDirty(const bool dirty);

    bool isLocal() const;
    void setLocal(const bool local);

    bool checkParameters(QString & errorDescription) const;

    bool hasId() const;
    qint32 id() const;
    void setId(const qint32 id);

    bool hasUsername() const;
    const QString username() const;
    void setUsername(const QString & username);

    bool hasEmail() const;
    const QString email() const;
    void setEmail(const QString & email);

    bool hasName() const;
    const QString name() const;
    void setName(const QString & name);

    bool hasTimezone() const;
    const QString timezone() const;
    void setTimezone(const QString & timezone);

    bool hasPrivilegeLevel() const;
    const qint8 privilegeLevel() const;
    void setPrivilegeLevel(const qint8 level);

    bool hasCreationTimestamp() const;
    qint64 creationTimestamp() const;
    void setCreationTimestamp(const qint64 timestamp);

    bool hasModificationTimestamp() const;
    qint64 modificationTimestamp() const;
    void setModificationTimestamp(const qint64 timestamp);

    bool hasDeletionTimestamp() const;
    qint64 deletionTimestamp() const;
    void setDeletionTimestamp(const qint64 timestamp);

    bool hasActive() const;
    bool active() const;
    void setActive(const bool active);

    bool hasUserAttributes() const;
    const evernote::edam::UserAttributes & userAttributes() const;
    evernote::edam::UserAttributes & userAttributes();
    void setHasAttributes(const bool hasAttributes);

    bool hasAccounting() const;
    const evernote::edam::Accounting & accounting() const;
    evernote::edam::Accounting & accounting();
    void setHasAccounting(const bool hasAccounting);

    bool hasPremiumInfo() const;
    const evernote::edam::PremiumInfo & premiumInfo() const;
    evernote::edam::PremiumInfo & premiumInfo();
    void setHasPremiumInfo(const bool hasPremiumInfo);

    bool hasBusinessUserInfo() const;
    const evernote::edam::BusinessUserInfo & businessUserInfo() const;
    evernote::edam::BusinessUserInfo & businessUserInfo();
    void setHasBusinessUserInfo(const bool hasBusinessUserInfo);

    friend class Notebook;

protected:
    IUser(const IUser & other);

    virtual const evernote::edam::User & GetEnUser() const = 0;
    virtual evernote::edam::User & GetEnUser() = 0;

private:
    // prevent slicing:
    IUser & operator=(const IUser & other) = delete;

    virtual QTextStream & Print(QTextStream & strm) const;

    bool m_isDirty;
    bool m_isLocal;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__IUSER_H
