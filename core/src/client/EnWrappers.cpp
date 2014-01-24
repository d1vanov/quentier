#include "EnWrappers.h"
#include <QObject>

namespace qute_note {

QDataStream & operator<<(QDataStream & out, const evernote::edam::NoteAttributes & noteAttributes)
{
    const auto & isSet = noteAttributes.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = isSet.attribute; \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(noteAttributes.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(subjectDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(latitude);
    CHECK_AND_SET_ATTRIBUTE(longitude);
    CHECK_AND_SET_ATTRIBUTE(altitude);
    CHECK_AND_SET_ATTRIBUTE(author, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(source, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(sourceURL, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(sourceApplication, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(shareDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(reminderOrder, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(reminderDoneTime, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(reminderTime, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(placeName, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(contentClass, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(lastEditedBy, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(creatorId, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(lastEditorId, static_cast<qint32>);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = isSet.applicationData;
    out << isSetApplicationData;
    if (isSetApplicationData)
    {
        const auto & keys = noteAttributes.applicationData.keysOnly;
        size_t numKeys = keys.size();
        out << static_cast<quint32>(numKeys);
        for(const auto & key: keys) {
            out << QString::fromStdString(key);
        }

        const auto & map = noteAttributes.applicationData.fullMap;
        size_t mapSize = map.size();
        out << static_cast<quint32>(mapSize);   // NOTE: the reasonable constraint is numKeys == mapSize but it's better to ensure
        for(const auto & items: map) {
            out << QString::fromStdString(items.first) << QString::fromStdString(items.second);
        }
    }

    bool isSetClassifications = isSet.classifications;
    out << isSetClassifications;
    if (isSetClassifications)
    {
        const auto & map = noteAttributes.classifications;
        size_t mapSize = map.size();
        out << static_cast<quint32>(mapSize);
        for(const auto & items: map) {
            out << QString::fromStdString(items.first) << QString::fromStdString(items.second);
        }
    }

    return out;
}

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

    CHECK_AND_SET_ATTRIBUTE(subjectDate, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(latitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(longitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(altitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(author, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(source, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(sourceURL, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(sourceApplication, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(shareDate, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(reminderOrder, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(reminderDoneTime, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(reminderTime, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(placeName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(contentClass, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(lastEditedBy, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(creatorId, qint32, UserID);
    CHECK_AND_SET_ATTRIBUTE(lastEditorId, qint32, UserID);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetApplicationData = false;
    in >> isSetApplicationData;
    isSet.applicationData = isSetApplicationData;
    if (isSetApplicationData)
    {
        quint64 numKeys = 0;
        in >> numKeys;
        auto & keys = noteAttributes.applicationData.keysOnly;
        QString key;
        for(quint64 i = 0; i < numKeys; ++i) {
            in >> key;
            keys.insert(key.toStdString());
        }

        quint64 mapSize = 0;
        in >> mapSize;
        auto & map = noteAttributes.applicationData.fullMap;
        QString value;
        for(quint64 i = 0; i < mapSize; ++i) {
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
        quint64 mapSize = 0;
        in >> mapSize;
        auto & map = noteAttributes.classifications;
        QString key;
        QString value;
        for(quint64 i = 0; i < mapSize; ++i) {
            in >> key;
            in >> value;
            map[key.toStdString()] = value.toStdString();
        }
    }

    return in;
}

bool Notebook::CheckParameters(QString & errorDescription) const
{
    if (!en_notebook.__isset.guid) {
        errorDescription = QObject::tr("Notebook's guid is not set");
        return false;
    }

    if (en_notebook.guid.empty()) {
        errorDescription = QObject::tr("Notebook's guid is empty");
        return false;
    }

    if (!en_notebook.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Notebook's update sequence number is not set");
        return false;
    }

    if ( (en_notebook.updateSequenceNum < 0) ||
         (en_notebook.updateSequenceNum == std::numeric_limits<int32_t>::max()) ||
         (en_notebook.updateSequenceNum == std::numeric_limits<int32_t>::min()) )
    {
        errorDescription = QObject::tr("Notebook's update sequence number is invalid");
        return false;
    }

    if (!en_notebook.__isset.name) {
        errorDescription = QObject::tr("Notebook's name is not set");
        return false;
    }

    if (en_notebook.name.empty()) {
        errorDescription = QObject::tr("Notebook's name is empty");
        return false;
    }

    if (!en_notebook.__isset.serviceCreated) {
        errorDescription = QObject::tr("Notebook's creation timestamp is not set");
        return false;
    }

    if (!en_notebook.__isset.serviceUpdated) {
        errorDescription = QObject::tr("Notebook's modification timestamp is not set");
        return false;
    }

    for(const auto & sharedNotebook: en_notebook.sharedNotebooks)
    {
        if (!sharedNotebook.__isset.id) {
            errorDescription = QObject::tr("Notebook has shared notebook without share id set");
            return false;
        }

        if (!sharedNotebook.__isset.notebookGuid) {
            errorDescription = QObject::tr("Notebook has shared notebook without real notebook's guid set");
            return false;
        }
    }

    return true;
}

bool Note::CheckParameters(QString & errorDescription) const
{
    if (!en_note.__isset.guid) {
        errorDescription = QObject::tr("Note's guid is not set");
        return false;
    }

    if (en_note.guid.empty()) {
        errorDescription = QObject::tr("Note's guid is empty");
        return false;
    }

    if (en_note.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Note's update sequence number is not set");
        return false;
    }

    if ( (en_note.updateSequenceNum < 0) ||
         (en_note.updateSequenceNum == std::numeric_limits<int32_t>::max()) ||
         (en_note.updateSequenceNum == std::numeric_limits<int32_t>::min()) )
    {
        errorDescription = QObject::tr("Note's update sequence number is invalid");
        return false;
    }

    if (!en_note.__isset.title) {
        errorDescription = QObject::tr("Note's title is not set");
        return false;
    }

    if (en_note.title.empty()) {
        errorDescription = QObject::tr("Note's title is empty");
        return false;
    }

    if (!en_note.__isset.content) {
        errorDescription = QObject::tr("Note's content is not set");
        return false;
    }

    if (en_note.content.empty()) {
        errorDescription = QObject::tr("Note's content is empty");
        return false;
    }

    if (!en_note.__isset.notebookGuid) {
        errorDescription = QObject::tr("Note's notebook Guid is not set");
        return false;
    }

    if (en_note.notebookGuid.empty()) {
        errorDescription = QObject::tr("Note's notebook Guid is empty");
        return false;
    }

    if (!en_note.__isset.created) {
        errorDescription = QObject::tr("Note's creation timestamp is not set");
        return false;
    }

    if (!en_note.__isset.updated) {
        errorDescription = QObject::tr("Note's modification timestamp is not set");
        return false;
    }

    return true;
}

QDataStream & operator<<(QDataStream & out, const evernote::edam::UserAttributes & userAttributes)
{
    const auto & isSet = userAttributes.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = isSet.attribute; \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(userAttributes.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(defaultLocationName, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(defaultLatitude);
    CHECK_AND_SET_ATTRIBUTE(defaultLongitude);
    CHECK_AND_SET_ATTRIBUTE(preactivation);
    CHECK_AND_SET_ATTRIBUTE(incomingEmailAddress, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(comments, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(dateAgreedToTermsOfService, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(maxReferrals);
    CHECK_AND_SET_ATTRIBUTE(referralCount);
    CHECK_AND_SET_ATTRIBUTE(refererCode, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(sentEmailDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(sentEmailCount);
    CHECK_AND_SET_ATTRIBUTE(dailyEmailLimit);
    CHECK_AND_SET_ATTRIBUTE(emailOptOutDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(partnerEmailOptInDate, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(preferredLanguage, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(preferredCountry, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(clipFullPage);
    CHECK_AND_SET_ATTRIBUTE(twitterUserName, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(twitterId, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(groupName, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(recognitionLanguage, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(referralProof, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(educationalDiscount);
    CHECK_AND_SET_ATTRIBUTE(businessAddress, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(hideSponsorBilling);
    CHECK_AND_SET_ATTRIBUTE(taxExempt);
    CHECK_AND_SET_ATTRIBUTE(useEmailAutoFiling);
    CHECK_AND_SET_ATTRIBUTE(reminderEmailConfig, static_cast<quint8>);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetViewedPromotions = isSet.viewedPromotions;
    out << isSetViewedPromotions;
    if (isSetViewedPromotions)
    {
        const auto & viewedPromotions = userAttributes.viewedPromotions;
        size_t numViewedPromotions = viewedPromotions.size();
        out << static_cast<quint32>(numViewedPromotions);
        for(const auto & viewedPromotion: viewedPromotions) {
            out << QString::fromStdString(viewedPromotion);
        }
    }

    bool isSetRecentMailedAddresses = isSet.recentMailedAddresses;
    out << isSetRecentMailedAddresses;
    if (isSetRecentMailedAddresses)
    {
        const auto & recentMailedAddresses = userAttributes.recentMailedAddresses;
        size_t numRecentMailedAddresses = recentMailedAddresses.size();
        out << static_cast<quint32>(numRecentMailedAddresses);
        for(const auto & recentMailedAddress: recentMailedAddresses) {
            out << QString::fromStdString(recentMailedAddress);
        }
    }

    return out;
}

QDataStream & operator>>(QDataStream & in, evernote::edam::UserAttributes & userAttributes)
{
    userAttributes = evernote::edam::UserAttributes();

    auto & isSet = userAttributes.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        isSet.attribute = isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            userAttributes.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(defaultLocationName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(defaultLatitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(defaultLongitude, double, double);
    CHECK_AND_SET_ATTRIBUTE(preactivation, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(incomingEmailAddress, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(comments, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(dateAgreedToTermsOfService, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(maxReferrals, int32_t, int32_t);
    CHECK_AND_SET_ATTRIBUTE(referralCount, int32_t, int32_t);
    CHECK_AND_SET_ATTRIBUTE(refererCode, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(sentEmailDate, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(sentEmailCount, int32_t, int32_t);
    CHECK_AND_SET_ATTRIBUTE(dailyEmailLimit, int32_t, int32_t);
    CHECK_AND_SET_ATTRIBUTE(emailOptOutDate, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(partnerEmailOptInDate, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(preferredLanguage, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(preferredCountry, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(clipFullPage, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(twitterUserName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(twitterId, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(groupName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(recognitionLanguage, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(referralProof, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(educationalDiscount, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(businessAddress, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(hideSponsorBilling, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(taxExempt, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(useEmailAutoFiling, bool, bool);
    CHECK_AND_SET_ATTRIBUTE(reminderEmailConfig, quint8, evernote::edam::ReminderEmailConfig::type);

#undef CHECK_AND_SET_ATTRIBUTE

    bool isSetViewedPromotions = false;
    in >> isSetViewedPromotions;
    isSet.viewedPromotions = isSetViewedPromotions;
    if (isSetViewedPromotions)
    {
        quint32 numViewedPromotions = 0;
        in >> numViewedPromotions;
        auto & viewedPromotions = userAttributes.viewedPromotions;
        viewedPromotions.reserve(numViewedPromotions);
        QString viewedPromotion;
        for(quint32 i = 0; i < numViewedPromotions; ++i) {
            in >> viewedPromotion;
            viewedPromotions.push_back(viewedPromotion.toStdString());
        }
    }

    bool isSetRecentMailedAddresses = false;
    in >> isSetRecentMailedAddresses;
    isSet.recentMailedAddresses = isSetRecentMailedAddresses;
    if (isSetRecentMailedAddresses)
    {
        quint32 numRecentMailedAddresses = 0;
        in >> numRecentMailedAddresses;
        auto & recentMailedAddresses = userAttributes.recentMailedAddresses;
        recentMailedAddresses.reserve(numRecentMailedAddresses);
        QString recentMailedAddress;
        for(quint32 i = 0; i < numRecentMailedAddresses; ++i) {
            in >> recentMailedAddress;
            recentMailedAddresses.push_back(recentMailedAddress.toStdString());
        }
    }

    return in;
}

QDataStream & operator<<(QDataStream & out, const evernote::edam::Accounting & accounting)
{
    const auto & isSet = accounting.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = isSet.attribute; \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(accounting.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(uploadLimit, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(uploadLimitEnd, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(uploadLimitNextMonth, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceStatus, static_cast<quint8>);
    CHECK_AND_SET_ATTRIBUTE(premiumOrderNumber, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(premiumCommerceService, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceStart, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceSKU, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(lastSuccessfulCharge, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(lastFailedCharge, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(lastFailedChargeReason, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(nextPaymentDue, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(premiumLockUntil, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(updated, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(premiumSubscriptionNumber, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(lastRequestedCharge, static_cast<qint64>);
    CHECK_AND_SET_ATTRIBUTE(currency, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(unitPrice, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(businessId, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(businessName, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(businessRole, static_cast<quint8>);
    CHECK_AND_SET_ATTRIBUTE(unitDiscount, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(nextChargeDate, static_cast<qint64>);

#undef CHECK_AND_SET_ATTRIBUTE

    return out;
}

QDataStream & operator>>(QDataStream & in, evernote::edam::Accounting & accounting)
{
    accounting = evernote::edam::Accounting();

    auto & isSet = accounting.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        isSet.attribute = isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            accounting.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(uploadLimit, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(uploadLimitEnd, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(uploadLimitNextMonth, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceStatus, quint8, evernote::edam::PremiumOrderStatus::type);
    CHECK_AND_SET_ATTRIBUTE(premiumOrderNumber, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(premiumCommerceService, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(premiumServiceStart, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(premiumServiceSKU, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(lastSuccessfulCharge, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(lastFailedCharge, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(lastFailedChargeReason, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(nextPaymentDue, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(premiumLockUntil, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(updated, qint64, Timestamp);
    CHECK_AND_SET_ATTRIBUTE(premiumSubscriptionNumber, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(lastRequestedCharge, qint64, int64_t);
    CHECK_AND_SET_ATTRIBUTE(currency, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(unitPrice, qint32, int32_t);
    CHECK_AND_SET_ATTRIBUTE(businessId, qint32, int32_t);
    CHECK_AND_SET_ATTRIBUTE(businessName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(businessRole, quint8, evernote::edam::BusinessUserRole::type);
    CHECK_AND_SET_ATTRIBUTE(unitDiscount, qint32, int32_t);
    CHECK_AND_SET_ATTRIBUTE(nextChargeDate, qint64, Timestamp);

#undef CHECK_AND_SET_ATTRIBUTE

    return in;
}

#define GET_SERIALIZED(type) \
    const QByteArray GetSerialized##type(const evernote::edam::type & in) \
    { \
        QByteArray data; \
        QDataStream strm(data); \
        strm << in; \
        return std::move(data); \
    }

    GET_SERIALIZED(Accounting)
    GET_SERIALIZED(UserAttributes)
    GET_SERIALIZED(NoteAttributes)

#undef GET_SERIALIZED

#define GET_DESERIALIZED(type) \
    const evernote::edam::type GetDeserialized##type(const QByteArray & data) \
    { \
        evernote::edam::type out; \
        QDataStream strm(data); \
        strm >> out; \
        return std::move(out); \
    }

    GET_DESERIALIZED(Accounting)
    GET_DESERIALIZED(UserAttributes)
    GET_DESERIALIZED(NoteAttributes)

#undef GET_DESERIALIZED
}
