#include "IUser.h"
#include "../Utility.h"
#include <Types_types.h>
#include <Limits_constants.h>
#include <QObject>
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
    return ( (m_isDirty == other.m_isDirty) && (m_isLocal == other.m_isLocal) &&
             (GetEnUser() == other.GetEnUser()) );
}

bool IUser::operator!=(const IUser & other) const
{
    return !(*this == other);
}

void IUser::clear()
{
    setDirty(true);
    setLocal(true);
    GetEnUser() = evernote::edam::User();
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
    const auto & isSet = enUser.__isset;

    if (!isSet.id) {
        errorDescription = QObject::tr("User id is not set");
        return false;
    }

    if (!isSet.username) {
        errorDescription = QObject::tr("User name is not set");
        return false;
    }

    QString username = QString::fromStdString(enUser.username);
    size_t usernameSize = username.size();
    if ( (usernameSize > evernote::limits::g_Limits_constants.EDAM_USER_USERNAME_LEN_MAX) ||
         (usernameSize < evernote::limits::g_Limits_constants.EDAM_USER_USERNAME_LEN_MIN) )
    {
        errorDescription = QObject::tr("User name should have length from ");
        errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_USER_USERNAME_LEN_MIN);
        errorDescription += QObject::tr(" to ");
        errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_USER_USERNAME_LEN_MAX);

        return false;
    }

    QRegExp usernameRegExp(QString::fromStdString(evernote::limits::g_Limits_constants.EDAM_USER_USERNAME_REGEX));
    if (usernameRegExp.indexIn(username) < 0) {
        errorDescription = QObject::tr("User name can contain only \"a-z\" or \"0-9\""
                                       "or \"-\" but should not start or end with \"-\"");
        return false;
    }

    // NOTE: ignore everything about email because "Third party applications that
    // authenticate using OAuth do not have access to this field"

    if (!isSet.name) {
        errorDescription = QObject::tr("User's displayed name is not set");
        return false;
    }

    QString name = QString::fromStdString(enUser.name);
    size_t nameSize = name.size();
    if ( (nameSize > evernote::limits::g_Limits_constants.EDAM_USER_NAME_LEN_MAX) ||
         (nameSize < evernote::limits::g_Limits_constants.EDAM_USER_NAME_LEN_MIN) )
    {
        errorDescription = QObject::tr("User displayed name must have length from ");
        errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_USER_NAME_LEN_MIN);
        errorDescription += QObject::tr(" to ");
        errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_USER_NAME_LEN_MAX);

        return false;
    }

    QRegExp nameRegExp(QString::fromStdString(evernote::limits::g_Limits_constants.EDAM_USER_NAME_REGEX));
    if (nameRegExp.indexIn(name) < 0) {
        errorDescription = QObject::tr("User displayed name doesn't match its regular expression. "
                                       "Consider removing any special characters");
        return false;
    }

    if (isSet.timezone)
    {
        QString timezone = QString::fromStdString(enUser.timezone);
        size_t timezoneSize = timezone.size();
        if ( (timezoneSize > evernote::limits::g_Limits_constants.EDAM_TIMEZONE_LEN_MAX) ||
             (timezoneSize < evernote::limits::g_Limits_constants.EDAM_TIMEZONE_LEN_MIN) )
        {
            errorDescription = QObject::tr("User timezone should have length from ");
            errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_TIMEZONE_LEN_MIN);
            errorDescription += QObject::tr(" to ");
            errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_TIMEZONE_LEN_MAX);

            return false;
        }

        QRegExp timezoneRegExp(QString::fromStdString(evernote::limits::g_Limits_constants.EDAM_TIMEZONE_REGEX));
        if (timezoneRegExp.indexIn(timezone) < 0) {
            errorDescription = QObject::tr("User timezone doesn't match its regular expression. "
                                           "It must be encoded as a standard zone ID such as \"America/Los_Angeles\" "
                                           "or \"GMT+08:00\".");
            return false;
        }
    }

    if (isSet.attributes)
    {
        const auto & attributes = enUser.attributes;

        if (attributes.__isset.defaultLocationName)
        {
            QString defaultLocationName = QString::fromStdString(attributes.defaultLocationName);
            size_t defaultLocationNameSize = defaultLocationName.size();
            if ( (defaultLocationNameSize > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) ||
                 (defaultLocationNameSize < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription = QObject::tr("User default location name must have length from ");
                errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN);
                errorDescription += QObject::tr(" to ");
                errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX);

                return false;
            }
        }

        if (attributes.__isset.viewedPromotions)
        {
            const auto & viewedPromotions = attributes.viewedPromotions;
            for(const auto & item: viewedPromotions)
            {
                QString viewedPromotion = QString::fromStdString(item);
                size_t viewedPromotionSize = viewedPromotion.size();
                if ( (viewedPromotionSize > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) ||
                     (viewedPromotionSize < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) )
                {
                    errorDescription = QObject::tr("Each User's viewed promotion must have length from ");
                    errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN);
                    errorDescription += QObject::tr(" to ");
                    errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX);

                    return false;
                }
            }
        }

        if (attributes.__isset.incomingEmailAddress)
        {
            QString incomingEmailAddress = QString::fromStdString(attributes.incomingEmailAddress);
            size_t incomingEmailAddressSize = incomingEmailAddress.size();
            if ( (incomingEmailAddressSize > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) ||
                 (incomingEmailAddressSize < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription = QObject::tr("User's incoming email address must have length from ");
                errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN);
                errorDescription += QObject::tr(" to ");
                errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX);

                return false;
            }
        }

        if (attributes.__isset.recentMailedAddresses)
        {
            const auto & recentMailedAddresses = attributes.recentMailedAddresses;
            size_t numRecentMailedAddresses = recentMailedAddresses.size();
            if (numRecentMailedAddresses > evernote::limits::g_Limits_constants.EDAM_USER_RECENT_MAILED_ADDRESSES_MAX)
            {
                errorDescription = QObject::tr("User must have no more recent mailed addresses than ");
                errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_USER_RECENT_MAILED_ADDRESSES_MAX);

                return false;
            }

            for(const auto & item: recentMailedAddresses)
            {
                QString recentMailedAddress = QString::fromStdString(item);
                size_t recentMailedAddressSize = recentMailedAddress.size();
                if ( (recentMailedAddressSize > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) ||
                     (recentMailedAddressSize < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) )
                {
                    errorDescription = QObject::tr("Each user's recent emailed address must have length from ");
                    errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN);
                    errorDescription += QObject::tr(" to ");
                    errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX);

                    return false;
                }
            }
        }

        if (attributes.__isset.comments)
        {
            QString comments = QString::fromStdString(attributes.comments);
            size_t commentsSize = comments.size();
            if ( (commentsSize > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) ||
                 (commentsSize < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) )
            {
                errorDescription = QObject::tr("User's comments must have length from ");
                errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN);
                errorDescription += QObject::tr(" to ");
                errorDescription += QString::number(evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX);

                return false;
            }
        }
    }

    return true;
}

