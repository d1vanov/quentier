#include "Serialization.h"
#include <QEverCloud.h>
#include "types/QEverCloudHelpers.h"
#include <QDataStream>
#include <QByteArray>

namespace qute_note {

// TODO: consider setting some fixed version to each QDataStream

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

    GET_DESERIALIZED(ResourceAttributes)

#undef GET_DESERIALIZED

} // namespace qute_note
