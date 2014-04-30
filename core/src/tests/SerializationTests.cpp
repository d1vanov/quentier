#include "SerializationTests.h"
#include <client/Serialization.h>
#include <client/types/QEverCloudHelpers.h>
#include <tools/Printable.h>
#include <Types_types.h>
#include <bitset>

namespace qute_note {
namespace test {

bool TestBusinessUserInfoSerialization(QString & errorDescription)
{
    qevercloud::BusinessUserInfo info;

    // number of optional data components of BusinessUserInfo
#define BUSINESS_USER_INFO_NUM_COMPONENTS 4

    for(int mask = 0; mask != (1 << BUSINESS_USER_INFO_NUM_COMPONENTS); ++mask)
    {
        std::bitset<BUSINESS_USER_INFO_NUM_COMPONENTS> bits(mask);
        info = qevercloud::BusinessUserInfo();

        bool isSetBusinessId   = bits[0];
        bool isSetBusinessName = bits[1];
        bool isSetEmail = bits[2];
        bool isSetRole  = bits[3];

        if (isSetBusinessId) {
            info.businessId = mask;
        }

        if (isSetBusinessName) {
            info.businessName = "Charlie";
        }

        if (isSetEmail) {
            info.email = "nevertrustaliens@frustration.com";
        }

        if (isSetRole) {
            info.role = qevercloud::BusinessUserRole::NORMAL;
        }

        QByteArray serializedInfo = GetSerializedBusinessUserInfo(info);
        qevercloud::BusinessUserInfo deserializedInfo = GetDeserializedBusinessUserInfo(serializedInfo);

        if (info != deserializedInfo)
        {
            errorDescription = "Serialization test for BusinessUserInfo FAILED! ";
            errorDescription.append("Initial BusinessUserInfo: \n");
            errorDescription.append(ToQString<qevercloud::BusinessUserInfo>(info));
            errorDescription.append("Deserialized BusinessUserInfo: \n");
            errorDescription.append(ToQString<qevercloud::BusinessUserInfo>(deserializedInfo));

            return false;
        }
    }

#undef BUSINESS_USER_INFO_NUM_COMPONENTS

    return true;
}

bool TestPremiumInfoSerialization(QString & errorDescription)
{
    qevercloud::PremiumInfo info;

    // number of optional data components in PremiumInfo
#define PREMIUM_INFO_NUM_COMPONENTS 9

    for(int mask = 0; mask != (1 << PREMIUM_INFO_NUM_COMPONENTS); ++mask)
    {
        info = qevercloud::PremiumInfo();
        std::bitset<PREMIUM_INFO_NUM_COMPONENTS> bits(mask);

        bool isSetPremiumExpirationDate = bits[0];

        info.premiumExtendable = bits[1];
        info.premiumPending = bits[2];
        info.premiumCancellationPending = bits[3];
        info.canPurchaseUploadAllowance = bits[4];

        bool isSetSponsoredGroupName = bits[5];
        bool isSetSponsoredGroupRole = bits[6];
        bool isSetPremiumUpgradable = bits[7];

        if (isSetPremiumExpirationDate) {
            info.premiumExpirationDate = static_cast<qevercloud::Timestamp>(757688758765);
        }

        if (isSetSponsoredGroupName) {
            info.sponsoredGroupName = "Chicago";
        }

        if (isSetSponsoredGroupRole) {
            info.sponsoredGroupRole = qevercloud::SponsoredGroupRole::GROUP_MEMBER;
        }

        if (isSetPremiumUpgradable) {
            info.premiumUpgradable = bits[8];
        }

        QByteArray serializedInfo = GetSerializedPremiumInfo(info);
        qevercloud::PremiumInfo deserializedInfo = GetDeserializedPremiumInfo(serializedInfo);

        if (info != deserializedInfo)
        {
            errorDescription = "Serialization test for PremiumInfo FAILED! ";
            errorDescription.append("Initial PremiumInfo: \n");
            errorDescription.append(ToQString<qevercloud::PremiumInfo>(info));
            errorDescription.append("Deserialized PremiumInfo: \n");
            errorDescription.append(ToQString<qevercloud::PremiumInfo>(deserializedInfo));

            return false;
        }
    }

#undef PREMIUM_INFO_NUM_COMPONENTS

    return true;
}

bool TestAccountingSerialization(QString & errorDescription)
{
    qevercloud::Accounting accounting;

    // number of optional data components in Accounting
#define ACCOUNTING_NUM_COMPONENTS 23
    for(int mask = 0; mask != (1 << ACCOUNTING_NUM_COMPONENTS); ++mask)
    {
        accounting = qevercloud::Accounting();

        std::bitset<ACCOUNTING_NUM_COMPONENTS> bits(mask);

        bool isSetUploadLimit = bits[0];
        bool isSetUploadLimitEnd = bits[1];
        bool isSetUploadLimitNextMonth = bits[2];
        bool isSetPremiumServiceStatus = bits[3];
        bool isSetPremiumOrderNumber = bits[4];
        bool isSetPremiumCommerceService = bits[5];
        bool isSetPremiumServiceStart = bits[6];
        bool isSetPremiumServiceSKU = bits[7];
        bool isSetLastSuccessfulCharge = bits[8];
        bool isSetLastFailedCharge = bits[9];
        bool isSetLastFailedChargeReason = bits[10];
        bool isSetNextPaymentDue = bits[11];
        bool isSetPremiumLockUntil = bits[12];
        bool isSetUpdated = bits[13];
        bool isSetPremiumSubscriptionNumber = bits[14];
        bool isSetLastRequestedCharge = bits[15];
        bool isSetCurrency = bits[16];
        bool isSetUnitPrice = bits[17];
        bool isSetBusinessId = bits[18];
        bool isSetBusinessName = bits[19];
        bool isSetBusinessRole = bits[20];
        bool isSetUnitDiscount = bits[21];
        bool isSetNextChargeDate = bits[22];

        if (isSetUploadLimit) {
            accounting.uploadLimit = 512;
        }

        if (isSetUploadLimitEnd) {
            accounting.uploadLimitEnd = 1024;
        }

        if (isSetUploadLimitNextMonth) {
            accounting.uploadLimitNextMonth = 2048;
        }

        if (isSetPremiumServiceStatus) {
            accounting.premiumServiceStatus = qevercloud::PremiumOrderStatus::ACTIVE;
        }

        if (isSetPremiumOrderNumber) {
            accounting.premiumOrderNumber = "mycoolpremiumordernumberhah";
        }

        if (isSetPremiumCommerceService) {
            accounting.premiumCommerceService = "atyourservice";
        }

        if (isSetPremiumServiceStart) {
            accounting.premiumServiceStart = static_cast<qevercloud::Timestamp>(300);
        }

        if (isSetPremiumServiceSKU) {
            accounting.premiumServiceSKU = "premiumservicesku";
        }

        if (isSetLastSuccessfulCharge) {
            accounting.lastSuccessfulCharge = static_cast<qevercloud::Timestamp>(305);
        }

        if (isSetLastFailedCharge) {
            accounting.lastFailedCharge = static_cast<qevercloud::Timestamp>(295);
        }

        if (isSetLastFailedChargeReason) {
            accounting.lastFailedChargeReason = "youtriedtotrickme!";
        }

        if (isSetNextPaymentDue) {
            accounting.nextPaymentDue = static_cast<qevercloud::Timestamp>(400);
        }

        if (isSetPremiumLockUntil) {
            accounting.premiumLockUntil = static_cast<qevercloud::Timestamp>(310);
        }

        if (isSetUpdated) {
            accounting.updated = static_cast<qevercloud::Timestamp>(306);
        }

        if (isSetPremiumSubscriptionNumber) {
            accounting.premiumSubscriptionNumber = "premiumsubscriptionnumber";
        }

        if (isSetLastRequestedCharge) {
            accounting.lastRequestedCharge = static_cast<qevercloud::Timestamp>(297);
        }

        if (isSetCurrency) {
            accounting.currency = "USD";
        }

        if (isSetUnitPrice) {
            accounting.unitPrice = 25;
        }

        if (isSetBusinessId) {
            accounting.businessId = 1234558;
        }

        if (isSetBusinessName) {
            accounting.businessName = "businessname";
        }

        if (isSetBusinessRole) {
            accounting.businessRole = qevercloud::BusinessUserRole::NORMAL;
        }

        if (isSetUnitDiscount) {
            accounting.unitDiscount = 5;
        }

        if (isSetNextChargeDate) {
            accounting.nextChargeDate = static_cast<qevercloud::Timestamp>(395);
        }

        QByteArray serializedAccounting = GetSerializedAccounting(accounting);
        qevercloud::Accounting deserializedAccounting = GetDeserializedAccounting(serializedAccounting);

        if (accounting != deserializedAccounting)
        {
            errorDescription = "Serialization test for Accounting FAILED! ";
            errorDescription.append("Initial Accounting: \n");
            errorDescription.append(ToQString<qevercloud::Accounting>(accounting));
            errorDescription.append("Deserialized Accounting: \n");
            errorDescription.append(ToQString<qevercloud::Accounting>(deserializedAccounting));

            return false;
        }
    }

#undef ACCOUNTING_NUM_COMPONENTS

    return true;
}

bool TestUserAttributesSerialization(QString & errorDescription)
{
    qevercloud::UserAttributes attributes;

    // number of optional data components in UserAttributes
#define USER_ATTRIBUTES_NUM_COMPONENTS 20
    for(int mask = 0; mask != (1 << USER_ATTRIBUTES_NUM_COMPONENTS); ++mask)
    {
        attributes = qevercloud::UserAttributes();

        std::bitset<USER_ATTRIBUTES_NUM_COMPONENTS> bits(mask);

        bool isSetDefaultLocationName = bits[0];
        bool isSetDefaultLatitude = bits[1];
        bool isSetDefaultLongitude = bits[1];
        bool isSetPreactivation = bits[2];
        bool isSetIncomingEmailAddress = bits[3];
        bool isSetComments = bits[4];
        bool isSetDateAgreedToTermsOfService = bits[5];
        bool isSetMaxReferrals = bits[6];
        bool isSetReferralCount = bits[6];
        bool isSetRefererCode = bits[6];
        bool isSetSentEmailDate = bits[7];
        bool isSetSentEmailCount = bits[7];
        bool isSetDailyEmailLimit = bits[7];
        bool isSetEmailOptOutDate = bits[8];
        bool isSetPartnerEmailOptInDate = bits[8];
        bool isSetPreferredCountry = bits[9];
        bool isSetPreferredLanguage = bits[9];
        bool isSetClipFullPage = bits[10];
        bool isSetTwitterUserName = bits[11];
        bool isSetTwitterId = bits[11];
        bool isSetGroupName = bits[12];
        bool isSetRecognitionLanguage = bits[13];
        bool isSetReferralProof = bits[14];
        bool isSetEducationalDiscount = bits[15];
        bool isSetBusinessAddress = bits[16];
        bool isSetHideSponsorBilling = bits[17];
        bool isSetTaxExempt = bits[18];
        bool isSetUseEmailAutoFiling = bits[19];
        bool isSetReminderEmailConfig = bits[20];

        if (isSetDefaultLocationName) {
            attributes.defaultLocationName = "Saint-Petersburg";
        }

        if (isSetDefaultLatitude) {
            attributes.defaultLatitude = 12.0;
        }

        if (isSetDefaultLongitude) {
            attributes.defaultLongitude = 42.0;
        }

        if (isSetPreactivation) {
            attributes.preactivation = true;
        }

        if (isSetIncomingEmailAddress) {
            attributes.incomingEmailAddress = "hola@amigo.com";
        }

        if (isSetComments) {
            attributes.comments = "I always wondered what people write in comments sections like this?";
        }

        if (isSetDateAgreedToTermsOfService) {
            attributes.dateAgreedToTermsOfService = static_cast<qevercloud::Timestamp>(512);
        }

        if (isSetMaxReferrals) {
            attributes.maxReferrals = 42;
        }

        if (isSetReferralCount) {
            attributes.referralCount = 41;
        }

        if (isSetRefererCode) {
            attributes.refererCode = "err, don't know what to write here";
        }

        if (isSetSentEmailDate) {
            attributes.sentEmailDate = static_cast<qevercloud::Timestamp>(100);
        }

        if (isSetSentEmailCount) {
            attributes.sentEmailCount = 10;
        }

        if (isSetDailyEmailLimit) {
            attributes.dailyEmailLimit = 100;
        }

        if (isSetEmailOptOutDate) {
            attributes.emailOptOutDate = static_cast<qevercloud::Timestamp>(90);
        }

        if (isSetPartnerEmailOptInDate) {
            attributes.partnerEmailOptInDate = static_cast<qevercloud::Timestamp>(80);
        }

        if (isSetPreferredCountry) {
            attributes.preferredCountry = "Russia, Soviet one preferably";
        }

        if (isSetPreferredLanguage) {
            attributes.preferredLanguage = "Russian. No, seriously, it's a very nice language";
        }

        if (isSetClipFullPage) {
            attributes.clipFullPage = true;
        }

        if (isSetTwitterUserName) {
            attributes.twitterUserName = "140 symbols would always be enough for everyone";
        }

        if (isSetTwitterId) {
            attributes.twitterId = "why?";
        }

        if (isSetGroupName) {
            attributes.groupName = "In Flames. This band's music is pretty neat for long development evenings";
        }

        if (isSetRecognitionLanguage) {
            attributes.recognitionLanguage = "Chinese. Haha, come on, try to recognize it";
        }

        if (isSetReferralProof) {
            attributes.referralProof = "Always have all the necessary proofs with you";
        }

        if (isSetEducationalDiscount) {
            attributes.educationalDiscount = true;
        }

        if (isSetBusinessAddress) {
            attributes.businessAddress = "Milky Way galaxy";
        }

        if (isSetHideSponsorBilling) {
            attributes.hideSponsorBilling = true;   // Really, hide this annoying thing!
        }

        if (isSetTaxExempt) {
            attributes.taxExempt = false;
        }

        if (isSetReminderEmailConfig) {
            attributes.reminderEmailConfig = qevercloud::ReminderEmailConfig::SEND_DAILY_EMAIL;
        }

        QByteArray serializedAttributes = GetSerializedUserAttributes(attributes);
        qevercloud::UserAttributes deserializedAttributes = GetDeserializedUserAttributes(serializedAttributes);

        if (attributes != deserializedAttributes)
        {
            errorDescription = "Serialization test for UserAttributes FAILED! ";
            errorDescription.append("Initial UserAttributes: \n");
            errorDescription.append(ToQString<qevercloud::UserAttributes>(attributes));
            errorDescription.append("Deserialized UserAttributes: \n");
            errorDescription.append(ToQString<qevercloud::UserAttributes>(deserializedAttributes));

            return false;
        }
    }

#undef USER_ATTRIBUTES_NUM_COMPONENTS

    return true;
}

bool TestNoteAttributesSerialization(QString & errorDescription)
{
    evernote::edam::NoteAttributes attributes;
    auto & isSet = attributes.__isset;

    // number of optional data components in NoteAttributes
#define NOTE_ATTRIBUTES_NUM_COMPONENTS 19
    for(int mask = 0; mask != (1 << NOTE_ATTRIBUTES_NUM_COMPONENTS); ++mask)
    {
        attributes = evernote::edam::NoteAttributes();

        std::bitset<NOTE_ATTRIBUTES_NUM_COMPONENTS> bits(mask);

        isSet.subjectDate = bits[0];
        isSet.latitude = bits[1];
        isSet.longitude = bits[2];
        isSet.altitude = bits[3];
        isSet.author = bits[4];
        isSet.source = bits[5];
        isSet.sourceURL = bits[6];
        isSet.sourceApplication = bits[7];
        isSet.shareDate = bits[8];
        isSet.reminderOrder = bits[9];
        isSet.reminderDoneTime = bits[10];
        isSet.reminderTime = bits[11];
        isSet.placeName = bits[12];
        isSet.contentClass = bits[13];
        isSet.applicationData = bits[14];
        isSet.lastEditedBy = bits[15];
        isSet.classifications = bits[16];
        isSet.creatorId = bits[17];
        isSet.lastEditorId = bits[18];

        if (isSet.subjectDate) {
            attributes.subjectDate = static_cast<evernote::edam::Timestamp>(512);
        }

        if (isSet.latitude) {
            attributes.latitude = 42.0;
        }

        if (isSet.longitude) {
            attributes.longitude = 43.0;
        }

        if (isSet.altitude) {
            attributes.altitude = 42.0;
        }

        if (isSet.author) {
            attributes.author = "haxpeha";
        }

        if (isSet.source) {
            attributes.source = "brain";
        }

        if (isSet.sourceURL) {
            attributes.sourceURL = "https://github.com/d1vanov";
        }

        if (isSet.sourceApplication) {
            attributes.sourceApplication = "Qt Creator";
        }

        if (isSet.shareDate) {
            attributes.shareDate = static_cast<evernote::edam::Timestamp>(10);
        }

        if (isSet.reminderOrder) {
            attributes.reminderOrder = 2;
        }

        if (isSet.reminderDoneTime) {
            attributes.reminderDoneTime = static_cast<evernote::edam::Timestamp>(20);
        }

        if (isSet.reminderTime) {
            attributes.reminderTime = static_cast<evernote::edam::Timestamp>(40);
        }

        if (isSet.placeName) {
            attributes.placeName = "My place";
        }

        if (isSet.contentClass) {
            attributes.contentClass = "text";
        }

        if (isSet.applicationData)
        {
            auto & applicationData = attributes.applicationData;
            auto & applicationDataKeys = applicationData.keysOnly;
            auto & applicationDataMap  = applicationData.fullMap;

            applicationDataKeys.insert("key1");
            applicationDataKeys.insert("key2");
            applicationDataKeys.insert("key3");

            applicationDataMap["key1"] = "value1";
            applicationDataMap["key2"] = "value2";
            applicationDataMap["key3"] = "value3";
        }

        if (isSet.lastEditedBy) {
            attributes.lastEditedBy = "Me";
        }

        if (isSet.classifications)
        {
            auto & classifications = attributes.classifications;

            classifications["classificationKey1"] = "classificationValue1";
            classifications["classificationKey2"] = "classificationValue2";
            classifications["classificationKey3"] = "classificationValue3";
        }

        if (isSet.lastEditorId) {
            attributes.lastEditorId = static_cast<evernote::edam::UserID>(10);
        }

        QByteArray serializedAttributes = GetSerializedNoteAttributes(attributes);
        evernote::edam::NoteAttributes deserializedAttributes = GetDeserializedNoteAttributes(serializedAttributes);

        if (attributes != deserializedAttributes)
        {
            errorDescription = "Serialization test for NoteAttributes FAILED! ";
            errorDescription.append("Initial NoteAttributes: \n");
            errorDescription.append(ToQString<evernote::edam::NoteAttributes>(attributes));
            errorDescription.append("Deserialized NoteAttributes: \n");
            errorDescription.append(ToQString<evernote::edam::NoteAttributes>(deserializedAttributes));

            return false;
        }
    }

#undef NOTE_ATTRIBUTES_NUM_COMPONENTS

    return true;
}

bool TestResourceAttributesSerialization(QString & errorDescription)
{
    evernote::edam::ResourceAttributes attributes;
    auto & isSet = attributes.__isset;

    // number of optional data components in ResourceAttributes
#define RESOURCE_ATTRIBUTES_NUM_COMPONENTS 14
    for(int mask = 0; mask != (1 << RESOURCE_ATTRIBUTES_NUM_COMPONENTS); ++mask)
    {
        attributes = evernote::edam::ResourceAttributes();

        std::bitset<RESOURCE_ATTRIBUTES_NUM_COMPONENTS> bits(mask);

        isSet.sourceURL = bits[0];
        isSet.timestamp = bits[1];
        isSet.latitude = bits[2];
        isSet.longitude = bits[3];
        isSet.altitude = bits[4];
        isSet.cameraMake = bits[5];
        isSet.cameraModel = bits[6];
        isSet.clientWillIndex = bits[7];
        isSet.recoType = bits[8];
        isSet.fileName = bits[9];
        isSet.attachment = bits[10];
        isSet.applicationData = bits[11];

        if (isSet.sourceURL) {
            attributes.sourceURL = "https://github.com/d1vanov";
        }

        if (isSet.timestamp) {
            attributes.timestamp = static_cast<evernote::edam::Timestamp>(10);
        }

        if (isSet.latitude) {
            attributes.latitude = 20.0;
        }

        if (isSet.longitude) {
            attributes.longitude = 30.0;
        }

        if (isSet.altitude) {
            attributes.altitude = 40.0;
        }

        if (isSet.cameraMake) {
            attributes.cameraMake = "Something...";
        }

        if (isSet.cameraModel) {
            attributes.cameraModel = "Canon or Nikon?";
        }

        if (isSet.clientWillIndex) {
            attributes.clientWillIndex = bits[12];
        }

        if (isSet.recoType) {
            attributes.recoType = "text";
        }

        if (isSet.fileName) {
            attributes.fileName = "some file";
        }

        if (isSet.attachment) {
            attributes.attachment = bits[13];
        }

        if (isSet.applicationData)
        {
            auto & applicationData = attributes.applicationData;
            auto & applicationDataKeys = applicationData.keysOnly;
            auto & applicationDataMap  = applicationData.fullMap;

            applicationDataKeys.insert("key1");
            applicationDataKeys.insert("key2");
            applicationDataKeys.insert("key3");

            applicationDataMap["key1"] = "value1";
            applicationDataMap["key2"] = "value2";
            applicationDataMap["key3"] = "value3";
        }

        QByteArray serializedAttributes = GetSerializedResourceAttributes(attributes);
        evernote::edam::ResourceAttributes deserializedAttributes = GetDeserializedResourceAttributes(serializedAttributes);

        if (attributes != deserializedAttributes)
        {
            errorDescription = "Serialization test for ResourceAttributes FAILED! ";
            errorDescription.append("Initial ResourceAttributes: \n");
            errorDescription.append(ToQString<evernote::edam::ResourceAttributes>(attributes));
            errorDescription.append("Deserialized ResourceAttributes: \n");
            errorDescription.append(ToQString<evernote::edam::ResourceAttributes>(deserializedAttributes));

            return false;
        }
    }

#undef RESOURCE_ATTRIBUTES_NUM_COMPONENTS

    return true;
}

}
}
