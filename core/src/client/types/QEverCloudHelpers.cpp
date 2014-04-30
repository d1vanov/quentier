#include "QEverCloudHelpers.h"

namespace qevercloud {

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

}
