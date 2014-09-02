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

#define CHECK_AND_PRINT_ATTRIBUTE(object, name, ...) \
    bool isSet##name = object.name.isSet(); \
    strm << #name " is " << (isSet##name ? "set" : "not set") << "\n"; \
    if (isSet##name) { \
        strm << #name " = " << __VA_ARGS__(object.name) << "\n"; \
    }

QTextStream & operator <<(QTextStream & strm, const qevercloud::BusinessUserInfo & info)
{
    strm << "BusinessUserInfo: {\n";

    CHECK_AND_PRINT_ATTRIBUTE(info, businessId);
    CHECK_AND_PRINT_ATTRIBUTE(info, businessName);
    CHECK_AND_PRINT_ATTRIBUTE(info, role, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(info, email);

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::PremiumInfo & info)
{
    strm << "PremiumUserInfo {\n";

    CHECK_AND_PRINT_ATTRIBUTE(info, premiumExpirationDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(info, sponsoredGroupName);
    CHECK_AND_PRINT_ATTRIBUTE(info, sponsoredGroupRole, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(info, premiumUpgradable);

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

    CHECK_AND_PRINT_ATTRIBUTE(accounting, uploadLimit, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, uploadLimitEnd, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, uploadLimitNextMonth, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumServiceStatus, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumOrderNumber);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumCommerceService);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumServiceStart, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumServiceSKU);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, lastSuccessfulCharge, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, lastFailedCharge, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, lastFailedChargeReason);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, nextPaymentDue, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumLockUntil, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, updated, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumSubscriptionNumber);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, lastRequestedCharge, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, currency);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, unitPrice, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, businessId, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, businessName);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, businessRole, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, unitDiscount, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, nextChargeDate, static_cast<qint64>);

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::UserAttributes & attributes)
{
    strm << "UserAttributes: {\n";

    CHECK_AND_PRINT_ATTRIBUTE(attributes, defaultLocationName);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, defaultLatitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, defaultLongitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, preactivation);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, incomingEmailAddress);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, comments);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, dateAgreedToTermsOfService, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, maxReferrals);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, referralCount);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, refererCode);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sentEmailDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sentEmailCount);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, dailyEmailLimit);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, emailOptOutDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, partnerEmailOptInDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, preferredLanguage);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, preferredCountry);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, clipFullPage);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, twitterUserName);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, twitterId);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, groupName);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, recognitionLanguage);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, referralProof);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, educationalDiscount);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, businessAddress);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, hideSponsorBilling);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, taxExempt);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, useEmailAutoFiling);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderEmailConfig, static_cast<quint8>);

    if (attributes.viewedPromotions.isSet()) {
        strm << "viewedPromotions: { \n";
        foreach(const QString & promotion, attributes.viewedPromotions.ref()) {
            strm << promotion << "\n";
        }
        strm << "}; \n";
    }

    if (attributes.recentMailedAddresses.isSet()) {
        strm << "recentMailedAddresses: { \n";
        foreach(const QString & address, attributes.recentMailedAddresses.ref()) {
            strm << address << "\n";
        }
        strm << "}; \n";
    }

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::NoteAttributes & attributes)
{
    strm << "NoteAttributes: {\n";

    CHECK_AND_PRINT_ATTRIBUTE(attributes, subjectDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, latitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, longitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, altitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, author);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, source);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceURL);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceApplication);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, shareDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderOrder, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderDoneTime, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderTime, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, placeName);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, contentClass);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, lastEditedBy);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, creatorId, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, lastEditorId, static_cast<qint32>);

    if (attributes.applicationData.isSet())
    {
        const qevercloud::LazyMap & applicationData = attributes.applicationData;

        if (applicationData.keysOnly.isSet()) {
            const QSet<QString> & keysOnly = applicationData.keysOnly;
            strm << "ApplicationData: keys only: \n";
            foreach(const QString & key, keysOnly) {
                strm << key << "; ";
            }
            strm << "\n";
        }

        if (applicationData.fullMap.isSet()) {
            const QMap<QString, QString> & fullMap = applicationData.fullMap;
            strm << "ApplicationData: full map: \n";
            foreach(const QString & key, fullMap.keys()) {
                strm << "[" << key << "] = " << fullMap.value(key) << "; ";
            }
            strm << "\n";
        }
    }

    if (attributes.classifications.isSet())
    {
        const QMap<QString, QString> & classifications = attributes.classifications;
        foreach(const QString & key, classifications) {
            strm << "[" << key << "] = " << classifications.value(key) << "; ";
        }
        strm << "\n";
    }

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::ResourceAttributes & attributes)
{
    strm << "ResourceAttributes: {\n";

    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceURL);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, timestamp, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, latitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, longitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, altitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, cameraMake);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, cameraModel);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, clientWillIndex);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, recoType);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, fileName);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, attachment);

    if (attributes.applicationData.isSet())
    {
        const qevercloud::LazyMap & applicationData = attributes.applicationData;

        if (applicationData.keysOnly.isSet()) {
            const QSet<QString> & keysOnly = applicationData.keysOnly;
            strm << "ApplicationData: keys only: \n";
            foreach(const QString & key, keysOnly) {
                strm << key << "; ";
            }
            strm << "\n";
        }

        if (applicationData.fullMap.isSet()) {
            const QMap<QString, QString> & fullMap = applicationData.fullMap;
            strm << "ApplicationData: full map: \n";
            foreach(const QString & key, fullMap) {
                strm << "[" << key << "] = " << fullMap.value(key) << "; ";
            }
            strm << "\n";
        }
    }

    strm << "}; \n";
    return strm;
}

