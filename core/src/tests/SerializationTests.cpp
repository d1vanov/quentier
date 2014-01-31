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
            errorDescription.append("Initial BusinessUserInfo: \n");
            errorDescription.append(ToQString<evernote::edam::BusinessUserInfo>(info));
            errorDescription.append("Deserialized BusinessUserInfo: \n");
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
            errorDescription.append("Initial PremiumInfo: \n");
            errorDescription.append(ToQString<evernote::edam::PremiumInfo>(info));
            errorDescription.append("Deserialized PremiumInfo: \n");
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
            errorDescription.append("Initial Accounting: \n");
            errorDescription.append(ToQString<evernote::edam::Accounting>(accounting));
            errorDescription.append("Deserialized Accounting: \n");
            errorDescription.append(ToQString<evernote::edam::Accounting>(deserializedAccounting));

            return false;
        }
    }

#undef ACCOUNTING_NUM_COMPONENTS

    return true;
}

bool TestUserAttributesSerialization(QString & errorDescription)
{
    evernote::edam::UserAttributes attributes;
    auto & isSet = attributes.__isset;

    // number of optional data components in UserAttributes
#define USER_ATTRIBUTES_NUM_COMPONENTS 20
    for(int mask = 0; mask != (1 << USER_ATTRIBUTES_NUM_COMPONENTS); ++mask)
    {
        attributes = evernote::edam::UserAttributes();

        std::bitset<USER_ATTRIBUTES_NUM_COMPONENTS> bits(mask);

        isSet.defaultLocationName = bits[0];
        isSet.defaultLatitude = bits[1];
        isSet.defaultLongitude = bits[1];
        isSet.preactivation = bits[2];
        isSet.incomingEmailAddress = bits[3];
        isSet.comments = bits[4];
        isSet.dateAgreedToTermsOfService = bits[5];
        isSet.maxReferrals = bits[6];
        isSet.referralCount = bits[6];
        isSet.refererCode = bits[6];
        isSet.sentEmailDate = bits[7];
        isSet.sentEmailCount = bits[7];
        isSet.dailyEmailLimit = bits[7];
        isSet.emailOptOutDate = bits[8];
        isSet.partnerEmailOptInDate = bits[8];
        isSet.preferredCountry = bits[9];
        isSet.preferredLanguage = bits[9];
        isSet.clipFullPage = bits[10];
        isSet.twitterUserName = bits[11];
        isSet.twitterId = bits[11];
        isSet.groupName = bits[12];
        isSet.recognitionLanguage = bits[13];
        isSet.referralProof = bits[14];
        isSet.educationalDiscount = bits[15];
        isSet.businessAddress = bits[16];
        isSet.hideSponsorBilling = bits[17];
        isSet.taxExempt = bits[18];
        isSet.useEmailAutoFiling = bits[19];
        isSet.reminderEmailConfig = bits[20];

        if (isSet.defaultLocationName) {
            attributes.defaultLocationName = "Saint-Petersburg";
        }

        if (isSet.defaultLatitude) {
            attributes.defaultLatitude = 12.0;
        }

        if (isSet.defaultLongitude) {
            attributes.defaultLongitude = 42.0;
        }

        if (isSet.preactivation) {
            attributes.preactivation = true;
        }

        if (isSet.incomingEmailAddress) {
            attributes.incomingEmailAddress = "hola@amigo.com";
        }

        if (isSet.comments) {
            attributes.comments = "I always wondered what people write in comments sections like this?";
        }

        if (isSet.dateAgreedToTermsOfService) {
            attributes.dateAgreedToTermsOfService = static_cast<Timestamp>(512);
        }

        if (isSet.maxReferrals) {
            attributes.maxReferrals = 42;
        }

        if (isSet.referralCount) {
            attributes.referralCount = 41;
        }

        if (isSet.refererCode) {
            attributes.refererCode = "err, don't know what to write here";
        }

        if (isSet.sentEmailDate) {
            attributes.sentEmailDate = static_cast<Timestamp>(100);
        }

        if (isSet.sentEmailCount) {
            attributes.sentEmailCount = 10;
        }

        if (isSet.dailyEmailLimit) {
            attributes.dailyEmailLimit = 100;
        }

        if (isSet.emailOptOutDate) {
            attributes.emailOptOutDate = static_cast<Timestamp>(90);
        }

        if (isSet.partnerEmailOptInDate) {
            attributes.partnerEmailOptInDate = static_cast<Timestamp>(80);
        }

        if (isSet.preferredCountry) {
            attributes.preferredCountry = "Russia, Soviet one preferably";
        }

        if (isSet.preferredLanguage) {
            attributes.preferredLanguage = "Russian. No, seriously, it's a very nice language";
        }

        if (isSet.clipFullPage) {
            attributes.clipFullPage = true;
        }

        if (isSet.twitterUserName) {
            attributes.twitterUserName = "140 symbols would always be enough for everyone";
        }

        if (isSet.twitterId) {
            attributes.twitterId = "why?";
        }

        if (isSet.groupName) {
            attributes.groupName = "In Flames. This band's music is pretty neat for long development evenings";
        }

        if (isSet.recognitionLanguage) {
            attributes.recognitionLanguage = "Chinese. Haha, come on, try to recognize it";
        }

        if (isSet.referralProof) {
            attributes.referralProof = "Always have all the necessary proofs with you";
        }

        if (isSet.educationalDiscount) {
            attributes.educationalDiscount = true;
        }

        if (isSet.businessAddress) {
            attributes.businessAddress = "Milky Way galaxy";
        }

        if (isSet.hideSponsorBilling) {
            attributes.hideSponsorBilling = true;   // Really, hide this annoying thing!
        }

        if (isSet.taxExempt) {
            attributes.taxExempt = false;
        }

        if (isSet.reminderEmailConfig) {
            attributes.reminderEmailConfig = evernote::edam::ReminderEmailConfig::SEND_DAILY_EMAIL;
        }

        QByteArray serializedAttributes = GetSerializedUserAttributes(attributes);
        evernote::edam::UserAttributes deserializedAttributes = GetDeserializedUserAttributes(serializedAttributes);

        if (attributes != deserializedAttributes)
        {
            errorDescription = "Serialization test for UserAttributes FAILED! ";
            errorDescription.append("Initial UserAttributes: \n");
            errorDescription.append(ToQString<evernote::edam::UserAttributes>(attributes));
            errorDescription.append("Deserialized UserAttributes: \n");
            errorDescription.append(ToQString<evernote::edam::UserAttributes>(deserializedAttributes));

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
            attributes.subjectDate = static_cast<Timestamp>(512);
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
            attributes.shareDate = static_cast<Timestamp>(10);
        }

        if (isSet.reminderOrder) {
            attributes.reminderOrder = 2;
        }

        if (isSet.reminderDoneTime) {
            attributes.reminderDoneTime = static_cast<Timestamp>(20);
        }

        if (isSet.reminderTime) {
            attributes.reminderTime = static_cast<Timestamp>(40);
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
            attributes.lastEditorId = static_cast<UserID>(10);
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

}
}
