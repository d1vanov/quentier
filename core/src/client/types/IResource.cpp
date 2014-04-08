#include "IResource.h"
#include "../Serialization.h"
#include <client/Utility.h>
#include <Limits_constants.h>
#include <QObject>

namespace qute_note {

IResource::IResource() :
    NoteStoreDataElement(),
    m_isFreeAccount(true)
{}

IResource::IResource(const bool isFreeAccount) :
    NoteStoreDataElement(),
    m_isFreeAccount(isFreeAccount)
{}

IResource::~IResource()
{}

bool IResource::operator==(const IResource & other) const
{
    return (GetEnResource() == other.GetEnResource()) && (isDirty() == other.isDirty());
}

bool IResource::operator!=(const IResource & other) const
{
    return !(*this == other);
}

void IResource::clear()
{
    setDirty(true);
    GetEnResource() = evernote::edam::Resource();
}

bool IResource::hasGuid() const
{
    return GetEnResource().__isset.guid;
}

const QString IResource::guid() const
{
    return std::move(QString::fromStdString(GetEnResource().guid));
}

void IResource::setGuid(const QString & guid)
{
    auto & enResource = GetEnResource();
    enResource.guid = guid.toStdString();
    enResource.__isset.guid = true;
}

bool IResource::hasUpdateSequenceNumber() const
{
    return GetEnResource().__isset.updateSequenceNum;
}

qint32 IResource::updateSequenceNumber() const
{
    return GetEnResource().updateSequenceNum;
}

void IResource::setUpdateSequenceNumber(const qint32 updateSequenceNumber)
{
    auto & enResource = GetEnResource();
    enResource.updateSequenceNum = updateSequenceNumber;
    enResource.__isset.updateSequenceNum = true;
}

bool IResource::checkParameters(QString & errorDescription) const
{
    const auto & enResource = GetEnResource();

    if (!enResource.__isset.guid) {
        errorDescription = QObject::tr("Resource's guid is not set");
        return false;
    }
    else if (!CheckGuid(enResource.guid)) {
        errorDescription = QObject::tr("Resource's guid is invalid");
        return false;
    }

    if (!enResource.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Resource's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(enResource.updateSequenceNum)) {
        errorDescription = QObject::tr("Resource's update sequence number is invalid");
        return false;
    }

    if (enResource.__isset.noteGuid && !CheckGuid(enResource.noteGuid)) {
        errorDescription = QObject::tr("Resource's note guid is invalid");
        return false;
    }

#define CHECK_RESOURCE_DATA(name) \
    if (enResource.__isset.name) \
    { \
        if (!enResource.name.__isset.body) { \
            errorDescription = QObject::tr("Resource's " #name " body is not set"); \
            return false; \
        } \
        \
        int32_t dataSize = static_cast<int32_t>(enResource.name.body.size()); \
        int32_t allowedSize = (m_isFreeAccount \
                               ? evernote::limits::g_Limits_constants.EDAM_RESOURCE_SIZE_MAX_FREE \
                               : evernote::limits::g_Limits_constants.EDAM_RESOURCE_SIZE_MAX_PREMIUM); \
        if (dataSize > allowedSize) { \
            errorDescription = QObject::tr("Resource's " #name " body size is too large"); \
            return false; \
        } \
        \
        if (!enResource.name.__isset.size) { \
            errorDescription = QObject::tr("Resource's " #name " size is not set"); \
            return false; \
        } \
        \
        if (!enResource.name.__isset.bodyHash) { \
            errorDescription = QObject::tr("Resource's " #name " hash is not set"); \
            return false; \
        } \
        else { \
            int32_t hashSize = static_cast<int32_t>(enResource.name.bodyHash.size()); \
            if (hashSize != evernote::limits::g_Limits_constants.EDAM_HASH_LEN) { \
                errorDescription = QObject::tr("Invalid " #name " hash size"); \
                return false; \
            } \
        } \
    }

    CHECK_RESOURCE_DATA(data);
    // CHECK_RESOURCE_DATA(recognition);
    CHECK_RESOURCE_DATA(alternateData);

#undef CHECK_RESOURCE_DATA


    if (enResource.__isset.recognition)
    {
        if (!enResource.recognition.__isset.body) {
            errorDescription = QObject::tr("Resource's recognition data body is not set");
            return false;
        }

        int32_t dataSize = static_cast<int32_t>(enResource.recognition.body.size());
        int32_t allowedSize = (m_isFreeAccount
                               ? evernote::limits::g_Limits_constants.EDAM_RESOURCE_SIZE_MAX_FREE
                               : evernote::limits::g_Limits_constants.EDAM_RESOURCE_SIZE_MAX_PREMIUM);
        if (dataSize > allowedSize) {
            errorDescription = QObject::tr("Resource's recognition body size is too large");
            return false;
        }

        if (!enResource.recognition.__isset.size) {
            errorDescription = QObject::tr("Resource's recognition size is not set");
            return false;
        }

        if (!enResource.recognition.__isset.bodyHash) {
            errorDescription = QObject::tr("Resource's recognition hash is not set");
            return false;
        }
        else {
            int32_t hashSize = static_cast<int32_t>(enResource.recognition.bodyHash.size());
            if (hashSize != evernote::limits::g_Limits_constants.EDAM_HASH_LEN) {
                errorDescription = QObject::tr("Invalid recognition hash size");
                return false;
            }
        }
    }


    if (!enResource.__isset.data && enResource.__isset.alternateData) {
        errorDescription = QObject::tr("Resource has no data set but alternate data is present");
        return false;
    }

    if (!enResource.__isset.mime)
    {
        errorDescription = QObject::tr("Resource's mime type is not set");
        return false;
    }
    else
    {
        int32_t mimeSize = static_cast<int32_t>(enResource.mime.size());
        if ( (mimeSize < evernote::limits::g_Limits_constants.EDAM_MIME_LEN_MIN) ||
             (mimeSize > evernote::limits::g_Limits_constants.EDAM_MIME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Resource's mime type has invalid length");
            return false;
        }
    }

    if (enResource.__isset.attributes)
    {
        if (enResource.attributes.__isset.sourceURL)
        {
            int32_t sourceURLSize = static_cast<int32_t>(enResource.attributes.sourceURL.size());
            if ( (sourceURLSize < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) ||
                 (sourceURLSize > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription = QObject::tr("Resource's sourceURL attribute has invalid length");
                return false;
            }
        }

        if (enResource.attributes.__isset.cameraMake)
        {
            int32_t cameraMakeSize = static_cast<int32_t>(enResource.attributes.cameraMake.size());
            if ( (cameraMakeSize < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) ||
                 (cameraMakeSize > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription = QObject::tr("Resource's cameraMake attribute has invalid length");
                return false;
            }
        }

        if (enResource.attributes.__isset.cameraModel)
        {
            int32_t cameraModelSize = static_cast<int32_t>(enResource.attributes.cameraModel.size());
            if ( (cameraModelSize < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) ||
                 (cameraModelSize > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription = QObject::tr("Resource's cameraModel attribute has invalid length");
                return false;
            }
        }
    }

    return true;
}

bool IResource::isFreeAccount() const
{
    return m_isFreeAccount;
}

void IResource::setFreeAccount(const bool isFreeAccount)
{
    m_isFreeAccount = isFreeAccount;
}

bool IResource::hasNoteGuid() const
{
    return GetEnResource().__isset.noteGuid;
}

const QString IResource::noteGuid() const
{
    return std::move(QString::fromStdString(GetEnResource().noteGuid));
}

void IResource::setNoteGuid(const QString & guid)
{
    auto & enResource = GetEnResource();
    enResource.noteGuid = guid.toStdString();
    enResource.__isset.noteGuid = true;
}

bool IResource::hasData() const
{
    return GetEnResource().__isset.data;
}

bool IResource::hasDataHash() const
{
    if (!hasData()) {
        return false;
    }

    return GetEnResource().data.__isset.bodyHash;
}

const QString IResource::dataHash() const
{
    return std::move(QString::fromStdString(GetEnResource().data.bodyHash));
}

#define CHECK_AND_SET_DATA \
    const auto & isSet = enResource.data.__isset; \
    if (isSet.bodyHash && isSet.size && isSet.body) { \
        enResource.__isset.data = true; \
    }

void IResource::setDataHash(const QString & hash)
{
    auto & enResource = GetEnResource();
    enResource.data.bodyHash = hash.toStdString();
    enResource.data.__isset.bodyHash = true;
    CHECK_AND_SET_DATA
}

bool IResource::hasDataSize() const
{
    if (!hasData()) {
        return false;
    }

    return GetEnResource().data.__isset.size;
}

qint32 IResource::dataSize() const
{
    return GetEnResource().data.size;
}

void IResource::setDataSize(const qint32 size)
{
    auto & enResource = GetEnResource();
    enResource.data.size = size;
    enResource.data.__isset.size = true;
    CHECK_AND_SET_DATA
}

bool IResource::hasDataBody() const
{
    if (!hasData()) {
        return false;
    }

    return GetEnResource().data.__isset.body;
}

const QString IResource::dataBody() const
{
    return std::move(QString::fromStdString(GetEnResource().data.body));
}

void IResource::setDataBody(const QString & body)
{
    auto & enResource = GetEnResource();
    enResource.data.body = body.toStdString();
    enResource.data.__isset.body = true;
    CHECK_AND_SET_DATA
}

bool IResource::hasMime() const
{
    return GetEnResource().__isset.mime;
}

const QString IResource::mime() const
{
    return std::move(QString::fromStdString(GetEnResource().mime));
}

void IResource::setMime(const QString & mime)
{
    auto & enResource = GetEnResource();
    enResource.mime = mime.toStdString();
    enResource.__isset.mime = true;
}

bool IResource::hasWidth() const
{
    return GetEnResource().__isset.width;
}

qint16 IResource::width() const
{
    return GetEnResource().width;
}

void IResource::setWidth(const qint16 width)
{
    auto & enResource = GetEnResource();
    enResource.width = width;
    enResource.__isset.width = true;
}

bool IResource::hasHeight() const
{
    return GetEnResource().__isset.height;
}

qint16 IResource::height() const
{
    return GetEnResource().height;
}

void IResource::setHeight(const qint16 height)
{
    auto & enResource = GetEnResource();
    enResource.height = height;
    enResource.__isset.height = true;
}

#undef CHECK_AND_SET_DATA

bool IResource::hasRecognitionData() const
{
    return GetEnResource().__isset.recognition;
}

bool IResource::hasRecognitionDataHash() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return GetEnResource().recognition.__isset.bodyHash;
}

const QString IResource::recognitionDataHash() const
{
    return std::move(QString::fromStdString(GetEnResource().recognition.bodyHash));
}

#define CHECK_AND_SET_RECOGNITION \
    const auto & isSet = enResource.recognition.__isset; \
    if (isSet.bodyHash && isSet.size && isSet.body) { \
        enResource.__isset.recognition = true; \
    }

void IResource::setRecognitionDataHash(const QString & hash)
{
    auto & enResource = GetEnResource();
    enResource.recognition.bodyHash = hash.toStdString();
    enResource.recognition.__isset.bodyHash = true;
    CHECK_AND_SET_RECOGNITION
}

bool IResource::hasRecognitionDataSize() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return GetEnResource().recognition.__isset.size;
}

qint32 IResource::recognitionDataSize() const
{
    return GetEnResource().recognition.size;
}

void IResource::setRecognitionDataSize(const qint32 size)
{
    auto & enResource = GetEnResource();
    enResource.recognition.size = size;
    enResource.recognition.__isset.size = true;
    CHECK_AND_SET_RECOGNITION
}

bool IResource::hasRecognitionDataBody() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return GetEnResource().recognition.__isset.body;
}

const QString IResource::recognitionDataBody() const
{
    return std::move(QString::fromStdString(GetEnResource().recognition.body));
}

void IResource::setRecognitionDataBody(const QString & body)
{
    auto & enResource = GetEnResource();
    enResource.recognition.body = body.toStdString();
    enResource.recognition.__isset.body = true;
    CHECK_AND_SET_RECOGNITION
}

#undef CHECK_AND_SET_RECOGNITION

bool IResource::hasAlternateData() const
{
    return GetEnResource().__isset.alternateData;
}

bool IResource::hasAlternateDataHash() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return GetEnResource().alternateData.__isset.bodyHash;
}

const QString IResource::alternateDataHash() const
{
    return std::move(QString::fromStdString(GetEnResource().alternateData.bodyHash));
}

#define CHECK_AND_SET_ALTERNATE_DATA \
    const auto & isSet = enResource.alternateData.__isset; \
    if (isSet.bodyHash && isSet.size && isSet.body) { \
        enResource.__isset.alternateData = true; \
    }

void IResource::setAlternateDataHash(const QString & hash)
{
    auto & enResource = GetEnResource();
    enResource.alternateData.body = hash.toStdString();
    enResource.alternateData.__isset.body = true;
    CHECK_AND_SET_ALTERNATE_DATA
}

bool IResource::hasAlternateDataSize() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return GetEnResource().alternateData.__isset.size;
}

qint32 IResource::alternateDataSize() const
{
    return GetEnResource().alternateData.size;
}

void IResource::setAlternateDataSize(const qint32 size)
{
    auto & enResource = GetEnResource();
    enResource.alternateData.size = size;
    enResource.alternateData.__isset.size = true;
    CHECK_AND_SET_ALTERNATE_DATA
}

bool IResource::hasAlternateDataBody() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return GetEnResource().alternateData.__isset.body;
}

const QString IResource::alternateDataBody() const
{
    return std::move(QString::fromStdString(GetEnResource().alternateData.body));
}

void IResource::setAlternateDataBody(const QString &body)
{
    auto & enResource = GetEnResource();
    enResource.alternateData.body = body.toStdString();
    enResource.alternateData.__isset.body = true;
    CHECK_AND_SET_ALTERNATE_DATA
}

#undef CHECK_AND_SET_ALTERNATE_DATA

bool IResource::hasResourceAttributes() const
{
    return GetEnResource().__isset.attributes;
}

const QByteArray IResource::resourceAttributes() const
{
    return std::move(GetSerializedResourceAttributes(GetEnResource().attributes));
}

void IResource::setResourceAttributes(const QByteArray & resourceAttributes)
{
    auto & enResource = GetEnResource();
    enResource.attributes = GetDeserializedResourceAttributes(resourceAttributes);
    enResource.__isset.attributes = true;
}

#undef CHECK_AND_SET_RECOGNITION

IResource::IResource(const IResource & other) :
    NoteStoreDataElement(other),
    m_isFreeAccount(other.m_isFreeAccount)
{}

IResource & IResource::operator=(const IResource & other)
{
    if (this != &other)
    {
        NoteStoreDataElement::operator =(other);
        setFreeAccount(other.m_isFreeAccount);
    }

    return *this;
}

QTextStream & IResource::Print(QTextStream & strm) const
{
    const auto & enResource = GetEnResource();
    const auto & isSet = enResource.__isset;

    if (isSet.guid) {
        strm << "guid = " << QString::fromStdString(enResource.guid) << "; \n";
    }
    else {
        strm << "guid is not set" << "; \n";
    }

    if (isSet.updateSequenceNum) {
        strm << "updateSequenceNumber = " << enResource.updateSequenceNum << "; \n";
    }
    else {
        strm << "updateSequenceNumber is not set";
    }

    if (isSet.noteGuid) {
        strm << "noteGuid = " << QString::fromStdString(enResource.noteGuid) << "; \n";
    }
    else {
        strm << "noteGuid is not set" << "; \n";
    }

    if (isSet.data)
    {
        if (enResource.data.__isset.size) {
            strm << "dataSize = " << enResource.data.size << "; \n";
        }
        else {
            strm << "dataSize is not set" << "; \n";
        }

        if (enResource.data.__isset.bodyHash) {
            strm << "dataHash = " << QString::fromStdString(enResource.data.bodyHash) << "; \n";
        }
        else {
            strm << "dataHash is not set" << "; \n";
        }

        if (enResource.data.__isset.body) {
            strm << "dataBody is set" << "; \n";
        }
        else {
            strm << "dataBody is not set" << "; \n";
        }
    }

    if (isSet.mime) {
        strm << "mime = " << QString::fromStdString(enResource.mime) << "; \n";
    }
    else {
        strm << "mime is not set" << "; \n";
    }

    if (isSet.width) {
        strm << "width = " << enResource.width << "; \n";
    }
    else {
        strm << "width is not set" << "; \n";
    }

    if (isSet.height) {
        strm << "height = " << enResource.height << "; \n";
    }
    else {
        strm << "height is not set" << "; \n";
    }

    if (isSet.recognition)
    {
        if (enResource.recognition.__isset.size) {
            strm << "recognitionDataSize = " << enResource.recognition.size << "; \n";
        }
        else {
            strm << "recognitionDataSize is not set" << "; \n";
        }

        if (enResource.recognition.__isset.bodyHash) {
            strm << "recognitionDataHash = " << QString::fromStdString(enResource.recognition.bodyHash) << "; \n";
        }
        else {
            strm << "recognitionDataHash is not set" << "; \n";
        }

        if (enResource.recognition.__isset.body) {
            strm << "recognitionDataBody is set" << "; \n";
        }
        else {
            strm << "recognitionDataBody is not set" << "; \n";
        }
    }

    if (isSet.alternateData)
    {
        if (enResource.alternateData.__isset.size) {
            strm << "alternateDataSize = " << enResource.alternateData.size << "; \n";
        }
        else {
            strm << "alternateDataSize is not set" << "; \n";
        }

        if (enResource.alternateData.__isset.bodyHash) {
            strm << "alternateDataHash = " << QString::fromStdString(enResource.alternateData.bodyHash) << "; \n";
        }
        else {
            strm << "alternateDataHash is not set" << "; \n";
        }

        if (enResource.alternateData.__isset.body) {
            strm << "alternateDataBody is set" << "; \n";
        }
        else {
            strm << "alternateDataBody is not set" << "; \n";
        }
    }

    strm << "isDirty = " << (isDirty() ? "true" : "false") << "; \n";

    strm << "isFreeAccount = " << (m_isFreeAccount ? "true" : "false") << "; \n";

    if (isSet.attributes)
    {
        const auto & attributes = enResource.attributes;
        const auto & attributesIsSet = attributes.__isset;

        if (attributesIsSet.sourceURL) {
            strm << "sourceURL = " << QString::fromStdString(attributes.sourceURL) << "; \n";
        }
        else {
            strm << "sourceURL is not set" << "; \n";
        }

        if (attributesIsSet.timestamp) {
            strm << "timestamp = " << attributes.timestamp << "; \n";
        }
        else {
            strm << "timestamp is not set" << "; \n";
        }

        if (attributesIsSet.latitude) {
            strm << "latitude = " << attributes.latitude << "; \n";
        }
        else {
            strm << "latitude is not set" << "; \n";
        }

        if (attributesIsSet.longitude) {
            strm << "longitude = " << attributes.longitude << "; \n";
        }
        else {
            strm << "longitude is not set" << "; \n";
        }

        if (attributesIsSet.altitude) {
            strm << "altitude = " << attributes.altitude << "; \n";
        }
        else {
            strm << "altitude is not set" << "; \n";
        }

        if (attributesIsSet.cameraMake) {
            strm << "cameraMake = " << attributes.cameraMake << "; \n";
        }
        else {
            strm << "cameraMake is not set" << "; \n";
        }

        if (attributesIsSet.cameraModel) {
            strm << "cameraModel = " << QString::fromStdString(attributes.cameraModel) << "; \n";
        }
        else {
            strm << "cameraModel is not set" << "; \n";
        }

        if (attributesIsSet.clientWillIndex) {
            strm << "clientWillIndex = " << (attributes.clientWillIndex ? "true" : "false") << "; \n";
        }
        else {
            strm << "clientWillIndex is not set" << "; \n";
        }

        if (attributesIsSet.fileName) {
            strm << "fileName = " << QString::fromStdString(attributes.fileName) << "; \n";
        }
        else {
            strm << "fileName is not set" << "; \n";
        }

        if (attributesIsSet.attachment) {
            strm << "attachment = " << (attributes.attachment ? "true" : "false") << "; \n";
        }
        else {
            strm << "attachment is not set" << "; \n";
        }

        if (attributesIsSet.applicationData)
        {
            const auto & applicationData = attributes.applicationData;

            const auto & keysOnly = applicationData.keysOnly;
            for(const auto & item: keysOnly) {
                strm << "applicationData key: " << QString::fromStdString(item) << "; \n";
            }

            const auto & fullMap = applicationData.fullMap;
            for(const auto & pair: fullMap) {
                strm << "applicationData[" << QString::fromStdString(pair.first)
                     << "] = " << QString::fromStdString(pair.second) << "; \n";
            }
        }
    }

    return strm;
}

}