#undef CHECK_AND_PRINT_ATTRIBUTE

QTextStream & operator <<(QTextStream & strm, const qevercloud::PrivilegeLevel::type & level)
{
    strm << "PrivilegeLevel: ";

    switch (level)
    {
    case qevercloud::PrivilegeLevel::NORMAL:
        strm << "NORMAL";
        break;
    case qevercloud::PrivilegeLevel::PREMIUM:
        strm << "PREMIUM";
        break;
    case qevercloud::PrivilegeLevel::VIP:
        strm << "VIP";
        break;
    case qevercloud::PrivilegeLevel::MANAGER:
        strm << "MANAGER";
        break;
    case qevercloud::PrivilegeLevel::SUPPORT:
        strm << "SUPPORT";
        break;
    case qevercloud::PrivilegeLevel::ADMIN:
        strm << "ADMIN";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::QueryFormat::type & format)
{
    switch (format)
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

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::SharedNotebookPrivilegeLevel::type & privilege)
{
    switch(privilege)
    {
    case qevercloud::SharedNotebookPrivilegeLevel::READ_NOTEBOOK:
        strm << "READ_NOTEBOOK";
        break;
    case qevercloud::SharedNotebookPrivilegeLevel::MODIFY_NOTEBOOK_PLUS_ACTIVITY:
        strm << "MODIFY_NOTEBOOK_PLUS_ACTIVITY";
        break;
    case qevercloud::SharedNotebookPrivilegeLevel::READ_NOTEBOOK_PLUS_ACTIVITY:
        strm << "READ_NOTEBOOK_PLUS_ACTIVITY";
        break;
    case qevercloud::SharedNotebookPrivilegeLevel::GROUP:
        strm << "GROUP";
        break;
    case qevercloud::SharedNotebookPrivilegeLevel::FULL_ACCESS:
        strm << "FULL_ACCESS";
        break;
    case qevercloud::SharedNotebookPrivilegeLevel::BUSINESS_FULL_ACCESS:
        strm << "BUSINESS_FULL_ACCESS";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::NoteSortOrder::type & order)
{
    switch(order)
    {
    case qevercloud::NoteSortOrder::CREATED:
        strm << "CREATED";
        break;
    case qevercloud::NoteSortOrder::RELEVANCE:
        strm << "RELEVANCE";
        break;
    case qevercloud::NoteSortOrder::TITLE:
        strm << "TITLE";
        break;
    case qevercloud::NoteSortOrder::UPDATED:
        strm << "UPDATED";
        break;
    case qevercloud::NoteSortOrder::UPDATE_SEQUENCE_NUMBER:
        strm << "UPDATE_SEQUENCE_NUMBER";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::NotebookRestrictions & restrictions)
{
    strm << "{ \n";

#define INSERT_DELIMITER \
    strm << "; \n"

    if (restrictions.noReadNotes.isSet()) {
        strm << "noReadNotes: " << (restrictions.noReadNotes ? "true" : "false");
    }
    else {
        strm << "noReadNotes is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noCreateNotes.isSet()) {
        strm << "noCreateNotes: " << (restrictions.noCreateNotes ? "true" : "false");
    }
    else {
        strm << "noCreateNotes is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noUpdateNotes.isSet()) {
        strm << "noUpdateNotes: " << (restrictions.noUpdateNotes ? "true" : "false");
    }
    else {
        strm << "noUpdateNotes is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noExpungeNotes.isSet()) {
        strm << "noExpungeNotes: " << (restrictions.noExpungeNotes ? "true" : "false");
    }
    else {
        strm << "noExpungeNotes is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noShareNotes.isSet()) {
        strm << "noShareNotes: " << (restrictions.noShareNotes ? "true" : "false");
    }
    else {
        strm << "noShareNotes is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noEmailNotes.isSet()) {
        strm << "noEmailNotes: " << (restrictions.noEmailNotes ? "true" : "false");
    }
    else {
        strm << "noEmailNotes is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noSendMessageToRecipients.isSet()) {
        strm << "noSendMessageToRecipients: " << (restrictions.noSendMessageToRecipients ? "true" : "false");
    }
    else {
        strm << "noSendMessageToRecipients is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noUpdateNotebook.isSet()) {
        strm << "noUpdateNotebook: " << (restrictions.noUpdateNotebook ? "true" : "false");
    }
    else {
        strm << "noUpdateNotebook is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noExpungeNotebook.isSet()) {
        strm << "noExpungeNotebook: " << (restrictions.noExpungeNotebook ? "true" : "false");
    }
    else {
        strm << "noExpungeNotebook is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noSetDefaultNotebook.isSet()) {
        strm << "noSetDefaultNotebook: " << (restrictions.noSetDefaultNotebook ? "true" : "false");
    }
    else {
        strm << "noSetDefaultNotebook is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noSetNotebookStack.isSet()) {
        strm << "noSetNotebookStack: " << (restrictions.noSetNotebookStack ? "true" : "false");
    }
    else {
        strm << "noSetNotebookStack is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noPublishToPublic.isSet()) {
        strm << "noPublishToPublic: " << (restrictions.noPublishToPublic ? "true" : "false");
    }
    else {
        strm << "noPublishToPublic is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noPublishToBusinessLibrary.isSet()) {
        strm << "noPublishToBusinessLibrary: " << (restrictions.noPublishToBusinessLibrary ? "true" : "false");
    }
    else {
        strm << "noPublishToBusinessLibrary is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noCreateTags.isSet()) {
        strm << "noCreateTags: " << (restrictions.noCreateTags ? "true" : "false");
    }
    else {
        strm << "noCreateTags is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noUpdateTags.isSet()) {
        strm << "noUpdateTags: " << (restrictions.noUpdateTags ? "true" : "false");
    }
    else {
        strm << "noUpdateTags is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noExpungeTags.isSet()) {
        strm << "noExpungeTags: " << (restrictions.noExpungeTags ? "true" : "false");
    }
    else {
        strm << "noExpungeTags is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noSetParentTag.isSet()) {
        strm << "noSetParentTag: " << (restrictions.noSetParentTag ? "true" : "false");
    }
    else {
        strm << "noSetParentTag is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.noCreateSharedNotebooks.isSet()) {
        strm << "noCreateSharedNotebooks: " << (restrictions.noCreateSharedNotebooks ? "true" : "false");
    }
    else {
        strm << "noCreateSharedNotebooks is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.updateWhichSharedNotebookRestrictions.isSet()) {
        strm << "updateWhichSharedNotebookRestrictions: " << restrictions.updateWhichSharedNotebookRestrictions;
    }
    else {
        strm << "updateWhichSharedNotebookRestrictions is not set";
    }
    INSERT_DELIMITER;

    if (restrictions.expungeWhichSharedNotebookRestrictions.isSet()) {
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

QTextStream & operator <<(QTextStream & strm, const qevercloud::SharedNotebookInstanceRestrictions::type & restrictions)
{
    switch(restrictions)
    {
    case qevercloud::SharedNotebookInstanceRestrictions::NO_SHARED_NOTEBOOKS:
        strm << "NO_SHARED_NOTEBOOKS";
        break;
    case qevercloud::SharedNotebookInstanceRestrictions::ONLY_JOINED_OR_PREVIEW:
        strm << "ONLY_JOINED_OR_PREVIEW";
        break;
    default:
        strm << "UNKNOWN";
        break;
    }

    return strm;
}
