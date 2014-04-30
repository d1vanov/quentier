#include "IUser.h"
#include "../Utility.h"
#include "QEverCloudHelpers.h"
#include <QEverCloud.h>
#include <QRegExp>

namespace qute_note {

IUser::IUser() :
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

    const qevercloud::User & user = GetEnUser();
    const qevercloud::User & otherUser = other.GetEnUser();

    if (user.id != otherUser.id) {
        return false;
    }
    else if (user.username != otherUser.username) {
        return false;
    }
    else if (user.email != otherUser.email) {
        return false;
    }
    else if (user.name != otherUser.name) {
        return false;
    }
    else if (user.timezone != otherUser.timezone) {
        return false;
    }
    else if (user.privilege != otherUser.privilege) {
        return false;
    }
    else if (user.created != otherUser.created) {
        return false;
    }
    else if (user.updated != otherUser.updated) {
        return false;
    }
    else if (user.deleted != otherUser.deleted) {
        return false;
    }
    else if (user.active != otherUser.active) {
        return false;
    }
    else if (user.attributes.isSet() != otherUser.attributes.isSet()) {
        return false;
    }
    else if (user.accounting.isSet() != otherUser.accounting.isSet()) {
        return false;
    }
    else if (user.premiumInfo.isSet() != otherUser.premiumInfo.isSet()) {
        return false;
    }
    else if (user.businessUserInfo.isSet() != otherUser.businessUserInfo.isSet()) {
        return false;
    }

    if (user.attributes.isSet())
    {
        const qevercloud::UserAttributes & attributes = user.attributes;
        const qevercloud::UserAttributes & otherAttributes = otherUser.attributes;

        if (attributes != otherAttributes) {
            return false;
        }
    }

    if (user.accounting.isSet())
    {
        const qevercloud::Accounting & accounting = user.accounting;
        const qevercloud::Accounting & otherAccounting = otherUser.accounting;

        if (accounting != otherAccounting) {
            return false;
        }
    }

    if (user.premiumInfo.isSet())
    {
        const qevercloud::PremiumInfo & premiumInfo = user.premiumInfo;
        const qevercloud::PremiumInfo & otherPremiumInfo = otherUser.premiumInfo;

        if (premiumInfo != otherPremiumInfo) {
            return false;
        }
    }

    if (user.businessUserInfo.isSet())
    {
        const qevercloud::BusinessUserInfo & info = user.businessUserInfo;
        const qevercloud::BusinessUserInfo & otherInfo = otherUser.businessUserInfo;

        if (info != otherInfo) {
            return false;
        }
    }

    return true;
}

