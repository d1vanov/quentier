#include "Serialization.h"
#include <QEverCloud.h>
#include "types/QEverCloudHelpers.h"
#include <Types_types.h>
#include <QDataStream>
#include <QByteArray>

// FIXME: switch all this to QEverCloud

namespace qute_note {

// TODO: consider setting some fixed version to each QDataStream

QDataStream & operator<<(QDataStream & out, const qevercloud::BusinessUserInfo & info)
{
#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = info.attribute.isSet(); \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(info.attribute.ref()); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(businessId, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(businessName);
    CHECK_AND_SET_ATTRIBUTE(role, static_cast<quint8>);
    CHECK_AND_SET_ATTRIBUTE(email);

#undef CHECK_AND_SET_ATTRIBUTE

    return out;
}

QDataStream & operator>>(QDataStream & in, qevercloud::BusinessUserInfo & info)
{
    info = qevercloud::BusinessUserInfo();

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            info.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(businessId, qint32, int32_t);
    CHECK_AND_SET_ATTRIBUTE(businessName, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(role, quint8, qevercloud::BusinessUserRole::type);
    CHECK_AND_SET_ATTRIBUTE(email, QString, QString);

#undef CHECK_AND_SET_ATTRIBUTE

    return in;
}

QDataStream & operator<<(QDataStream & out, const qevercloud::PremiumInfo & info)
{
    out << static_cast<qint64>(info.currentTime);
    out << info.premium;
    out << info.premiumRecurring;

#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = info.attribute.isSet(); \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(info.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(premiumExpirationDate, static_cast<qint64>);

    out << info.premiumExtendable;
    out << info.premiumPending;
    out << info.premiumCancellationPending;
    out << info.canPurchaseUploadAllowance;

    CHECK_AND_SET_ATTRIBUTE(sponsoredGroupName);
    CHECK_AND_SET_ATTRIBUTE(sponsoredGroupRole, static_cast<quint8>);
    CHECK_AND_SET_ATTRIBUTE(premiumUpgradable);

#undef CHECK_AND_SET_ATTRIBUTE

    return out;
}

QDataStream & operator>>(QDataStream & in, qevercloud::PremiumInfo & info)
{
    info = qevercloud::PremiumInfo();

    qint64 currentTime;
    in >> currentTime;
    info.currentTime = static_cast<qevercloud::Timestamp>(currentTime);

    in >> info.premium;
    in >> info.premiumRecurring;

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            info.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(premiumExpirationDate, qint64, qevercloud::Timestamp);

    in >> info.premiumExtendable;
    in >> info.premiumPending;
    in >> info.premiumCancellationPending;
    in >> info.canPurchaseUploadAllowance;

    CHECK_AND_SET_ATTRIBUTE(sponsoredGroupName, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(sponsoredGroupRole, quint8, qevercloud::SponsoredGroupRole::type);
    CHECK_AND_SET_ATTRIBUTE(premiumUpgradable, bool, bool);

#undef CHECK_AND_SET_ATTRIBUTE

    return in;
}

QDataStream & operator<<(QDataStream & out, const qevercloud::NoteAttributes & noteAttributes)
{
    const auto & isSet = noteAttributes.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = noteAttributes.attribute.isSet(); \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(noteAttributes.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(subjectDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(latitude);
    CHECK_AND_SET_ATTRIBUTE(longitude);
    CHECK_AND_SET_ATTRIBUTE(altitude);
    CHECK_AND_SET_ATTRIBUTE(author);
    CHECK_AND_SET_ATTRIBUTE(source);
    CHECK_AND_SET_ATTRIBUTE(sourceURL);
    CHECK_AND_SET_ATTRIBUTE(sourceApplication);
    CHECK_AND_SET_ATTRIBUTE(shareDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(reminderOrder, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(reminderDoneTime, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(reminderTime, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(placeName);
    CHECK_AND_SET_ATTRIBUTE(contentClass);
    CHECK_AND_SET_ATTRIBUTE(lastEditedBy);
    CHECK_AND_SET_ATTRIBUTE(creatorId, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(lastEditorId, static_cast<qint32>);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = noteAttributes.applicationData.isSet();
    out << isSetApplicationData;
    if (isSetApplicationData)
    {
        bool isSetKeysOnly = noteAttributes.applicationData->keysOnly.isSet();
        out << isSetKeysOnly;
        if (isSetKeysOnly) {
            const QSet<QString> & keysOnly = noteAttributes.applicationData->keysOnly;
            size_t numKeys = keys.size();
            out << static_cast<quint32>(numKeys);
            foreach(const QString & key, keysOnly) {
                out << key;
            }
        }

        bool isSetFullMap = noteAttributes.applicationData->fullMap.isSet();
        out << isSetFullMap;
        if (isSetFullMap) {
            const QMap<QString, QString> & fullMap = noteAttributes.applicationData->fullMap;
            size_t mapSize = map.size();
            out << static_cast<quint32>(mapSize);   // NOTE: the reasonable constraint is numKeys == mapSize but it's better to ensure
            foreach(const QString & key, fullMap) {
                out << key << fullMap.value(key);
            }
        }
    }

    bool isSetClassifications = noteAttributes.classifications.isSet();
    out << isSetClassifications;
    if (isSetClassifications)
    {
        const QMap<QString, QString> & fullMap = noteAttributes.classifications;
        size_t mapSize = fullMap.size();
        out << static_cast<quint32>(mapSize);
        foreach(const QString & key, fullMap) {
            out << key << fullMap.value(key);
        }
    }

    return out;
}

// TODO: continue from here

QDataStream & operator>>(QDataStream & in, evernote::edam::NoteAttributes & noteAttributes)
{
    noteAttributes = evernote::edam::NoteAttributes();

    auto & isSet = noteAttributes.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        isSet.attribute = isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            noteAttributes.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(subjectDate, qint64, evernote::edam::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(latitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(longitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(altitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(author, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(source, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(sourceURL, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(sourceApplication, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(shareDate, qint64, evernote::edam::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(reminderOrder, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(reminderDoneTime, qint64, evernote::edam::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(reminderTime, qint64, evernote::edam::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(placeName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(contentClass, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(lastEditedBy, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(creatorId, qint32, evernote::edam::UserID);
    CHECK_AND_SET_ATTRIBUTE(lastEditorId, qint32, evernote::edam::UserID);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = false;
    in >> isSetApplicationData;
    isSet.applicationData = isSetApplicationData;
    if (isSetApplicationData)
    {
        quint32 numKeys = 0;
        in >> numKeys;
        auto & keys = noteAttributes.applicationData.keysOnly;
        QString key;
        for(quint32 i = 0; i < numKeys; ++i) {
            in >> key;
            keys.insert(key.toStdString());
        }

        quint32 mapSize = 0;
        in >> mapSize;
        auto & map = noteAttributes.applicationData.fullMap;
        QString value;
        for(quint32 i = 0; i < mapSize; ++i) {
            in >> key;
            in >> value;
            map[key.toStdString()] = value.toStdString();
        }
    }

    bool isSetClassifications = false;
    in >> isSetClassifications;
    isSet.classifications = isSetClassifications;
    if (isSetClassifications)
    {
        quint32 mapSize;
        in >> mapSize;
        auto & map = noteAttributes.classifications;
        QString key;
        QString value;
        for(quint32 i = 0; i < mapSize; ++i) {
            in >> key;
            in >> value;
            map[key.toStdString()] = value.toStdString();
        }
    }

    return in;
}

QDataStream & operator<<(QDataStream & out, const qevercloud::UserAttributes & userAttributes)
{
#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = userAttributes.attribute.isSet(); \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(userAttributes.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(defaultLocationName);
    CHECK_AND_SET_ATTRIBUTE(defaultLatitude);
    CHECK_AND_SET_ATTRIBUTE(defaultLongitude);
    CHECK_AND_SET_ATTRIBUTE(preactivation);
    CHECK_AND_SET_ATTRIBUTE(incomingEmailAddress);
    CHECK_AND_SET_ATTRIBUTE(comments);
    CHECK_AND_SET_ATTRIBUTE(dateAgreedToTermsOfService, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(maxReferrals);
    CHECK_AND_SET_ATTRIBUTE(referralCount);
    CHECK_AND_SET_ATTRIBUTE(refererCode);
    CHECK_AND_SET_ATTRIBUTE(sentEmailDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(sentEmailCount);
    CHECK_AND_SET_ATTRIBUTE(dailyEmailLimit);
    CHECK_AND_SET_ATTRIBUTE(emailOptOutDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(partnerEmailOptInDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(preferredLanguage);
    CHECK_AND_SET_ATTRIBUTE(preferredCountry);
    CHECK_AND_SET_ATTRIBUTE(clipFullPage);
    CHECK_AND_SET_ATTRIBUTE(twitterUserName);
    CHECK_AND_SET_ATTRIBUTE(twitterId);
    CHECK_AND_SET_ATTRIBUTE(groupName);
    CHECK_AND_SET_ATTRIBUTE(recognitionLanguage);
    CHECK_AND_SET_ATTRIBUTE(referralProof);
    CHECK_AND_SET_ATTRIBUTE(educationalDiscount);
    CHECK_AND_SET_ATTRIBUTE(businessAddress);
    CHECK_AND_SET_ATTRIBUTE(hideSponsorBilling);
    CHECK_AND_SET_ATTRIBUTE(taxExempt);
    CHECK_AND_SET_ATTRIBUTE(useEmailAutoFiling);
    CHECK_AND_SET_ATTRIBUTE(reminderEmailConfig, static_cast<quint8>);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetViewedPromotions = userAttributes.viewedPromotions.isSet();
    out << isSetViewedPromotions;
    if (isSetViewedPromotions)
    {
        const QStringList & viewedPromotions = userAttributes.viewedPromotions;
        size_t numViewedPromotions = viewedPromotions.size();
        out << static_cast<quint32>(numViewedPromotions);
        foreach(const QString & viewedPromotion, viewedPromotions) {
            out << viewedPromotion;
        }
    }

    bool isSetRecentMailedAddresses = userAttributes.recentMailedAddresses.isSet();
    out << isSetRecentMailedAddresses;
    if (isSetRecentMailedAddresses)
    {
        const QStringList & recentMailedAddresses = userAttributes.recentMailedAddresses;
        size_t numRecentMailedAddresses = recentMailedAddresses.size();
        out << static_cast<quint32>(numRecentMailedAddresses);
        foreach(const QString & recentMailedAddress, recentMailedAddresses) {
            out << recentMailedAddress;
        }
    }

    return out;
}

QDataStream & operator>>(QDataStream & in, qevercloud::UserAttributes & userAttributes)
{
    userAttributes = qevercloud::UserAttributes();

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            userAttributes.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(defaultLocationName, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(defaultLatitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(defaultLongitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(preactivation, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(incomingEmailAddress, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(comments, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(dateAgreedToTermsOfService, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(maxReferrals, int32_t, int32_t);
    CHECK_AND_SET_ATTRIBUTE(referralCount, int32_t, int32_t);
    CHECK_AND_SET_ATTRIBUTE(refererCode, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(sentEmailDate, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(sentEmailCount, int32_t, int32_t);
    CHECK_AND_SET_ATTRIBUTE(dailyEmailLimit, int32_t, int32_t);
    CHECK_AND_SET_ATTRIBUTE(emailOptOutDate, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(partnerEmailOptInDate, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(preferredLanguage, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(preferredCountry, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(clipFullPage, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(twitterUserName, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(twitterId, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(groupName, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(recognitionLanguage, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(referralProof, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(educationalDiscount, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(businessAddress, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(hideSponsorBilling, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(taxExempt, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(useEmailAutoFiling, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(reminderEmailConfig, quint8, qevercloud::ReminderEmailConfig::type);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetViewedPromotions = false;
    in >> isSetViewedPromotions;
    if (isSetViewedPromotions)
    {
        quint32 numViewedPromotions = 0;
        in >> numViewedPromotions;
        userAttributes.viewedPromotions = QStringList();
        QStringList & viewedPromotions = userAttributes.viewedPromotions;
        viewedPromotions.reserve(numViewedPromotions);
        QString viewedPromotion;
        for(quint32 i = 0; i < numViewedPromotions; ++i) {
            in >> viewedPromotion;
            viewedPromotions << viewedPromotion;
        }
    }

    bool isSetRecentMailedAddresses = false;
    in >> isSetRecentMailedAddresses;
    if (isSetRecentMailedAddresses)
    {
        quint32 numRecentMailedAddresses = 0;
        in >> numRecentMailedAddresses;
        userAttributes.recentMailedAddresses = QStringList();
        QStringList & recentMailedAddresses = userAttributes.recentMailedAddresses;
        recentMailedAddresses.reserve(numRecentMailedAddresses);
        QString recentMailedAddress;
        for(quint32 i = 0; i < numRecentMailedAddresses; ++i) {
            in >> recentMailedAddress;
            recentMailedAddresses << recentMailedAddress;
        }
    }

    return in;
}

QDataStream & operator<<(QDataStream & out, const qevercloud::Accounting & accounting)
{
#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = accounting.attribute.isSet(); \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(accounting.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(uploadLimit, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(uploadLimitEnd, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(uploadLimitNextMonth, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceStatus, static_cast<quint8>);
    CHECK_AND_SET_ATTRIBUTE(premiumOrderNumber);
    CHECK_AND_SET_ATTRIBUTE(premiumCommerceService);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceStart, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceSKU);
    CHECK_AND_SET_ATTRIBUTE(lastSuccessfulCharge, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(lastFailedCharge, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(lastFailedChargeReason);
    CHECK_AND_SET_ATTRIBUTE(nextPaymentDue, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(premiumLockUntil, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(updated, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(premiumSubscriptionNumber);
    CHECK_AND_SET_ATTRIBUTE(lastRequestedCharge, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(currency);
    CHECK_AND_SET_ATTRIBUTE(unitPrice, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(businessId, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(businessName);
    CHECK_AND_SET_ATTRIBUTE(businessRole, static_cast<quint8>);
    CHECK_AND_SET_ATTRIBUTE(unitDiscount, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(nextChargeDate, static_cast<qint64>);

#undef CHECK_AND_SET_ATTRIBUTE

    return out;
}

QDataStream & operator>>(QDataStream & in, qevercloud::Accounting & accounting)
{
    accounting = qevercloud::Accounting();

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            accounting.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(uploadLimit, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(uploadLimitEnd, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(uploadLimitNextMonth, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceStatus, quint8, qevercloud::PremiumOrderStatus::type);
    CHECK_AND_SET_ATTRIBUTE(premiumOrderNumber, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(premiumCommerceService, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceStart, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceSKU, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(lastSuccessfulCharge, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(lastFailedCharge, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(lastFailedChargeReason, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(nextPaymentDue, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(premiumLockUntil, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(updated, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(premiumSubscriptionNumber, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(lastRequestedCharge, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(currency, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(unitPrice, qint32, int32_t);
    CHECK_AND_SET_ATTRIBUTE(businessId, qint32, int32_t);
    CHECK_AND_SET_ATTRIBUTE(businessName, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(businessRole, quint8, qevercloud::BusinessUserRole::type);
    CHECK_AND_SET_ATTRIBUTE(unitDiscount, qint32, int32_t);
    CHECK_AND_SET_ATTRIBUTE(nextChargeDate, qint64, qevercloud::Timestamp);

#undef CHECK_AND_SET_ATTRIBUTE

    return in;
}

QDataStream & operator<<(QDataStream & out, const evernote::edam::ResourceAttributes & resourceAttributes)
{
    const auto & isSet = resourceAttributes.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = isSet.attribute; \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(resourceAttributes.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(sourceURL, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(timestamp, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(latitude);
    CHECK_AND_SET_ATTRIBUTE(longitude);
    CHECK_AND_SET_ATTRIBUTE(altitude);
    CHECK_AND_SET_ATTRIBUTE(cameraMake, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(cameraModel, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(clientWillIndex);
    CHECK_AND_SET_ATTRIBUTE(recoType, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(fileName, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(attachment);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = isSet.applicationData;
    out << isSetApplicationData;
    if (isSetApplicationData)
    {
        const auto & applicationData = resourceAttributes.applicationData;

        const auto & keys = applicationData.keysOnly;
        size_t numKeys = keys.size();
        out << static_cast<quint32>(numKeys);
        for(const auto & key: keys) {
            out << QString::fromStdString(key);
        }

        const auto & map = applicationData.fullMap;
        size_t mapSize = map.size();
        out << static_cast<quint32>(mapSize);   // NOTE: the reasonable constraint is numKeys == mapSize but it's better to ensure
        for(const auto & items: map) {
            out << QString::fromStdString(items.first) << QString::fromStdString(items.second);
        }
    }

    return out;
}

QDataStream & operator>>(QDataStream & in, evernote::edam::ResourceAttributes & resourceAttributes)
{
    resourceAttributes = evernote::edam::ResourceAttributes();

    auto & isSet = resourceAttributes.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        isSet.attribute = isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            resourceAttributes.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(sourceURL, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(timestamp, qint64, evernote::edam::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(latitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(longitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(altitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(cameraMake, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(cameraModel, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(clientWillIndex, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(recoType, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(fileName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(attachment, bool, bool);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = false;
    in >> isSetApplicationData;
    isSet.applicationData = isSetApplicationData;
    if (isSetApplicationData)
    {
        quint32 numKeys = 0;
        in >> numKeys;
        auto & keys = resourceAttributes.applicationData.keysOnly;
        QString key;
        for(quint32 i = 0; i < numKeys; ++i) {
            in >> key;
            keys.insert(key.toStdString());
        }

        quint32 mapSize = 0;
        in >> mapSize;
        auto & map = resourceAttributes.applicationData.fullMap;
        QString value;
        for(quint32 i = 0; i < mapSize; ++i) {
            in >> key;
            in >> value;
            map[key.toStdString()] = value.toStdString();
        }
    }

    return in;
}

#define GET_SERIALIZED(type) \
    const QByteArray GetSerialized##type(const qevercloud::type & in) \
    { \
        QByteArray data; \
        QDataStream strm(&data, QIODevice::WriteOnly); \
        strm << in; \
        return std::move(data); \
    }

    GET_SERIALIZED(BusinessUserInfo)
    GET_SERIALIZED(PremiumInfo)
    GET_SERIALIZED(Accounting)
    GET_SERIALIZED(UserAttributes)
    GET_SERIALIZED(NoteAttributes)
    GET_SERIALIZED(ResourceAttributes)

#undef GET_SERIALIZED

#define GET_DESERIALIZED(type) \
    const qevercloud::type GetDeserialized##type(const QByteArray & data) \
    { \
        qevercloud::type out; \
        QDataStream strm(data); \
        strm >> out; \
        return std::move(out); \
    }

    GET_DESERIALIZED(BusinessUserInfo)
    GET_DESERIALIZED(PremiumInfo)
    GET_DESERIALIZED(Accounting)
    GET_DESERIALIZED(UserAttributes)
    GET_DESERIALIZED(NoteAttributes)
    GET_DESERIALIZED(ResourceAttributes)

#undef GET_DESERIALIZED

} // namespace qute_note
