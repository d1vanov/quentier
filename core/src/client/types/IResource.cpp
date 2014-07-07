#include "IResource.h"
#include "QEverCloudHelpers.h"
#include <client/Utility.h>

namespace qute_note {

IResource::IResource() :
    NoteStoreDataElement(),
    m_isFreeAccount(true),
    m_indexInNote(-1),
    m_noteLocalGuid()
{}

IResource::IResource(const bool isFreeAccount) :
    NoteStoreDataElement(),
    m_isFreeAccount(isFreeAccount),
    m_indexInNote(-1),
    m_noteLocalGuid()
{}

IResource::~IResource()
{}

bool IResource::operator==(const IResource & other) const
{   
    return (GetEnResource() == other.GetEnResource()) &&
           (isDirty() == other.isDirty()) &&
           (m_noteLocalGuid.isEqual(other.m_noteLocalGuid)) &&
           (localGuid() == other.localGuid());

    // NOTE: m_indexInNote does not take any part in comparison
    // as it is by nature a helper parameter intended to preserve sorting of resources
    // in note between insertions and selections from the database
}

bool IResource::operator!=(const IResource & other) const
{
    return !(*this == other);
}

void IResource::clear()
{
    setDirty(true);
    GetEnResource() = qevercloud::Resource();
}

bool IResource::hasGuid() const
{
    return GetEnResource().guid.isSet();
}

const QString & IResource::guid() const
{
    return GetEnResource().guid;
}

void IResource::setGuid(const QString & guid)
{
    GetEnResource().guid = guid;
}

bool IResource::hasUpdateSequenceNumber() const
{
    return GetEnResource().updateSequenceNum.isSet();
}

qint32 IResource::updateSequenceNumber() const
{
    return GetEnResource().updateSequenceNum;
}

void IResource::setUpdateSequenceNumber(const qint32 updateSequenceNumber)
{
    GetEnResource().updateSequenceNum = updateSequenceNumber;
}

bool IResource::checkParameters(QString & errorDescription) const
{
    const qevercloud::Resource & enResource = GetEnResource();

    if (localGuid().isEmpty() && !enResource.guid.isSet()) {
        errorDescription = QT_TR_NOOP("Both resource's local and remote guids are empty");
        return false;
    }

    if (enResource.guid.isSet() && !CheckGuid(enResource.guid.ref())) {
        errorDescription = QT_TR_NOOP("Resource's guid is invalid");
        return false;
    }

    if (!enResource.updateSequenceNum.isSet()) {
        errorDescription = QT_TR_NOOP("Resource's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(enResource.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Resource's update sequence number is invalid");
        return false;
    }

    if (enResource.noteGuid.isSet() && !CheckGuid(enResource.noteGuid.ref())) {
        errorDescription = QT_TR_NOOP("Resource's note guid is invalid");
        return false;
    }

#define CHECK_RESOURCE_DATA(name) \
    if (enResource.name.isSet()) \
    { \
        if (!enResource.name->body.isSet()) { \
            errorDescription = QT_TR_NOOP("Resource's " #name " body is not set"); \
            return false; \
        } \
        \
        int32_t dataSize = static_cast<int32_t>(enResource.name->body->size()); \
        int32_t allowedSize = (m_isFreeAccount \
                               ? qevercloud::EDAM_RESOURCE_SIZE_MAX_FREE \
                               : qevercloud::EDAM_RESOURCE_SIZE_MAX_PREMIUM); \
        if (dataSize > allowedSize) { \
            errorDescription = QT_TR_NOOP("Resource's " #name " body size is too large"); \
            return false; \
        } \
        \
        if (!enResource.name->size.isSet()) { \
            errorDescription = QT_TR_NOOP("Resource's " #name " size is not set"); \
            return false; \
        } \
        \
        if (!enResource.name->bodyHash.isSet()) { \
            errorDescription = QT_TR_NOOP("Resource's " #name " hash is not set"); \
            return false; \
        } \
        else { \
            int32_t hashSize = static_cast<int32_t>(enResource.name->bodyHash->size()); \
            if (hashSize != qevercloud::EDAM_HASH_LEN) { \
                errorDescription = QT_TR_NOOP("Invalid " #name " hash size"); \
                return false; \
            } \
        } \
    }

    CHECK_RESOURCE_DATA(data);
    CHECK_RESOURCE_DATA(recognition);
    CHECK_RESOURCE_DATA(alternateData);

#undef CHECK_RESOURCE_DATA

    if (!enResource.data.isSet() && enResource.alternateData.isSet()) {
        errorDescription = QT_TR_NOOP("Resource has no data set but alternate data is present");
        return false;
    }

    if (!enResource.mime.isSet())
    {
        errorDescription = QT_TR_NOOP("Resource's mime type is not set");
        return false;
    }
    else
    {
        int32_t mimeSize = static_cast<int32_t>(enResource.mime->size());
        if ( (mimeSize < qevercloud::EDAM_MIME_LEN_MIN) ||
             (mimeSize > qevercloud::EDAM_MIME_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Resource's mime type has invalid length");
            return false;
        }
    }

    if (enResource.attributes.isSet())
    {
        if (enResource.attributes->sourceURL.isSet())
        {
            int32_t sourceURLSize = static_cast<int32_t>(enResource.attributes->sourceURL->size());
            if ( (sourceURLSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) ||
                 (sourceURLSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Resource's sourceURL attribute has invalid length");
                return false;
            }
        }

        if (enResource.attributes->cameraMake.isSet())
        {
            int32_t cameraMakeSize = static_cast<int32_t>(enResource.attributes->cameraMake->size());
            if ( (cameraMakeSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) ||
                 (cameraMakeSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Resource's cameraMake attribute has invalid length");
                return false;
            }
        }

        if (enResource.attributes->cameraModel.isSet())
        {
            int32_t cameraModelSize = static_cast<int32_t>(enResource.attributes->cameraModel->size());
            if ( (cameraModelSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) ||
                 (cameraModelSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Resource's cameraModel attribute has invalid length");
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

int IResource::indexInNote() const
{
    return m_indexInNote;
}

void IResource::setIndexInNote(const int index)
{
    m_indexInNote = index;
}

bool IResource::hasNoteGuid() const
{
    return GetEnResource().noteGuid.isSet();
}

const QString & IResource::noteGuid() const
{
    return GetEnResource().noteGuid;
}

void IResource::setNoteGuid(const QString & guid)
{
    GetEnResource().noteGuid = guid;
}

bool IResource::hasNoteLocalGuid() const
{
    return m_noteLocalGuid.isSet();
}

const QString & IResource::noteLocalGuid() const
{
    return m_noteLocalGuid;
}

void IResource::setNoteLocalGuid(const QString & guid)
{
    m_noteLocalGuid = guid;
}

bool IResource::hasData() const
{
    return GetEnResource().data.isSet();
}

bool IResource::hasDataHash() const
{
    if (!hasData()) {
        return false;
    }

    return GetEnResource().data->bodyHash.isSet();
}

const QByteArray & IResource::dataHash() const
{
    return GetEnResource().data->bodyHash;
}

#define CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD(empty_condition, data_field, field, \
                                              other_field1, other_field2) \
    if (empty_condition && enResource.data_field.isSet() && enResource.data_field->field.isSet()) \
    { \
        qevercloud::Optional<qevercloud::Data> & data_field = enResource.data_field; \
        data_field->field.clear(); \
        if (!data_field->other_field1.isSet() && !data_field->other_field2.isSet()) { \
            data_field.clear(); \
        } \
        return; \
    } \
    \
    if (!enResource.data_field.isSet()) { \
        enResource.data_field = qevercloud::Data(); \
    }

void IResource::setDataHash(const QByteArray & hash)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD(hash.isEmpty(), data, bodyHash, body, size);
    enResource.data->bodyHash = hash;
}

bool IResource::hasDataSize() const
{
    if (!hasData()) {
        return false;
    }

    return GetEnResource().data->size.isSet();
}

qint32 IResource::dataSize() const
{
    return GetEnResource().data->size;
}

void IResource::setDataSize(const qint32 size)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD((size <= 0), data, size, body, bodyHash);
    enResource.data->size = size;
}

bool IResource::hasDataBody() const
{
    if (!hasData()) {
        return false;
    }

    return GetEnResource().data->body.isSet();
}

const QByteArray & IResource::dataBody() const
{
    return GetEnResource().data->body;
}

void IResource::setDataBody(const QByteArray & body)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD(body.isEmpty(), data, body, size, bodyHash);
    enResource.data->body = body;
}

bool IResource::hasMime() const
{
    return GetEnResource().mime.isSet();
}

const QString & IResource::mime() const
{
    return GetEnResource().mime;
}

void IResource::setMime(const QString & mime)
{
    GetEnResource().mime = mime;
}

bool IResource::hasWidth() const
{
    return GetEnResource().width.isSet();
}

qint16 IResource::width() const
{
    return GetEnResource().width;
}

void IResource::setWidth(const qint16 width)
{
    if (width < 0) {
        GetEnResource().width.clear();
    }
    else {
        GetEnResource().width = width;
    }
}

bool IResource::hasHeight() const
{
    return GetEnResource().height.isSet();
}

qint16 IResource::height() const
{
    return GetEnResource().height;
}

void IResource::setHeight(const qint16 height)
{
    if (height < 0) {
        GetEnResource().height.clear();
    }
    else {
        GetEnResource().height = height;
    }
}

bool IResource::hasRecognitionData() const
{
    return GetEnResource().recognition.isSet();
}

bool IResource::hasRecognitionDataHash() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return GetEnResource().recognition->bodyHash.isSet();
}

const QByteArray & IResource::recognitionDataHash() const
{
    return GetEnResource().recognition->bodyHash;
}

void IResource::setRecognitionDataHash(const QByteArray & hash)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD(hash.isEmpty(), recognition, bodyHash, body, size);
    enResource.recognition->bodyHash = hash;
}

bool IResource::hasRecognitionDataSize() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return GetEnResource().recognition->size.isSet();
}

qint32 IResource::recognitionDataSize() const
{
    return GetEnResource().recognition->size;
}

void IResource::setRecognitionDataSize(const qint32 size)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD((size <= 0), recognition, size, body, bodyHash);
    enResource.recognition->size = size;
}

bool IResource::hasRecognitionDataBody() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return GetEnResource().recognition->body.isSet();
}

const QByteArray & IResource::recognitionDataBody() const
{
    return GetEnResource().recognition->body;
}

void IResource::setRecognitionDataBody(const QByteArray & body)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD(body.isEmpty(), recognition, body, size, bodyHash);
    enResource.recognition->body = body;
}

bool IResource::hasAlternateData() const
{
    return GetEnResource().alternateData.isSet();
}

bool IResource::hasAlternateDataHash() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return GetEnResource().alternateData->bodyHash.isSet();
}

const QByteArray & IResource::alternateDataHash() const
{
    return GetEnResource().alternateData->bodyHash;
}

void IResource::setAlternateDataHash(const QByteArray & hash)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD(hash.isEmpty(), alternateData, bodyHash, size, body);
    enResource.alternateData->bodyHash = hash;
}

bool IResource::hasAlternateDataSize() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return GetEnResource().alternateData->size.isSet();
}

qint32 IResource::alternateDataSize() const
{
    return GetEnResource().alternateData->size;
}

void IResource::setAlternateDataSize(const qint32 size)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD((size <= 0), alternateData, size, body, bodyHash);
    enResource.alternateData->size = size;
}