bool IUser::hasId() const
{
    return GetEnUser().__isset.id;
}

qint32 IUser::id() const
{
    return GetEnUser().id;
}

void IUser::setId(const qint32 id)
{
    auto & enUser = GetEnUser();
    enUser.id = id;
    enUser.__isset.id = true;
}

bool IUser::hasUsername() const
{
    return GetEnUser().__isset.username;
}

const QString IUser::username() const
{
    return std::move(QString::fromStdString(GetEnUser().username));
}

void IUser::setUsername(const QString & username)
{
    auto & enUser = GetEnUser();
    enUser.username = username.toStdString();
    enUser.__isset.username = !username.isEmpty();
}

bool IUser::hasEmail() const
{
    return GetEnUser().__isset.email;
}

const QString IUser::email() const
{
    return std::move(QString::fromStdString(GetEnUser().email));
}

void IUser::setEmail(const QString & email)
{
    auto & enUser = GetEnUser();
    enUser.email = email.toStdString();
    enUser.__isset.email = !email.isEmpty();
}

bool IUser::hasName() const
{
    return GetEnUser().__isset.name;
}

const QString IUser::name() const
{
    return std::move(QString::fromStdString(GetEnUser().name));
}

void IUser::setName(const QString & name)
{
    auto & enUser = GetEnUser();
    enUser.name = name.toStdString();
    enUser.__isset.name = !name.isEmpty();
}

