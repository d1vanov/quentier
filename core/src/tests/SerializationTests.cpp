#include "SerializationTests.h"
#include "../client/EnWrappers.h"
#include "../tools/Printable.h"
#include <Types_types.h>
#include <bitset>

namespace qute_note {
namespace test {

bool TestBusinessUserInfoSerialization(QString & errorDescription)
{
    evernote::edam::BusinessUserInfo info;
    auto & isSet = info.__isset;

    // number of optional data components of BusinessUserInfo
#define BUSINESS_USER_INFO_NUM_COMPONENTS 4

    for(int mask = 0; mask != (1 << BUSINESS_USER_INFO_NUM_COMPONENTS); ++mask)
    {
        std::bitset<BUSINESS_USER_INFO_NUM_COMPONENTS> bits(mask);
        info = evernote::edam::BusinessUserInfo();

        isSet.businessId   = bits[0];
        isSet.businessName = bits[1];
        isSet.email = bits[2];
        isSet.role  = bits[3];

        if (isSet.businessId) {
            info.businessId = mask;
        }

        if (isSet.businessName) {
            info.businessName = "Charlie";
        }

        if (isSet.email) {
            info.email = "nevertrustaliens@frustration.com";
        }

        if (isSet.role) {
            info.role = evernote::edam::BusinessUserRole::NORMAL;
        }

        QByteArray serializedInfo = GetSerializedBusinessUserInfo(info);
        evernote::edam::BusinessUserInfo deserializedInfo = GetDeserializedBusinessUserInfo(serializedInfo);

        if (info != deserializedInfo)
        {
            errorDescription = "Serialization test for BusinessUserInfo FAILED! ";
            errorDescription.append("Initial BusinessUserInfo: ");
            errorDescription.append(ToQString<evernote::edam::BusinessUserInfo>(info));
            errorDescription.append("Deserialized BusinessUserInfo: ");
            errorDescription.append(ToQString<evernote::edam::BusinessUserInfo>(deserializedInfo));

            return false;
        }
    }

#undef BUSINESS_USER_INFO_NUM_COMPONENTS

    return true;
}

bool TestPremiumInfoSerialization(QString & errorDescription)
{
    evernote::edam::PremiumInfo info;
    auto & isSet = info.__isset;

    // number of optional data components in PremiumInfo
#define PREMIUM_INFO_NUM_COMPONENTS 9

    for(int mask = 0; mask != (1 << PREMIUM_INFO_NUM_COMPONENTS); ++mask)
    {
        info = evernote::edam::PremiumInfo();
        std::bitset<PREMIUM_INFO_NUM_COMPONENTS> bits(mask);

        isSet.premiumExpirationDate = bits[0];
        info.premiumExtendable = bits[1];
        info.premiumPending = bits[2];
        info.premiumCancellationPending = bits[3];
        info.canPurchaseUploadAllowance = bits[4];
        isSet.sponsoredGroupName = bits[5];
        isSet.sponsoredGroupRole = bits[6];
        isSet.premiumUpgradable = bits[7];

        if (isSet.premiumExpirationDate) {
            info.premiumExpirationDate = static_cast<Timestamp>(757688758765);
        }

        if (isSet.sponsoredGroupName) {
            info.sponsoredGroupName = "Chicago";
        }

        if (isSet.sponsoredGroupRole) {
            info.sponsoredGroupRole = evernote::edam::SponsoredGroupRole::GROUP_MEMBER;
        }

        if (isSet.premiumUpgradable) {
            info.premiumUpgradable = bits[8];
        }

        QByteArray serializedInfo = GetSerializedPremiumInfo(info);
        evernote::edam::PremiumInfo deserializedInfo = GetDeserializedPremiumInfo(serializedInfo);

        if (info != deserializedInfo)
        {
            errorDescription = "Serialization test for PremiumInfo FAILED! ";
            errorDescription.append("Initial PremiumInfo: ");
            errorDescription.append(ToQString<evernote::edam::PremiumInfo>(info));
            errorDescription.append("Deserialized PremiumInfo: ");
            errorDescription.append(ToQString<evernote::edam::PremiumInfo>(deserializedInfo));

            return false;
        }
    }

#undef PREMIUM_INFO_NUM_COMPONENTS

    return true;
}

bool TestAccountingSerialization(QString & errorDescription)
{
    evernote::edam::Accounting accounting;
    auto & isSet = accounting.__isset;

    // number of optional data components in Accounting
#define ACCOUNTING_NUM_COMPONENTS 23
    for(int mask = 0; mask != (1 << ACCOUNTING_NUM_COMPONENTS); ++mask)
    {
        accounting = evernote::edam::Accounting();

        std::bitset<ACCOUNTING_NUM_COMPONENTS> bits(mask);

        isSet.uploadLimit = bits[0];
        isSet.uploadLimitEnd = bits[1];
        isSet.uploadLimitNextMonth = bits[2];
        isSet.premiumServiceStatus = bits[3];
        isSet.premiumOrderNumber = bits[4];
        isSet.premiumCommerceService = bits[5];
        isSet.premiumServiceStart = bits[6];
        isSet.premiumServiceSKU = bits[7];
        isSet.lastSuccessfulCharge = bits[8];
        isSet.lastFailedCharge = bits[9];
        isSet.lastFailedChargeReason = bits[10];
        isSet.nextPaymentDue = bits[11];
        isSet.premiumLockUntil = bits[12];
        isSet.updated = bits[13];
        isSet.premiumSubscriptionNumber = bits[14];
        isSet.lastRequestedCharge = bits[15];
        isSet.currency = bits[16];
        isSet.unitPrice = bits[17];
        isSet.businessId = bits[18];
        isSet.businessName = bits[19];
        isSet.businessRole = bits[20];
        isSet.unitDiscount = bits[21];
        isSet.nextChargeDate = bits[22];

        if (isSet.uploadLimit) {
            accounting.uploadLimit = 512;
        }

        if (isSet.uploadLimitEnd) {
            accounting.uploadLimitEnd = 1024;
        }

        if (isSet.uploadLimitNextMonth) {
            accounting.uploadLimitNextMonth = 2048;
        }

        if (isSet.premiumServiceStatus) {
            accounting.premiumServiceStatus = evernote::edam::PremiumOrderStatus::ACTIVE;
        }

        if (isSet.premiumOrderNumber) {
            accounting.premiumOrderNumber = "mycoolpremiumordernumberhah";
        }

        if (isSet.premiumCommerceService) {
            accounting.premiumCommerceService = "atyourservice";
        }

        if (isSet.premiumServiceStart) {
            accounting.premiumServiceStart = static_cast<Timestamp>(300);
        }

        if (isSet.premiumServiceSKU) {
            accounting.premiumServiceSKU = "premiumservicesku";
        }

        if (isSet.lastSuccessfulCharge) {
            accounting.lastSuccessfulCharge = static_cast<Timestamp>(305);
        }

        if (isSet.lastFailedCharge) {
            accounting.lastFailedCharge = static_cast<Timestamp>(295);
        }

        if (isSet.lastFailedChargeReason) {
            accounting.lastFailedChargeReason = "youtriedtotrickme!";
        }

        if (isSet.nextPaymentDue) {
            accounting.nextPaymentDue = static_cast<Timestamp>(400);
        }

        if (isSet.premiumLockUntil) {
            accounting.premiumLockUntil = static_cast<Timestamp>(310);
        }

        if (isSet.updated) {
            accounting.updated = static_cast<Timestamp>(306);
        }

        if (isSet.premiumSubscriptionNumber) {
            accounting.premiumSubscriptionNumber = "premiumsubscriptionnumber";
        }

        if (isSet.lastRequestedCharge) {
            accounting.lastRequestedCharge = static_cast<Timestamp>(297);
        }

        if (isSet.currency) {
            accounting.currency = "USD";
        }

        if (isSet.unitPrice) {
            accounting.unitPrice = 25;
        }

        if (isSet.businessId) {
            accounting.businessId = 1234558;
        }

        if (isSet.businessName) {
            accounting.businessName = "businessname";
        }

        if (isSet.businessRole) {
            accounting.businessRole = evernote::edam::BusinessUserRole::NORMAL;
        }

        if (isSet.unitDiscount) {
            accounting.unitDiscount = 5;
        }

        if (isSet.nextChargeDate) {
            accounting.nextChargeDate = static_cast<Timestamp>(395);
        }

        QByteArray serializedAccounting = GetSerializedAccounting(accounting);
        evernote::edam::Accounting deserializedAccounting = GetDeserializedAccounting(serializedAccounting);

        if (accounting != deserializedAccounting)
        {
            errorDescription = "Serialization test for Accounting FAILED! ";
            errorDescription.append("Initial Accounting: ");
            errorDescription.append(ToQString<evernote::edam::Accounting>(accounting));
            errorDescription.append("Deserialized Accounting: ");
            errorDescription.append(ToQString<evernote::edam::Accounting>(deserializedAccounting));

            return false;
        }
    }

    return true;
}

}
}
