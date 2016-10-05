/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "data/NoteStoreDataElementData.h"
#include <quentier/types/IResource.h>
#include <quentier/utility/Utility.h>
#include <quentier/logging/QuentierLogger.h>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

namespace quentier {

QN_DEFINE_LOCAL_UID(IResource)
QN_DEFINE_DIRTY(IResource)
QN_DEFINE_LOCAL(IResource)

IResource::IResource() :
    INoteStoreDataElement(),
    d(new NoteStoreDataElementData),
    m_indexInNote(-1),
    m_noteLocalUid()
{}

IResource::~IResource()
{}

bool IResource::operator==(const IResource & other) const
{
    return (GetEnResource() == other.GetEnResource()) &&
           (isDirty() == other.isDirty()) &&
           (m_noteLocalUid.isEqual(other.m_noteLocalUid)) &&
           (localUid() == other.localUid());

    // NOTE: m_indexInNote does not take any part in comparison
    // as it is by nature a helper parameter intended to preserve sorting of resources
    // in note between insertions and selections from the database
}

bool IResource::operator!=(const IResource & other) const
{
    return !(*this == other);
}

IResource::operator const qevercloud::Resource &() const
{
    return GetEnResource();
}

IResource::operator qevercloud::Resource &()
{
    return GetEnResource();
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
    if (!guid.isEmpty()) {
        GetEnResource().guid = guid;
    }
    else {
        GetEnResource().guid.clear();
    }
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

bool IResource::checkParameters(QNLocalizedString & errorDescription) const
{
    const qevercloud::Resource & enResource = GetEnResource();

    if (localUid().isEmpty() && !enResource.guid.isSet()) {
        errorDescription = QT_TR_NOOP("both resource's local and remote guids are empty");
        return false;
    }

    if (enResource.guid.isSet() && !checkGuid(enResource.guid.ref())) {
        errorDescription = QT_TR_NOOP("resource's guid is invalid");
        return false;
    }

    if (enResource.updateSequenceNum.isSet() && !checkUpdateSequenceNumber(enResource.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("resource's update sequence number is invalid");
        return false;
    }

    if (enResource.noteGuid.isSet() && !checkGuid(enResource.noteGuid.ref())) {
        errorDescription = QT_TR_NOOP("resource's note guid is invalid");
        return false;
    }

#define CHECK_RESOURCE_DATA(name) \
    if (enResource.name.isSet()) \
    { \
        if (!enResource.name->body.isSet()) { \
            errorDescription = QT_TR_NOOP("resource's " #name " body is not set"); \
            return false; \
        } \
        \
        if (!enResource.name->size.isSet()) { \
            errorDescription = QT_TR_NOOP("resource's " #name " size is not set"); \
            return false; \
        } \
        \
        if (!enResource.name->bodyHash.isSet()) { \
            errorDescription = QT_TR_NOOP("resource's " #name " hash is not set"); \
            return false; \
        } \
        else { \
            int32_t hashSize = static_cast<int32_t>(enResource.name->bodyHash->size()); \
            if (hashSize != qevercloud::EDAM_HASH_LEN) { \
                errorDescription = QT_TR_NOOP("invalid " #name " hash size"); \
                return false; \
            } \
        } \
    }

    CHECK_RESOURCE_DATA(data);
    CHECK_RESOURCE_DATA(recognition);
    CHECK_RESOURCE_DATA(alternateData);

#undef CHECK_RESOURCE_DATA

    if (enResource.mime.isSet())
    {
        int32_t mimeSize = static_cast<int32_t>(enResource.mime->size());
        if ( (mimeSize < qevercloud::EDAM_MIME_LEN_MIN) ||
             (mimeSize > qevercloud::EDAM_MIME_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("resource's mime type has invalid length");
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
                errorDescription = QT_TR_NOOP("resource's sourceURL attribute has invalid length");
                return false;
            }
        }

        if (enResource.attributes->cameraMake.isSet())
        {
            int32_t cameraMakeSize = static_cast<int32_t>(enResource.attributes->cameraMake->size());
            if ( (cameraMakeSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) ||
                 (cameraMakeSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("resource's cameraMake attribute has invalid length");
                return false;
            }
        }

        if (enResource.attributes->cameraModel.isSet())
        {
            int32_t cameraModelSize = static_cast<int32_t>(enResource.attributes->cameraModel->size());
            if ( (cameraModelSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) ||
                 (cameraModelSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("resource's cameraModel attribute has invalid length");
                return false;
            }
        }
    }

    return true;
}

QString IResource::displayName() const
{
    const qevercloud::Resource & enResource = GetEnResource();
    if (enResource.attributes.isSet())
    {
        if (enResource.attributes->fileName.isSet()) {
            return enResource.attributes->fileName.ref();
        }
        else if (enResource.attributes->sourceURL.isSet()) {
            return enResource.attributes->sourceURL.ref();
        }
    }

    return QString();
}

void IResource::setDisplayName(const QString & displayName)
{
    qevercloud::Resource & enResource = GetEnResource();
    if (!enResource.attributes.isSet()) {
        enResource.attributes = qevercloud::ResourceAttributes();
    }

    enResource.attributes->fileName = displayName;
}

QString IResource::preferredFileSuffix() const
{
    const qevercloud::Resource & enResource = GetEnResource();
    if (enResource.attributes.isSet() && enResource.attributes->fileName.isSet())
    {
        QFileInfo fileInfo(enResource.attributes->fileName.ref());
        QString completeSuffix = fileInfo.completeSuffix();
        if (!completeSuffix.isEmpty()) {
            return completeSuffix;
        }
    }

    if (enResource.mime.isSet()) {
        QMimeDatabase mimeDatabase;
        QMimeType mimeType = mimeDatabase.mimeTypeForName(enResource.mime.ref());
        return mimeType.preferredSuffix();
    }

    return QString();
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
    if (!guid.isEmpty()) {
        GetEnResource().noteGuid = guid;
    }
    else {
        GetEnResource().noteGuid.clear();
    }
}

bool IResource::hasNoteLocalUid() const
{
    return m_noteLocalUid.isSet();
}

const QString & IResource::noteLocalUid() const
{
    return m_noteLocalUid;
}

void IResource::setNoteLocalUid(const QString & guid)
{
    if (!guid.isEmpty()) {
        m_noteLocalUid = guid;
    }
    else {
        m_noteLocalUid.clear();
    }
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
    QNTRACE(QStringLiteral("IResource::setDataBody: body to set is ") << (body.isEmpty() ? QStringLiteral("empty") : QStringLiteral("not empty")));

    qevercloud::Resource & enResource = GetEnResource();

    if (!enResource.data.isSet())
    {
        if (body.isEmpty()) {
            QNTRACE(QStringLiteral("Body to set is empty and resource's data is not set as well"));
            return;
        }

        enResource.data = qevercloud::Data();
    }

    if (body.isEmpty())
    {
        enResource.data->body.clear();
        QNTRACE(QStringLiteral("Cleared data body"));

        if (!enResource.data->bodyHash.isSet() && !enResource.data->size.isSet()) {
            enResource.data.clear();
            QNTRACE(QStringLiteral("Cleared the entire data field"));
        }
    }
    else
    {
        enResource.data->body = body;
        QNTRACE(QStringLiteral("Set resource data body"));
    }
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
    INoteStoreDataElement(other),
    d(other.d),
    m_indexInNote(other.indexInNote()),
    m_noteLocalUid(other.m_noteLocalUid)
{}

IResource & IResource::operator=(const IResource & other)
{
    if (this != &other)
    {
        d = other.d;
        setIndexInNote(other.m_indexInNote);
        setNoteLocalUid(other.m_noteLocalUid.isSet() ? other.m_noteLocalUid.ref() : QString());
    }

    return *this;
}

QTextStream & IResource::print(QTextStream & strm) const
{
    const qevercloud::Resource & enResource = GetEnResource();

    strm << QStringLiteral("Resource: { \n");

    QString indent = QStringLiteral("  ");

    const QString localUid_ = localUid();
    if (!localUid_.isEmpty()) {
        strm << indent << QStringLiteral("local uid = ") << localUid_ << QStringLiteral("; \n");
    }
    else {
        strm << indent << QStringLiteral("localUid is empty; \n");
    }

    strm << indent << QStringLiteral("isDirty = ") << (isDirty() ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral("; \n");
    strm << indent << QStringLiteral("indexInNote = ") << QString::number(m_indexInNote) << QStringLiteral("; \n");
    strm << indent << QStringLiteral("note local uid = ") << (m_noteLocalUid.isSet() ? m_noteLocalUid.ref() : QStringLiteral("<not set>"))
         << QStringLiteral("; \n");

    strm << indent << enResource;

    strm << QStringLiteral("}; \n");

    return strm;
}

} // namespace quentier