bool IUser::operator!=(const IUser & other) const
{
    return !(*this == other);
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

bool IUser::checkParameters(QString & errorDescription) const
{
    const auto & enUser = GetEnUser();

    if (!enUser.id.isSet()) {
        errorDescription = QObject::tr("User id is not set");
        return false;
    }

    if (!enUser.username.isSet()) {
        errorDescription = QObject::tr("User name is not set");
        return false;
    }

    const QString & username = enUser.username;
    size_t usernameSize = username.size();
    if ( (usernameSize > qevercloud::EDAM_USER_USERNAME_LEN_MAX) ||
         (usernameSize < qevercloud::EDAM_USER_USERNAME_LEN_MIN) )
    {
        errorDescription = QObject::tr("User name should have length from ");
        errorDescription += QString::number(qevercloud::EDAM_USER_USERNAME_LEN_MIN);
        errorDescription += QObject::tr(" to ");
        errorDescription += QString::number(qevercloud::EDAM_USER_USERNAME_LEN_MAX);

        return false;
    }

    QRegExp usernameRegExp(qevercloud::EDAM_USER_USERNAME_REGEX);
    if (usernameRegExp.indexIn(username) < 0) {
        errorDescription = QObject::tr("User name can contain only \"a-z\" or \"0-9\""
                                       "or \"-\" but should not start or end with \"-\"");
        return false;
    }

    // NOTE: ignore everything about email because "Third party applications that
    // authenticate using OAuth do not have access to this field"

    if (!enUser.name.isSet()) {
        errorDescription = QObject::tr("User's displayed name is not set");
        return false;
    }

    const QString & name = enUser.name;
    size_t nameSize = name.size();
    if ( (nameSize > qevercloud::EDAM_USER_NAME_LEN_MAX) ||
         (nameSize < qevercloud::EDAM_USER_NAME_LEN_MIN) )
    {
        errorDescription = QObject::tr("User displayed name must have length from ");
        errorDescription += QString::number(qevercloud::EDAM_USER_NAME_LEN_MIN);
        errorDescription += QObject::tr(" to ");
        errorDescription += QString::number(qevercloud::EDAM_USER_NAME_LEN_MAX);

        return false;
    }

    QRegExp nameRegExp(qevercloud::EDAM_USER_NAME_REGEX);
    if (nameRegExp.indexIn(name) < 0) {
        errorDescription = QObject::tr("User displayed name doesn't match its regular expression. "
                                       "Consider removing any special characters");
        return false;
    }

    if (enUser.timezone.isSet())
    {
        const QString & timezone = enUser.timezone;
        size_t timezoneSize = timezone.size();
        if ( (timezoneSize > qevercloud::EDAM_TIMEZONE_LEN_MAX) ||
             (timezoneSize < qevercloud::EDAM_TIMEZONE_LEN_MIN) )
        {
            errorDescription = QObject::tr("User timezone should have length from ");
            errorDescription += QString::number(qevercloud::EDAM_TIMEZONE_LEN_MIN);
            errorDescription += QObject::tr(" to ");
            errorDescription += QString::number(qevercloud::EDAM_TIMEZONE_LEN_MAX);

            return false;
        }

        QRegExp timezoneRegExp(qevercloud::EDAM_TIMEZONE_REGEX);
        if (timezoneRegExp.indexIn(timezone) < 0) {
            errorDescription = QObject::tr("User timezone doesn't match its regular expression. "
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
            size_t defaultLocationNameSize = defaultLocationName.size();
            if ( (defaultLocationNameSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                 (defaultLocationNameSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription = QObject::tr("User default location name must have length from ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                errorDescription += QObject::tr(" to ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);

                return false;
            }
        }

        if (attributes.viewedPromotions.isSet())
        {
            const QStringList & viewedPromotions = attributes.viewedPromotions;
            foreach(const QString & viewedPromotion, viewedPromotions)
            {
                size_t viewedPromotionSize = viewedPromotion.size();
                if ( (viewedPromotionSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                     (viewedPromotionSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
                {
                    errorDescription = QObject::tr("Each User's viewed promotion must have length from ");
                    errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                    errorDescription += QObject::tr(" to ");
                    errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);

                    return false;
                }
            }
        }

        if (attributes.incomingEmailAddress.isSet())
        {
            const QString & incomingEmailAddress = attributes.incomingEmailAddress;
            size_t incomingEmailAddressSize = incomingEmailAddress.size();
            if ( (incomingEmailAddressSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                 (incomingEmailAddressSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription = QObject::tr("User's incoming email address must have length from ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                errorDescription += QObject::tr(" to ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);

                return false;
            }
        }

        if (attributes.recentMailedAddresses.isSet())
        {
            const QStringList & recentMailedAddresses = attributes.recentMailedAddresses;
            size_t numRecentMailedAddresses = recentMailedAddresses.size();
            if (numRecentMailedAddresses > qevercloud::EDAM_USER_RECENT_MAILED_ADDRESSES_MAX)
            {
                errorDescription = QObject::tr("User must have no more recent mailed addresses than ");
                errorDescription += QString::number(qevercloud::EDAM_USER_RECENT_MAILED_ADDRESSES_MAX);

                return false;
            }

            foreach(const QString & recentMailedAddress, recentMailedAddresses)
            {
                size_t recentMailedAddressSize = recentMailedAddress.size();
                if ( (recentMailedAddressSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                     (recentMailedAddressSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
                {
                    errorDescription = QObject::tr("Each user's recent emailed address must have length from ");
                    errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                    errorDescription += QObject::tr(" to ");
                    errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);

                    return false;
                }
            }
        }

        if (attributes.comments.isSet())
        {
            const QString & comments = attributes.comments;
            size_t commentsSize = comments.size();
            if ( (commentsSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ||
                 (commentsSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription = QObject::tr("User's comments must have length from ");
                errorDescription += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);
                errorDescription += QObject::tr(" to ");
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
    GetEnUser().username = username;
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
    GetEnUser().email = email;
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
    GetEnUser().name = name;
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

const IUser::PrivilegeLevel IUser::privilegeLevel() const
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

bool IUser::hasPremiumInfo() const
{
    return GetEnUser().premiumInfo.isSet();
}

const qevercloud::PremiumInfo & IUser::premiumInfo() const
{
    return GetEnUser().premiumInfo;
}

void IUser::setPremiumInfo(qevercloud::PremiumInfo && premiumInfo)
{
    GetEnUser().premiumInfo = std::move(premiumInfo);
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

IUser::IUser(const IUser & other) :
    m_isDirty(other.m_isDirty),
    m_isLocal(other.m_isLocal)
{}

IUser::IUser(IUser && other) :
    m_isDirty(std::move(other.m_isDirty)),
    m_isLocal(std::move(other.m_isLocal))
{}

QTextStream & IUser::Print(QTextStream & strm) const
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
        // TODO: (re)implement printing of PrivilegeLevel
        strm << "privilege = " << QString::number(enUser.privilege) << "; \n";
    }
    else {
        strm << "privilege is not set" << "; \n";
    }

    if (enUser.created.isSet()) {
        strm << "created = " << PrintableDateTimeFromTimestamp(enUser.created) << "; \n";
    }
    else {
        strm << "created is not set" << "; \n";
    }

    if (enUser.updated.isSet()) {
        strm << "updated = " << PrintableDateTimeFromTimestamp(enUser.updated) << "; \n";
    }
    else {
        strm << "updated is not set" << "; \n";
    }

    if (enUser.deleted.isSet()) {
        strm << "deleted = " << PrintableDateTimeFromTimestamp(enUser.deleted) << "; \n";
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
        // TODO: (re)implement printing of UserAttributes
        // strm << enUser.attributes;
    }
    else {
        strm << "attributes are not set" << "; \n";
    }

    if (enUser.accounting.isSet()) {
        // TODO: (re)implement printing of Accounting
        // strm << enUser.accounting;
    }
    else {
        strm << "accounting is not set" << "; \n";
    }

    if (enUser.premiumInfo.isSet()) {
        // TODO: (re)implement printing of PremiumInfo
        // strm << enUser.premiumInfo;
    }
    else {
        strm << "premium info is not set" << "; \n";
    }

    if (enUser.businessUserInfo.isSet()) {
        // TODO: (re)implement printing of BusinessUserInfo
        // strm << enUser.businessUserInfo;
    }
    else {
        strm << "business user info is not set" << "; \n";
    }

    return strm;
}

} // namespace qute_note
