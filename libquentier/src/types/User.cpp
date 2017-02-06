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

#include "data/UserData.h"
#include <quentier/types/User.h>
#include <quentier/utility/Utility.h>
#include <QRegExp>

namespace quentier {

User::User() :
    Printable(),
    d(new UserData)
{}

User::User(const qevercloud::User & user) :
    Printable(),
    d(new UserData(user))
{}

User::User(qevercloud::User && user) :
    Printable(),
    d(new UserData(std::move(user)))
{}

User::User(const User & other) :
    Printable(),
    d(other.d)
{}

User::User(User && other) :
    Printable(),
    d(std::move(other.d))
{}

User & User::operator=(const User & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

User & User::operator=(User && other)
{
    if (this != &other) {
        d = std::move(other.d);
    }

    return *this;
}

User::~User()
{}

bool User::operator==(const User & other) const
{
    if (d->m_isDirty != other.d->m_isDirty) {
        return false;
    }
    else if (d->m_isLocal != other.d->m_isLocal) {
        return false;
    }
    else if (d->m_qecUser != other.d->m_qecUser) {
        return false;
    }

    return true;
}

bool User::operator!=(const User & other) const
{
    return !(*this == other);
}

User::operator const qevercloud::User &() const
{
    return d->m_qecUser;
}

User::operator qevercloud::User &()
{
    return d->m_qecUser;
}

void User::clear()
{
    setDirty(true);
    setLocal(true);
    d->m_qecUser = qevercloud::User();
}

bool User::isDirty() const
{
    return d->m_isDirty;
}

void User::setDirty(const bool dirty)
{
    d->m_isDirty = dirty;
}

bool User::isLocal() const
{
    return d->m_isLocal;
}

void User::setLocal(const bool local)
{
    d->m_isLocal = local;
}

bool User::checkParameters(ErrorString & errorDescription) const
{
    const auto & enUser = d->m_qecUser;

    if (!enUser.id.isSet()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "User id is not set");
        return false;
    }

    if (enUser.username.isSet())
    {
        const QString & username = enUser.username.ref();
        int usernameSize = username.size();

        if ( (usernameSize > qevercloud::EDAM_USER_USERNAME_LEN_MAX) ||
             (usernameSize < qevercloud::EDAM_USER_USERNAME_LEN_MIN) )
        {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "User's name has invalid size");
            errorDescription.details() = username;
            return false;
        }

        QRegExp usernameRegExp(qevercloud::EDAM_USER_USERNAME_REGEX);
        if (usernameRegExp.indexIn(username) < 0) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "User's name can contain only \"a-z\" or \"0-9\""
                                                        "or \"-\" but should not start or end with \"-\"");
            return false;
        }
    }

    // NOTE: ignore everything about email because "Third party applications that
    // authenticate using OAuth do not have access to this field"

    if (enUser.name.isSet())
    {
        const QString & name = enUser.name;
        int nameSize = name.size();

        if ( (nameSize > qevercloud::EDAM_USER_NAME_LEN_MAX) ||
             (nameSize < qevercloud::EDAM_USER_NAME_LEN_MIN) )
        {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "User's displayed name has invalid size");
            errorDescription.details() = name;
            return false;
        }

        QRegExp nameRegExp(qevercloud::EDAM_USER_NAME_REGEX);
        if (nameRegExp.indexIn(name) < 0) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "User's displayed name doesn't match its regular expression. "
                                                        "Consider removing any special characters");
            return false;
        }
    }

    if (enUser.timezone.isSet())
    {
        const QString & timezone = enUser.timezone;
        int timezoneSize = timezone.size();

        if ( (timezoneSize > qevercloud::EDAM_TIMEZONE_LEN_MAX) ||
             (timezoneSize < qevercloud::EDAM_TIMEZONE_LEN_MIN) )
        {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "User's timezone has invalid size");
            errorDescription.details() = timezone;
            return false;
        }

        QRegExp timezoneRegExp(qevercloud::EDAM_TIMEZONE_REGEX);
        if (timezoneRegExp.indexIn(timezone) < 0) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "User's timezone doesn't match its regular expression. "
                                                        "It must be encoded as a standard zone ID such as \"America/Los_Angeles\" "
                                                        "or \"GMT+08:00\".");
            return false;
        }
    }

    if (enUser.attributes.isSet())
    {
        const qevercloud::UserAttributes & attributes = enUser.attributes;

        if (attributes.defaultLocationName.isSet())
        {
            const QString & defaultLocationName = attributes.defaultLocationName;
            int defaultLocationNameSize = defaultLocationName.size();

            if ( (defaultLocationNameSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                 (defaultLocationNameSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "User's default location name has invalid size");
                errorDescription.details() = defaultLocationName;
                return false;
            }
        }

        if (attributes.viewedPromotions.isSet())
        {
            const QStringList & viewedPromotions = attributes.viewedPromotions;
            foreach(const QString & viewedPromotion, viewedPromotions)
            {
                int viewedPromotionSize = viewedPromotion.size();
                if ( (viewedPromotionSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                     (viewedPromotionSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
                {
                    errorDescription.base() = QT_TRANSLATE_NOOP("", "User's viewed promotion has invalid size");
                    errorDescription.details() = viewedPromotion;
                    return false;
                }
            }
        }

        if (attributes.incomingEmailAddress.isSet())
        {
            const QString & incomingEmailAddress = attributes.incomingEmailAddress;
            int incomingEmailAddressSize = incomingEmailAddress.size();

            if ( (incomingEmailAddressSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                 (incomingEmailAddressSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "User's incoming email address has invalid size");
                errorDescription.details() = incomingEmailAddress;
                return false;
            }
        }

        if (attributes.recentMailedAddresses.isSet())
        {
            const QStringList & recentMailedAddresses = attributes.recentMailedAddresses;
            int numRecentMailedAddresses = recentMailedAddresses.size();

            if (numRecentMailedAddresses > qevercloud::EDAM_USER_RECENT_MAILED_ADDRESSES_MAX)
            {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "User recent mailed addresses size is invalid");
                errorDescription.details() = QString::number(numRecentMailedAddresses);
                return false;
            }

            foreach(const QString & recentMailedAddress, recentMailedAddresses)
            {
                int recentMailedAddressSize = recentMailedAddress.size();
                if ( (recentMailedAddressSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                     (recentMailedAddressSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
                {
                    errorDescription.base() = QT_TRANSLATE_NOOP("", "User's recent emailed address has invalid size");
                    errorDescription.details() = recentMailedAddress;
                    return false;
                }
            }
        }

        if (attributes.comments.isSet())
        {
            const QString & comments = attributes.comments;
            int commentsSize = comments.size();

            if ( (commentsSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                 (commentsSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "User's comments have invalid size");
                errorDescription.details() = QString::number(commentsSize);
                return false;
            }
        }
    }

    return true;
}

bool User::hasId() const
{
    return d->m_qecUser.id.isSet();
}

qint32 User::id() const
{
    return d->m_qecUser.id;
}

void User::setId(const qint32 id)
{
    d->m_qecUser.id = id;
}

bool User::hasUsername() const
{
    return d->m_qecUser.username.isSet();
}

const QString & User::username() const
{
    return d->m_qecUser.username;
}

void User::setUsername(const QString & username)
{
    if (!username.isEmpty()) {
        d->m_qecUser.username = username;
    }
    else {
        d->m_qecUser.username.clear();
    }
}

bool User::hasEmail() const
{
    return d->m_qecUser.email.isSet();
}

const QString & User::email() const
{
    return d->m_qecUser.email;
}

void User::setEmail(const QString & email)
{
    if (!email.isEmpty()) {
        d->m_qecUser.email = email;
    }
    else {
        d->m_qecUser.email.clear();
    }
}

bool User::hasName() const
{
    return d->m_qecUser.name.isSet();
}

const QString & User::name() const
{
    return d->m_qecUser.name;
}

void User::setName(const QString & name)
{
    if (!name.isEmpty()) {
        d->m_qecUser.name = name;
    }
    else {
        d->m_qecUser.name.clear();
    }
}

bool User::hasTimezone() const
{
    return d->m_qecUser.timezone.isSet();
}

const QString & User::timezone() const
{
    return d->m_qecUser.timezone;
}

void User::setTimezone(const QString & timezone)
{
    d->m_qecUser.timezone = timezone;
}

bool User::hasPrivilegeLevel() const
{
    return d->m_qecUser.privilege.isSet();
}

User::PrivilegeLevel User::privilegeLevel() const
{
    return d->m_qecUser.privilege;
}

void User::setPrivilegeLevel(const qint8 level)
{
    qevercloud::User & enUser = d->m_qecUser;

    if (level <= static_cast<qint8>(qevercloud::PrivilegeLevel::ADMIN)) {
        enUser.privilege = static_cast<PrivilegeLevel>(level);
    }
    else {
        enUser.privilege.clear();
    }
}

bool User::hasServiceLevel() const
{
    return d->m_qecUser.serviceLevel.isSet();
}

User::ServiceLevel User::serviceLevel() const
{
    return d->m_qecUser.serviceLevel;
}

void User::setServiceLevel(const qint8 level)
{
    qevercloud::User & enUser = d->m_qecUser;

    if (level <= static_cast<qint8>(qevercloud::ServiceLevel::PREMIUM)) {
        enUser.serviceLevel = static_cast<ServiceLevel>(level);
    }
    else {
        enUser.serviceLevel.clear();
    }
}

bool User::hasCreationTimestamp() const
{
    return d->m_qecUser.created.isSet();
}

qint64 User::creationTimestamp() const
{
    return d->m_qecUser.created;
}

void User::setCreationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecUser.created = timestamp;
    }
    else {
        d->m_qecUser.created.clear();
    }
}

bool User::hasModificationTimestamp() const
{
    return d->m_qecUser.updated.isSet();
}

qint64 User::modificationTimestamp() const
{
    return d->m_qecUser.updated;
}

void User::setModificationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecUser.updated = timestamp;
    }
    else {
        d->m_qecUser.updated.clear();
    }
}

bool User::hasDeletionTimestamp() const
{
    return d->m_qecUser.deleted.isSet();
}

qint64 User::deletionTimestamp() const
{
    return d->m_qecUser.deleted;
}

void User::setDeletionTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecUser.deleted = timestamp;
    }
    else {
        d->m_qecUser.deleted.clear();
    }
}

bool User::hasActive() const
{
    return d->m_qecUser.active.isSet();
}

bool User::active() const
{
    return d->m_qecUser.active;
}

void User::setActive(const bool active)
{
    d->m_qecUser.active = active;
}

bool User::hasShardId() const
{
    return d->m_qecUser.shardId.isSet();
}

const QString & User::shardId() const
{
    return d->m_qecUser.shardId.ref();
}

void User::setShardId(const QString & shardId)
{
    if (!shardId.isEmpty()) {
        d->m_qecUser.shardId = shardId;
    }
    else {
        d->m_qecUser.shardId.clear();
    }
}

bool User::hasUserAttributes() const
{
    return d->m_qecUser.attributes.isSet();
}

const qevercloud::UserAttributes & User::userAttributes() const
{
    return d->m_qecUser.attributes;
}

void User::setUserAttributes(qevercloud::UserAttributes && attributes)
{
    d->m_qecUser.attributes = std::move(attributes);
}

bool User::hasAccounting() const
{
    return d->m_qecUser.accounting.isSet();
}

const qevercloud::Accounting & User::accounting() const
{
    return d->m_qecUser.accounting;
}

void User::setAccounting(qevercloud::Accounting && accounting)
{
    d->m_qecUser.accounting = std::move(accounting);
}

bool User::hasBusinessUserInfo() const
{
    return d->m_qecUser.businessUserInfo.isSet();
}

const qevercloud::BusinessUserInfo & User::businessUserInfo() const
{
    return d->m_qecUser.businessUserInfo;
}

void User::setBusinessUserInfo(qevercloud::BusinessUserInfo && info)
{
    d->m_qecUser.businessUserInfo = std::move(info);
}

bool User::hasPhotoUrl() const
{
    return d->m_qecUser.photoUrl.isSet();
}

QString User::photoUrl() const
{
    return d->m_qecUser.photoUrl;
}

void User::setPhotoUrl(const QString & photoUrl)
{
    if (photoUrl.isEmpty()) {
        d->m_qecUser.photoUrl.clear();
    }
    else {
        d->m_qecUser.photoUrl = photoUrl;
    }
}

bool User::hasPhotoLastUpdateTimestamp() const
{
    return d->m_qecUser.photoLastUpdated.isSet();
}

qint64 User::photoLastUpdateTimestamp() const
{
    return d->m_qecUser.photoLastUpdated;
}

void User::setPhotoLastUpdateTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecUser.photoLastUpdated = timestamp;
    }
    else {
        d->m_qecUser.photoLastUpdated.clear();
    }
}

