#include "QEverCloudHelpers.h"

namespace qevercloud {
/*
bool operator==(const UserAttributes & lhs, const UserAttributes & rhs)
{
    if (lhs.defaultLocationName != rhs.defaultLocationName) {
        return false;
    }
    else if (lhs.defaultLatitude != rhs.defaultLatitude) {
        return false;
    }
    else if (lhs.defaultLongitude != rhs.defaultLongitude) {
        return false;
    }
    else if (lhs.preactivation != rhs.preactivation) {
        return false;
    }
    else if (lhs.viewedPromotions != rhs.viewedPromotions) {
        return false;
    }
    else if (lhs.incomingEmailAddress != rhs.incomingEmailAddress) {
        return false;
    }
    else if (lhs.recentMailedAddresses != rhs.recentMailedAddresses) {
        return false;
    }
    else if (lhs.comments != rhs.comments) {
        return false;
    }
    else if (lhs.dateAgreedToTermsOfService != rhs.dateAgreedToTermsOfService) {
        return false;
    }
    else if (lhs.maxReferrals != rhs.maxReferrals) {
        return false;
    }
    else if (lhs.referralCount != rhs.referralCount) {
        return false;
    }
    else if (lhs.sentEmailDate != rhs.sentEmailDate) {
        return false;
    }
    else if (lhs.sentEmailCount != rhs.sentEmailCount) {
        return false;
    }
    else if (lhs.dailyEmailLimit != rhs.dailyEmailLimit) {
        return false;
    }
    else if (lhs.emailOptOutDate != rhs.emailOptOutDate) {
        return false;
    }
    else if (lhs.partnerEmailOptInDate != rhs.partnerEmailOptInDate) {
        return false;
    }
    else if (lhs.preferredLanguage != rhs.preferredLanguage) {
        return false;
    }
    else if (lhs.preferredCountry != rhs.preferredCountry) {
        return false;
    }
    else if (lhs.clipFullPage != rhs.clipFullPage) {
        return false;
    }
    else if (lhs.twitterUserName != rhs.twitterUserName) {
        return false;
    }
    else if (lhs.twitterId != rhs.twitterId) {
        return false;
    }
    else if (lhs.groupName != rhs.groupName) {
        return false;
    }
    else if (lhs.recognitionLanguage != rhs.recognitionLanguage) {
        return false;
    }
    else if (lhs.referralProof != rhs.referralProof) {
        return false;
    }
    else if (lhs.educationalDiscount != rhs.educationalDiscount) {
        return false;
    }
    else if (lhs.businessAddress != rhs.businessAddress) {
        return false;
    }
    else if (lhs.hideSponsorBilling != rhs.hideSponsorBilling) {
        return false;
    }
    else if (lhs.taxExempt != rhs.taxExempt) {
        return false;
    }
    else if (lhs.useEmailAutoFiling != rhs.useEmailAutoFiling) {
        return false;
    }
    else if (lhs.reminderEmailConfig != rhs.reminderEmailConfig) {
        return false;
    }

    return true;
}

bool operator!=(const UserAttributes & lhs, const UserAttributes & rhs)
{
    return !(operator==(lhs, rhs));
}

bool operator==(const Accounting & lhs, const Accounting & rhs)
{
    if (lhs.uploadLimit != rhs.uploadLimit) {
        return false;
    }
    else if (lhs.uploadLimitEnd != rhs.uploadLimitEnd) {
        return false;
    }
    else if (lhs.uploadLimitNextMonth != rhs.uploadLimitNextMonth) {
        return false;
    }
    else if (lhs.premiumServiceStatus != rhs.premiumServiceStatus) {
        return false;
    }
    else if (lhs.premiumOrderNumber != rhs.premiumOrderNumber) {
        return false;
    }
    else if (lhs.premiumCommerceService != rhs.premiumCommerceService) {
        return false;
    }
    else if (lhs.premiumServiceStart != rhs.premiumServiceStart) {
        return false;
    }
    else if (lhs.premiumServiceSKU != rhs.premiumServiceSKU) {
        return false;
    }
    else if (lhs.lastSuccessfulCharge != rhs.lastSuccessfulCharge) {
        return false;
    }
    else if (lhs.lastFailedCharge != rhs.lastFailedCharge) {
        return false;
    }
    else if (lhs.lastFailedChargeReason != rhs.lastFailedChargeReason) {
        return false;
    }
    else if (lhs.nextPaymentDue != rhs.nextPaymentDue) {
        return false;
    }
    else if (lhs.premiumLockUntil != rhs.premiumLockUntil) {
        return false;
    }
    else if (lhs.updated != rhs.updated) {
        return false;
    }
    else if (lhs.premiumSubscriptionNumber != rhs.premiumSubscriptionNumber) {
        return false;
    }
    else if (lhs.lastRequestedCharge != rhs.lastRequestedCharge) {
        return false;
    }
    else if (lhs.currency != rhs.currency) {
        return false;
    }
    else if (lhs.unitPrice != rhs.unitPrice) {
        return false;
    }
    else if (lhs.businessId != rhs.businessId) {
        return false;
    }
    else if (lhs.businessName != rhs.businessName) {
        return false;
    }
    else if (lhs.businessRole != rhs.businessRole) {
        return false;
    }
    else if (lhs.unitDiscount != rhs.unitDiscount) {
        return false;
    }
    else if (lhs.nextChargeDate != rhs.nextChargeDate) {
        return false;
    }

    return true;
}

bool operator!=(const Accounting & lhs, const Accounting & rhs)
{
    return !(operator==(lhs, rhs));
}

bool operator==(const PremiumInfo & lhs, const PremiumInfo & rhs)
{
    if (lhs.currentTime != rhs.currentTime) {
        return false;
    }
    else if (lhs.premium != rhs.premium) {
        return false;
    }
    else if (lhs.premiumRecurring != rhs.premiumRecurring) {
        return false;
    }
    else if (lhs.premiumExpirationDate != rhs.premiumExpirationDate) {
        return false;
    }
    else if (lhs.premiumExtendable != rhs.premiumExtendable) {
        return false;
    }
    else if (lhs.premiumPending != rhs.premiumPending) {
        return false;
    }
    else if (lhs.premiumCancellationPending != rhs.premiumCancellationPending) {
        return false;
    }
    else if (lhs.canPurchaseUploadAllowance != rhs.canPurchaseUploadAllowance) {
        return false;
    }
    else if (lhs.sponsoredGroupName != rhs.sponsoredGroupName) {
        return false;
    }
    else if (lhs.sponsoredGroupRole != rhs.sponsoredGroupRole) {
        return false;
    }
    else if (lhs.premiumUpgradable != rhs.premiumUpgradable) {
        return false;
    }

    return true;
}

bool operator!=(const PremiumInfo & lhs, const PremiumInfo & rhs)
{
    return !(operator==(lhs, rhs));
}

bool operator==(const BusinessUserInfo & lhs, const BusinessUserInfo & rhs)
{
    if (lhs.businessId != rhs.businessId) {
        return false;
    }
    else if (lhs.businessName != rhs.businessName) {
        return false;
    }
    else if (lhs.email != rhs.email) {
        return false;
    }
    else if (lhs.role != rhs.role) {
        return false;
    }

    return true;
}

bool operator!=(const BusinessUserInfo & lhs, const BusinessUserInfo & rhs)
{
    return !(operator==(lhs, rhs));
}

bool operator==(const Data & lhs, const Data & rhs)
{
    if (lhs.bodyHash != rhs.bodyHash) {
        return false;
    }
    else if (lhs.size != rhs.size) {
        return false;
    }
    else if (lhs.body != rhs.body) {
        return false;
    }

    return true;
}

bool operator!=(const Data & lhs, const Data & rhs)
{
    return !operator==(lhs, rhs);
}

bool operator==(const LazyMap & lhs, const LazyMap & rhs)
{
    if (lhs.keysOnly != rhs.keysOnly) {
        return false;
    }
    else if (lhs.fullMap != rhs.fullMap) {
        return false;
    }

    return true;
}

bool operator!=(const LazyMap & lhs, const LazyMap & rhs)
{
    return !operator==(lhs, rhs);
}

bool operator==(const ResourceAttributes & lhs, const ResourceAttributes & rhs)
{
    if (lhs.sourceURL != rhs.sourceURL) {
        return false;
    }
    else if (lhs.timestamp != rhs.timestamp) {
        return false;
    }
    else if (lhs.latitude != rhs.latitude) {
        return false;
    }
    else if (lhs.longitude != rhs.longitude) {
        return false;
    }
    else if (lhs.altitude != rhs.altitude) {
        return false;
    }
    else if (lhs.cameraMake != rhs.cameraMake) {
        return false;
    }
    else if (lhs.cameraModel != rhs.cameraModel) {
        return false;
    }
    else if (lhs.clientWillIndex != rhs.clientWillIndex) {
        return false;
    }
    else if (lhs.recoType != rhs.recoType) {
        return false;
    }
    else if (lhs.fileName != rhs.fileName) {
        return false;
    }
    else if (lhs.attachment != rhs.attachment) {
        return false;
    }
    else if (lhs.applicationData != rhs.applicationData) {
        return false;
    }

    return true;
}

bool operator!=(const ResourceAttributes & lhs, const ResourceAttributes & rhs)
{
    return !operator==(lhs, rhs);
}

bool operator==(const Resource & lhs, const Resource & rhs)
{
    if (lhs.guid != rhs.guid) {
        return false;
    }
    else if (lhs.noteGuid != rhs.noteGuid) {
        return false;
    }
    else if (lhs.data != rhs.data) {
        return false;
    }
    else if (lhs.mime != rhs.mime) {
        return false;
    }
    else if (lhs.width != rhs.width) {
        return false;
    }
    else if (lhs.height != rhs.height) {
        return false;
    }
    else if (lhs.recognition != rhs.recognition) {
        return false;
    }
    else if (lhs.attributes != rhs.attributes) {
        return false;
    }
    else if (lhs.updateSequenceNum != rhs.updateSequenceNum) {
        return false;
    }
    else if (lhs.alternateData != rhs.alternateData) {
        return false;
    }

    return true;
}

bool operator!=(const Resource & lhs, const Resource & rhs)
{
    return !operator==(lhs, rhs);
}

bool operator==(const NoteAttributes & lhs, const NoteAttributes & rhs)
{
    if (lhs.subjectDate != rhs.subjectDate) {
        return false;
    }
    else if (lhs.latitude != rhs.latitude) {
        return false;
    }
    else if (lhs.longitude != rhs.longitude) {
        return false;
    }
    else if (lhs.altitude != rhs.altitude) {
        return false;
    }
    else if (lhs.author != rhs.author) {
        return false;
    }
    else if (lhs.source != rhs.source) {
        return false;
    }
    else if (lhs.sourceURL != rhs.sourceURL) {
        return false;
    }
    else if (lhs.sourceApplication != rhs.sourceApplication) {
        return false;
    }
    else if (lhs.shareDate != rhs.shareDate) {
        return false;
    }
    else if (lhs.reminderOrder != rhs.reminderOrder) {
        return false;
    }
    else if (lhs.reminderDoneTime != rhs.reminderDoneTime) {
        return false;
    }
    else if (lhs.reminderTime != rhs.reminderTime) {
        return false;
    }
    else if (lhs.placeName != rhs.placeName) {
        return false;
    }
    else if (lhs.contentClass != rhs.contentClass) {
        return false;
    }
    else if (lhs.applicationData != rhs.applicationData) {
        return false;
    }
    else if (lhs.lastEditedBy != rhs.lastEditedBy) {
        return false;
    }
    else if (lhs.classifications != rhs.classifications) {
        return false;
    }
    else if (lhs.creatorId != rhs.creatorId) {
        return false;
    }
    else if (lhs.lastEditorId != rhs.lastEditorId) {
        return false;
    }

    return true;
}

bool operator!=(const NoteAttributes & lhs, const NoteAttributes & rhs)
{
    return !(operator==(lhs, rhs));
}

bool operator==(const Note & lhs, const Note & rhs)
{
    if (lhs.guid != rhs.guid) {
        return false;
    }
    else if (lhs.title != rhs.title) {
        return false;
    }
    else if (lhs.content != rhs.content) {
        return false;
    }
    else if (lhs.contentHash != rhs.contentHash) {
        return false;
    }
    else if (lhs.contentLength != rhs.contentLength) {
        return false;
    }
    else if (lhs.created != rhs.created) {
        return false;
    }
    else if (lhs.updated != rhs.updated) {
        return false;
    }
    else if (lhs.deleted != rhs.deleted) {
        return false;
    }
    else if (lhs.active != rhs.active) {
        return false;
    }
    else if (lhs.updateSequenceNum != rhs.updateSequenceNum) {
        return false;
    }
    else if (lhs.notebookGuid != rhs.notebookGuid) {
        return false;
    }
    else if (lhs.tagGuids != rhs.tagGuids) {
        return false;
    }
    else if (lhs.resources != rhs.resources) {
        return false;
    }
    else if (lhs.attributes != rhs.attributes) {
        return false;
    }

    return true;
}

bool operator!=(const Note & lhs, const Note & rhs)
{
    return !operator==(lhs, rhs);
}
*/
}