bool IResource::hasAlternateDataBody() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return GetEnResource().alternateData->body.isSet();
}

const QByteArray & IResource::alternateDataBody() const
{
    return GetEnResource().alternateData->body;
}

void IResource::setAlternateDataBody(const QByteArray & body)
{
    qevercloud::Resource & enResource = GetEnResource();
    CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD(body.isEmpty(), alternateData, body, size, bodyHash);
    enResource.alternateData->body = body;
}

#undef CHECK_AND_EMPTIFY_RESOURCE_DATA_FIELD

bool IResource::hasResourceAttributes() const
{
    return GetEnResource().attributes.isSet();
}

const qevercloud::ResourceAttributes & IResource::resourceAttributes() const
{
    return GetEnResource().attributes;
}

qevercloud::ResourceAttributes & IResource::resourceAttributes()
{
    qevercloud::Resource & enResource = GetEnResource();
    if (!enResource.attributes.isSet()) {
        enResource.attributes = qevercloud::ResourceAttributes();
    }

    return enResource.attributes;
}

void IResource::setResourceAttributes(const qevercloud::ResourceAttributes & attributes)
{
    GetEnResource().attributes = attributes;
}

void IResource::setResourceAttributes(qevercloud::ResourceAttributes && attributes)
{
    GetEnResource().attributes = std::move(attributes);
}