bool IUser::hasTimezone() const
{
    return GetEnUser().__isset.timezone;
}

const QString IUser::timezone() const
{
    return std::move(QString::fromStdString(GetEnUser().timezone));
}

void IUser::setTimezone(const QString & timezone)
{
    auto & enUser = GetEnUser();
    enUser.timezone = timezone.toStdString();
    enUser.__isset.timezone = !timezone.isEmpty();
}

bool IUser::hasPrivilegeLevel() const
{
    return GetEnUser().__isset.privilege;
}

const qint8 IUser::privilegeLevel() const
{
    return GetEnUser().privilege;
}

void IUser::setPrivilegeLevel(const qint8 level)
{
    auto & enUser = GetEnUser();
    if (level <= static_cast<qint8>(evernote::edam::PrivilegeLevel::ADMIN)) {
        enUser.privilege = static_cast<evernote::edam::PrivilegeLevel::type>(level);
        enUser.__isset.privilege = true;
    }
    else {
        enUser.__isset.privilege = false;
    }
}

bool IUser::hasCreationTimestamp() const
{
    return GetEnUser().__isset.created;
}

qint64 IUser::creationTimestamp() const
{
    return GetEnUser().created;
}

void IUser::setCreationTimestamp(const qint64 timestamp)
{
    auto & enUser = GetEnUser();
    enUser.created = timestamp;
    enUser.__isset.created = (timestamp >= 0);
}

bool IUser::hasModificationTimestamp() const
{
    return GetEnUser().__isset.updated;
}

qint64 IUser::modificationTimestamp() const
{
    return GetEnUser().updated;
}

void IUser::setModificationTimestamp(const qint64 timestamp)
{
    auto & enUser = GetEnUser();
    enUser.updated = timestamp;
    enUser.__isset.updated = (timestamp >= 0);
}

bool IUser::hasDeletionTimestamp() const
{
    return GetEnUser().__isset.deleted;
}

qint64 IUser::deletionTimestamp() const
{
    return GetEnUser().deleted;
}

void IUser::setDeletionTimestamp(const qint64 timestamp)
{
    auto & enUser = GetEnUser();
    enUser.deleted = timestamp;
    enUser.__isset.deleted = (timestamp >= 0);
}

bool IUser::hasActive() const
{
    return GetEnUser().__isset.active;
}

bool IUser::active() const
{
    return GetEnUser().active;
}

void IUser::setActive(const bool active)
{
    auto & enUser = GetEnUser();
    enUser.active = active;
    enUser.__isset.active = true;
}

bool IUser::hasUserAttributes() const
{
    return GetEnUser().__isset.attributes;
}

const evernote::edam::UserAttributes & IUser::userAttributes() const
{
    return GetEnUser().attributes;
}

evernote::edam::UserAttributes & IUser::userAttributes()
{
    return GetEnUser().attributes;
}

void IUser::setHasAttributes(const bool hasAttributes)
{
    GetEnUser().__isset.attributes = hasAttributes;
}

bool IUser::hasAccounting() const
{
    return GetEnUser().__isset.accounting;
}

const evernote::edam::Accounting & IUser::accounting() const
{
    return GetEnUser().accounting;
}

evernote::edam::Accounting & IUser::accounting()
{
    return GetEnUser().accounting;
}

void IUser::setHasAccounting(const bool hasAccounting)
{
    GetEnUser().__isset.accounting = hasAccounting;
}

bool IUser::hasPremiumInfo() const
{
    return GetEnUser().__isset.premiumInfo;
}

