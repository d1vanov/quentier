/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_TYPES_I_USER_H
#define LIB_QUENTIER_TYPES_I_USER_H

#include <quentier/utility/Printable.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QtGlobal>
#include <QEverCloud.h>

namespace quentier {

class QUENTIER_EXPORT IUser: public Printable
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

    bool checkParameters(QNLocalizedString & errorDescription) const;

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

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_I_USER_H
