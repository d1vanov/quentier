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

#include <quentier/utility/Printable.h>
#include <quentier/utility/Utility.h>

namespace quentier {

const QString Printable::toString() const
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << *this;
    return str;
}

Printable::Printable()
{}

Printable::Printable(const Printable &)
{}

Printable::~Printable()
{}

QDebug & operator <<(QDebug & debug, const Printable & printable)
{
    debug << printable.toString();
    return debug;
}

QTextStream & operator <<(QTextStream & strm,
                          const Printable & printable)
{
    return printable.print(strm);
}

} // namespace quentier

inline QString __contactTypeToString(const qevercloud::ContactType::type & type)
{
    switch(type)
    {
    case qevercloud::ContactType::EVERNOTE:
        return QStringLiteral("EVERNOTE");
    case qevercloud::ContactType::SMS:
        return QStringLiteral("SMS");
    case qevercloud::ContactType::FACEBOOK:
        return QStringLiteral("FACEBOOK");
    case qevercloud::ContactType::EMAIL:
        return QStringLiteral("EMAIL");
    case qevercloud::ContactType::TWITTER:
        return QStringLiteral("TWITTER");
    case qevercloud::ContactType::LINKEDIN:
        return QStringLiteral("LINKEDIN");
    default:
        return QStringLiteral("Unknown");
    }
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::Contact & contact)
{
    strm << QStringLiteral("qevercloud::Contact: {\n");
    const char * indent = "  ";

#define PRINT_FIELD(field, ...) \
    strm << indent << QStringLiteral( #field " = ") << (contact.field.isSet() \
                                                        ? __VA_ARGS__(contact.field.ref()) \
                                                        : QStringLiteral("<empty>")) << QStringLiteral(";\n")

    PRINT_FIELD(name);
    PRINT_FIELD(id);
    PRINT_FIELD(type, __contactTypeToString);
    PRINT_FIELD(photoUrl);
    PRINT_FIELD(photoLastUpdated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(messagingPermit);
    PRINT_FIELD(messagingPermitExpires, quentier::printableDateTimeFromTimestamp);

#undef PRINT_FIELD

    strm << QStringLiteral("}; \n");
    return strm;
}

QString boolToString(const bool value) { return (value ? QStringLiteral("true") : QStringLiteral("false")); }

QTextStream & operator <<(QTextStream & strm, const qevercloud::Identity & identity)
{
    strm << QStringLiteral("qevercloud::Identity: {\n");
    const char * indent = "  ";

    strm << indent << QStringLiteral("id = ") << QString::number(identity.id) << QStringLiteral(";\n");

#define PRINT_FIELD(field, ...) \
    strm << indent << QStringLiteral( #field " = ") << (identity.field.isSet() \
                                                        ? __VA_ARGS__(identity.field.ref()) \
                                                        : QStringLiteral("<empty>")) << QStringLiteral(";\n")

    PRINT_FIELD(contact, ToString);
    PRINT_FIELD(userId, QString::number);
    PRINT_FIELD(deactivated, boolToString);
    PRINT_FIELD(sameBusiness, boolToString);
    PRINT_FIELD(blocked, boolToString);
    PRINT_FIELD(userConnected, boolToString);
    PRINT_FIELD(eventId, QString::number);

#undef PRINT_FIELD

    strm << QStringLiteral("}; \n");
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::BusinessUserInfo & info)
{
    strm << QStringLiteral("qevercloud::BusinessUserInfo: {\n");
    const char * indent = "  ";

#define PRINT_FIELD(field, ...) \
    strm << indent << QStringLiteral( #field " = ") << (info.field.isSet() \
                                                        ? __VA_ARGS__(info.field.ref()) \
                                                        : QStringLiteral("<empty>")) << QStringLiteral(";\n")

    PRINT_FIELD(businessId, QString::number);
    PRINT_FIELD(businessName);
    PRINT_FIELD(role, ToString);
    PRINT_FIELD(email);
    PRINT_FIELD(updated, quentier::printableDateTimeFromTimestamp);

#undef PRINT_FIELD

    strm << QStringLiteral("}; \n");
    return strm;
}

QString premiumOrderStatusToString(const qevercloud::PremiumOrderStatus::type & status)
{
    switch(status)
    {
    case qevercloud::PremiumOrderStatus::NONE:
        return QStringLiteral("NONE");
    case qevercloud::PremiumOrderStatus::PENDING:
        return QStringLiteral("PENDING");
    case qevercloud::PremiumOrderStatus::ACTIVE:
        return QStringLiteral("ACTIVE");
    case qevercloud::PremiumOrderStatus::FAILED:
        return QStringLiteral("FAILED");
    case qevercloud::PremiumOrderStatus::CANCELLATION_PENDING:
        return QStringLiteral("CANCELLATION_PENDING");
    case qevercloud::PremiumOrderStatus::CANCELED:
        return QStringLiteral("CANCELED");
    default:
        return QStringLiteral("Unknown");
    }
}

QString businessUserRoleToString(const qevercloud::BusinessUserRole::type & role)
{
    switch(role)
    {
    case qevercloud::BusinessUserRole::ADMIN:
        return QStringLiteral("ADMIN");
    case qevercloud::BusinessUserRole::NORMAL:
        return QStringLiteral("NORMAL");
    default:
        return QStringLiteral("Unknown");
    }
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::Accounting & accounting)
{
    strm << QStringLiteral("qevercloud::Accounting: { \n");
    const char * indent = "  ";

#define PRINT_FIELD(field, ...) \
    strm << indent << QStringLiteral( #field " = ") << (accounting.field.isSet() \
                                                        ? __VA_ARGS__(accounting.field.ref()) \
                                                        : QStringLiteral("<empty>")) << QStringLiteral(";\n")

    PRINT_FIELD(uploadLimitEnd, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(uploadLimitNextMonth, QString::number);
    PRINT_FIELD(premiumServiceStatus, premiumOrderStatusToString);
    PRINT_FIELD(premiumOrderNumber);
    PRINT_FIELD(premiumCommerceService);
    PRINT_FIELD(premiumServiceStart, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(premiumServiceSKU);
    PRINT_FIELD(lastSuccessfulCharge, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(lastFailedCharge, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(lastFailedChargeReason);
    PRINT_FIELD(nextPaymentDue, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(premiumLockUntil, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(updated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(premiumSubscriptionNumber);
    PRINT_FIELD(lastRequestedCharge, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(currency);
    PRINT_FIELD(unitPrice, QString::number);
    PRINT_FIELD(businessId, QString::number);
    PRINT_FIELD(businessRole, businessUserRoleToString);
    PRINT_FIELD(unitDiscount, QString::number);
    PRINT_FIELD(nextChargeDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(availablePoints, QString::number);

#undef PRINT_FIELD

    strm << QStringLiteral("}; \n");
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::AccountLimits & limits)
{
    strm << QStringLiteral("qevercloud::AccountLimits: {\n");
    const char * indent = "  ";

#define PRINT_FIELD(field) \
    strm << indent << QStringLiteral( #field " =") << (limits.field.isSet() \
                                                       ? QString::number(limits.field.ref()) \
                                                       : QStringLiteral("<empty>")) << "; \n"

    PRINT_FIELD(userMailLimitDaily);
    PRINT_FIELD(noteSizeMax);
    PRINT_FIELD(resourceSizeMax);
    PRINT_FIELD(userLinkedNotebookMax);
    PRINT_FIELD(uploadLimit);
    PRINT_FIELD(userNoteCountMax);
    PRINT_FIELD(userNotebookCountMax);
    PRINT_FIELD(userTagCountMax);
    PRINT_FIELD(noteTagCountMax);
    PRINT_FIELD(userSavedSearchesMax);
    PRINT_FIELD(noteResourceCountMax);

#undef PRINT_FIELD

    return strm;
}

QString reminderEmailConfigToString(const qevercloud::ReminderEmailConfig::type & type)
{
    switch(type)
    {
    case qevercloud::ReminderEmailConfig::DO_NOT_SEND:
        return QStringLiteral("DO NOT SEND");
    case qevercloud::ReminderEmailConfig::SEND_DAILY_EMAIL:
        return QStringLiteral("SEND DAILY EMAIL");
    default:
        return QStringLiteral("Unknown");
    }
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::UserAttributes & attributes)
{
    strm << QStringLiteral("qevercloud::UserAttributes: {\n");
    const char * indent = "  ";

#define PRINT_FIELD(field, ...) \
    strm << indent << QStringLiteral( #field " = ") << (attributes.field.isSet() \
                                                        ? __VA_ARGS__(attributes.field.ref()) \
                                                        : QStringLiteral("<empty>")) << QStringLiteral("; \n")

#define PRINT_LIST_FIELD(field, ...) \
    strm << indent << QStringLiteral( #field ); \
    if (attributes.field.isSet()) \
    { \
        strm << QStringLiteral(": { \n"); \
        const auto & field##List = attributes.field.ref(); \
        const int num##field = field##List.size(); \
        for(int i = 0; i < num##field; ++i) { \
            strm << indent << indent << QStringLiteral("[") << i << QStringLiteral("]: ") \
                 << __VA_ARGS__(field##List[i]) << QStringLiteral(";\n"); \
        } \
        strm << indent << QStringLiteral("};\n"); \
    } \
    else \
    { \
        strm << QStringLiteral(" = <empty>;\n"); \
    }

    PRINT_FIELD(defaultLocationName);
    PRINT_FIELD(defaultLatitude, QString::number);
    PRINT_FIELD(defaultLongitude, QString::number);
    PRINT_FIELD(preactivation, boolToString);
    PRINT_LIST_FIELD(viewedPromotions)
    PRINT_FIELD(incomingEmailAddress);
    PRINT_LIST_FIELD(recentMailedAddresses)
    PRINT_FIELD(comments);
    PRINT_FIELD(dateAgreedToTermsOfService, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(maxReferrals, QString::number);
    PRINT_FIELD(referralCount, QString::number);
    PRINT_FIELD(refererCode);
    PRINT_FIELD(sentEmailDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(dailyEmailLimit, QString::number);
    PRINT_FIELD(emailOptOutDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(partnerEmailOptInDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(preferredLanguage);
    PRINT_FIELD(preferredCountry);
    PRINT_FIELD(clipFullPage, boolToString);
    PRINT_FIELD(twitterUserName);
    PRINT_FIELD(twitterId);
    PRINT_FIELD(groupName);
    PRINT_FIELD(recognitionLanguage);
    PRINT_FIELD(referralProof);
    PRINT_FIELD(educationalDiscount, boolToString);
    PRINT_FIELD(businessAddress);
    PRINT_FIELD(hideSponsorBilling, boolToString);
    PRINT_FIELD(useEmailAutoFiling, boolToString);
    PRINT_FIELD(reminderEmailConfig, reminderEmailConfigToString);
    PRINT_FIELD(passwordUpdated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(salesforcePushEnabled, boolToString);

#undef PRINT_LIST_FIELD
#undef PRINT_FIELD

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::NoteAttributes & attributes)
{
    strm << QStringLiteral("qevercloud::NoteAttributes: {\n");
    const char * indent = "  ";

#define PRINT_FIELD(field, ...) \
    strm << indent << QStringLiteral( #field " = ") << (attributes.field.isSet() \
                                                        ? __VA_ARGS__(attributes.field.ref()) \
                                                        : QStringLiteral("<empty>")) << QStringLiteral("; \n")

    PRINT_FIELD(subjectDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(latitude, QString::number);
    PRINT_FIELD(longitude, QString::number);
    PRINT_FIELD(altitude, QString::number);
    PRINT_FIELD(author);
    PRINT_FIELD(source);
    PRINT_FIELD(sourceURL);
    PRINT_FIELD(sourceApplication);
    PRINT_FIELD(shareDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(reminderOrder, QString::number);
    PRINT_FIELD(reminderDoneTime, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(reminderTime, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(placeName);
    PRINT_FIELD(contentClass);
    PRINT_FIELD(lastEditedBy);
    PRINT_FIELD(creatorId, QString::number);
    PRINT_FIELD(lastEditorId, QString::number);

#undef PRINT_FIELD

    if (attributes.applicationData.isSet())
    {
        const qevercloud::LazyMap & applicationData = attributes.applicationData;

        if (applicationData.keysOnly.isSet()) {
            const QSet<QString> & keysOnly = applicationData.keysOnly;
            strm << indent << QStringLiteral("applicationData: keys only: \n");
            for(auto it = keysOnly.begin(), end = keysOnly.end(); it != end; ++it) {
                strm << *it << QStringLiteral("; ");
            }
            strm << QStringLiteral("\n");
        }

        if (applicationData.fullMap.isSet()) {
            const QMap<QString, QString> & fullMap = applicationData.fullMap;
            strm << indent << QStringLiteral("applicationData: full map: \n");
            for(auto it = fullMap.begin(), end = fullMap.end(); it != end; ++it) {
                strm << QStringLiteral("[") << it.key() << QStringLiteral("] = ")
                     << it.value() << QStringLiteral("; ");
            }
            strm << QStringLiteral("\n");
        }
    }

    if (attributes.classifications.isSet())
    {
        strm << indent << QStringLiteral("classifications: ");
        const QMap<QString, QString> & classifications = attributes.classifications;
        for(auto it = classifications.begin(), end = classifications.end(); it != end; ++it) {
            strm << QStringLiteral("[") << it.key() << QStringLiteral("] = ")
                 << it.value() << QStringLiteral("; ");
        }
        strm << QStringLiteral("\n");
    }

    strm << QStringLiteral("};\n");
    return strm;
}

QString privilegeLevelToString(const qevercloud::PrivilegeLevel::type & level)
{
    switch (level)
    {
    case qevercloud::PrivilegeLevel::NORMAL:
        return QStringLiteral("NORMAL");
    case qevercloud::PrivilegeLevel::PREMIUM:
        return QStringLiteral("PREMIUM");
    case qevercloud::PrivilegeLevel::VIP:
        return QStringLiteral("VIP");
    case qevercloud::PrivilegeLevel::MANAGER:
        return QStringLiteral("MANAGER");
    case qevercloud::PrivilegeLevel::SUPPORT:
        return QStringLiteral("SUPPORT");
    case qevercloud::PrivilegeLevel::ADMIN:
        return QStringLiteral("ADMIN");
    default:
        return QStringLiteral("Unknown");
    }
}

QString serviceLevelToString(const qevercloud::ServiceLevel::type & level)
{
    switch(level)
    {
    case qevercloud::ServiceLevel::BASIC:
        return QStringLiteral("BASIC");
    case qevercloud::ServiceLevel::PLUS:
        return QStringLiteral("PLUS");
    case qevercloud::ServiceLevel::PREMIUM:
        return QStringLiteral("PREMIUM");
    default:
        return QStringLiteral("Unknown");
    }
}

QString queryFormatToString(const qevercloud::QueryFormat::type & format)
{
    switch (format)
    {
    case qevercloud::QueryFormat::USER:
        return QStringLiteral("USER");
    case qevercloud::QueryFormat::SEXP:
        return QStringLiteral("SEXP");
    default:
        return QStringLiteral("Unknown");
    }
}

QString sharedNotebookPrivilegeLevelToString(const qevercloud::SharedNotebookPrivilegeLevel::type & privilege)
{
    switch(privilege)
    {
    case qevercloud::SharedNotebookPrivilegeLevel::READ_NOTEBOOK:
        return QStringLiteral("READ_NOTEBOOK");
    case qevercloud::SharedNotebookPrivilegeLevel::MODIFY_NOTEBOOK_PLUS_ACTIVITY:
        return QStringLiteral("MODIFY_NOTEBOOK_PLUS_ACTIVITY");
    case qevercloud::SharedNotebookPrivilegeLevel::READ_NOTEBOOK_PLUS_ACTIVITY:
        return QStringLiteral("READ_NOTEBOOK_PLUS_ACTIVITY");
    case qevercloud::SharedNotebookPrivilegeLevel::GROUP:
        return QStringLiteral("GROUP");
    case qevercloud::SharedNotebookPrivilegeLevel::FULL_ACCESS:
        return QStringLiteral("FULL_ACCESS");
    case qevercloud::SharedNotebookPrivilegeLevel::BUSINESS_FULL_ACCESS:
        return QStringLiteral("BUSINESS_FULL_ACCESS");
    default:
        return QStringLiteral("Unknown");
    }
}

QString noteSortOrderToString(const qevercloud::NoteSortOrder::type & order)
{
    switch(order)
    {
    case qevercloud::NoteSortOrder::CREATED:
        return QStringLiteral("CREATED");
    case qevercloud::NoteSortOrder::RELEVANCE:
        return QStringLiteral("RELEVANCE");
    case qevercloud::NoteSortOrder::TITLE:
        return QStringLiteral("TITLE");
    case qevercloud::NoteSortOrder::UPDATED:
        return QStringLiteral("UPDATED");
    case qevercloud::NoteSortOrder::UPDATE_SEQUENCE_NUMBER:
        return QStringLiteral("UPDATE_SEQUENCE_NUMBER");
    default:
        return QStringLiteral("Unknown");
    }
}

QString sharedNotebookInstanceRestrictionsToString(const qevercloud::SharedNotebookInstanceRestrictions::type & restrictions)
{
    switch(restrictions)
    {
    case qevercloud::SharedNotebookInstanceRestrictions::ASSIGNED:
        return QStringLiteral("ASSIGNED");
    case qevercloud::SharedNotebookInstanceRestrictions::NO_SHARED_NOTEBOOKS:
        return QStringLiteral("NO_SHARED_NOTEBOOKS");
    default:
        return QStringLiteral("Unknown");
    }
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::NotebookRestrictions & restrictions)
{
    strm << QStringLiteral("NotebookRestrictions: {\n");
    const char * indent = "  ";

#define PRINT_FIELD(field, ...) \
    strm << indent << QStringLiteral( #field " = ") << (restrictions.field.isSet() \
                                                        ? __VA_ARGS__(restrictions.field.ref()) \
                                                        : QStringLiteral("<empty>")) << QStringLiteral("; \n")

    PRINT_FIELD(noReadNotes, boolToString);
    PRINT_FIELD(noCreateNotes, boolToString);
    PRINT_FIELD(noUpdateNotes, boolToString);
    PRINT_FIELD(noExpungeNotes, boolToString);
    PRINT_FIELD(noShareNotes, boolToString);
    PRINT_FIELD(noEmailNotes, boolToString);
    PRINT_FIELD(noSendMessageToRecipients, boolToString);
    PRINT_FIELD(noUpdateNotebook, boolToString);
    PRINT_FIELD(noExpungeNotebook, boolToString);
    PRINT_FIELD(noSetDefaultNotebook, boolToString);
    PRINT_FIELD(noSetNotebookStack, boolToString);
    PRINT_FIELD(noPublishToPublic, boolToString);
    PRINT_FIELD(noPublishToBusinessLibrary, boolToString);
    PRINT_FIELD(noCreateTags, boolToString);
    PRINT_FIELD(noUpdateTags, boolToString);
    PRINT_FIELD(noExpungeTags, boolToString);
    PRINT_FIELD(noSetParentTag, boolToString);
    PRINT_FIELD(noCreateSharedNotebooks, boolToString);
    PRINT_FIELD(updateWhichSharedNotebookRestrictions, sharedNotebookInstanceRestrictionsToString);
    PRINT_FIELD(expungeWhichSharedNotebookRestrictions, sharedNotebookInstanceRestrictionsToString);

#undef PRINT_FIELD

    strm << QStringLiteral("};\n");

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::ResourceAttributes & attributes)
{
    strm << "ResourceAttributes: { \n";

    const char * indent = "  ";

    if (attributes.sourceURL.isSet()) {
        strm << indent << "sourceURL = " << attributes.sourceURL << "; \n";
    }
    else {
        strm << indent << "sourceURL is not set; \n";
    }

    if (attributes.timestamp.isSet()) {
        strm << indent << "timestamp = " << attributes.timestamp << "; \n";
    }
    else {
        strm << indent << "timestamp is not set; \n";
    }

    if (attributes.latitude.isSet()) {
        strm << indent << "latitude = " << attributes.latitude << "; \n";
    }
    else {
        strm << indent << "latitude is not set; \n";
    }

    if (attributes.longitude.isSet()) {
        strm << indent << "longitude = " << attributes.longitude << "; \n";
    }
    else {
        strm << indent << "longitude is not set; \n";
    }

    if (attributes.altitude.isSet()) {
        strm << indent << "altitude = " << attributes.altitude << "; \n";
    }
    else {
        strm << indent << "altitude is not set; \n";
    }

    if (attributes.cameraMake.isSet()) {
        strm << indent << "cameraMake = " << attributes.cameraMake << "; \n";
    }
    else {
        strm << indent << "cameraMake is not set; \n";
    }

    if (attributes.cameraModel.isSet()) {
        strm << indent << "cameraModel = " << attributes.cameraModel << "; \n";
    }
    else {
        strm << indent << "cameraModel is not set; \n";
    }

    if (attributes.clientWillIndex.isSet()) {
        strm << indent << "clientWillIndex = " << (attributes.clientWillIndex ? "true" : "false") << "; \n";
    }
    else {
        strm << indent << "clientWillIndex is not set; \n";
    }

    if (attributes.fileName.isSet()) {
        strm << indent << "fileName = " << attributes.fileName << "; \n";
    }
    else {
        strm << indent << "fileName is not set; \n";
    }

    if (attributes.attachment.isSet()) {
        strm << indent << "attachment = " << (attributes.attachment ? "true" : "false") << "; \n";
    }
    else {
        strm << indent << "attachment is not set; \n";
    }

    if (attributes.applicationData.isSet())
    {
        const qevercloud::LazyMap & applicationData = attributes.applicationData;

        if (applicationData.keysOnly.isSet()) {
            const QSet<QString> & keysOnly = applicationData.keysOnly;
            foreach(const QString & item, keysOnly) {
                strm << indent << "applicationData key: " << item << "; \n";
            }
        }

        if (applicationData.fullMap.isSet()) {
            const QMap<QString, QString> & fullMap = applicationData.fullMap;
            foreach(const QString & key, fullMap.keys()) {
                strm << indent << "applicationData[" << key << "] = " << fullMap.value(key) << "; \n";
            }
        }
    }

    strm << "}; \n";

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::Resource & resource)
{
    strm << "qevercloud::Resource { \n";
    const char * indent = "  ";

    if (resource.guid.isSet()) {
        strm << indent << "guid = " << resource.guid << "; \n";
    }
    else {
        strm << "guid is not set; \n";
    }

    if (resource.updateSequenceNum.isSet()) {
        strm << indent << "updateSequenceNumber = " << QString::number(resource.updateSequenceNum) << "; \n";
    }
    else {
        strm << indent << "updateSequenceNumber is not set; \n";
    }

    if (resource.noteGuid.isSet()) {
        strm << indent << "noteGuid = " << resource.noteGuid << "; \n";
    }
    else {
        strm << indent << "noteGuid is not set; \n";
    }

    if (resource.data.isSet())
    {
        strm << indent << "Data: { \n";

        if (resource.data->size.isSet()) {
            strm << indent << indent << "size = " << QString::number(resource.data->size) << "; \n";
        }
        else {
            strm << indent << indent << "size is not set; \n";
        }

        if (resource.data->bodyHash.isSet()) {
            strm << indent << indent << "hash = " << resource.data->bodyHash->toHex() << "; \n";
        }
        else {
            strm << indent << indent << "hash is not set; \n";
        }

        if (resource.data->body.isSet()) {
            strm << indent << indent << "body is set" << "; \n";
        }
        else {
            strm << indent << indent << "body is not set; \n";
        }

        strm << indent << "}; \n";
    }

    if (resource.mime.isSet()) {
        strm << indent << "mime = " << resource.mime << "; \n";
    }
    else {
        strm << indent << "mime is not set; \n";
    }

    if (resource.width.isSet()) {
        strm << indent << "width = " << QString::number(resource.width) << "; \n";
    }
    else {
        strm << indent << "width is not set; \n";
    }

    if (resource.height.isSet()) {
        strm << indent << "height = " << QString::number(resource.height) << "; \n";
    }
    else {
        strm << indent << "height is not set; \n";
    }

    if (resource.recognition.isSet())
    {
        strm << indent << "Recognition data: { \n";
        if (resource.recognition->size.isSet()) {
            strm << indent << indent << "size = "
                 << QString::number(resource.recognition->size) << "; \n";
        }
        else {
            strm << indent << indent << "size is not set; \n";
        }

        if (resource.recognition->bodyHash.isSet()) {
            strm << indent << indent << "hash = " << resource.recognition->bodyHash->toHex() << "; \n";
        }
        else {
            strm << indent << indent << "hash is not set; \n";
        }

        if (resource.recognition->body.isSet()) {
            strm << indent << indent << "body is set; \n";
        }
        else {
            strm << indent << indent << "body is not set; \n";
        }

        strm << indent << "}; \n";
    }

    if (resource.alternateData.isSet())
    {
        strm << indent << "Alternate data: { \n";

        if (resource.alternateData->size.isSet()) {
            strm << indent << indent << "size = " << QString::number(resource.alternateData->size) << "; \n";
        }
        else {
            strm << indent << indent << "size is not set; \n";
        }

        if (resource.alternateData->bodyHash.isSet()) {
            strm << indent << indent << "hash = " << resource.alternateData->bodyHash->toHex() << "; \n";
        }
        else {
            strm << indent << indent << "hash is not set; \n";
        }

        if (resource.alternateData->body.isSet()) {
            strm << indent << indent << "body is set; \n";
        }
        else {
            strm << indent << indent << "body is not set; \n";
        }

        strm << "}; \n";
    }

    if (resource.attributes.isSet()) {
        strm << indent << resource.attributes.ref();
    }

    strm << "}; \n";

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SyncChunk & syncChunk)
{
    strm << "qevercloud::SyncChunk: { \n";
    const char * indent = "  ";

    strm << indent << "chunkHighUSN = "
         << (syncChunk.chunkHighUSN.isSet() ? QString::number(syncChunk.chunkHighUSN.ref()) : "<empty>") << "; \n";
    strm << indent << "currentTime = "
         << syncChunk.currentTime << " ("
         << QDateTime::fromMSecsSinceEpoch(syncChunk.currentTime).toString(Qt::ISODate) << "); \n";
    strm << indent << "updateCount = " << syncChunk.updateCount << "; \n";

    if (syncChunk.expungedLinkedNotebooks.isSet())
    {
        foreach(const qevercloud::Guid & guid, syncChunk.expungedLinkedNotebooks.ref()) {
            strm << indent << "expunged linked notebook guid = " << guid << "; \n";
        }
    }

    if (syncChunk.expungedNotebooks.isSet())
    {
        foreach(const qevercloud::Guid & guid, syncChunk.expungedNotebooks.ref()) {
            strm << indent << "expunged notebook guid = " << guid << "; \n";
        }
    }

    if (syncChunk.expungedNotes.isSet())
    {
        foreach(const qevercloud::Guid & guid, syncChunk.expungedNotes.ref()) {
            strm << indent << "expunged note guid = " << guid << "; \n";
        }
    }

    if (syncChunk.expungedSearches.isSet())
    {
        foreach(const qevercloud::Guid & guid, syncChunk.expungedSearches.ref()) {
            strm << indent << "expunged search guid = " << guid << "; \n";
        }
    }

    if (syncChunk.expungedTags.isSet())
    {
        foreach(const qevercloud::Guid & guid, syncChunk.expungedTags.ref()) {
            strm << indent << "expunged tag guid = " << guid << "; \n";
        }
    }

    if (syncChunk.linkedNotebooks.isSet()) {
        const QList<qevercloud::LinkedNotebook> & linkedNotebooks = syncChunk.linkedNotebooks.ref();
        const int numLinkedNotebooks = linkedNotebooks.size();
        for(int i = 0; i < numLinkedNotebooks; ++i) {
            strm << indent << ", linked notebook: " << linkedNotebooks[i] << "; \n";
        }
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::Tag & tag)
{
    strm << "qevercloud::Tag: { \n";
    const char * indent = "  ";

    strm << indent << "guid = " << (tag.guid.isSet() ? tag.guid.ref() : "<empty>") << "; \n";
    strm << indent << "name = " << (tag.name.isSet() ? tag.name.ref() : "<empty>") << "; \n";
    strm << indent << "parentGuid = " << (tag.parentGuid.isSet() ? tag.parentGuid.ref() : "<empty>") << "; \n";
    strm << indent << "updateSequenceNum = " << (tag.updateSequenceNum.isSet()
                                                 ? QString::number(tag.updateSequenceNum.ref())
                                                 : "<empty>") << "; \n";

    strm << "}; \n";

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SavedSearch & savedSearch)
{
    strm << "qevercloud::SavedSearch: { \n";
    const char * indent = "  ";

    strm << indent << "guid = " << (savedSearch.guid.isSet() ? savedSearch.guid.ref() : "<empty>") << "; \n";
    strm << indent << "name = " << (savedSearch.name.isSet() ? savedSearch.name.ref() : "<empty>") << "; \n";
    strm << indent << "query = " << (savedSearch.query.isSet() ? savedSearch.query.ref() : "<empty>") << "; \n";

    strm << indent << "format = ";
    if (savedSearch.format.isSet())
    {
        switch(savedSearch.format.ref())
        {
        case qevercloud::QueryFormat::USER:
            strm << "USER";
            break;
        case qevercloud::QueryFormat::SEXP:
            strm << "SEXP";
            break;
        default:
            strm << "UNKNOWN";
            break;
        }
    }
    else
    {
        strm << "<empty>";
    }

    strm << "; \n";

    strm << indent << "updateSequenceNum = " << (savedSearch.updateSequenceNum.isSet()
                                                 ? QString::number(savedSearch.updateSequenceNum.ref())
                                                 : "<empty>");
    strm << indent << "SavedSearchScope = ";
    if (savedSearch.scope.isSet())
    {
        strm << "{ \n";
        strm << indent << indent << "includeAccount = " << (savedSearch.scope->includeAccount ? "true" : "false") << "; \n";
        strm << indent << indent << "includePersonalLinkedNotebooks = " << (savedSearch.scope->includePersonalLinkedNotebooks ? "true" : "false") << "; \n";
        strm << indent << indent << "includeBusinessLinkedNotebooks = " << (savedSearch.scope->includeBusinessLinkedNotebooks ? "true" : "false") << "; \n";
        strm << indent << "}; \n";
    }
    else
    {
        strm << "<empty>; \n";
    }

    strm << "}; \n";

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::LinkedNotebook & linkedNotebook)
{
    strm << "qevercloud::LinkedNotebook: { \n";
    const char * indent = "  ";

    strm << indent << "shareName = " << (linkedNotebook.shareName.isSet() ? linkedNotebook.shareName.ref() : "<empty>") << "; \n";
    strm << indent << "username = " << (linkedNotebook.username.isSet() ? linkedNotebook.username.ref() : "<empty>") << "; \n";
    strm << indent << "shardId = " << (linkedNotebook.shardId.isSet() ? linkedNotebook.shardId.ref() : "<empty>") << "; \n";
    strm << indent << "uri = " << (linkedNotebook.uri.isSet() ? linkedNotebook.uri.ref() : "<empty>") << "; \n";
    strm << indent << "guid = " << (linkedNotebook.guid.isSet() ? linkedNotebook.guid.ref() : "<empty>") << "; \n";
    strm << indent << "updateSequenceNum = " << (linkedNotebook.updateSequenceNum.isSet()
                                                 ? QString::number(linkedNotebook.updateSequenceNum.ref())
                                                 : "<empty>") << "; \n";
    strm << indent << "noteStoreUrl = " << (linkedNotebook.noteStoreUrl.isSet() ? linkedNotebook.noteStoreUrl.ref() : "<empty>") << "; \n";
    strm << indent << "webApiUrlPrefix = " << (linkedNotebook.webApiUrlPrefix.isSet() ? linkedNotebook.webApiUrlPrefix.ref() : "<empty>") << "; \n";
    strm << indent << "stack = " << (linkedNotebook.stack.isSet() ? linkedNotebook.stack.ref() : "<empty>") << "; \n";
    strm << indent << "businessId = " << (linkedNotebook.businessId.isSet()
                                          ? QString::number(linkedNotebook.businessId.ref())
                                          : "<empty>") << "; \n";
    strm << "}; \n";

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::Notebook & notebook)
{
    strm << "qevercloud::Notebook: { \n";
    const char * indent = "  ";

    strm << indent << "guid = " << (notebook.guid.isSet() ? notebook.guid.ref() : "<empty>") << "; \n";
    strm << indent << "name = " << (notebook.name.isSet() ? notebook.name.ref() : "<empty>") << "; \n";
    strm << indent << "updateSequenceNum = " << (notebook.updateSequenceNum.isSet()
                                                 ? QString::number(notebook.updateSequenceNum.ref())
                                                 : "<empty>") << "; \n";
    strm << indent << "defaultNotebook = " << (notebook.defaultNotebook.isSet()
                                               ? (notebook.defaultNotebook.ref() ? "true" : "false")
                                               : "<empty>") << "; \n";
    strm << indent << "serviceCreated = " << (notebook.serviceCreated.isSet()
                                              ? quentier::printableDateTimeFromTimestamp(notebook.serviceCreated.ref())
                                              : "<empty>") << "; \n";
    strm << indent << "serviceUpdated = " << (notebook.serviceUpdated.isSet()
                                              ? quentier::printableDateTimeFromTimestamp(notebook.serviceUpdated.ref())
                                              : "<empty>") << "; \n";

    strm << indent << "publishing = ";
    if (notebook.publishing.isSet()) {
        strm << notebook.publishing.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "published = " << (notebook.published.isSet()
                                         ? (notebook.published.ref() ? "true" : "false")
                                         : "<empty>") << "; \n";
    strm << indent << "stack = " << (notebook.stack.isSet() ? notebook.stack.ref() : "<empty>") << "; \n";

    if (notebook.sharedNotebooks.isSet())
    {
        strm << indent << "sharedNotebooks: { \n";

        const QList<qevercloud::SharedNotebook> & sharedNotebooks = notebook.sharedNotebooks.ref();
        typedef QList<qevercloud::SharedNotebook>::const_iterator CIter;
        CIter sharedNotebooksEnd = sharedNotebooks.end();
        for(CIter it = sharedNotebooks.begin(); it != sharedNotebooksEnd; ++it) {
            strm << indent << indent << *it;
        }

        strm << indent << "}; \n";
    }

    strm << indent << "businessNotebook = ";
    if (notebook.businessNotebook.isSet()) {
        strm << notebook.businessNotebook.ref();
    }
    else {
         strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "contact = ";
    if (notebook.contact.isSet()) {
        strm << notebook.contact.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    if (notebook.restrictions.isSet()) {
        strm << notebook.restrictions.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::Publishing & publishing)
{
    strm << "qevercloud::Publishing: { \n";
    const char * indent = "  ";

    strm << indent << "uri = " << (publishing.uri.isSet() ? publishing.uri.ref() : "<empty>") << "; \n";

    strm << indent << "order = ";
    if (publishing.order.isSet()) {
        strm << publishing.order.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "ascending = " << (publishing.ascending.isSet()
                                         ? (publishing.ascending.ref() ? "true" : "false")
                                         : "<empty>") << "; \n";
    strm << indent << "publicDescription = " << (publishing.publicDescription.isSet()
                                                 ? publishing.publicDescription.ref()
                                                 : "<empty>") << "; \n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SharedNotebook & sharedNotebook)
{
    strm << "qevercloud::SharedNotebook: { \n";
    const char * indent = "  ";

    strm << indent << "id = " << (sharedNotebook.id.isSet() ? QString::number(sharedNotebook.id.ref()) : "<empty>") << "; \n";
    strm << indent << "userId = " << (sharedNotebook.userId.isSet() ? QString::number(sharedNotebook.userId.ref()) : "<empty>") << "; \n";
    strm << indent << "notebookGuid = " << (sharedNotebook.notebookGuid.isSet() ? sharedNotebook.notebookGuid.ref() : "<empty>") << "; \n";
    strm << indent << "email = " << (sharedNotebook.email.isSet() ? sharedNotebook.email.ref() : "<empty>") << "; \n";
    strm << indent << "serviceCreated = " << (sharedNotebook.serviceCreated.isSet()
                                              ? quentier::printableDateTimeFromTimestamp(sharedNotebook.serviceCreated.ref())
                                              : "<empty>") << "; \n";
    strm << indent << "serviceUpdated = " << (sharedNotebook.serviceUpdated.isSet()
                                              ? quentier::printableDateTimeFromTimestamp(sharedNotebook.serviceUpdated.ref())
                                              : "<empty>") << "; \n";

    strm << indent << "privilege = ";
    if (sharedNotebook.privilege.isSet()) {
        strm << sharedNotebook.privilege.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "recipientSettings = ";
    if (sharedNotebook.recipientSettings.isSet()) {
        strm << sharedNotebook.recipientSettings.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::BusinessNotebook & businessNotebook)
{
    strm << "qevercloud::BusinessNotebook: { \n";
    const char * indent = "  ";

    strm << indent << "notebookDescription = " << (businessNotebook.notebookDescription.isSet()
                                                   ? businessNotebook.notebookDescription.ref()
                                                   : "<empty>") << "; \n";

    strm << indent << "privilege = ";
    if (businessNotebook.privilege.isSet()) {
        strm << businessNotebook.privilege.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "recommended = " << (businessNotebook.recommended.isSet()
                                           ? (businessNotebook.recommended.ref() ? "true" : "false")
                                           : "<empty>") << "; \n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::User & user)
{
    strm << "qevercloud::User: { \n";
    const char * indent = "  ";

    strm << indent << "id = " << (user.id.isSet() ? QString::number(user.id.ref()) : "<empty>") << "; \n";
    strm << indent << "username = " << (user.username.isSet() ? user.username.ref() : "<empty>") << "; \n";
    strm << indent << "email = " << (user.email.isSet() ? user.email.ref() : "<empty>") << "; \n";
    strm << indent << "name = " << (user.name.isSet() ? user.name.ref() : "<empty>") << "; \n";
    strm << indent << "timezone = " << (user.timezone.isSet() ? user.timezone.ref() : "<empty>") << "; \n";

    strm << indent << "privilege = ";
    if (user.privilege.isSet()) {
        strm << user.privilege.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "created = " << (user.created.isSet()
                                       ? quentier::printableDateTimeFromTimestamp(user.created.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "updated = " << (user.updated.isSet()
                                       ? quentier::printableDateTimeFromTimestamp(user.updated.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "deleted = " << (user.deleted.isSet()
                                       ? quentier::printableDateTimeFromTimestamp(user.deleted.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "active = " << (user.active.isSet()
                                      ? (user.active.ref() ? "true" : "false")
                                      : "<empty>") << "; \n";

    strm << indent << "attributes";
    if (user.attributes.isSet()) {
        strm << ": \n" << user.attributes.ref();
    }
    else {
        strm << " = <empty>; \n";
    }

    strm << indent << "accounting";
    if (user.accounting.isSet()) {
        strm << ": \n" << user.accounting.ref();
    }
    else {
        strm << " = empty>; \n";
    }

    strm << indent << "businessUserInfo";
    if (user.businessUserInfo.isSet()) {
        strm << ": \n" << user.businessUserInfo.ref();
    }
    else {
        strm << " = <empty>; \n";
    }

    strm << "}; \n";
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SharedNotebookRecipientSettings & settings)
{
    strm << "qevercloud::SharedNotebookRecipientSettings: { \n";
    const char * indent = "  ";

    strm << indent << "reminderNotifyEmail = " << (settings.reminderNotifyEmail.isSet()
                                                   ? (settings.reminderNotifyEmail.ref() ? "true" : "false")
                                                   : "<empty>") << "; \n";
    strm << indent << "reminderNotifyInApp = " << (settings.reminderNotifyInApp.isSet()
                                                   ? (settings.reminderNotifyInApp.ref() ? "true" : "false")
                                                   : "<empty>") << "; \n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::ReminderEmailConfig::type & config)
{
    strm << "qevercloud::ReminderEmailConfig: ";

    switch(config)
    {
    case qevercloud::ReminderEmailConfig::DO_NOT_SEND:
        strm << "DO_NOT_SEND";
        break;
    case qevercloud::ReminderEmailConfig::SEND_DAILY_EMAIL:
        strm << "SEND_DAILY_EMAIL";
        break;
    default:
        strm << "Unknown";
        break;
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::PremiumOrderStatus::type & status)
{
    strm << "qevercloud::PremiumOrderStatus: ";

    switch(status)
    {
    case qevercloud::PremiumOrderStatus::NONE:
        strm << "NONE";
        break;
    case qevercloud::PremiumOrderStatus::PENDING:
        strm << "PENDING";
        break;
    case qevercloud::PremiumOrderStatus::ACTIVE:
        strm << "ACTIVE";
        break;
    case qevercloud::PremiumOrderStatus::FAILED:
        strm << "FAILED";
        break;
    case qevercloud::PremiumOrderStatus::CANCELLATION_PENDING:
        strm << "CANCELLATION_PENDING";
        break;
    case qevercloud::PremiumOrderStatus::CANCELED:
        strm << "CANCELED";
        break;
    default:
        strm << "Unknown";
        break;
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::BusinessUserRole::type & role)
{
    strm << "qevercloud::BusinessUserRole: ";

    switch(role)
    {
    case qevercloud::BusinessUserRole::ADMIN:
        strm << "ADMIN";
        break;
    case qevercloud::BusinessUserRole::NORMAL:
        strm << "NORMAL";
        break;
    default:
        strm << "Unknown";
        break;
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SponsoredGroupRole::type & role)
{
    strm << "qevercloud::SponsoredGroupRole: ";

    switch (role)
    {
    case qevercloud::SponsoredGroupRole::GROUP_MEMBER:
        strm << "GROUP_MEMBER";
        break;
    case qevercloud::SponsoredGroupRole::GROUP_ADMIN:
        strm << "GROUP_ADMIN";
        break;
    case qevercloud::SponsoredGroupRole::GROUP_OWNER:
        strm << "GROUP_OWNER";
        break;
    default:
        strm << "Unknown";
        break;
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::Note & note)
{
    strm << "qevercloud::Note { \n";
    const char * indent = "  ";

    strm << indent << "guid = " << (note.guid.isSet() ? note.guid.ref() : "<empty>") << "; \n";
    strm << indent << "title = " << (note.title.isSet() ? note.title.ref() : "<empty>") << "; \n";
    strm << indent << "content = " << (note.content.isSet() ? note.content.ref() : "<empty>") << "; \n";
    strm << indent << "contentHash = " << (note.contentHash.isSet() ? note.contentHash.ref() : "<empty>") << "; \n";
    strm << indent << "created = " << (note.created.isSet()
                                       ? quentier::printableDateTimeFromTimestamp(note.created.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "updated = " << (note.updated.isSet()
                                       ? quentier::printableDateTimeFromTimestamp(note.updated.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "deleted = " << (note.deleted.isSet()
                                       ? quentier::printableDateTimeFromTimestamp(note.deleted.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "active = " << (note.active.isSet()
                                      ? (note.active.ref() ? "true" : "false")
                                      : "<empty>") << "; \n";
    strm << indent << "updateSequenceNum = " << (note.updateSequenceNum.isSet()
                                                 ? QString::number(note.updateSequenceNum.ref())
                                                 : "<empty>") << "; \n";
    strm << indent << "notebookGuid = " << (note.notebookGuid.isSet() ? note.notebookGuid.ref() : "<empty>") << "; \n";

    strm << indent << "tagGuids = ";
    if (note.tagGuids.isSet())
    {
        strm << "\n";
        const auto & tagGuids = note.tagGuids.ref();
        const int numTagGuids = tagGuids.size();
        for(int i = 0; i < numTagGuids; ++i) {
            strm << "tagGuid[" << i << "] = " << tagGuids[i] << "; ";
        }
        strm << "\n";
    }
    else
    {
        strm << "<empty>; \n";
    }

    strm << indent << "resources = ";
    if (note.resources.isSet())
    {
        strm << "\n";
        const auto & resources = note.resources.ref();
        const int numResources = resources.size();
        for(int i = 0; i < numResources; ++i) {
            strm << "resource[" << i << "] = " << resources[i] << "; ";
        }
        strm << "\n";
    }
    else
    {
        strm << "<empty>; \n";
    }

    strm << indent << "attributes = " << (note.attributes.isSet() ? ToString(note.attributes.ref()) : "<empty>") << "; \n";

    strm << indent << "tagNames = ";
    if (note.tagNames.isSet())
    {
        strm << "\n";
        const QStringList & tagNames = note.tagNames.ref();
        const int numTagNames = tagNames.size();
        for(int i = 0; i < numTagNames; ++i) {
            strm << "tagName[" << i << "] = " << tagNames[i] << "; ";
        }
        strm << "\n";
    }
    else {
        strm << "<empty>; \n";
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::EDAMErrorCode::type & obj)
{
    strm << "qevercloud::EDAMErrorCode: ";

    switch(obj)
    {
    case qevercloud::EDAMErrorCode::UNKNOWN:
        strm << "UNKNOWN";
        break;
    case qevercloud::EDAMErrorCode::BAD_DATA_FORMAT:
        strm << "BAD_DATA_FORMAT";
        break;
    case qevercloud::EDAMErrorCode::PERMISSION_DENIED:
        strm << "PERMISSION_DENIED";
        break;
    case qevercloud::EDAMErrorCode::INTERNAL_ERROR:
        strm << "INTERNAL_ERROR";
        break;
    case qevercloud::EDAMErrorCode::DATA_REQUIRED:
        strm << "DATA_REQUIRED";
        break;
    case qevercloud::EDAMErrorCode::LIMIT_REACHED:
        strm << "LIMIT_REACHED";
        break;
    case qevercloud::EDAMErrorCode::QUOTA_REACHED:
        strm << "QUOTA_REACHED";
        break;
    case qevercloud::EDAMErrorCode::INVALID_AUTH:
        strm << "INVALID_AUTH";
        break;
    case qevercloud::EDAMErrorCode::AUTH_EXPIRED:
        strm << "AUTH_EXPIRED";
        break;
    case qevercloud::EDAMErrorCode::DATA_CONFLICT:
        strm << "DATA_CONFLICT";
        break;
    case qevercloud::EDAMErrorCode::ENML_VALIDATION:
        strm << "ENML_VALIDATION";
        break;
    case qevercloud::EDAMErrorCode::SHARD_UNAVAILABLE:
        strm << "SHARD_UNAVAILABLE";
        break;
    case qevercloud::EDAMErrorCode::LEN_TOO_SHORT:
        strm << "LEN_TOO_SHORT";
        break;
    case qevercloud::EDAMErrorCode::LEN_TOO_LONG:
        strm << "LEN_TOO_LONG";
        break;
    case qevercloud::EDAMErrorCode::TOO_FEW:
        strm << "TOO_FEW";
        break;
    case qevercloud::EDAMErrorCode::TOO_MANY:
        strm << "TOO_MANY";
        break;
    case qevercloud::EDAMErrorCode::UNSUPPORTED_OPERATION:
        strm << "UNSUPPORTED_OPERATION";
        break;
    case qevercloud::EDAMErrorCode::TAKEN_DOWN:
        strm << "TAKEN_DOWN";
        break;
    case qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED:
        strm << "RATE_LIMIT_REACHED";
        break;
    default:
        strm << "<unknown>";
        break;
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::EvernoteOAuthWebView::OAuthResult & result)
{
    strm << "qevercloud::EvernoteOAuthWebView::OAuthResult {\n";

    strm << "noteStoreUrl = " << result.noteStoreUrl << "; \n";
    strm << "expires = " << quentier::printableDateTimeFromTimestamp(result.expires) << "; \n";
    strm << "shardId = " << result.shardId << "; \n";
    strm << "userId = " << QString::number(result.userId) << "; \n";
    strm << "webApiUrlPrefix = " << result.webApiUrlPrefix << "; \n";
    strm << "authenticationToken " << (result.authenticationToken.isEmpty() ? "is empty" : "is not empty") << "; \n";

    strm << "}; ";
    return strm;
}
