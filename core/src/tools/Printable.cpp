#include "Printable.h"

namespace qute_note {

const QString Printable::ToQString() const
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << *this;
    return str;
}

QDebug & operator <<(QDebug & debug, const Printable & printable)
{
    debug << printable.ToQString();
    return debug;
}

QTextStream & operator <<(QTextStream & strm,
                          const Printable & printable)
{
    return printable.Print(strm);
}

} // namespace qute_note

#define CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(object, name, ...) \
    bool isSet##name = object.name.isSet(); \
    strm << #name " is " << (isSet##name ? "set" : "not set") << "\n"; \
    if (isSet##name) { \
        strm << #name " = " << __VA_ARGS__(object.name) << "\n"; \
    }


#define CHECK_AND_PRINT_ATTRIBUTE(object, name, ...) \
    bool isSet##name = isSet.name; \
    strm << #name " is " << (isSet##name ? "set" : "not set") << "\n"; \
    if (isSet##name) { \
        strm << #name " = " << __VA_ARGS__(object.name) << "\n"; \
    }

QTextStream & operator <<(QTextStream & strm, const qevercloud::BusinessUserInfo & info)
{
    strm << "BusinessUserInfo: {\n";

    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(info, businessId);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(info, businessName);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(info, role, static_cast<quint8>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(info, email);

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::PremiumInfo & info)
{
    strm << "PremiumUserInfo {\n";

    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(info, premiumExpirationDate, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(info, sponsoredGroupName);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(info, sponsoredGroupRole, static_cast<quint8>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(info, premiumUpgradable);

    strm << "premiumExtendable = " << info.premiumExtendable << "\n";
    strm << "premiumPending = " << info.premiumPending << "\n";
    strm << "premiumCancellationPending = " << info.premiumCancellationPending << "\n";
    strm << "canPurchaseUploadAllowance = " << info.canPurchaseUploadAllowance << "\n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::Accounting & accounting)
{
    strm << "Accounting: {\n";

    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, uploadLimit, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, uploadLimitEnd, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, uploadLimitNextMonth, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, premiumServiceStatus, static_cast<quint8>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, premiumOrderNumber);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, premiumCommerceService);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, premiumServiceStart, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, premiumServiceSKU);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, lastSuccessfulCharge, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, lastFailedCharge, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, lastFailedChargeReason);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, nextPaymentDue, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, premiumLockUntil, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, updated, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, premiumSubscriptionNumber);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, lastRequestedCharge, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, currency);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, unitPrice, static_cast<qint32>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, businessId, static_cast<qint32>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, businessName);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, businessRole, static_cast<quint8>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, unitDiscount, static_cast<qint32>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(accounting, nextChargeDate, static_cast<qint64>);

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::UserAttributes & attributes)
{
    strm << "UserAttributes: {\n";

    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, defaultLocationName);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, defaultLatitude);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, defaultLongitude);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, preactivation);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, incomingEmailAddress);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, comments);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, dateAgreedToTermsOfService, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, maxReferrals);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, referralCount);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, refererCode);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, sentEmailDate, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, sentEmailCount);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, dailyEmailLimit);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, emailOptOutDate, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, partnerEmailOptInDate, static_cast<qint64>);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, preferredLanguage);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, preferredCountry);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, clipFullPage);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, twitterUserName);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, twitterId);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, groupName);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, recognitionLanguage);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, referralProof);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, educationalDiscount);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, businessAddress);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, hideSponsorBilling);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, taxExempt);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, useEmailAutoFiling);
    CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE(attributes, reminderEmailConfig, static_cast<quint8>);

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::NoteAttributes & attributes)
{
    strm << "NoteAttributes: {\n";

    const auto & isSet = attributes.__isset;

    CHECK_AND_PRINT_ATTRIBUTE(attributes, subjectDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, latitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, longitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, altitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, author, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, source, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceURL, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceApplication, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, shareDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderOrder, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderDoneTime, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderTime, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, placeName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, contentClass, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, lastEditedBy, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, creatorId, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, lastEditorId, static_cast<qint32>);

    strm << "applicationData is " << (isSet.applicationData ? "set" : "not set") << "\n";
    if (isSet.applicationData)
    {
        const auto & applicationData = attributes.applicationData;
        const auto & keysOnly = applicationData.keysOnly;
        const auto & fullMap = applicationData.fullMap;

        strm << "ApplicationData: keys only: \n";
        for(const auto & key: keysOnly) {
            strm << QString::fromStdString(key) << "; ";
        }
        strm << "\n";

        strm << "ApplicationData: full map: \n";
        for(const auto & pair: fullMap) {
            strm << "(" << QString::fromStdString(pair.first)
                 << ", " << QString::fromStdString(pair.second) << "); ";
        }
        strm << "\n";
    }

    strm << "classificatiopns are " << (isSet.classifications ? "set" : "not set") << "\n";
    if (isSet.classifications)
    {
        const auto & classifications = attributes.classifications;
        strm << "Classifications: \n";
        for(const auto & pair: classifications) {
            strm << "(" << QString::fromStdString(pair.first)
                 << ", " << QString::fromStdString(pair.second) << "); ";
        }
        strm << "\n";
    }

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::ResourceAttributes & attributes)
{
    strm << "ResourceAttributes: {\n";

    const auto & isSet = attributes.__isset;

    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceURL, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, timestamp, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, latitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, longitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, altitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, cameraMake, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, cameraModel, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, clientWillIndex);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, recoType, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, fileName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, attachment);

    strm << "applicationData is " << (isSet.applicationData ? "set" : "not set") << "\n";
    if (isSet.applicationData)
    {
        const auto & applicationData = attributes.applicationData;
        const auto & keysOnly = applicationData.keysOnly;
        const auto & fullMap = applicationData.fullMap;

        strm << "ApplicationData: keys only: \n";
        for(const auto & key: keysOnly) {
            strm << QString::fromStdString(key) << "; ";
        }
        strm << "\n";

        strm << "ApplicationData: full map: \n";
        for(const auto & pair: fullMap) {
            strm << "(" << QString::fromStdString(pair.first)
                 << ", " << QString::fromStdString(pair.second) << "); ";
        }
        strm << "\n";
    }

    strm << "}; \n";
    return strm;
}