const evernote::edam::PremiumInfo & IUser::premiumInfo() const
{
    return GetEnUser().premiumInfo;
}

evernote::edam::PremiumInfo & IUser::premiumInfo()
{
    return GetEnUser().premiumInfo;
}

void IUser::setHasPremiumInfo(const bool hasPremiumInfo)
{
    GetEnUser().__isset.premiumInfo = hasPremiumInfo;
}

bool IUser::hasBusinessUserInfo() const
{
    return GetEnUser().__isset.businessUserInfo;
}

const evernote::edam::BusinessUserInfo & IUser::businessUserInfo() const
{
    return GetEnUser().businessUserInfo;
}

evernote::edam::BusinessUserInfo & IUser::businessUserInfo()
{
    return GetEnUser().businessUserInfo;
}

void IUser::setHasBusinessUserInfo(const bool hasBusinessUserInfo)
{
    GetEnUser().__isset.businessUserInfo = hasBusinessUserInfo;
}



IUser::IUser(const IUser & other) :
    m_isDirty(other.m_isDirty),
    m_isLocal(other.m_isLocal)
{}

QTextStream & IUser::Print(QTextStream & strm) const
{
    strm << "IUser { \n";
    strm << "isDirty = " << (m_isDirty ? "true" : "false") << "; \n";
    strm << "isLocal = " << (m_isLocal ? "true" : "false") << "; \n";

    const auto & enUser = GetEnUser();
    const auto & isSet = enUser.__isset;

    if (isSet.id) {
        strm << "User ID = " << QString::number(enUser.id) << "; \n";
    }
    else {
        strm << "User ID is not set" << "; \n";
    }

    if (isSet.username) {
        strm << "username = " << QString::fromStdString(enUser.username) << "; \n";
    }
    else {
        strm << "username is not set" << "; \n";
    }

    if (isSet.email) {
        strm << "email = " << QString::fromStdString(enUser.email) << "; \n";
    }
    else {
        strm << "email is not set" << "; \n";
    }

    if (isSet.name) {
        strm << "name = " << QString::fromStdString(enUser.name) << "; \n";
    }
    else {
        strm << "name is not set" << "; \n";
    }

    if (isSet.timezone) {
        strm << "timezone = " << QString::fromStdString(enUser.timezone) << "; \n";
    }
    else {
        strm << "timezone is not set" << "; \n";
    }

    if (isSet.privilege) {
        strm << "privilege = " << enUser.privilege << "; \n";
    }
    else {
        strm << "privilege is not set" << "; \n";
    }

    if (isSet.created) {
        strm << "created = " << PrintableDateTimeFromTimestamp(enUser.created) << "; \n";
    }
    else {
        strm << "created is not set" << "; \n";
    }

    if (isSet.updated) {
        strm << "updated = " << PrintableDateTimeFromTimestamp(enUser.updated) << "; \n";
    }
    else {
        strm << "updated is not set" << "; \n";
    }

    if (isSet.deleted) {
        strm << "deleted = " << PrintableDateTimeFromTimestamp(enUser.deleted) << "; \n";
    }
    else {
        strm << "deleted is not set" << "; \n";
    }

    if (isSet.active) {
        strm << "active = " << (enUser.active ? "true" : "false") << "; \n";
    }
    else {
        strm << "active is not set" << "; \n";
    }

    if (isSet.attributes) {
        strm << enUser.attributes;
    }
    else {
        strm << "attributes are not set" << "; \n";
    }

    if (isSet.accounting) {
        strm << enUser.accounting;
    }
    else {
        strm << "accounting is not set" << "; \n";
    }

    if (isSet.premiumInfo) {
        strm << enUser.premiumInfo;
    }
    else {
        strm << "premium info is not set" << "; \n";
    }

    if (isSet.businessUserInfo) {
        strm << enUser.businessUserInfo;
    }
    else {
        strm << "business user info is not set" << "; \n";
    }

    return strm;
}

} // namespace qute_note
