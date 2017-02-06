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

#ifndef LIB_QUENTIER_TYPES_USER_H
#define LIB_QUENTIER_TYPES_USER_H

#include <quentier/utility/Printable.h>
#include <quentier/types/ErrorString.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

#include <QtGlobal>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(UserData)

class QUENTIER_EXPORT User: public Printable
{
public:
    typedef qevercloud::PrivilegeLevel::type    PrivilegeLevel;
    typedef qevercloud::ServiceLevel::type      ServiceLevel;

public:
    User();
    User(const qevercloud::User & user);
    User(qevercloud::User && user);
    User(const User & other);
    User(User && other);
    User & operator=(const User & other);
    User & operator=(User && other);
    virtual ~User();

    bool operator==(const User & other) const;
    bool operator!=(const User & other) const;

    operator const qevercloud::User&() const;
    operator qevercloud::User&();

    void clear();

    bool isDirty() const;
    void setDirty(const bool dirty);

    bool isLocal() const;
    void setLocal(const bool local);

    bool checkParameters(ErrorString & errorDescription) const;

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

    bool hasServiceLevel() const;
    ServiceLevel serviceLevel() const;
    void setServiceLevel(const qint8 level);

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

    bool hasShardId() const;
    const QString & shardId() const;
    void setShardId(const QString & shardId);

    bool hasUserAttributes() const;
    const qevercloud::UserAttributes & userAttributes() const;
    void setUserAttributes(qevercloud::UserAttributes && attributes);

    bool hasAccounting() const;
    const qevercloud::Accounting & accounting() const;
    void setAccounting(qevercloud::Accounting && accounting);

    bool hasBusinessUserInfo() const;
    const qevercloud::BusinessUserInfo & businessUserInfo() const;
    void setBusinessUserInfo(qevercloud::BusinessUserInfo && info);

    bool hasPhotoUrl() const;
    QString photoUrl() const;
    void setPhotoUrl(const QString & photoUrl);

    bool hasPhotoLastUpdateTimestamp() const;
    qint64 photoLastUpdateTimestamp() const;
    void setPhotoLastUpdateTimestamp(const qint64 timestamp);

    bool hasAccountLimits() const;
    const qevercloud::AccountLimits & accountLimits() const;
    void setAccountLimits(qevercloud::AccountLimits && limits);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    friend class Notebook;

private:
    QSharedDataPointer<UserData> d;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_USER_H
