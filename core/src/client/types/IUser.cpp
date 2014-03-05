#include "IUser.h"
#include <Types_types.h>
#include <Limits_constants.h>
#include <QObject>
#include <QRegExp>
#include <QDateTime>

namespace qute_note {

IUser::IUser() :
    m_isDirty(true),
    m_isLocal(true)
{}

IUser::~IUser()
{}

void IUser::Clear()
{
    SetDirty();
    SetLocal();
    GetEnUser() = evernote::edam::User();
}

bool IUser::IsDirty() const
{
    return m_isDirty;
}

void IUser::SetDirty()
{
    m_isDirty = true;
}

void IUser::SetClean()
{
    m_isDirty = false;
}

bool IUser::IsLocal() const
{
    return m_isLocal;
}

void IUser::SetLocal()
{
    m_isLocal = true;
}

void IUser::SetNonLocal()
{
    m_isLocal = false;
}

bool IUser::CheckParameters(QString & errorDescription) const
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
        // TODO: print this better
        strm << "privilege = " << QString::number(enUser.privilege) << "; \n";
    }
    else {
        strm << "privilege is not set" << "; \n";
    }

    if (isSet.created) {
        strm << "created = " << QDateTime::fromMSecsSinceEpoch(enUser.created).toString(Qt::ISODate);
    }
    else {
        strm << "created is not set" << "; \n";
    }

    // TODO: continue from here

    return strm;
}

} // namespace qute_note
