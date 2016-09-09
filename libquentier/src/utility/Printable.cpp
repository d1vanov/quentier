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

#define PRINT_FIELD(obj, field, ...) \
    strm << indent << QStringLiteral( #field " = ") << (obj.field.isSet() \
                                                        ? __VA_ARGS__(obj.field.ref()) \
                                                        : QStringLiteral("<empty>")) << QStringLiteral("; \n")

#define PRINT_LIST_FIELD(obj, field, ...) \
    strm << indent << QStringLiteral( #field ); \
    if (obj.field.isSet()) \
    { \
        strm << QStringLiteral(": { \n"); \
        const auto & field##List = obj.field.ref(); \
        const int num##field = field##List.size(); \
        for(int i = 0; i < num##field; ++i) { \
            strm << indent << indent << QStringLiteral("[") << i << QStringLiteral("]: ") \
                 << __VA_ARGS__(field##List[i]) << QStringLiteral(";\n"); \
        } \
        strm << indent << QStringLiteral("};\n"); \
    } \
    else \
    { \
        strm << QStringLiteral("<empty>;\n"); \
    }

#define PRINT_APPLICATION_DATA(obj) \
    if (obj.applicationData.isSet()) \
    { \
        const qevercloud::LazyMap & applicationData = obj.applicationData; \
        if (applicationData.keysOnly.isSet()) { \
            const QSet<QString> & keysOnly = applicationData.keysOnly; \
            strm << indent << QStringLiteral("applicationData: keys only: \n"); \
            for(auto it = keysOnly.begin(), end = keysOnly.end(); it != end; ++it) { \
                strm << *it << QStringLiteral("; "); \
            } \
            strm << QStringLiteral("\n"); \
        } \
        if (applicationData.fullMap.isSet()) { \
            const QMap<QString, QString> & fullMap = applicationData.fullMap; \
            strm << indent << QStringLiteral("applicationData: full map: \n"); \
            for(auto it = fullMap.begin(), end = fullMap.end(); it != end; ++it) { \
                strm << QStringLiteral("[") << it.key() << QStringLiteral("] = ") \
                     << it.value() << QStringLiteral("; "); \
            } \
            strm << QStringLiteral("\n"); \
        } \
    }

QString byteArrayToHex(const QByteArray & bytes)
{
    return bytes.toHex();
}

QString contactTypeToString(const qevercloud::ContactType::type & type)
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

    PRINT_FIELD(contact, name);
    PRINT_FIELD(contact, id);
    PRINT_FIELD(contact, type, contactTypeToString);
    PRINT_FIELD(contact, photoUrl);
    PRINT_FIELD(contact, photoLastUpdated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(contact, messagingPermit);
    PRINT_FIELD(contact, messagingPermitExpires, quentier::printableDateTimeFromTimestamp);

    strm << QStringLiteral("}; \n");
    return strm;
}

QString boolToString(const bool value) { return (value ? QStringLiteral("true") : QStringLiteral("false")); }

QTextStream & operator <<(QTextStream & strm, const qevercloud::Identity & identity)
{
    strm << QStringLiteral("qevercloud::Identity: {\n");
    const char * indent = "  ";

    strm << indent << QStringLiteral("id = ") << QString::number(identity.id) << QStringLiteral(";\n");

    PRINT_FIELD(identity, contact, ToString);
    PRINT_FIELD(identity, userId, QString::number);
    PRINT_FIELD(identity, deactivated, boolToString);
    PRINT_FIELD(identity, sameBusiness, boolToString);
    PRINT_FIELD(identity, blocked, boolToString);
    PRINT_FIELD(identity, userConnected, boolToString);
    PRINT_FIELD(identity, eventId, QString::number);

    strm << QStringLiteral("}; \n");
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::BusinessUserInfo & info)
{
    strm << QStringLiteral("qevercloud::BusinessUserInfo: {\n");
    const char * indent = "  ";

    PRINT_FIELD(info, businessId, QString::number);
    PRINT_FIELD(info, businessName);
    PRINT_FIELD(info, role, ToString);
    PRINT_FIELD(info, email);
    PRINT_FIELD(info, updated, quentier::printableDateTimeFromTimestamp);

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

    PRINT_FIELD(accounting, uploadLimitEnd, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, uploadLimitNextMonth, QString::number);
    PRINT_FIELD(accounting, premiumServiceStatus, premiumOrderStatusToString);
    PRINT_FIELD(accounting, premiumOrderNumber);
    PRINT_FIELD(accounting, premiumCommerceService);
    PRINT_FIELD(accounting, premiumServiceStart, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, premiumServiceSKU);
    PRINT_FIELD(accounting, lastSuccessfulCharge, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, lastFailedCharge, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, lastFailedChargeReason);
    PRINT_FIELD(accounting, nextPaymentDue, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, premiumLockUntil, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, updated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, premiumSubscriptionNumber);
    PRINT_FIELD(accounting, lastRequestedCharge, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, currency);
    PRINT_FIELD(accounting, unitPrice, QString::number);
    PRINT_FIELD(accounting, businessId, QString::number);
    PRINT_FIELD(accounting, businessRole, businessUserRoleToString);
    PRINT_FIELD(accounting, unitDiscount, QString::number);
    PRINT_FIELD(accounting, nextChargeDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(accounting, availablePoints, QString::number);

    strm << QStringLiteral("}; \n");
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::AccountLimits & limits)
{
    strm << QStringLiteral("qevercloud::AccountLimits: {\n");
    const char * indent = "  ";

    PRINT_FIELD(limits, userMailLimitDaily, QString::number);
    PRINT_FIELD(limits, noteSizeMax, QString::number);
    PRINT_FIELD(limits, resourceSizeMax, QString::number);
    PRINT_FIELD(limits, userLinkedNotebookMax, QString::number);
    PRINT_FIELD(limits, uploadLimit, QString::number);
    PRINT_FIELD(limits, userNoteCountMax, QString::number);
    PRINT_FIELD(limits, userNotebookCountMax, QString::number);
    PRINT_FIELD(limits, userTagCountMax, QString::number);
    PRINT_FIELD(limits, noteTagCountMax, QString::number);
    PRINT_FIELD(limits, userSavedSearchesMax, QString::number);
    PRINT_FIELD(limits, noteResourceCountMax, QString::number);

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

    PRINT_FIELD(attributes, defaultLocationName);
    PRINT_FIELD(attributes, defaultLatitude, QString::number);
    PRINT_FIELD(attributes, defaultLongitude, QString::number);
    PRINT_FIELD(attributes, preactivation, boolToString);
    PRINT_LIST_FIELD(attributes, viewedPromotions)
    PRINT_FIELD(attributes, incomingEmailAddress);
    PRINT_LIST_FIELD(attributes, recentMailedAddresses)
    PRINT_FIELD(attributes, comments);
    PRINT_FIELD(attributes, dateAgreedToTermsOfService, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, maxReferrals, QString::number);
    PRINT_FIELD(attributes, referralCount, QString::number);
    PRINT_FIELD(attributes, refererCode);
    PRINT_FIELD(attributes, sentEmailDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, dailyEmailLimit, QString::number);
    PRINT_FIELD(attributes, emailOptOutDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, partnerEmailOptInDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, preferredLanguage);
    PRINT_FIELD(attributes, preferredCountry);
    PRINT_FIELD(attributes, clipFullPage, boolToString);
    PRINT_FIELD(attributes, twitterUserName);
    PRINT_FIELD(attributes, twitterId);
    PRINT_FIELD(attributes, groupName);
    PRINT_FIELD(attributes, recognitionLanguage);
    PRINT_FIELD(attributes, referralProof);
    PRINT_FIELD(attributes, educationalDiscount, boolToString);
    PRINT_FIELD(attributes, businessAddress);
    PRINT_FIELD(attributes, hideSponsorBilling, boolToString);
    PRINT_FIELD(attributes, useEmailAutoFiling, boolToString);
    PRINT_FIELD(attributes, reminderEmailConfig, reminderEmailConfigToString);
    PRINT_FIELD(attributes, passwordUpdated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, salesforcePushEnabled, boolToString);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::NoteAttributes & attributes)
{
    strm << QStringLiteral("qevercloud::NoteAttributes: {\n");
    const char * indent = "  ";

    PRINT_FIELD(attributes, subjectDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, latitude, QString::number);
    PRINT_FIELD(attributes, longitude, QString::number);
    PRINT_FIELD(attributes, altitude, QString::number);
    PRINT_FIELD(attributes, author);
    PRINT_FIELD(attributes, source);
    PRINT_FIELD(attributes, sourceURL);
    PRINT_FIELD(attributes, sourceApplication);
    PRINT_FIELD(attributes, shareDate, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, reminderOrder, QString::number);
    PRINT_FIELD(attributes, reminderDoneTime, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, reminderTime, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, placeName);
    PRINT_FIELD(attributes, contentClass);
    PRINT_FIELD(attributes, lastEditedBy);
    PRINT_FIELD(attributes, creatorId, QString::number);
    PRINT_FIELD(attributes, lastEditorId, QString::number);

    PRINT_APPLICATION_DATA(attributes)

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

    PRINT_FIELD(restrictions, noReadNotes, boolToString);
    PRINT_FIELD(restrictions, noCreateNotes, boolToString);
    PRINT_FIELD(restrictions, noUpdateNotes, boolToString);
    PRINT_FIELD(restrictions, noExpungeNotes, boolToString);
    PRINT_FIELD(restrictions, noShareNotes, boolToString);
    PRINT_FIELD(restrictions, noEmailNotes, boolToString);
    PRINT_FIELD(restrictions, noSendMessageToRecipients, boolToString);
    PRINT_FIELD(restrictions, noUpdateNotebook, boolToString);
    PRINT_FIELD(restrictions, noExpungeNotebook, boolToString);
    PRINT_FIELD(restrictions, noSetDefaultNotebook, boolToString);
    PRINT_FIELD(restrictions, noSetNotebookStack, boolToString);
    PRINT_FIELD(restrictions, noPublishToPublic, boolToString);
    PRINT_FIELD(restrictions, noPublishToBusinessLibrary, boolToString);
    PRINT_FIELD(restrictions, noCreateTags, boolToString);
    PRINT_FIELD(restrictions, noUpdateTags, boolToString);
    PRINT_FIELD(restrictions, noExpungeTags, boolToString);
    PRINT_FIELD(restrictions, noSetParentTag, boolToString);
    PRINT_FIELD(restrictions, noCreateSharedNotebooks, boolToString);
    PRINT_FIELD(restrictions, updateWhichSharedNotebookRestrictions, sharedNotebookInstanceRestrictionsToString);
    PRINT_FIELD(restrictions, expungeWhichSharedNotebookRestrictions, sharedNotebookInstanceRestrictionsToString);

    strm << QStringLiteral("};\n");

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::ResourceAttributes & attributes)
{
    strm << QStringLiteral("ResourceAttributes: {\n");
    const char * indent = "  ";

    PRINT_FIELD(attributes, sourceURL);
    PRINT_FIELD(attributes, timestamp, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(attributes, latitude, QString::number);
    PRINT_FIELD(attributes, longitude, QString::number);
    PRINT_FIELD(attributes, altitude, QString::number);
    PRINT_FIELD(attributes, cameraMake);
    PRINT_FIELD(attributes, cameraModel);
    PRINT_FIELD(attributes, clientWillIndex, boolToString);
    PRINT_FIELD(attributes, fileName);
    PRINT_FIELD(attributes, attachment, boolToString);
    PRINT_APPLICATION_DATA(attributes)

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::Resource & resource)
{
    strm << QStringLiteral("qevercloud::Resource {\n");
    const char * indent = "  ";

    PRINT_FIELD(resource, guid);
    PRINT_FIELD(resource, updateSequenceNum, QString::number);
    PRINT_FIELD(resource, noteGuid);

    if (resource.data.isSet())
    {
        strm << indent << QStringLiteral("Data: {\n");

        PRINT_FIELD(resource.data.ref(), size, QString::number);
        PRINT_FIELD(resource.data.ref(), bodyHash, byteArrayToHex);

        strm << indent << QStringLiteral("body: ") << (resource.data->body.isSet()
                                                       ? QStringLiteral("is set")
                                                       : QStringLiteral("is not set")) << QStringLiteral(";\n");

        strm << indent << QStringLiteral("};\n");
    }

    PRINT_FIELD(resource, mime);
    PRINT_FIELD(resource, width, QString::number);
    PRINT_FIELD(resource, height, QString::number);

    if (resource.recognition.isSet())
    {
        strm << indent << QStringLiteral("Recognition data: {\n");

        PRINT_FIELD(resource.recognition.ref(), size, QString::number);
        PRINT_FIELD(resource.recognition.ref(), bodyHash, byteArrayToHex);
        PRINT_FIELD(resource.recognition.ref(), body);

        strm << indent << QStringLiteral("};\n");
    }

    if (resource.alternateData.isSet())
    {
        strm << indent << QStringLiteral("Alternate data: {\n");

        PRINT_FIELD(resource.alternateData.ref(), size, QString::number);
        PRINT_FIELD(resource.alternateData.ref(), bodyHash, byteArrayToHex);

        strm << indent << QStringLiteral("body: ") << (resource.alternateData->body.isSet()
                                                       ? QStringLiteral("is set")
                                                       : QStringLiteral("is not set")) << QStringLiteral(";\n");

        strm << indent << QStringLiteral("};\n");
    }

    if (resource.attributes.isSet()) {
        strm << indent << resource.attributes.ref();
    }

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SyncChunk & syncChunk)
{
    strm << QStringLiteral("qevercloud::SyncChunk: {\n");
    const char * indent = "  ";

    PRINT_FIELD(syncChunk, chunkHighUSN, QString::number);

    strm << indent << QStringLiteral("currentTime = ") << quentier::printableDateTimeFromTimestamp(syncChunk.currentTime)
         << QStringLiteral(";\n");
    strm << indent << QStringLiteral("updateCount = ") << syncChunk.updateCount << QStringLiteral(";\n");

    if (syncChunk.expungedLinkedNotebooks.isSet())
    {
        for(auto it = syncChunk.expungedLinkedNotebooks->begin(), end = syncChunk.expungedLinkedNotebooks->end(); it != end; ++it) {
            strm << indent << QStringLiteral("expunged linked notebook guid = ") << *it << QStringLiteral(";\n");
        }
    }

    if (syncChunk.expungedNotebooks.isSet())
    {
        for(auto it = syncChunk.expungedNotebooks->begin(), end = syncChunk.expungedNotebooks->end(); it != end; ++it) {
            strm << indent << QStringLiteral("expunged notebook guid = ") << *it << QStringLiteral(";\n");
        }
    }

    if (syncChunk.expungedNotes.isSet())
    {
        for(auto it = syncChunk.expungedNotes->begin(), end = syncChunk.expungedNotes->end(); it != end; ++it) {
            strm << indent << QStringLiteral("expunged note guid = ") << *it << QStringLiteral(";\n");
        }
    }

    if (syncChunk.expungedSearches.isSet())
    {
        for(auto it = syncChunk.expungedSearches->begin(), end = syncChunk.expungedSearches->end(); it != end; ++it) {
            strm << indent << QStringLiteral("expunged search guid = ") << *it << QStringLiteral(";\n");
        }
    }

    if (syncChunk.expungedTags.isSet())
    {
        for(auto it = syncChunk.expungedTags->begin(), end = syncChunk.expungedTags->end(); it != end; ++it) {
            strm << indent << QStringLiteral("expunged tag guid = ") << *it << QStringLiteral(";\n");
        }
    }

    if (syncChunk.linkedNotebooks.isSet())
    {
        for(auto it = syncChunk.linkedNotebooks->begin(), end = syncChunk.linkedNotebooks->end(); it != end; ++it) {
            strm << indent << QStringLiteral(", linked notebook: ") << *it << QStringLiteral(";\n");
        }
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::Tag & tag)
{
    strm << QStringLiteral("qevercloud::Tag: {\n");
    const char * indent = "  ";

    PRINT_FIELD(tag, guid);
    PRINT_FIELD(tag, name);
    PRINT_FIELD(tag, parentGuid);
    PRINT_FIELD(tag, updateSequenceNum, QString::number);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SavedSearch & savedSearch)
{
    strm << QStringLiteral("qevercloud::SavedSearch: {\n");
    const char * indent = "  ";

    PRINT_FIELD(savedSearch, guid);
    PRINT_FIELD(savedSearch, name);
    PRINT_FIELD(savedSearch, query);
    PRINT_FIELD(savedSearch, format, queryFormatToString);
    PRINT_FIELD(savedSearch, updateSequenceNum, QString::number);

    strm << indent << QStringLiteral("SavedSearchScope = ");
    if (savedSearch.scope.isSet())
    {
        strm << QStringLiteral("{\n");
        strm << indent << indent << QStringLiteral("includeAccount = ")
             << (savedSearch.scope->includeAccount ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(";\n");
        strm << indent << indent << QStringLiteral("includePersonalLinkedNotebooks = ")
             << (savedSearch.scope->includePersonalLinkedNotebooks ? QStringLiteral("true") : QStringLiteral("false"))
             << QStringLiteral(";\n");
        strm << indent << indent << QStringLiteral("includeBusinessLinkedNotebooks = ")
             << (savedSearch.scope->includeBusinessLinkedNotebooks ? QStringLiteral("true") : QStringLiteral("false"))
             << QStringLiteral(";\n");
        strm << indent << QStringLiteral("};\n");
    }
    else
    {
        strm << QStringLiteral("<empty>;\n");
    }

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::LinkedNotebook & linkedNotebook)
{
    strm << QStringLiteral("qevercloud::LinkedNotebook: {\n");
    const char * indent = "  ";

    PRINT_FIELD(linkedNotebook, shareName);
    PRINT_FIELD(linkedNotebook, username);
    PRINT_FIELD(linkedNotebook, shardId);
    PRINT_FIELD(linkedNotebook, uri);
    PRINT_FIELD(linkedNotebook, guid);
    PRINT_FIELD(linkedNotebook, updateSequenceNum, QString::number);
    PRINT_FIELD(linkedNotebook, noteStoreUrl);
    PRINT_FIELD(linkedNotebook, webApiUrlPrefix);
    PRINT_FIELD(linkedNotebook, stack);
    PRINT_FIELD(linkedNotebook, businessId, QString::number);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::Notebook & notebook)
{
    strm << QStringLiteral("qevercloud::Notebook: {\n");
    const char * indent = "  ";

    PRINT_FIELD(notebook, guid);
    PRINT_FIELD(notebook, name);
    PRINT_FIELD(notebook, updateSequenceNum, QString::number);
    PRINT_FIELD(notebook, defaultNotebook, boolToString);
    PRINT_FIELD(notebook, serviceCreated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(notebook, serviceUpdated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(notebook, publishing, ToString);
    PRINT_FIELD(notebook, published, boolToString);
    PRINT_FIELD(notebook, stack);

    if (notebook.sharedNotebooks.isSet())
    {
        strm << indent << QStringLiteral("sharedNotebooks: {\n");

        for(auto it = notebook.sharedNotebooks->begin(), end = notebook.sharedNotebooks->end(); it != end; ++it) {
            strm << indent << indent << *it;
        }

        strm << indent << QStringLiteral("};\n");
    }

    PRINT_FIELD(notebook, businessNotebook, ToString);
    PRINT_FIELD(notebook, contact, ToString);
    PRINT_FIELD(notebook, restrictions, ToString);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::Publishing & publishing)
{
    strm << QStringLiteral("qevercloud::Publishing: {\n");
    const char * indent = "  ";

    PRINT_FIELD(publishing, uri);
    PRINT_FIELD(publishing, order, noteSortOrderToString);
    PRINT_FIELD(publishing, ascending, boolToString);
    PRINT_FIELD(publishing, publicDescription);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SharedNotebook & sharedNotebook)
{
    strm << QStringLiteral("qevercloud::SharedNotebook: {\n");
    const char * indent = "  ";

    PRINT_FIELD(sharedNotebook, id, QString::number);
    PRINT_FIELD(sharedNotebook, userId, QString::number);
    PRINT_FIELD(sharedNotebook, notebookGuid);
    PRINT_FIELD(sharedNotebook, email);
    PRINT_FIELD(sharedNotebook, serviceCreated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(sharedNotebook, serviceUpdated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(sharedNotebook, privilege, sharedNotebookPrivilegeLevelToString);
    PRINT_FIELD(sharedNotebook, recipientSettings, ToString);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::BusinessNotebook & businessNotebook)
{
    strm << QStringLiteral("qevercloud::BusinessNotebook: {\n");
    const char * indent = "  ";

    PRINT_FIELD(businessNotebook, notebookDescription);
    PRINT_FIELD(businessNotebook, privilege, sharedNotebookPrivilegeLevelToString);
    PRINT_FIELD(businessNotebook, recommended, boolToString);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::User & user)
{
    strm << QStringLiteral("qevercloud::User: {\n");
    const char * indent = "  ";

    PRINT_FIELD(user, id, QString::number);
    PRINT_FIELD(user, username);
    PRINT_FIELD(user, email);
    PRINT_FIELD(user, name);
    PRINT_FIELD(user, timezone);
    PRINT_FIELD(user, privilege, privilegeLevelToString);
    PRINT_FIELD(user, created, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(user, updated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(user, deleted, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(user, active, boolToString);
    PRINT_FIELD(user, attributes, ToString);
    PRINT_FIELD(user, accounting, ToString);
    PRINT_FIELD(user, businessUserInfo, ToString);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SharedNotebookRecipientSettings & settings)
{
    strm << QStringLiteral("qevercloud::SharedNotebookRecipientSettings: {\n");
    const char * indent = "  ";

    PRINT_FIELD(settings, reminderNotifyEmail, boolToString);
    PRINT_FIELD(settings, reminderNotifyInApp, boolToString);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::ReminderEmailConfig::type & config)
{
    strm << reminderEmailConfigToString(config);
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::PremiumOrderStatus::type & status)
{
    strm << premiumOrderStatusToString(status);
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::BusinessUserRole::type & role)
{
    strm << businessUserRoleToString(role);
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SponsoredGroupRole::type & role)
{
    strm << QStringLiteral("qevercloud::SponsoredGroupRole: ");

    switch (role)
    {
    case qevercloud::SponsoredGroupRole::GROUP_MEMBER:
        strm << QStringLiteral("GROUP_MEMBER");
        break;
    case qevercloud::SponsoredGroupRole::GROUP_ADMIN:
        strm << QStringLiteral("GROUP_ADMIN");
        break;
    case qevercloud::SponsoredGroupRole::GROUP_OWNER:
        strm << QStringLiteral("GROUP_OWNER");
        break;
    default:
        strm << QStringLiteral("Unknown");
        break;
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::NoteRestrictions & restrictions)
{
    strm << QStringLiteral("qevercloud::NoteRestrictions {\n");
    const char * indent = "  ";

    PRINT_FIELD(restrictions, noUpdateTitle, boolToString);
    PRINT_FIELD(restrictions, noUpdateContent, boolToString);
    PRINT_FIELD(restrictions, noEmail, boolToString);
    PRINT_FIELD(restrictions, noShare, boolToString);
    PRINT_FIELD(restrictions, noSharePublicly, boolToString);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::NoteLimits & limits)
{
    strm << QStringLiteral("qevercloud::NoteLimits {\n");
    const char * indent = "  ";

    PRINT_FIELD(limits, noteResourceCountMax, QString::number);
    PRINT_FIELD(limits, uploadLimit, QString::number);
    PRINT_FIELD(limits, resourceSizeMax, QString::number);
    PRINT_FIELD(limits, noteSizeMax, QString::number);
    PRINT_FIELD(limits, uploaded, QString::number);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::Note & note)
{
    strm << QStringLiteral("qevercloud::Note {\n");
    const char * indent = "  ";

    PRINT_FIELD(note, guid);
    PRINT_FIELD(note, title);
    PRINT_FIELD(note, content);
    PRINT_FIELD(note, contentHash);
    PRINT_FIELD(note, created, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(note, updated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(note, deleted, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(note, active, boolToString);
    PRINT_FIELD(note, updateSequenceNum, QString::number);
    PRINT_FIELD(note, notebookGuid);
    PRINT_LIST_FIELD(note, tagGuids)
    PRINT_LIST_FIELD(note, resources, ToString)
    PRINT_FIELD(note, attributes, ToString);
    PRINT_LIST_FIELD(note, tagNames)
    PRINT_LIST_FIELD(note, sharedNotes)
    PRINT_FIELD(note, restrictions, ToString);
    PRINT_FIELD(note, limits, ToString);

    strm << QStringLiteral("};\n");
    return strm;
}

QString sharedNotePrivilegeLevelToString(const qevercloud::SharedNotePrivilegeLevel::type level)
{
    switch(level)
    {
    case qevercloud::SharedNotePrivilegeLevel::READ_NOTE:
        return QStringLiteral("READ_NOTE");
    case qevercloud::SharedNotePrivilegeLevel::MODIFY_NOTE:
        return QStringLiteral("MODIFY_NOTE");
    case qevercloud::SharedNotePrivilegeLevel::FULL_ACCESS:
        return QStringLiteral("FULL_ACCESS");
    default:
        return QStringLiteral("Unknown");
    }
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::SharedNote & sharedNote)
{
    strm << QStringLiteral("qevercloud::SharedNote: {\n");
    const char * indent = "  ";

    PRINT_FIELD(sharedNote, sharerUserID, QString::number);
    PRINT_FIELD(sharedNote, recipientIdentity, ToString);
    PRINT_FIELD(sharedNote, privilege, sharedNotePrivilegeLevelToString);
    PRINT_FIELD(sharedNote, serviceCreated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(sharedNote, serviceUpdated, quentier::printableDateTimeFromTimestamp);
    PRINT_FIELD(sharedNote, serviceAssigned, quentier::printableDateTimeFromTimestamp);

    strm << QStringLiteral("};\n");
    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::EDAMErrorCode::type & obj)
{
    strm << QStringLiteral("qevercloud::EDAMErrorCode: ");

    switch(obj)
    {
    case qevercloud::EDAMErrorCode::UNKNOWN:
        strm << QStringLiteral("UNKNOWN");
        break;
    case qevercloud::EDAMErrorCode::BAD_DATA_FORMAT:
        strm << QStringLiteral("BAD_DATA_FORMAT");
        break;
    case qevercloud::EDAMErrorCode::PERMISSION_DENIED:
        strm << QStringLiteral("PERMISSION_DENIED");
        break;
    case qevercloud::EDAMErrorCode::INTERNAL_ERROR:
        strm << QStringLiteral("INTERNAL_ERROR");
        break;
    case qevercloud::EDAMErrorCode::DATA_REQUIRED:
        strm << QStringLiteral("DATA_REQUIRED");
        break;
    case qevercloud::EDAMErrorCode::LIMIT_REACHED:
        strm << QStringLiteral("LIMIT_REACHED");
        break;
    case qevercloud::EDAMErrorCode::QUOTA_REACHED:
        strm << QStringLiteral("QUOTA_REACHED");
        break;
    case qevercloud::EDAMErrorCode::INVALID_AUTH:
        strm << QStringLiteral("INVALID_AUTH");
        break;
    case qevercloud::EDAMErrorCode::AUTH_EXPIRED:
        strm << QStringLiteral("AUTH_EXPIRED");
        break;
    case qevercloud::EDAMErrorCode::DATA_CONFLICT:
        strm << QStringLiteral("DATA_CONFLICT");
        break;
    case qevercloud::EDAMErrorCode::ENML_VALIDATION:
        strm << QStringLiteral("ENML_VALIDATION");
        break;
    case qevercloud::EDAMErrorCode::SHARD_UNAVAILABLE:
        strm << QStringLiteral("SHARD_UNAVAILABLE");
        break;
    case qevercloud::EDAMErrorCode::LEN_TOO_SHORT:
        strm << QStringLiteral("LEN_TOO_SHORT");
        break;
    case qevercloud::EDAMErrorCode::LEN_TOO_LONG:
        strm << QStringLiteral("LEN_TOO_LONG");
        break;
    case qevercloud::EDAMErrorCode::TOO_FEW:
        strm << QStringLiteral("TOO_FEW");
        break;
    case qevercloud::EDAMErrorCode::TOO_MANY:
        strm << QStringLiteral("TOO_MANY");
        break;
    case qevercloud::EDAMErrorCode::UNSUPPORTED_OPERATION:
        strm << QStringLiteral("UNSUPPORTED_OPERATION");
        break;
    case qevercloud::EDAMErrorCode::TAKEN_DOWN:
        strm << QStringLiteral("TAKEN_DOWN");
        break;
    case qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED:
        strm << QStringLiteral("RATE_LIMIT_REACHED");
        break;
    default:
        strm << QStringLiteral("<unknown>");
        break;
    }

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const qevercloud::EvernoteOAuthWebView::OAuthResult & result)
{
    strm << QStringLiteral("qevercloud::EvernoteOAuthWebView::OAuthResult {\n");

    strm << QStringLiteral("  noteStoreUrl = ") << result.noteStoreUrl << QStringLiteral(";\n");
    strm << QStringLiteral("  expires = ") << quentier::printableDateTimeFromTimestamp(result.expires) << QStringLiteral(";\n");
    strm << QStringLiteral("  shardId = ") << result.shardId << QStringLiteral(";\n");
    strm << QStringLiteral("  userId = ") << QString::number(result.userId) << QStringLiteral(";\n");
    strm << QStringLiteral("  webApiUrlPrefix = ") << result.webApiUrlPrefix << QStringLiteral(";\n");
    strm << QStringLiteral("  authenticationToken ") << (result.authenticationToken.isEmpty()
                                                         ? QStringLiteral("is empty")
                                                         : QStringLiteral("is not empty")) << QStringLiteral(";\n");

    strm << QStringLiteral("};\n");
    return strm;
}
