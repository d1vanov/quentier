#include "SerializationTests.h"
#include <client/Serialization.h>
#include <client/types/QEverCloudHelpers.h>
#include <tools/Printable.h>
#include <bitset>

namespace qute_note {
namespace test {

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
