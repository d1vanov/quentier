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

    GET_DESERIALIZED(NoteAttributes)
    GET_DESERIALIZED(ResourceAttributes)

#undef GET_DESERIALIZED

} // namespace qute_note