bool User::hasAccountLimits() const
{
    return d->m_qecUser.accountLimits.isSet();
}

const qevercloud::AccountLimits & User::accountLimits() const
{
    return d->m_qecUser.accountLimits;
}

void User::setAccountLimits(qevercloud::AccountLimits && limits)
{
    d->m_qecUser.accountLimits = std::move(limits);
}

QTextStream & User::print(QTextStream & strm) const
{
    strm << QStringLiteral("User { \n");
    strm << QStringLiteral("isDirty = ") << (d->m_isDirty ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral("; \n");
    strm << QStringLiteral("isLocal = ") << (d->m_isLocal ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral("; \n");

    const auto & enUser = d->m_qecUser;

    if (enUser.id.isSet()) {
        strm << QStringLiteral("User ID = ") << QString::number(enUser.id) << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("User ID is not set; \n");
    }

    if (enUser.username.isSet()) {
        strm << QStringLiteral("username = ") << enUser.username << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("username is not set; \n");
    }

    if (enUser.email.isSet()) {
        strm << QStringLiteral("email = ") << enUser.email << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("email is not set; \n");
    }

    if (enUser.name.isSet()) {
        strm << QStringLiteral("name = ") << enUser.name << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("name is not set; \n");
    }

    if (enUser.timezone.isSet()) {
        strm << QStringLiteral("timezone = ") << enUser.timezone << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("timezone is not set; \n");
    }

    if (enUser.privilege.isSet()) {
        strm << QStringLiteral("privilege = ") << enUser.privilege << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("privilege is not set; \n");
    }

    if (enUser.serviceLevel.isSet()) {
        strm << QStringLiteral("service level = ") << enUser.serviceLevel << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("service level is not set; \n");
    }

    if (enUser.created.isSet()) {
        strm << QStringLiteral("created = ") << printableDateTimeFromTimestamp(enUser.created) << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("created is not set; \n");
    }

    if (enUser.updated.isSet()) {
        strm << QStringLiteral("updated = ") << printableDateTimeFromTimestamp(enUser.updated) << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("updated is not set; \n");
    }

    if (enUser.deleted.isSet()) {
        strm << QStringLiteral("deleted = ") << printableDateTimeFromTimestamp(enUser.deleted) << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("deleted is not set; \n");
    }

    if (enUser.active.isSet()) {
        strm << QStringLiteral("active = ") << (enUser.active ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("active is not set; \n");
    }

    if (enUser.attributes.isSet()) {
        strm << enUser.attributes;
    }
    else {
        strm << QStringLiteral("attributes are not set; \n");
    }

    if (enUser.accounting.isSet()) {
        strm << enUser.accounting;
    }
    else {
        strm << QStringLiteral("accounting is not set; \n");
    }

    if (enUser.businessUserInfo.isSet()) {
        strm << enUser.businessUserInfo;
    }
    else {
        strm << QStringLiteral("business user info is not set; \n");
    }

    if (enUser.photoUrl.isSet()) {
        strm << QStringLiteral("photo url = ") << enUser.photoUrl << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("photo url is not set; \n");
    }

    if (enUser.photoLastUpdated.isSet()) {
        strm << QStringLiteral("photo url last updated = ") << printableDateTimeFromTimestamp(enUser.photoLastUpdated)
             << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("photo url last updates is not set; \n");
    }

    if (enUser.accountLimits.isSet()) {
        strm << enUser.accountLimits;
    }
    else {
        strm << QStringLiteral("account limits are not set; \n");
    }

    return strm;
}

} // namespace quentier
