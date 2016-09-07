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

#include <quentier/types/IUser.h>
#include <quentier/utility/Utility.h>
#include <QRegExp>

namespace quentier {

IUser::IUser() :
    Printable(),
    m_isDirty(true),
    m_isLocal(true)
{}

IUser::~IUser()
{}

bool IUser::operator==(const IUser & other) const
{
    if (m_isDirty != other.m_isDirty) {
        return false;
    }
    else if (m_isLocal != other.m_isLocal) {
        return false;
    }
    else if (GetEnUser() != other.GetEnUser()) {
        return false;
    }

    return true;
}

bool IUser::operator!=(const IUser & other) const
{
    return !(*this == other);
}

IUser::operator const qevercloud::User &() const
{
    return GetEnUser();
}

IUser::operator qevercloud::User &()
{
    return GetEnUser();
}

void IUser::clear()
{
    setDirty(true);
    setLocal(true);
    GetEnUser() = qevercloud::User();
}

bool IUser::isDirty() const
{
    return m_isDirty;
}

void IUser::setDirty(const bool dirty)
{
    m_isDirty = dirty;
}

bool IUser::isLocal() const
{
    return m_isLocal;
}

void IUser::setLocal(const bool local)
{
    m_isLocal = local;
}

bool IUser::checkParameters(QNLocalizedString & errorDescription) const
{
    const auto & enUser = GetEnUser();

    if (!enUser.id.isSet()) {
        errorDescription = QT_TR_NOOP("user id is not set");
        return false;
    }

    if (enUser.username.isSet())
    {
        const QString & username = enUser.username.ref();
        int usernameSize = username.size();

        if ( (usernameSize > qevercloud::EDAM_USER_USERNAME_LEN_MAX) ||
             (usernameSize < qevercloud::EDAM_USER_USERNAME_LEN_MIN) )
        {
            errorDescription = QT_TR_NOOP("user name should have length from ");
            errorDescription += QString::number(qevercloud::EDAM_USER_USERNAME_LEN_MIN);
            // TRANSLATOR "Previous part of the phrase was "User should have length from ..."
            errorDescription += QT_TR_NOOP(" to ");
            errorDescription += QString::number(qevercloud::EDAM_USER_USERNAME_LEN_MAX);

            return false;
        }

        QRegExp usernameRegExp(qevercloud::EDAM_USER_USERNAME_REGEX);
        if (usernameRegExp.indexIn(username) < 0) {
            errorDescription = QT_TR_NOOP("user name can contain only \"a-z\" or \"0-9\""
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
            errorDescription = QT_TR_NOOP("user displayed name must have length from ");
            errorDescription += QString::number(qevercloud::EDAM_USER_NAME_LEN_MIN);
            // TRANSLATOR "Previous part of the phrase was "User displayed name must have length from ..."
            errorDescription += QT_TR_NOOP(" to ");
            errorDescription += QString::number(qevercloud::EDAM_USER_NAME_LEN_MAX);
            return false;
        }

        QRegExp nameRegExp(qevercloud::EDAM_USER_NAME_REGEX);
        if (nameRegExp.indexIn(name) < 0) {
            errorDescription = QT_TR_NOOP("user displayed name doesn't match its regular expression. "
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
            errorDescription = QT_TR_NOOP("user timezone should have length from ");
            errorDescription += QString::number(qevercloud::EDAM_TIMEZONE_LEN_MIN);
            // TRANSlATOR Previous part of the phrase was "User timezone should have length from ..."
            errorDescription += QT_TR_NOOP(" to ");
            errorDescription += QString::number(qevercloud::EDAM_TIMEZONE_LEN_MAX);
            return false;
        }

        QRegExp timezoneRegExp(qevercloud::EDAM_TIMEZONE_REGEX);
        if (timezoneRegExp.indexIn(timezone) < 0) {
            errorDescription = QT_TR_NOOP("user timezone doesn't match its regular expression. "
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
                errorDescription = QT_TR_NOOP("user default location name must have length from ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                // TRANSLATOR previous part of the phrase was "User default location name must have length from ..."
                errorDescription += QT_TR_NOOP(" to ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);
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
                    errorDescription = QT_TR_NOOP("each User's viewed promotion must have length from ");
                    errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                    // TRANSLATOR "Each User's viewed promotion must have length from "
                    errorDescription += QT_TR_NOOP(" to ");
                    errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);
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
                errorDescription = QT_TR_NOOP("user's incoming email address must have length from ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                // TRANSLATOR Previous part of the phrase was "User's incoming email address must have length from ..."
                errorDescription += QT_TR_NOOP(" to ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);
                return false;
            }
        }

        if (attributes.recentMailedAddresses.isSet())
        {
            const QStringList & recentMailedAddresses = attributes.recentMailedAddresses;
            int numRecentMailedAddresses = recentMailedAddresses.size();

            if (numRecentMailedAddresses > qevercloud::EDAM_USER_RECENT_MAILED_ADDRESSES_MAX)
            {
                errorDescription = QT_TR_NOOP("user must have no more recent mailed addresses than ");
                errorDescription += QString::number(qevercloud::EDAM_USER_RECENT_MAILED_ADDRESSES_MAX);
                return false;
            }

            foreach(const QString & recentMailedAddress, recentMailedAddresses)
            {
                int recentMailedAddressSize = recentMailedAddress.size();
                if ( (recentMailedAddressSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                     (recentMailedAddressSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
                {
                    errorDescription = QT_TR_NOOP("each user's recent emailed address must have length from ");
                    errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                    // TRANSLATOR Previous part of the phrase was "Each user's recent emailed address must have length from ..."
                    errorDescription += QT_TR_NOOP(" to ");
                    errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);
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
                errorDescription = QT_TR_NOOP("user's comments must have length from ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                // TRANSLATOR Previous part of the phrase was "User's comments must have length from ..."
                errorDescription += QT_TR_NOOP(" to ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);
                return false;
            }
        }
    }

    return true;
}

bool IUser::hasId() const
{
    return GetEnUser().id.isSet();
}

qint32 IUser::id() const
{
    return GetEnUser().id;
}

void IUser::setId(const qint32 id)
{
    GetEnUser().id = id;
}

bool IUser::hasUsername() const
{
    return GetEnUser().username.isSet();
}

const QString & IUser::username() const
{
    return GetEnUser().username;
}

void IUser::setUsername(const QString & username)
{
    if (!username.isEmpty()) {
        GetEnUser().username = username;
    }
    else {
        GetEnUser().username.clear();
    }
}

bool IUser::hasEmail() const
{
    return GetEnUser().email.isSet();
}

const QString & IUser::email() const
{
    return GetEnUser().email;
}

void IUser::setEmail(const QString & email)
{
    if (!email.isEmpty()) {
        GetEnUser().email = email;
    }
    else {
        GetEnUser().email.clear();
    }
}

bool IUser::hasName() const
{
    return GetEnUser().name.isSet();
}

const QString & IUser::name() const
{
    return GetEnUser().name;
}

void IUser::setName(const QString & name)
{
    if (!name.isEmpty()) {
        GetEnUser().name = name;
    }
    else {
        GetEnUser().name.clear();
    }
}

bool IUser::hasTimezone() const
{
    return GetEnUser().timezone.isSet();
}

const QString & IUser::timezone() const
{
    return GetEnUser().timezone;
}

void IUser::setTimezone(const QString & timezone)
{
    GetEnUser().timezone = timezone;
}

bool IUser::hasPrivilegeLevel() const
{
    return GetEnUser().privilege.isSet();
}

IUser::PrivilegeLevel IUser::privilegeLevel() const
{
    return GetEnUser().privilege;
}

void IUser::setPrivilegeLevel(const qint8 level)
{
    qevercloud::User & enUser = GetEnUser();
    if (level <= static_cast<qint8>(qevercloud::PrivilegeLevel::ADMIN)) {
        enUser.privilege = static_cast<PrivilegeLevel>(level);
    }
    else {
        enUser.privilege.clear();
    }
}

bool IUser::hasServiceLevel() const
{
    return GetEnUser().serviceLevel.isSet();
}

IUser::ServiceLevel IUser::serviceLevel() const
{
    return GetEnUser().serviceLevel;
}

void IUser::setServiceLevel(const qint8 level)
{
    qevercloud::User & enUser = GetEnUser();
    if (level <= static_cast<qint8>(qevercloud::ServiceLevel::PREMIUM)) {
        enUser.serviceLevel = static_cast<ServiceLevel>(level);
    }
    else {
        enUser.serviceLevel.clear();
    }
}

bool IUser::hasCreationTimestamp() const
{
    return GetEnUser().created.isSet();
}

qint64 IUser::creationTimestamp() const
{
    return GetEnUser().created;
}

void IUser::setCreationTimestamp(const qint64 timestamp)
{
    // TODO: verify whether it really matters
    if (timestamp >= 0) {
        GetEnUser().created = timestamp;
    }
    else {
        GetEnUser().created.clear();
    }
}

bool IUser::hasModificationTimestamp() const
{
    return GetEnUser().updated.isSet();
}

qint64 IUser::modificationTimestamp() const
{
    return GetEnUser().updated;
}

void IUser::setModificationTimestamp(const qint64 timestamp)
{
    // TODO: verify whether it really matters
    if (timestamp >= 0) {
        GetEnUser().updated = timestamp;
    }
    else {
        GetEnUser().updated.clear();
    }
}

bool IUser::hasDeletionTimestamp() const
{
    return GetEnUser().deleted.isSet();
}

qint64 IUser::deletionTimestamp() const
{
    return GetEnUser().deleted;
}

void IUser::setDeletionTimestamp(const qint64 timestamp)
{
    // TODO: verify whether it really matters
    if (timestamp >= 0) {
        GetEnUser().deleted = timestamp;
    }
    else {
        GetEnUser().deleted.clear();
    }
}

bool IUser::hasActive() const
{
    return GetEnUser().active.isSet();
}

bool IUser::active() const
{
    return GetEnUser().active;
}

void IUser::setActive(const bool active)
{
    GetEnUser().active = active;
}

bool IUser::hasShardId() const
{
    return GetEnUser().shardId.isSet();
}

const QString & IUser::shardId() const
{
    return GetEnUser().shardId.ref();
}

void IUser::setShardId(const QString & shardId)
{
    if (!shardId.isEmpty()) {
        GetEnUser().shardId = shardId;
    }
    else {
        GetEnUser().shardId.clear();
    }
}

bool IUser::hasUserAttributes() const
{
    return GetEnUser().attributes.isSet();
}

const qevercloud::UserAttributes & IUser::userAttributes() const
{
    return GetEnUser().attributes;
}

void IUser::setUserAttributes(qevercloud::UserAttributes && attributes)
{
    GetEnUser().attributes = std::move(attributes);
}

bool IUser::hasAccounting() const
{
    return GetEnUser().accounting.isSet();
}

const qevercloud::Accounting & IUser::accounting() const
{
    return GetEnUser().accounting;
}

void IUser::setAccounting(qevercloud::Accounting && accounting)
{
    GetEnUser().accounting = std::move(accounting);
}

bool IUser::hasBusinessUserInfo() const
{
    return GetEnUser().businessUserInfo.isSet();
}

const qevercloud::BusinessUserInfo & IUser::businessUserInfo() const
{
    return GetEnUser().businessUserInfo;
}

void IUser::setBusinessUserInfo(qevercloud::BusinessUserInfo && info)
{
    GetEnUser().businessUserInfo = std::move(info);
}

bool IUser::hasPhotoUrl() const
{
    return GetEnUser().photoUrl.isSet();
}

QString IUser::photoUrl() const
{
    return GetEnUser().photoUrl;
}

void IUser::setPhotoUrl(const QString & photoUrl)
{
    if (photoUrl.isEmpty()) {
        GetEnUser().photoUrl.clear();
    }
    else {
        GetEnUser().photoUrl = photoUrl;
    }
}

bool IUser::hasPhotoLastUpdateTimestamp() const
{
    return GetEnUser().photoLastUpdated.isSet();
}

qint64 IUser::photoLastUpdateTimestamp() const
{
    return GetEnUser().photoLastUpdated;
}

void IUser::setPhotoLastUpdateTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        GetEnUser().photoLastUpdated = timestamp;
    }
    else {
        GetEnUser().photoLastUpdated.clear();
    }
}

bool IUser::hasAccountLimits() const
{
    return GetEnUser().accountLimits.isSet();
}

const qevercloud::AccountLimits & IUser::accountLimits() const
{
    return GetEnUser().accountLimits;
}

void IUser::setAccountLimits(qevercloud::AccountLimits && limits)
{
    GetEnUser().accountLimits = std::move(limits);
}

IUser::IUser(const IUser & other) :
    Printable(other),
    m_isDirty(other.m_isDirty),
    m_isLocal(other.m_isLocal)
{}

IUser::IUser(IUser && other) :
    Printable(std::move(other)),
    m_isDirty(std::move(other.m_isDirty)),
    m_isLocal(std::move(other.m_isLocal))
{}

IUser & IUser::operator=(const IUser & other)
{
    if (this != &other) {
        m_isDirty = other.m_isDirty;
        m_isLocal = other.m_isLocal;
    }

    return *this;
}

IUser & IUser::operator=(IUser && other)
{
    if (this != &other) {
        m_isDirty = std::move(other.m_isDirty);
        m_isLocal = std::move(other.m_isLocal);
    }

    return *this;
}

QTextStream & IUser::print(QTextStream & strm) const
{
    strm << "IUser { \n";
    strm << "isDirty = " << (m_isDirty ? "true" : "false") << "; \n";
    strm << "isLocal = " << (m_isLocal ? "true" : "false") << "; \n";

    const auto & enUser = GetEnUser();

    if (enUser.id.isSet()) {
        strm << "User ID = " << QString::number(enUser.id) << "; \n";
    }
    else {
        strm << "User ID is not set" << "; \n";
    }

    if (enUser.username.isSet()) {
        strm << "username = " << enUser.username << "; \n";
    }
    else {
        strm << "username is not set" << "; \n";
    }

    if (enUser.email.isSet()) {
        strm << "email = " << enUser.email << "; \n";
    }
    else {
        strm << "email is not set" << "; \n";
    }

    if (enUser.name.isSet()) {
        strm << "name = " << enUser.name << "; \n";
    }
    else {
        strm << "name is not set" << "; \n";
    }

    if (enUser.timezone.isSet()) {
        strm << "timezone = " << enUser.timezone << "; \n";
    }
    else {
        strm << "timezone is not set" << "; \n";
    }

    if (enUser.privilege.isSet()) {
        strm << "privilege = " << enUser.privilege << "; \n";
    }
    else {
        strm << "privilege is not set" << "; \n";
    }

    if (enUser.created.isSet()) {
        strm << "created = " << printableDateTimeFromTimestamp(enUser.created) << "; \n";
    }
    else {
        strm << "created is not set" << "; \n";
    }

    if (enUser.updated.isSet()) {
        strm << "updated = " << printableDateTimeFromTimestamp(enUser.updated) << "; \n";
    }
    else {
        strm << "updated is not set" << "; \n";
    }

    if (enUser.deleted.isSet()) {
        strm << "deleted = " << printableDateTimeFromTimestamp(enUser.deleted) << "; \n";
    }
    else {
        strm << "deleted is not set" << "; \n";
    }

    if (enUser.active.isSet()) {
        strm << "active = " << (enUser.active ? "true" : "false") << "; \n";
    }
    else {
        strm << "active is not set" << "; \n";
    }

    if (enUser.attributes.isSet()) {
        strm << enUser.attributes;
    }
    else {
        strm << "attributes are not set" << "; \n";
    }

    if (enUser.accounting.isSet()) {
        strm << enUser.accounting;
    }
    else {
        strm << "accounting is not set" << "; \n";
    }

    if (enUser.premiumInfo.isSet()) {
        strm << enUser.premiumInfo;
    }
    else {
        strm << "premium info is not set" << "; \n";
    }

    if (enUser.businessUserInfo.isSet()) {
        strm << enUser.businessUserInfo;
    }
    else {
        strm << "business user info is not set" << "; \n";
    }

    return strm;
}

} // namespace quentier
