#include "EnWrappers.h"
#include <QObject>
#include <Limits_constants.h>

namespace qute_note {

QDataStream & operator<<(QDataStream & out, const evernote::edam::BusinessUserInfo & info)
{
    const auto & isSet = info.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = isSet.attribute; \
        out << isSet##attribute; \
        if (isSet##attribute) { \
            out << __VA_ARGS__(info.attribute); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(businessId, static_cast<qint32>);
    CHECK_AND_SET_ATTRIBUTE(businessName, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(role, static_cast<quint8>);
    CHECK_AND_SET_ATTRIBUTE(email, QString::fromStdString);

#undef CHECK_AND_SET_ATTRIBUTE

    return out;
}

QDataStream & operator>>(QDataStream & in, evernote::edam::BusinessUserInfo & info)
{
    auto & isSet = info.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        isSet.attribute = isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            info.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(businessId, qint32, int32_t);
    CHECK_AND_SET_ATTRIBUTE(businessName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(role, quint8, evernote::edam::BusinessUserRole::type);
    CHECK_AND_SET_ATTRIBUTE(email, QString, std::string, .toStdString());

#undef CHECK_AND_SET_ATTRIBUTE

    return in;
}

QDataStream & operator<<(QDataStream & out, const evernote::edam::PremiumInfo & info)
{
    out << static_cast<qint64>(info.currentTime);
    out << info.premium;
    out << info.premiumRecurring;

    const auto & isSet = info.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, ...) \
    { \
        bool isSet##attribute = isSet.attribute; \
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

    CHECK_AND_SET_ATTRIBUTE(sponsoredGroupName, QString::fromStdString);
    CHECK_AND_SET_ATTRIBUTE(sponsoredGroupRole, static_cast<quint8>);
    CHECK_AND_SET_ATTRIBUTE(premiumUpgradable);

#undef CHECK_AND_SET_ATTRIBUTE

    return out;
}

QDataStream & operator>>(QDataStream & in, evernote::edam::PremiumInfo & info)
{
    info = evernote::edam::PremiumInfo();

    qint64 currentTime;
    in >> currentTime;
    info.currentTime = static_cast<Timestamp>(currentTime);

    in >> info.premium;
    in >> info.premiumRecurring;

    auto & isSet = info.__isset;

#define CHECK_AND_SET_ATTRIBUTE(attribute, qtype, true_type, ...) \
    { \
        bool isSet##attribute = false; \
        in >> isSet##attribute; \
        isSet.attribute = isSet##attribute; \
        if (isSet##attribute) { \
            qtype attribute; \
            in >> attribute; \
            info.attribute = static_cast<true_type>(attribute __VA_ARGS__); \
        } \
    }

    CHECK_AND_SET_ATTRIBUTE(premiumExpirationDate, qint64, Timestamp);

    in >> info.premiumExtendable;
    in >> info.premiumPending;
    in >> info.premiumCancellationPending;
    in >> info.canPurchaseUploadAllowance;

    CHECK_AND_SET_ATTRIBUTE(sponsoredGroupName, QString, std::string, .toStdString());
    CHECK_AND_SET_ATTRIBUTE(sponsoredGroupRole, quint8, evernote::edam::SponsoredGroupRole::type);
    CHECK_AND_SET_ATTRIBUTE(premiumUpgradable, bool, bool);

#undef CHECK_AND_SET_ATTRIBUTE

    return in;
}

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

bool Notebook::CheckParameters(QString & errorDescription) const
{
    if (!en_notebook.__isset.guid) {
        errorDescription = QObject::tr("Notebook's guid is not set");
        return false;
    }
    else if (!en_wrappers_private::CheckGuid(en_notebook.guid)) {
        errorDescription = QObject::tr("Notebook's guid is invalid");
        return false;
    }

    if (!en_notebook.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Notebook's update sequence number is not set");
        return false;
    }
    else if (!en_wrappers_private::CheckUpdateSequenceNumber(en_notebook.updateSequenceNum)) {
        errorDescription = QObject::tr("Notebook's update sequence number is invalid");
        return false;
    }

    if (!en_notebook.__isset.name) {
        errorDescription = QObject::tr("Notebook's name is not set");
        return false;
    }
    else {
        size_t nameSize = en_notebook.name.size();

        if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_NOTEBOOK_NAME_LEN_MIN) ||
             (nameSize > evernote::limits::g_Limits_constants.EDAM_NOTEBOOK_NAME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Notebook's name has invalid size");
            return false;
        }
    }

    if (!en_notebook.__isset.serviceCreated) {
        errorDescription = QObject::tr("Notebook's creation timestamp is not set");
        return false;
    }

    if (!en_notebook.__isset.serviceUpdated) {
        errorDescription = QObject::tr("Notebook's modification timestamp is not set");
        return false;
    }

    if (en_notebook.__isset.sharedNotebooks)
    {
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
            else if (!en_wrappers_private::CheckGuid(sharedNotebook.notebookGuid)) {
                errorDescription = QObject::tr("Notebook has shared notebook with invalid guid");
                return false;
            }
        }
    }

    if (en_notebook.__isset.businessNotebook)
    {
        if (!en_notebook.businessNotebook.__isset.notebookDescription) {
            errorDescription = QObject::tr("Description for business notebook is not set");
            return false;
        }
        else {
            size_t businessNotebookDescriptionSize = en_notebook.businessNotebook.notebookDescription.size();

            if ( (businessNotebookDescriptionSize < evernote::limits::g_Limits_constants.EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MIN) ||
                 (businessNotebookDescriptionSize > evernote::limits::g_Limits_constants.EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MAX) )
            {
                errorDescription = QObject::tr("Description for business notebook has invalid size");
                return false;
            }
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
    else if (!en_wrappers_private::CheckGuid(en_note.guid)) {
        errorDescription = QObject::tr("Note's guid is invalid");
        return false;
    }

    if (en_note.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Note's update sequence number is not set");
        return false;
    }
    else if (!en_wrappers_private::CheckUpdateSequenceNumber(en_note.updateSequenceNum)) {
        errorDescription = QObject::tr("Note's update sequence number is invalid");
        return false;
    }

    if (!en_note.__isset.title) {
        errorDescription = QObject::tr("Note's title is not set");
        return false;
    }
    else {
        size_t titleSize = en_note.title.size();

        if ( (titleSize < evernote::limits::g_Limits_constants.EDAM_NOTE_TITLE_LEN_MIN) ||
             (titleSize > evernote::limits::g_Limits_constants.EDAM_NOTE_TITLE_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note's title length is invalid");
            return false;
        }
    }

    if (!en_note.__isset.content) {
        errorDescription = QObject::tr("Note's content is not set");
        return false;
    }
    else {
        size_t contentSize = en_note.content.size();

        if ( (contentSize < evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_LEN_MIN) ||
             (contentSize > evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note's content length is invalid");
            return false;
        }
    }

    if (en_note.__isset.contentHash) {
        size_t contentHashSize = en_note.contentHash.size();

        if (contentHashSize != evernote::limits::g_Limits_constants.EDAM_HASH_LEN) {
            errorDescription = QObject::tr("Note's content hash size is invalid");
            return false;
        }
    }

    if (!en_note.__isset.notebookGuid) {
        errorDescription = QObject::tr("Note's notebook Guid is not set");
        return false;
    }
    else if (!en_wrappers_private::CheckGuid(en_note.notebookGuid)) {
        errorDescription = QObject::tr("Note's notebook guid is invalid");
        return false;
    }

    if (en_note.__isset.tagGuids) {
        size_t numTagGuids = en_note.tagGuids.size();

        if (numTagGuids > evernote::limits::g_Limits_constants.EDAM_NOTE_TAGS_MAX) {
            errorDescription = QObject::tr("Note has too many tags, max allowed ");
            errorDescription.append(QString::number(evernote::limits::g_Limits_constants.EDAM_NOTE_TAGS_MAX));
            return false;
        }
    }

    if (en_note.__isset.resources) {
        size_t numResources = en_note.resources.size();

        if (numResources > evernote::limits::g_Limits_constants.EDAM_NOTE_RESOURCES_MAX) {
            errorDescription = QObject::tr("Note has too many resources, max allowed ");
            errorDescription.append(QString::number(evernote::limits::g_Limits_constants.EDAM_NOTE_RESOURCES_MAX));
            return false;
        }
    }

    if (!en_note.__isset.created) {
        errorDescription = QObject::tr("Note's creation timestamp is not set");
        return false;
    }

    if (!en_note.__isset.updated) {
        errorDescription = QObject::tr("Note's modification timestamp is not set");
        return false;
    }

    if (!en_note.__isset.attributes) {
        return true;
    }

    const auto & attributes = en_note.attributes;

#define CHECK_NOTE_ATTRIBUTE(name) \
    if (attributes.__isset.name) { \
        size_t name##Size = attributes.name.size(); \
        \
        if ( (name##Size < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) || \
             (name##Size > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) ) \
        { \
            errorDescription = QObject::tr("Note attributes' " #name " field has invalid size"); \
            return false; \
        } \
    }

    CHECK_NOTE_ATTRIBUTE(author);
    CHECK_NOTE_ATTRIBUTE(source);
    CHECK_NOTE_ATTRIBUTE(sourceURL);
    CHECK_NOTE_ATTRIBUTE(sourceApplication);

#undef CHECK_NOTE_ATTRIBUTE

    if (attributes.__isset.contentClass) {
        size_t contentClassSize = attributes.contentClass.size();

        if ( (contentClassSize < evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_CLASS_LEN_MIN) ||
             (contentClassSize > evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_CLASS_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note attributes' content class has invalid size");
            return false;
        }
    }

    if (attributes.__isset.applicationData) {
        const auto & applicationData = attributes.applicationData;
        const auto & keysOnly = applicationData.keysOnly;
        const auto & fullMap = applicationData.fullMap;

        for(const auto & key: keysOnly) {
            size_t keySize = key.size();
            if ( (keySize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid key (in keysOnly part)");
                return false;
            }
        }

        for(const auto & pair: fullMap) {
            size_t keySize = pair.first.size();
            if ( (keySize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid key (in fullMap part)");
                return false;
            }

            size_t valueSize = pair.second.size();
            if ( (valueSize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_VALUE_LEN_MIN) ||
                 (valueSize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_VALUE_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid value");
                return false;
            }

            size_t sumSize = keySize + valueSize;
            if (sumSize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_ENTRY_LEN_MAX) {
                errorDescription = QObject::tr("Note's attributes application data has invalid entry size");
                return false;
            }
        }
    }

    if (attributes.__isset.classifications) {
        const auto & classifications = attributes.classifications;
        for(const auto & pair: classifications) {
            const auto & value = pair.second;
            size_t pos = value.find("CLASSIFICATION_");
            if (pos == std::string::npos || pos != 0) {
                errorDescription = QObject::tr("Note's attributes classifications has invalid classification value");
                return false;
            }
        }
    }

    return true;
}

bool Tag::CheckParameters(QString & errorDescription) const
{
    if (!en_tag.__isset.guid) {
        errorDescription = QObject::tr("Tag's guid is not set");
        return false;
    }
    else if (!en_wrappers_private::CheckGuid(en_tag.guid)) {
        errorDescription = QObject::tr("Tag's guid is invalid: ");
        errorDescription.append(QString::fromStdString(en_tag.guid));
        return false;
    }

    if (!en_tag.__isset.name) {
        errorDescription = QObject::tr("Tag's name is not set");
        return false;
    }
    else {
        size_t nameSize = en_tag.name.size();

        if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MIN) ||
             (nameSize > evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Tag's name exceeds allowed size: ");
            errorDescription.append(QString::fromStdString(en_tag.name));
            return false;
        }

        if (en_tag.name.at(0) == ' ') {
            errorDescription = QObject::tr("Tag's name can't begin from space: ");
            errorDescription.append(QString::fromStdString(en_tag.name));
            return false;
        }
        else if (en_tag.name.at(en_tag.name.length() - 1) == ' ') {
            errorDescription = QObject::tr("Tag's name can't end with space: ");
            errorDescription.append(QString::fromStdString(en_tag.name));
            return false;
        }

        if (en_tag.name.find(',') != en_tag.name.npos) {
            errorDescription = QObject::tr("Tag's name can't contain comma: ");
            errorDescription.append(QString::fromStdString(en_tag.name));
            return false;
        }
    }

    if (!en_tag.updateSequenceNum) {
        errorDescription = QObject::tr("Tag's update sequence number is not set");
        return false;
    }
    else if (!en_wrappers_private::CheckUpdateSequenceNumber(en_tag.updateSequenceNum)) {
        errorDescription = QObject::tr("Tag's update sequence number is invalid: ");
        errorDescription.append(en_tag.updateSequenceNum);
        return false;
    }

    if (en_tag.__isset.parentGuid && !en_wrappers_private::CheckGuid(en_tag.parentGuid)) {
        errorDescription = QObject::tr("Tag's parent guid is invalid: ");
        errorDescription.append(QString::fromStdString(en_tag.parentGuid));
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
        QDataStream strm(&data, QIODevice::WriteOnly); \
        strm << in; \
        return std::move(data); \
    }

    GET_SERIALIZED(BusinessUserInfo)
    GET_SERIALIZED(PremiumInfo)
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

    GET_DESERIALIZED(BusinessUserInfo)
    GET_DESERIALIZED(PremiumInfo)
    GET_DESERIALIZED(Accounting)
    GET_DESERIALIZED(UserAttributes)
    GET_DESERIALIZED(NoteAttributes)

#undef GET_DESERIALIZED

namespace en_wrappers_private {

bool CheckGuid(const Guid & guid)
{
    size_t guidSize = guid.size();

    if (guidSize < evernote::limits::g_Limits_constants.EDAM_GUID_LEN_MIN) {
        return false;
    }

    if (guidSize > evernote::limits::g_Limits_constants.EDAM_GUID_LEN_MAX) {
        return false;
    }

    return true;
}

bool CheckUpdateSequenceNumber(const int32_t updateSequenceNumber)
{
    return ( (updateSequenceNumber < 0) ||
             (updateSequenceNumber == std::numeric_limits<int32_t>::min()) ||
             (updateSequenceNumber == std::numeric_limits<int32_t>::max()) );
}

}

}
