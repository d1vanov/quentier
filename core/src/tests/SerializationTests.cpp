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

}
}