#undef CHECK_AND_PRINT_ATTRIBUTE
#undef CHECK_AND_PRINT_QEVERCLOUD_ATTRIBUTE

QTextStream & operator <<(QTextStream & strm, const evernote::edam::PrivilegeLevel::type & level)
{
    strm << "PrivilegeLevel: ";

    switch (level)
    {
    case evernote::edam::PrivilegeLevel::NORMAL:
        strm << "NORMAL";
        break;
    case evernote::edam::PrivilegeLevel::PREMIUM:
        strm << "PREMIUM";
        break;
    case evernote::edam::PrivilegeLevel::VIP:
        strm << "VIP";
        break;
    case evernote::edam::PrivilegeLevel::MANAGER:
        strm << "MANAGER";
        break;
    case evernote::edam::PrivilegeLevel::SUPPORT:
        strm << "SUPPORT";
        break;
    case evernote::edam::PrivilegeLevel::ADMIN:
        strm << "ADMIN";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::QueryFormat::type & format)
{
    switch (format)
    {
    case evernote::edam::QueryFormat::USER:
        strm << "USER";
        break;
    case evernote::edam::QueryFormat::SEXP:
        strm << "SEXP";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::Guid & guid)
{
    strm << QString::fromStdString(guid);
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::SharedNotebookPrivilegeLevel::type & privilege)
{
    switch(privilege)
    {
    case evernote::edam::SharedNotebookPrivilegeLevel::READ_NOTEBOOK:
        strm << "READ_NOTEBOOK";
        break;
    case evernote::edam::SharedNotebookPrivilegeLevel::MODIFY_NOTEBOOK_PLUS_ACTIVITY:
        strm << "MODIFY_NOTEBOOK_PLUS_ACTIVITY";
        break;
    case evernote::edam::SharedNotebookPrivilegeLevel::READ_NOTEBOOK_PLUS_ACTIVITY:
        strm << "READ_NOTEBOOK_PLUS_ACTIVITY";
        break;
    case evernote::edam::SharedNotebookPrivilegeLevel::GROUP:
        strm << "GROUP";
        break;
    case evernote::edam::SharedNotebookPrivilegeLevel::FULL_ACCESS:
        strm << "FULL_ACCESS";
        break;
    case evernote::edam::SharedNotebookPrivilegeLevel::BUSINESS_FULL_ACCESS:
        strm << "BUSINESS_FULL_ACCESS";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::NoteSortOrder::type & order)
{
    switch(order)
    {
    case evernote::edam::NoteSortOrder::CREATED:
        strm << "CREATED";
        break;
    case evernote::edam::NoteSortOrder::RELEVANCE:
        strm << "RELEVANCE";
        break;
    case evernote::edam::NoteSortOrder::TITLE:
        strm << "TITLE";
        break;
    case evernote::edam::NoteSortOrder::UPDATED:
        strm << "UPDATED";
        break;
    case evernote::edam::NoteSortOrder::UPDATE_SEQUENCE_NUMBER:
        strm << "UPDATE_SEQUENCE_NUMBER";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::NotebookRestrictions & restrictions)
{
    const auto & isSet = restrictions.__isset;

    strm << "{ \n";

#define INSERT_DELIMITER \
    strm << "; \n"

    if (isSet.noReadNotes) {
        strm << "noReadNotes: " << (restrictions.noReadNotes ? "true" : "false");
    }
    else {
        strm << "noReadNotes is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noCreateNotes) {
        strm << "noCreateNotes: " << (restrictions.noCreateNotes ? "true" : "false");
    }
    else {
        strm << "noCreateNotes is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noUpdateNotes) {
        strm << "noUpdateNotes: " << (restrictions.noUpdateNotes ? "true" : "false");
    }
    else {
        strm << "noUpdateNotes is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noExpungeNotes) {
        strm << "noExpungeNotes: " << (restrictions.noExpungeNotes ? "true" : "false");
    }
    else {
        strm << "noExpungeNotes is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noShareNotes) {
        strm << "noShareNotes: " << (restrictions.noShareNotes ? "true" : "false");
    }
    else {
        strm << "noShareNotes is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noEmailNotes) {
        strm << "noEmailNotes: " << (restrictions.noEmailNotes ? "true" : "false");
    }
    else {
        strm << "noEmailNotes is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noSendMessageToRecipients) {
        strm << "noSendMessageToRecipients: " << (restrictions.noSendMessageToRecipients ? "true" : "false");
    }
    else {
        strm << "noSendMessageToRecipients is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noUpdateNotebook) {
        strm << "noUpdateNotebook: " << (restrictions.noUpdateNotebook ? "true" : "false");
    }
    else {
        strm << "noUpdateNotebook is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noExpungeNotebook) {
        strm << "noExpungeNotebook: " << (restrictions.noExpungeNotebook ? "true" : "false");
    }
    else {
        strm << "noExpungeNotebook is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noSetDefaultNotebook) {
        strm << "noSetDefaultNotebook: " << (restrictions.noSetDefaultNotebook ? "true" : "false");
    }
    else {
        strm << "noSetDefaultNotebook is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noSetNotebookStack) {
        strm << "noSetNotebookStack: " << (restrictions.noSetNotebookStack ? "true" : "false");
    }
    else {
        strm << "noSetNotebookStack is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noPublishToPublic) {
        strm << "noPublishToPublic: " << (restrictions.noPublishToPublic ? "true" : "false");
    }
    else {
        strm << "noPublishToPublic is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noPublishToBusinessLibrary) {
        strm << "noPublishToBusinessLibrary: " << (restrictions.noPublishToBusinessLibrary ? "true" : "false");
    }
    else {
        strm << "noPublishToBusinessLibrary is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noCreateTags) {
        strm << "noCreateTags: " << (restrictions.noCreateTags ? "true" : "false");
    }
    else {
        strm << "noCreateTags is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noUpdateTags) {
        strm << "noUpdateTags: " << (restrictions.noUpdateTags ? "true" : "false");
    }
    else {
        strm << "noUpdateTags is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noExpungeTags) {
        strm << "noExpungeTags: " << (restrictions.noExpungeTags ? "true" : "false");
    }
    else {
        strm << "noExpungeTags is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noSetParentTag) {
        strm << "noSetParentTag: " << (restrictions.noSetParentTag ? "true" : "false");
    }
    else {
        strm << "noSetParentTag is not set";
    }
    INSERT_DELIMITER;

    if (isSet.noCreateSharedNotebooks) {
        strm << "noCreateSharedNotebooks: " << (restrictions.noCreateSharedNotebooks ? "true" : "false");
    }
    else {
        strm << "noCreateSharedNotebooks is not set";
    }
    INSERT_DELIMITER;

    if (isSet.updateWhichSharedNotebookRestrictions) {
        strm << "updateWhichSharedNotebookRestrictions: " << restrictions.updateWhichSharedNotebookRestrictions;
    }
    else {
        strm << "updateWhichSharedNotebookRestrictions is not set";
    }
    INSERT_DELIMITER;

    if (isSet.expungeWhichSharedNotebookRestrictions) {
        strm << "expungeWhichSharedNotebookRestrictions: " << restrictions.expungeWhichSharedNotebookRestrictions;
    }
    else {
        strm << "expungeWhichSharedNotebookRestrictions is not set";
    }
    INSERT_DELIMITER;

#undef INSERT_DELIMITER

    strm << "}; \n";

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::SharedNotebookInstanceRestrictions::type & restrictions)
{
    switch(restrictions)
    {
    case evernote::edam::SharedNotebookInstanceRestrictions::NO_SHARED_NOTEBOOKS:
        strm << "NO_SHARED_NOTEBOOKS";
        break;
    case evernote::edam::SharedNotebookInstanceRestrictions::ONLY_JOINED_OR_PREVIEW:
        strm << "ONLY_JOINED_OR_PREVIEW";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}
