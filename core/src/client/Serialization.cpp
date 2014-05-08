#include "Serialization.h"
#include <QEverCloud.h>
#include "types/QEverCloudHelpers.h"
#include <QDataStream>
#include <QByteArray>

namespace qute_note {

// TODO: consider setting some fixed version to each QDataStream

QDataStream & operator<<(QDataStream & out, const qevercloud::NoteAttributes & noteAttributes)
{
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
            const QSet<QString> & keys = noteAttributes.applicationData->keysOnly;
            size_t numKeys = keys.size();
            out << static_cast<quint32>(numKeys);
            foreach(const QString & key, keys) {
                out << key;
            }
        }

        bool isSetFullMap = noteAttributes.applicationData->fullMap.isSet();
        out << isSetFullMap;
        if (isSetFullMap) {
            const QMap<QString, QString> & map = noteAttributes.applicationData->fullMap;
            size_t mapSize = map.size();
            out << static_cast<quint32>(mapSize);   // NOTE: the reasonable constraint is numKeys == mapSize but it's better to ensure
            foreach(const QString & key, map.keys()) {
                out << key << map.value(key);
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
        foreach(const QString & key, fullMap.keys()) {
            out << key << fullMap.value(key);
        }
    }

    return out;
}

QDataStream & operator>>(QDataStream & in, qevercloud::NoteAttributes & noteAttributes)
{
    noteAttributes = qevercloud::NoteAttributes();

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            noteAttributes.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(subjectDate, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(latitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(longitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(altitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(author, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(source, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(sourceURL, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(sourceApplication, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(shareDate, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(reminderOrder, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(reminderDoneTime, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(reminderTime, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(placeName, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(contentClass, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(lastEditedBy, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(creatorId, qint32, qevercloud::UserID);
    CHECK_AND_SET_ATTRIBUTE(lastEditorId, qint32, qevercloud::UserID);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = false;
    in >> isSetApplicationData;
    if (isSetApplicationData)
    {
        noteAttributes.applicationData = qevercloud::LazyMap();

        bool isSetKeysOnly = false;
        in >> isSetKeysOnly;
        if (isSetKeysOnly) {
            noteAttributes.applicationData->keysOnly = QSet<QString>();
            quint32 numKeys = 0;
            in >> numKeys;
            QSet<QString> & keys = noteAttributes.applicationData->keysOnly;
            QString key;
            for(quint32 i = 0; i < numKeys; ++i) {
                in >> key;
                keys.insert(key);
            }
        }

        bool isSetFullMap = false;
        in >> isSetFullMap;
        if (isSetFullMap) {
            noteAttributes.applicationData->fullMap = QMap<QString, QString>();
            quint32 mapSize = 0;
            in >> mapSize;
            QMap<QString, QString> & map = noteAttributes.applicationData->fullMap;
            QString key, value;
            for(quint32 i = 0; i < mapSize; ++i) {
                in >> key;
                in >> value;
                map[key] = value;
            }
        }
    }

    bool isSetClassifications = false;
    in >> isSetClassifications;
    if (isSetClassifications)
    {
        noteAttributes.classifications = QMap<QString, QString>();
        quint32 mapSize;
        in >> mapSize;
        QMap<QString, QString> & map = noteAttributes.classifications;
        QString key, value;
        for(quint32 i = 0; i < mapSize; ++i) {
            in >> key;
            in >> value;
            map[key] = value;
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

QDataStream & operator<<(QDataStream & out, const qevercloud::ResourceAttributes & resourceAttributes)
{
#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
    bool isSet##attribute = resourceAttributes.attribute.isSet(); \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(resourceAttributes.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(sourceURL);
    CHECK_AND_SET_ATTRIBUTE(timestamp, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(latitude);
    CHECK_AND_SET_ATTRIBUTE(longitude);
    CHECK_AND_SET_ATTRIBUTE(altitude);
    CHECK_AND_SET_ATTRIBUTE(cameraMake);
    CHECK_AND_SET_ATTRIBUTE(cameraModel);
    CHECK_AND_SET_ATTRIBUTE(clientWillIndex);
    CHECK_AND_SET_ATTRIBUTE(recoType);
    CHECK_AND_SET_ATTRIBUTE(fileName);
    CHECK_AND_SET_ATTRIBUTE(attachment);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = resourceAttributes.applicationData.isSet();
    out << isSetApplicationData;
    if (isSetApplicationData)
    {
        const qevercloud::LazyMap & applicationData = resourceAttributes.applicationData;

        bool isSetKeysOnly = applicationData.keysOnly.isSet();
        out << isSetKeysOnly;
        if (isSetKeysOnly) {
            const QSet<QString> & keys = applicationData.keysOnly;
            size_t numKeys = keys.size();
            out << static_cast<quint32>(numKeys);
            foreach(const QString & key, keys) {
                out << key;
            }
        }

        bool isSetFullMap = applicationData.fullMap.isSet();
        out << isSetFullMap;
        if (isSetFullMap) {
            const QMap<QString, QString> & map = applicationData.fullMap;
            size_t numKeys = map.size();
            out << static_cast<quint32>(numKeys);
            foreach(const QString & key, map.keys()) {
                out << key << map.value(key);
            }
        }
    }

    return out;
}

QDataStream & operator>>(QDataStream & in, qevercloud::ResourceAttributes & resourceAttributes)
{
    resourceAttributes = qevercloud::ResourceAttributes();

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            resourceAttributes.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(sourceURL, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(timestamp, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_ATTRIBUTE(latitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(longitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(altitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(cameraMake, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(cameraModel, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(clientWillIndex, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(recoType, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(fileName, QString, QString);
    CHECK_AND_SET_ATTRIBUTE(attachment, bool, bool);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = false;
    in >> isSetApplicationData;
    if (isSetApplicationData)
    {
        resourceAttributes.applicationData = qevercloud::LazyMap();

        bool isSetKeysOnly = false;
        in >> isSetKeysOnly;
        if (isSetKeysOnly) {
            resourceAttributes.applicationData->keysOnly = QSet<QString>();
            quint32 numKeys = 0;
            in >> numKeys;
            QSet<QString> & keys = resourceAttributes.applicationData->keysOnly;
            QString key;
            for(quint32 i = 0; i < numKeys; ++i) {
                in >> key;
                keys.insert(key);
            }
        }

        bool isSetFullMap = false;
        in >> isSetFullMap;
        if (isSetFullMap) {
            resourceAttributes.applicationData->fullMap = QMap<QString, QString>();
            quint32 mapSize = 0;
            in >> mapSize;
            QMap<QString, QString> & map = resourceAttributes.applicationData->fullMap;
            QString key, value;
            for(quint32 i = 0; i < mapSize; ++i) {
                in >> key;
                in >> value;
                map[key] = value;
            }
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

    GET_DESERIALIZED(UserAttributes)
    GET_DESERIALIZED(NoteAttributes)
    GET_DESERIALIZED(ResourceAttributes)

#undef GET_DESERIALIZED

} // namespace qute_note
