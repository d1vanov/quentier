#include "SerializationTests.h"
#include <client/Serialization.h>
#include <client/types/QEverCloudHelpers.h>
#include <tools/Printable.h>
#include <bitset>

namespace qute_note {
namespace test {

bool TestNoteAttributesSerialization(QString & errorDescription)
{
    qevercloud::NoteAttributes attributes;

    // number of optional data components in NoteAttributes
#define NOTE_ATTRIBUTES_NUM_COMPONENTS 19
    for(int mask = 0; mask != (1 << NOTE_ATTRIBUTES_NUM_COMPONENTS); ++mask)
    {
        attributes = qevercloud::NoteAttributes();

        std::bitset<NOTE_ATTRIBUTES_NUM_COMPONENTS> bits(mask);

        bool isSetSubjectDate = bits[0];
        bool isSetLatitude = bits[1];
        bool isSetLongitude = bits[2];
        bool isSetAltitude = bits[3];
        bool isSetAuthor = bits[4];
        bool isSetSource = bits[5];
        bool isSetSourceURL = bits[6];
        bool isSetSourceApplication = bits[7];
        bool isSetShareDate = bits[8];
        bool isSetReminderOrder = bits[9];
        bool isSetReminderDoneTime = bits[10];
        bool isSetReminderTime = bits[11];
        bool isSetPlaceName = bits[12];
        bool isSetContentClass = bits[13];
        bool isSetApplicationData = bits[14];
        bool isSetLastEditedBy = bits[15];
        bool isSetClassifications = bits[16];
        bool isSetCreatorId = bits[17];
        bool isSetLastEditorId = bits[18];

        if (isSetSubjectDate) {
            attributes.subjectDate = static_cast<qevercloud::Timestamp>(512);
        }

        if (isSetLatitude) {
            attributes.latitude = 42.0;
        }

        if (isSetLongitude) {
            attributes.longitude = 43.0;
        }

        if (isSetAltitude) {
            attributes.altitude = 42.0;
        }

        if (isSetAuthor) {
            attributes.author = "haxpeha";
        }

        if (isSetSource) {
            attributes.source = "brain";
        }

        if (isSetSourceURL) {
            attributes.sourceURL = "https://github.com/d1vanov";
        }

        if (isSetSourceApplication) {
            attributes.sourceApplication = "Qt Creator";
        }

        if (isSetShareDate) {
            attributes.shareDate = static_cast<qevercloud::Timestamp>(10);
        }

        if (isSetReminderOrder) {
            attributes.reminderOrder = 2;
        }

        if (isSetReminderDoneTime) {
            attributes.reminderDoneTime = static_cast<qevercloud::Timestamp>(20);
        }

        if (isSetReminderTime) {
            attributes.reminderTime = static_cast<qevercloud::Timestamp>(40);
        }

        if (isSetPlaceName) {
            attributes.placeName = "My place";
        }

        if (isSetContentClass) {
            attributes.contentClass = "text";
        }

        if (isSetApplicationData)
        {
            attributes.applicationData = qevercloud::LazyMap();
            qevercloud::LazyMap & applicationData = attributes.applicationData;
            applicationData.keysOnly = QSet<QString>();
            QSet<QString> & applicationDataKeys = applicationData.keysOnly;
            applicationData.fullMap = QMap<QString, QString>();
            QMap<QString, QString> & applicationDataMap  = applicationData.fullMap;

            applicationDataKeys.insert("key1");
            applicationDataKeys.insert("key2");
            applicationDataKeys.insert("key3");

            applicationDataMap["key1"] = "value1";
            applicationDataMap["key2"] = "value2";
            applicationDataMap["key3"] = "value3";
        }

        if (isSetLastEditedBy) {
            attributes.lastEditedBy = "Me";
        }

        if (isSetClassifications)
        {
            attributes.classifications = QMap<QString, QString>();
            QMap<QString, QString> & classifications = attributes.classifications;

            classifications["classificationKey1"] = "classificationValue1";
            classifications["classificationKey2"] = "classificationValue2";
            classifications["classificationKey3"] = "classificationValue3";
        }

        if (isSetCreatorId) {
            attributes.creatorId = static_cast<qevercloud::UserID>(10);
        }

        if (isSetLastEditorId) {
            attributes.lastEditorId = static_cast<qevercloud::UserID>(10);
        }

        QByteArray serializedAttributes = GetSerializedNoteAttributes(attributes);
        qevercloud::NoteAttributes deserializedAttributes = GetDeserializedNoteAttributes(serializedAttributes);

        if (attributes != deserializedAttributes)
        {
            errorDescription = "Serialization test for NoteAttributes FAILED! ";
            errorDescription.append("Initial NoteAttributes: \n");
            errorDescription.append(ToQString<qevercloud::NoteAttributes>(attributes));
            errorDescription.append("Deserialized NoteAttributes: \n");
            errorDescription.append(ToQString<qevercloud::NoteAttributes>(deserializedAttributes));

            return false;
        }
    }

#undef NOTE_ATTRIBUTES_NUM_COMPONENTS

    return true;
}

bool TestResourceAttributesSerialization(QString & errorDescription)
{
    qevercloud::ResourceAttributes attributes;

    // number of optional data components in ResourceAttributes
#define RESOURCE_ATTRIBUTES_NUM_COMPONENTS 14
    for(int mask = 0; mask != (1 << RESOURCE_ATTRIBUTES_NUM_COMPONENTS); ++mask)
    {
        attributes = qevercloud::ResourceAttributes();

        std::bitset<RESOURCE_ATTRIBUTES_NUM_COMPONENTS> bits(mask);

        bool isSetSourceURL = bits[0];
        bool isSetTimestamp = bits[1];
        bool isSetLatitude = bits[2];
        bool isSetLongitude = bits[3];
        bool isSetAltitude = bits[4];
        bool isSetCameraMake = bits[5];
        bool isSetCameraModel = bits[6];
        bool isSetClientWillIndex = bits[7];
        bool isSetRecoType = bits[8];
        bool isSetFileName = bits[9];
        bool isSetAttachment = bits[10];
        bool isSetApplicationData = bits[11];

        if (isSetSourceURL) {
            attributes.sourceURL = "https://github.com/d1vanov";
        }

        if (isSetTimestamp) {
            attributes.timestamp = static_cast<qevercloud::Timestamp>(10);
        }

        if (isSetLatitude) {
            attributes.latitude = 20.0;
        }

        if (isSetLongitude) {
            attributes.longitude = 30.0;
        }

        if (isSetAltitude) {
            attributes.altitude = 40.0;
        }

        if (isSetCameraMake) {
            attributes.cameraMake = "Something...";
        }

        if (isSetCameraModel) {
            attributes.cameraModel = "Canon or Nikon?";
        }

        if (isSetClientWillIndex) {
            attributes.clientWillIndex = bits[12];
        }

        if (isSetRecoType) {
            attributes.recoType = "text";
        }

        if (isSetFileName) {
            attributes.fileName = "some file";
        }

        if (isSetAttachment) {
            attributes.attachment = bits[13];
        }

        if (isSetApplicationData)
        {
            attributes.applicationData = qevercloud::LazyMap();
            qevercloud::LazyMap & applicationData = attributes.applicationData;
            applicationData.keysOnly = QSet<QString>();
            QSet<QString> & applicationDataKeys = applicationData.keysOnly;
            applicationData.fullMap = QMap<QString, QString>();
            QMap<QString, QString> & applicationDataMap = applicationData.fullMap;

            applicationDataKeys.insert("key1");
            applicationDataKeys.insert("key2");
            applicationDataKeys.insert("key3");

            applicationDataMap["key1"] = "value1";
            applicationDataMap["key2"] = "value2";
            applicationDataMap["key3"] = "value3";
        }

        QByteArray serializedAttributes = GetSerializedResourceAttributes(attributes);
        qevercloud::ResourceAttributes deserializedAttributes = GetDeserializedResourceAttributes(serializedAttributes);

        if (attributes != deserializedAttributes)
        {
            errorDescription = "Serialization test for ResourceAttributes FAILED! ";
            errorDescription.append("Initial ResourceAttributes: \n");
            errorDescription.append(ToQString<qevercloud::ResourceAttributes>(attributes));
            errorDescription.append("Deserialized ResourceAttributes: \n");
            errorDescription.append(ToQString<qevercloud::ResourceAttributes>(deserializedAttributes));

            return false;
        }
    }

#undef RESOURCE_ATTRIBUTES_NUM_COMPONENTS

    return true;
}

}
}