IResource::IResource(const IResource & other) :
    NoteStoreDataElement(other),
    m_isFreeAccount(other.m_isFreeAccount),
    m_indexInNote(other.indexInNote())
{}

IResource & IResource::operator=(const IResource & other)
{
    if (this != &other)
    {
        NoteStoreDataElement::operator =(other);
        setFreeAccount(other.m_isFreeAccount);
        setIndexInNote(other.m_indexInNote);
    }

    return *this;
}

QTextStream & IResource::Print(QTextStream & strm) const
{
    const qevercloud::Resource & enResource = GetEnResource();

    if (enResource.guid.isSet()) {
        strm << "guid = " << enResource.guid << "; \n";
    }
    else {
        strm << "guid is not set" << "; \n";
    }

    if (enResource.updateSequenceNum.isSet()) {
        strm << "updateSequenceNumber = " << QString::number(enResource.updateSequenceNum) << "; \n";
    }
    else {
        strm << "updateSequenceNumber is not set";
    }

    if (enResource.noteGuid.isSet()) {
        strm << "noteGuid = " << enResource.noteGuid << "; \n";
    }
    else {
        strm << "noteGuid is not set" << "; \n";
    }

    if (enResource.data.isSet())
    {
        if (enResource.data->size.isSet()) {
            strm << "dataSize = " << QString::number(enResource.data->size) << "; \n";
        }
        else {
            strm << "dataSize is not set" << "; \n";
        }

        if (enResource.data->bodyHash.isSet()) {
            strm << "dataHash = " << enResource.data->bodyHash << "; \n";
        }
        else {
            strm << "dataHash is not set" << "; \n";
        }

        if (enResource.data->body.isSet()) {
            strm << "dataBody is set" << "; \n";
        }
        else {
            strm << "dataBody is not set" << "; \n";
        }
    }

    if (enResource.mime.isSet()) {
        strm << "mime = " << enResource.mime << "; \n";
    }
    else {
        strm << "mime is not set" << "; \n";
    }

    if (enResource.width.isSet()) {
        strm << "width = " << QString::number(enResource.width) << "; \n";
    }
    else {
        strm << "width is not set" << "; \n";
    }

    if (enResource.height.isSet()) {
        strm << "height = " << QString::number(enResource.height) << "; \n";
    }
    else {
        strm << "height is not set" << "; \n";
    }

    if (enResource.recognition.isSet())
    {
        if (enResource.recognition->size.isSet()) {
            strm << "recognitionDataSize = " << QString::number(enResource.recognition->size) << "; \n";
        }
        else {
            strm << "recognitionDataSize is not set" << "; \n";
        }

        if (enResource.recognition->bodyHash.isSet()) {
            strm << "recognitionDataHash = " << enResource.recognition->bodyHash << "; \n";
        }
        else {
            strm << "recognitionDataHash is not set" << "; \n";
        }

        if (enResource.recognition->body.isSet()) {
            strm << "recognitionDataBody is set" << "; \n";
        }
        else {
            strm << "recognitionDataBody is not set" << "; \n";
        }
    }

    if (enResource.alternateData.isSet())
    {
        if (enResource.alternateData->size.isSet()) {
            strm << "alternateDataSize = " << QString::number(enResource.alternateData->size) << "; \n";
        }
        else {
            strm << "alternateDataSize is not set" << "; \n";
        }

        if (enResource.alternateData->bodyHash.isSet()) {
            strm << "alternateDataHash = " << enResource.alternateData->bodyHash << "; \n";
        }
        else {
            strm << "alternateDataHash is not set" << "; \n";
        }

        if (enResource.alternateData->body.isSet()) {
            strm << "alternateDataBody is set" << "; \n";
        }
        else {
            strm << "alternateDataBody is not set" << "; \n";
        }
    }

    strm << "isDirty = " << (isDirty() ? "true" : "false") << "; \n";

    strm << "isFreeAccount = " << (m_isFreeAccount ? "true" : "false") << "; \n";

    strm << "indexInNote = " << QString::number(m_indexInNote) << "; \n";

    if (enResource.attributes.isSet())
    {
        const qevercloud::ResourceAttributes & attributes = enResource.attributes;

        if (attributes.sourceURL.isSet()) {
            strm << "sourceURL = " << attributes.sourceURL << "; \n";
        }
        else {
            strm << "sourceURL is not set" << "; \n";
        }

        if (attributes.timestamp.isSet()) {
            strm << "timestamp = " << attributes.timestamp << "; \n";
        }
        else {
            strm << "timestamp is not set" << "; \n";
        }

        if (attributes.latitude.isSet()) {
            strm << "latitude = " << attributes.latitude << "; \n";
        }
        else {
            strm << "latitude is not set" << "; \n";
        }

        if (attributes.longitude.isSet()) {
            strm << "longitude = " << attributes.longitude << "; \n";
        }
        else {
            strm << "longitude is not set" << "; \n";
        }

        if (attributes.altitude.isSet()) {
            strm << "altitude = " << attributes.altitude << "; \n";
        }
        else {
            strm << "altitude is not set" << "; \n";
        }

        if (attributes.cameraMake.isSet()) {
            strm << "cameraMake = " << attributes.cameraMake << "; \n";
        }
        else {
            strm << "cameraMake is not set" << "; \n";
        }

        if (attributes.cameraModel.isSet()) {
            strm << "cameraModel = " << attributes.cameraModel << "; \n";
        }
        else {
            strm << "cameraModel is not set" << "; \n";
        }

        if (attributes.clientWillIndex.isSet()) {
            strm << "clientWillIndex = " << (attributes.clientWillIndex ? "true" : "false") << "; \n";
        }
        else {
            strm << "clientWillIndex is not set" << "; \n";
        }

        if (attributes.fileName.isSet()) {
            strm << "fileName = " << attributes.fileName << "; \n";
        }
        else {
            strm << "fileName is not set" << "; \n";
        }

        if (attributes.attachment.isSet()) {
            strm << "attachment = " << (attributes.attachment ? "true" : "false") << "; \n";
        }
        else {
            strm << "attachment is not set" << "; \n";
        }

        if (attributes.applicationData.isSet())
        {
            const qevercloud::LazyMap & applicationData = attributes.applicationData;

            if (applicationData.keysOnly.isSet()) {
                const QSet<QString> & keysOnly = applicationData.keysOnly;
                foreach(const QString & item, keysOnly) {
                    strm << "applicationData key: " << item << "; \n";
                }
            }

            if (applicationData.fullMap.isSet()) {
                const QMap<QString, QString> & fullMap = applicationData.fullMap;
                foreach(const QString & key, fullMap) {
                    strm << "applicationData[" << key << "] = " << fullMap.value(key) << "; \n";
                }
            }
        }
    }

    return strm;
}

}
