#include <qute_note/utility/Printable.h>
#include <qute_note/utility/Utility.h>

namespace qute_note {

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

} // namespace qute_note

#define CHECK_AND_PRINT_ATTRIBUTE(object, name, ...) \
    bool isSet##name = object.name.isSet(); \
    strm << #name " is " << (isSet##name ? "set" : "not set") << "\n"; \
    if (isSet##name) { \
        strm << #name " = " << __VA_ARGS__(object.name) << "\n"; \
    }

QTextStream & operator <<(QTextStream & strm, const qevercloud::BusinessUserInfo & info)
{
    strm << "qevercloud::BusinessUserInfo: {\n";
    QString indent = "  ";

    strm << indent << "businessId = " << (info.businessId.isSet()
                                          ? QString::number(info.businessId.ref())
                                          : "<empty>") << "; \n";
    strm << indent << "businessName = " << (info.businessName.isSet()
                                            ? info.businessName.ref()
                                            : "<empty>") << "; \n";
    strm << indent << "role = ";
    if (info.role.isSet()) {
        strm << info.role.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "email = " << (info.email.isSet() ? info.email.ref() : "<empty>") << "; \n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::PremiumInfo & info)
{
    strm << "qevercloud::PremiumUserInfo {\n";
    QString indent = "  ";

    strm << indent << "premiumExpirationDate = " << (info.premiumExpirationDate.isSet()
                                                     ? qute_note::printableDateTimeFromTimestamp(info.premiumExpirationDate.ref())
                                                     : "<empty>") << "; \n";
    strm << indent << "sponsoredGroupName = " << (info.sponsoredGroupName.isSet()
                                                  ? info.sponsoredGroupName.ref()
                                                  : "<empty>") << "; \n";
    strm << indent << "sponsoredGroupRole = ";
    if (info.sponsoredGroupRole.isSet()) {
        strm << info.sponsoredGroupRole.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "premiumUpgradable = " << (info.premiumUpgradable.isSet()
                                                 ? (info.premiumUpgradable.ref() ? "true" : "false")
                                                 : "<empty>") << "; \n";

    strm << indent << "premiumExtendable = " << (info.premiumExtendable ? "true" : "false") << "\n";
    strm << indent << "premiumPending = " << (info.premiumPending ? "true" : "false") << "\n";
    strm << indent << "premiumCancellationPending = " << (info.premiumCancellationPending ? "true" : "false") << "\n";
    strm << indent << "canPurchaseUploadAllowance = " << (info.canPurchaseUploadAllowance ? "true" : "false") << "\n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::Accounting & accounting)
{
    strm << "qevercloud::Accounting: { \n";
    QString indent = "  ";

    strm << indent << "uploadLimit = " << (accounting.uploadLimit.isSet()
                                           ? QString::number(accounting.uploadLimit.ref())
                                           : "<empty>") << "; \n";
    strm << indent << "uploadLimitEnd = " << (accounting.uploadLimitEnd.isSet()
                                              ? qute_note::printableDateTimeFromTimestamp(accounting.uploadLimitEnd.ref())
                                              : "<empty>") << "; \n";
    strm << indent << "uploadLimitNextMonth = " << (accounting.uploadLimitNextMonth.isSet()
                                                    ? QString::number(accounting.uploadLimitNextMonth.ref())
                                                    : "<empty>") << "; \n";
    strm << indent << "premiumServiceStatus = ";
    if (accounting.premiumServiceStatus.isSet()) {
        strm << accounting.premiumServiceStatus.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "premiumOrderNumber = " << (accounting.premiumOrderNumber.isSet()
                                                  ? accounting.premiumOrderNumber.ref()
                                                  : "<empty>") << "; \n";
    strm << indent << "premiumCommerceService = " << (accounting.premiumCommerceService.isSet()
                                                      ? accounting.premiumCommerceService.ref()
                                                      : "<empty>") << "; \n";
    strm << indent << "premiumServiceStart = " << (accounting.premiumServiceStart.isSet()
                                                   ? qute_note::printableDateTimeFromTimestamp(accounting.premiumServiceStart.ref())
                                                   : "<empty>") << "; \n";
    strm << indent << "premiumServiceSKU = " << (accounting.premiumServiceSKU.isSet()
                                                 ? accounting.premiumServiceSKU.ref()
                                                 : "<empty>") << "; \n";
    strm << indent << "lastSuccessfulCharge = " << (accounting.lastSuccessfulCharge.isSet()
                                                    ? qute_note::printableDateTimeFromTimestamp(accounting.lastSuccessfulCharge.ref())
                                                    : "<empty>") << "; \n";
    strm << indent << "lastFailedCharge = " << (accounting.lastFailedCharge.isSet()
                                                ? qute_note::printableDateTimeFromTimestamp(accounting.lastFailedCharge.ref())
                                                : "<empty>") << "; \n";
    strm << indent << "lastFailedChargeReason = " << (accounting.lastFailedChargeReason.isSet()
                                                      ? accounting.lastFailedChargeReason.ref()
                                                      : "<empty>") << "; \n";
    strm << indent << "nextPaymentDue = " << (accounting.nextPaymentDue.isSet()
                                              ? qute_note::printableDateTimeFromTimestamp(accounting.nextPaymentDue.ref())
                                              : "<empty>") << "; \n";
    strm << indent << "premiumLockUntil = " << (accounting.premiumLockUntil.isSet()
                                                ? qute_note::printableDateTimeFromTimestamp(accounting.premiumLockUntil.ref())
                                                : "<empty>") << "; \n";
    strm << indent << "updated = " << (accounting.updated.isSet()
                                       ? qute_note::printableDateTimeFromTimestamp(accounting.updated.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "premiumSubscriptionNumber = " << (accounting.premiumSubscriptionNumber.isSet()
                                                         ? accounting.premiumSubscriptionNumber.ref()
                                                         : "<empty>") << "; \n";
    strm << indent << "lastRequestedCharge = " << (accounting.lastRequestedCharge.isSet()
                                                   ? qute_note::printableDateTimeFromTimestamp(accounting.lastRequestedCharge.ref())
                                                   : "<empty>") << "; \n";
    strm << indent << "currency = " << (accounting.currency.isSet() ? accounting.currency.ref() : "<empty>") << "; \n";
    strm << indent << "unitPrice = " << (accounting.unitPrice.isSet() ? QString::number(accounting.unitPrice.ref()) : "<empty>") << "; \n";
    strm << indent << "businessId = " << (accounting.businessId.isSet() ? QString::number(accounting.businessId.ref()) : "<empty>") << "; \n";

    strm << indent << "businessRole = ";
    if (accounting.businessRole.isSet()) {
        strm << accounting.businessRole.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "unitDiscount = " << (accounting.unitDiscount.isSet()
                                            ? QString::number(accounting.unitDiscount.ref())
                                            : "<empty>") << "; \n";
    strm << indent << "nextChargeDate = " << (accounting.nextChargeDate.isSet()
                                              ? qute_note::printableDateTimeFromTimestamp(accounting.nextChargeDate.ref())
                                              : "<empty>") << "; \n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::UserAttributes & attributes)
{
    strm << "qevercloud::UserAttributes: {\n";
    QString indent = "  ";

    strm << indent << "defaultLocationName = " << (attributes.defaultLocationName.isSet()
                                                   ? attributes.defaultLocationName.ref()
                                                   : "<empty>") << "; \n";
    strm << indent << "defaultLatitude = " << (attributes.defaultLatitude.isSet()
                                               ? QString::number(attributes.defaultLatitude.ref())
                                               : "<empty>") << "; \n";
    strm << indent << "defaultLongitude = " << (attributes.defaultLongitude.isSet()
                                                ? QString::number(attributes.defaultLongitude.ref())
                                                : "<empty>") << "; \n";
    strm << indent << "preactivation = " << (attributes.preactivation.isSet()\
                                             ? (attributes.preactivation.ref() ? "true" : "false")
                                             : "<empty>") << "; \n";
    strm << indent << "viewedPromotions";
    if (attributes.viewedPromotions.isSet())
    {
        strm << ": { \n";
        const QStringList & viewedPromotions = attributes.viewedPromotions.ref();
        const int numViewedPromotions = viewedPromotions.size();
        for(int i = 0; i < numViewedPromotions; ++i) {
            strm << indent << indent << "[" << i << "]: " << viewedPromotions[i] << "; \n";
        }

        strm << indent << "}; \n";
    }
    else
    {
        strm << " = <empty>; \n";
    }

    strm << indent << "incomingEmailAddress = " << (attributes.incomingEmailAddress.isSet()
                                                    ? attributes.incomingEmailAddress.ref()
                                                    : "<empty>") << "; \n";

    strm << indent << "recentMailedAddresses";
    if (attributes.recentMailedAddresses.isSet())
    {
        strm << ": { \n";
        const QStringList & recentMailedAddresses = attributes.recentMailedAddresses.ref();
        const int numRecentMailedAddresses = recentMailedAddresses.size();
        for(int i = 0; i < numRecentMailedAddresses; ++i) {
            strm << indent << indent << "[" << i << "]: " << recentMailedAddresses[i] << "; \n";
        }

        strm << indent << "}; \n";
    }
    else
    {
        strm << " = <empty>; \n";
    }

    strm << indent << "comments = " << (attributes.comments.isSet() ? attributes.comments.ref() : "<empty>") << "; \n";

    strm << indent << "dateAgreedToTermsOfService = " << (attributes.dateAgreedToTermsOfService.isSet()
                                                          ? qute_note::printableDateTimeFromTimestamp(attributes.dateAgreedToTermsOfService.ref())
                                                          : "<empty>") << "; \n";
    strm << indent << "maxReferrals = " << (attributes.maxReferrals.isSet()
                                            ? QString::number(attributes.maxReferrals.ref())
                                            : "<empty>") << "; \n";
    strm << indent << "referralCount = " << (attributes.referralCount.isSet()
                                             ? QString::number(attributes.referralCount.ref())
                                             : "<empty>") << "; \n";
    strm << indent << "refererCode = " << (attributes.refererCode.isSet()
                                           ? attributes.refererCode.ref()
                                           : "<empty>") << "; \n";
    strm << indent << "sentEmailDate = " << (attributes.sentEmailDate.isSet()
                                             ? qute_note::printableDateTimeFromTimestamp(attributes.sentEmailDate.ref())
                                             : "<empty>") << "; \n";
    strm << indent << "sentEmailCount = " << (attributes.sentEmailCount.isSet()
                                              ? QString::number(attributes.sentEmailCount.ref())
                                              : "<empty>") << "; \n";
    strm << indent << "dailyEmailLimit = " << (attributes.dailyEmailLimit.isSet()
                                               ? QString::number(attributes.dailyEmailLimit.ref())
                                               : "<empty>") << "; \n";
    strm << indent << "emailOptOutDate = " << (attributes.emailOptOutDate.isSet()
                                               ? qute_note::printableDateTimeFromTimestamp(attributes.emailOptOutDate.ref())
                                               : "<empty>") << "; \n";
    strm << indent << "partnerEmailOptInDate = " << (attributes.partnerEmailOptInDate.isSet()
                                                     ? qute_note::printableDateTimeFromTimestamp(attributes.partnerEmailOptInDate.ref())
                                                     : "<empty>") << "; \n";
    strm << indent << "preferredLanguage = " << (attributes.preferredLanguage.isSet()
                                                 ? attributes.preferredLanguage.ref()
                                                 : "<empty>") << "; \n";
    strm << indent << "preferredCountry = " << (attributes.preferredCountry.isSet()
                                                ? attributes.preferredCountry.ref()
                                                : "<empty>") << "; \n";
    strm << indent << "clipFullPage = " << (attributes.clipFullPage.isSet()
                                            ? (attributes.clipFullPage.ref() ? "true" : "false")
                                            : "<empty>") << "; \n";
    strm << indent << "twitterUserName = " << (attributes.twitterUserName.isSet()
                                               ? attributes.twitterUserName.ref()
                                               : "<empty>") << "; \n";
    strm << indent << "twitterId = " << (attributes.twitterId.isSet()
                                         ? attributes.twitterId.ref()
                                         : "<empty>") << "; \n";
    strm << indent << "groupName = " << (attributes.groupName.isSet()
                                         ? attributes.groupName.ref()
                                         : "<empty>") << "; \n";
    strm << indent << "recognitionLanguage = " << (attributes.recognitionLanguage.isSet()
                                                   ? attributes.recognitionLanguage.ref()
                                                   : "<empty>") << "; \n";
    strm << indent << "referralProof = " << (attributes.referralProof.isSet()
                                             ? attributes.referralProof.ref()
                                             : "<empty>") << "; \n";
    strm << indent << "educationalDiscount = " << (attributes.educationalDiscount.isSet()
                                                   ? (attributes.educationalDiscount.ref() ? "true" : "false")
                                                   : "<empty>") << "; \n";
    strm << indent << "businessAddress = " << (attributes.businessAddress.isSet()
                                               ? attributes.businessAddress.ref()
                                               : "<empty>") << "; \n";
    strm << indent << "hideSponsorBilling = " << (attributes.hideSponsorBilling.isSet()
                                                  ? (attributes.hideSponsorBilling.ref() ? "true" : "false")
                                                  : "<empty>") << "; \n";
    strm << indent << "taxExempt = " << (attributes.taxExempt.isSet()
                                         ? (attributes.taxExempt.ref() ? "true" : "false")
                                         : "<empty>") << "; \n";
    strm << indent << "useEmailAutoFiling = " << (attributes.useEmailAutoFiling.isSet()
                                                  ? (attributes.useEmailAutoFiling.ref() ? "true" : "false")
                                                  : "<empty>") << "; \n";
    strm << indent << "reminderEmailConfig = ";
    if (attributes.reminderEmailConfig.isSet()) {
        strm << attributes.reminderEmailConfig.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::NoteAttributes & attributes)
{
    strm << "qevercloud::NoteAttributes: {\n";
    QString indent = "  ";

    strm << indent << "subjectDate = " << (attributes.subjectDate.isSet()
                                           ? qute_note::printableDateTimeFromTimestamp(attributes.subjectDate.ref())
                                           : "<empty>") << "; \n";
    strm << indent << "latitude = " << (attributes.latitude.isSet()
                                        ? QString::number(attributes.latitude.ref())
                                        : "<empty>") << "; \n";
    strm << indent << "longitude = " << (attributes.longitude.isSet()
                                         ? QString::number(attributes.longitude.ref())
                                         : "<empty>") << "; \n";
    strm << indent << "altitude = " << (attributes.altitude.isSet()
                                        ? QString::number(attributes.altitude.ref())
                                        : "<empty>") << "; \n";
    strm << indent << "author = " << (attributes.author.isSet()
                                      ? attributes.author.ref() : "<empty>") << "; \n";
    strm << indent << "source = " << (attributes.source.isSet()
                                      ? attributes.source.ref() : "<empty>") << "; \n";
    strm << indent << "sourceURL = " << (attributes.sourceURL.isSet()
                                         ? attributes.sourceURL.ref() : "<empty>") << "; \n";
    strm << indent << "sourceApplication = " << (attributes.sourceApplication.isSet()
                                                 ? attributes.sourceApplication.ref()
                                                 : "<empty>") << "; \n";
    strm << indent << "shareDate = " << (attributes.shareDate.isSet()
                                         ? qute_note::printableDateTimeFromTimestamp(attributes.shareDate.ref())
                                         : "<empty>") << "; \n";
    strm << indent << "reminderOrder = " << (attributes.reminderOrder.isSet()
                                             ? QString::number(attributes.reminderOrder.ref())
                                             : "<empty>") << "; \n";
    strm << indent << "reminderDoneTime = " << (attributes.reminderDoneTime.isSet()
                                                ? qute_note::printableDateTimeFromTimestamp(attributes.reminderDoneTime.ref())
                                                : "<empty>") << "; \n";
    strm << indent << "reminderTime = " << (attributes.reminderTime.isSet()
                                            ? qute_note::printableDateTimeFromTimestamp(attributes.reminderTime.ref())
                                            : "<empty>") << "; \n";
    strm << indent << "placeName = " << (attributes.placeName.isSet()
                                         ? attributes.placeName.ref() : "<empty>") << "; \n";
    strm << indent << "contentClass = " << (attributes.contentClass.isSet()
                                            ? attributes.contentClass.ref() : "<empty>") << "; \n";
    strm << indent << "lastEditedBy = " << (attributes.lastEditedBy.isSet()
                                            ? attributes.lastEditedBy.ref() : "<empty>") << "; \n";
    strm << indent << "creatorId = " << (attributes.creatorId.isSet()
                                         ? QString::number(attributes.creatorId.ref())
                                         : "<empty>") << "; \n";
    strm << indent << "lastEditorId = " << (attributes.lastEditorId.isSet()
                                            ? QString::number(attributes.lastEditorId.ref())
                                            : "<empty>") << "; \n";

    if (attributes.applicationData.isSet())
    {
        const qevercloud::LazyMap & applicationData = attributes.applicationData;

        if (applicationData.keysOnly.isSet()) {
            const QSet<QString> & keysOnly = applicationData.keysOnly;
            strm << indent << "ApplicationData: keys only: \n";
            foreach(const QString & key, keysOnly) {
                strm << key << "; ";
            }
            strm << "\n";
        }

        if (applicationData.fullMap.isSet()) {
            const QMap<QString, QString> & fullMap = applicationData.fullMap;
            strm << indent << "ApplicationData: full map: \n";
            foreach(const QString & key, fullMap.keys()) {
                strm << "[" << key << "] = " << fullMap.value(key) << "; ";
            }
            strm << "\n";
        }
    }

    if (attributes.classifications.isSet())
    {
        strm << indent << "Classifications: ";
        const QMap<QString, QString> & classifications = attributes.classifications;
        foreach(const QString & key, classifications) {
            strm << "[" << key << "] = " << classifications.value(key) << "; ";
        }
        strm << "\n";
    }

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::PrivilegeLevel::type & level)
{
    strm << "qevercloud::PrivilegeLevel: ";

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
        strm << "Unknown";
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
    strm << "qevercloud::SharedNotebookPrivilegeLevel: ";

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
        strm << "Unknown";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::NoteSortOrder::type & order)
{
    strm << "qevercloud::NoteSortOrder: ";

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
    strm << "qevercloud::SharedNotebookInstanceRestrictions: ";

    switch(restrictions)
    {
    case qevercloud::SharedNotebookInstanceRestrictions::NO_SHARED_NOTEBOOKS:
        strm << "NO_SHARED_NOTEBOOKS";
        break;
    case qevercloud::SharedNotebookInstanceRestrictions::ONLY_JOINED_OR_PREVIEW:
        strm << "ONLY_JOINED_OR_PREVIEW";
        break;
    default:
        strm << "Unknown";
        break;
    }

    return strm;
}

QTextStream & operator <<(QTextStream & strm, const qevercloud::ResourceAttributes & attributes)
{
    strm << "ResourceAttributes: { \n";

    QString indent = "  ";

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
    QString indent = "  ";

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
    QString indent = "  ";

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
    QString indent = "  ";

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
    QString indent = "  ";

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
    QString indent = "  ";

    strm << indent << "shareName = " << (linkedNotebook.shareName.isSet() ? linkedNotebook.shareName.ref() : "<empty>") << "; \n";
    strm << indent << "username = " << (linkedNotebook.username.isSet() ? linkedNotebook.username.ref() : "<empty>") << "; \n";
    strm << indent << "shardId = " << (linkedNotebook.shardId.isSet() ? linkedNotebook.shardId.ref() : "<empty>") << "; \n";
    strm << indent << "shareKey = " << (linkedNotebook.shareKey.isSet() ? linkedNotebook.shareKey.ref() : "<empty>") << "; \n";
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
    QString indent = "  ";

    strm << indent << "guid = " << (notebook.guid.isSet() ? notebook.guid.ref() : "<empty>") << "; \n";
    strm << indent << "name = " << (notebook.name.isSet() ? notebook.name.ref() : "<empty>") << "; \n";
    strm << indent << "updateSequenceNum = " << (notebook.updateSequenceNum.isSet()
                                                 ? QString::number(notebook.updateSequenceNum.ref())
                                                 : "<empty>") << "; \n";
    strm << indent << "defaultNotebook = " << (notebook.defaultNotebook.isSet()
                                               ? (notebook.defaultNotebook.ref() ? "true" : "false")
                                               : "<empty>") << "; \n";
    strm << indent << "serviceCreated = " << (notebook.serviceCreated.isSet()
                                              ? qute_note::printableDateTimeFromTimestamp(notebook.serviceCreated.ref())
                                              : "<empty>") << "; \n";
    strm << indent << "serviceUpdated = " << (notebook.serviceUpdated.isSet()
                                              ? qute_note::printableDateTimeFromTimestamp(notebook.serviceUpdated.ref())
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
    QString indent = "  ";

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
    QString indent = "  ";

    strm << indent << "id = " << (sharedNotebook.id.isSet() ? QString::number(sharedNotebook.id.ref()) : "<empty>") << "; \n";
    strm << indent << "userId = " << (sharedNotebook.userId.isSet() ? QString::number(sharedNotebook.userId.ref()) : "<empty>") << "; \n";
    strm << indent << "notebookGuid = " << (sharedNotebook.notebookGuid.isSet() ? sharedNotebook.notebookGuid.ref() : "<empty>") << "; \n";
    strm << indent << "email = " << (sharedNotebook.email.isSet() ? sharedNotebook.email.ref() : "<empty>") << "; \n";
    strm << indent << "serviceCreated = " << (sharedNotebook.serviceCreated.isSet()
                                              ? qute_note::printableDateTimeFromTimestamp(sharedNotebook.serviceCreated.ref())
                                              : "<empty>") << "; \n";
    strm << indent << "serviceUpdated = " << (sharedNotebook.serviceUpdated.isSet()
                                              ? qute_note::printableDateTimeFromTimestamp(sharedNotebook.serviceUpdated.ref())
                                              : "<empty>") << "; \n";

    strm << indent << "privilege = ";
    if (sharedNotebook.privilege.isSet()) {
        strm << sharedNotebook.privilege.ref();
    }
    else {
        strm << "<empty>";
    }
    strm << "; \n";

    strm << indent << "allowPreview = " << (sharedNotebook.allowPreview.isSet()
                                            ? (sharedNotebook.allowPreview.ref() ? "true" : "false")
                                            : "<empty>") << "; \n";

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
    QString indent = "  ";

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
    QString indent = "  ";

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
                                       ? qute_note::printableDateTimeFromTimestamp(user.created.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "updated = " << (user.updated.isSet()
                                       ? qute_note::printableDateTimeFromTimestamp(user.updated.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "deleted = " << (user.deleted.isSet()
                                       ? qute_note::printableDateTimeFromTimestamp(user.deleted.ref())
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

    strm << indent << "premiumInfo";
    if (user.premiumInfo.isSet()) {
        strm << ": \n" << user.premiumInfo.ref();
    }
    else {
        strm << " = <empty>; \n";
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
    QString indent = "  ";

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
    QString indent = "  ";

    strm << indent << "guid = " << (note.guid.isSet() ? note.guid.ref() : "<empty>") << "; \n";
    strm << indent << "title = " << (note.title.isSet() ? note.title.ref() : "<empty>") << "; \n";
    strm << indent << "content = " << (note.content.isSet() ? note.content.ref() : "<empty>") << "; \n";
    strm << indent << "contentHash = " << (note.contentHash.isSet() ? note.contentHash.ref() : "<empty>") << "; \n";
    strm << indent << "created = " << (note.created.isSet()
                                       ? qute_note::printableDateTimeFromTimestamp(note.created.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "updated = " << (note.updated.isSet()
                                       ? qute_note::printableDateTimeFromTimestamp(note.updated.ref())
                                       : "<empty>") << "; \n";
    strm << indent << "deleted = " << (note.deleted.isSet()
                                       ? qute_note::printableDateTimeFromTimestamp(note.deleted.ref())
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
    strm << "expires = " << qute_note::printableDateTimeFromTimestamp(result.expires) << "; \n";
    strm << "shardId = " << result.shardId << "; \n";
    strm << "userId = " << QString::number(result.userId) << "; \n";
    strm << "webApiUrlPrefix = " << result.webApiUrlPrefix << "; \n";
    strm << "authenticationToken " << (result.authenticationToken.isEmpty() ? "is empty" : "is not empty") << "; \n";

    strm << "}; ";
    return strm;
}
