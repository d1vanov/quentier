#ifndef LIB_QUTE_NOTE_TYPES_I_USER_H
#define LIB_QUTE_NOTE_TYPES_I_USER_H

#include <qute_note/utility/Printable.h>
#include <QtGlobal>
#include <QEverCloud.h>

namespace qute_note {

class QUTE_NOTE_EXPORT IUser: public Printable
{
public:
    typedef qevercloud::PrivilegeLevel::type  PrivilegeLevel;

public:
    IUser();
    virtual ~IUser();

    bool operator==(const IUser & other) const;
    bool operator!=(const IUser & other) const;

    operator const qevercloud::User&() const;
    operator qevercloud::User&();

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
    const QString & username() const;
    void setUsername(const QString & username);

    bool hasEmail() const;
    const QString & email() const;
    void setEmail(const QString & email);

    bool hasName() const;
    const QString & name() const;
    void setName(const QString & name);

    bool hasTimezone() const;
    const QString & timezone() const;
    void setTimezone(const QString & timezone);

    bool hasPrivilegeLevel() const;
    PrivilegeLevel privilegeLevel() const;
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
    const qevercloud::UserAttributes & userAttributes() const;
    void setUserAttributes(qevercloud::UserAttributes && attributes);

    bool hasAccounting() const;
    const qevercloud::Accounting & accounting() const;
    void setAccounting(qevercloud::Accounting && accounting);

    bool hasPremiumInfo() const;
    const qevercloud::PremiumInfo & premiumInfo() const;
    void setPremiumInfo(qevercloud::PremiumInfo && premiumInfo);

    bool hasBusinessUserInfo() const;
    const qevercloud::BusinessUserInfo & businessUserInfo() const;
    void setBusinessUserInfo(qevercloud::BusinessUserInfo && info);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    friend class Notebook;

protected:
    IUser(const IUser & other);
    IUser(IUser && other);
    IUser & operator=(const IUser & other);
    IUser & operator=(IUser && other);

    virtual const qevercloud::User & GetEnUser() const = 0;
    virtual qevercloud::User & GetEnUser() = 0;

private:
    bool m_isDirty;
    bool m_isLocal;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_I_USER_H
