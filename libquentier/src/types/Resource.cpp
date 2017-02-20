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

#include "data/ResourceData.h"
#include <quentier/types/Resource.h>
#include <quentier/utility/Utility.h>
#include <quentier/logging/QuentierLogger.h>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

namespace quentier {

QN_DEFINE_LOCAL_UID(Resource)
QN_DEFINE_DIRTY(Resource)
QN_DEFINE_LOCAL(Resource)

Resource::Resource() :
    INoteStoreDataElement(),
    d(new ResourceData)
{}

Resource::Resource(const Resource & other) :
    INoteStoreDataElement(other),
    d(other.d)
{}

Resource::Resource(Resource && other) :
    INoteStoreDataElement(std::move(other)),
    d(std::move(other.d))
{}

Resource::Resource(const qevercloud::Resource & resource) :
    INoteStoreDataElement(),
    d(new ResourceData(resource))
{}

Resource & Resource::operator=(const Resource & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

Resource & Resource::operator=(Resource && other)
{
    if (this != &other) {
        d = std::move(other.d);
    }

    return *this;
}

Resource::~Resource()
{}

bool Resource::operator==(const Resource & other) const
{
    return (d->m_qecResource == other.d->m_qecResource) &&
           (isDirty() == other.isDirty()) &&
           (d->m_noteLocalUid.isEqual(other.d->m_noteLocalUid)) &&
           (localUid() == other.localUid());

    // NOTE: d->m_indexInNote does not take any part in comparison
    // as it is by nature a helper parameter intended to preserve sorting of resources
    // in note between insertions and selections from the database
}

bool Resource::operator!=(const Resource & other) const
{
    return !(*this == other);
}

Resource::operator const qevercloud::Resource &() const
{
    return d->m_qecResource;
}

Resource::operator qevercloud::Resource &()
{
    return d->m_qecResource;
}

void Resource::clear()
{
    setDirty(true);
    d->m_qecResource = qevercloud::Resource();
    d->m_indexInNote = -1;
    d->m_noteLocalUid.clear();
}

bool Resource::hasGuid() const
{
    return d->m_qecResource.guid.isSet();
}

const QString & Resource::guid() const
{
    return d->m_qecResource.guid;
}

void Resource::setGuid(const QString & guid)
{
    if (!guid.isEmpty()) {
        d->m_qecResource.guid = guid;
    }
    else {
        d->m_qecResource.guid.clear();
    }
}

bool Resource::hasUpdateSequenceNumber() const
{
    return d->m_qecResource.updateSequenceNum.isSet();
}

qint32 Resource::updateSequenceNumber() const
{
    return d->m_qecResource.updateSequenceNum;
}

void Resource::setUpdateSequenceNumber(const qint32 updateSequenceNumber)
{
    d->m_qecResource.updateSequenceNum = updateSequenceNumber;
}

bool Resource::checkParameters(ErrorString & errorDescription) const
{
    const qevercloud::Resource & enResource = d->m_qecResource;

    if (localUid().isEmpty() && !enResource.guid.isSet()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Both resource's local and remote guids are empty");
        return false;
    }

    if (enResource.guid.isSet() && !checkGuid(enResource.guid.ref())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's guid is invalid");
        errorDescription.details() = enResource.guid.ref();
        return false;
    }

    if (enResource.updateSequenceNum.isSet() && !checkUpdateSequenceNumber(enResource.updateSequenceNum)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's update sequence number is invalid");
        errorDescription.details() = QString::number(enResource.updateSequenceNum.ref());
        return false;
    }

    if (enResource.noteGuid.isSet() && !checkGuid(enResource.noteGuid.ref())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's note guid is invalid");
        errorDescription.details() = enResource.noteGuid.ref();
        return false;
    }

    if (enResource.data.isSet())
    {
        if (!enResource.data->body.isSet()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's data body is not set");
            return false;
        }

        if (!enResource.data->size.isSet()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's data size is not set");
            return false;
        }

        if (!enResource.data->bodyHash.isSet())
        {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's data hash is not set");
            return false;
        }
        else
        {
            qint32 hashSize = static_cast<qint32>(enResource.data->bodyHash->size());
            if (hashSize != qevercloud::EDAM_HASH_LEN) {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's data hash has invalid size");
                errorDescription.details() = enResource.data->bodyHash.ref();
                return false;
            }
        }
    }

    if (enResource.recognition.isSet())
    {
        if (!enResource.recognition->body.isSet()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's recognition data body is not set");
            return false;
        }

        if (!enResource.recognition->size.isSet()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's recognition data size is not set");
            return false;
        }

        if (!enResource.recognition->bodyHash.isSet())
        {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's recognition data hash is not set");
            return false;
        }
        else
        {
            qint32 hashSize = static_cast<qint32>(enResource.recognition->bodyHash->size());
            if (hashSize != qevercloud::EDAM_HASH_LEN) {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's recognition data hash has invalid size");
                errorDescription.details() = enResource.recognition->bodyHash.ref();
                return false;
            }
        }
    }

    if (enResource.alternateData.isSet())
    {
        if (!enResource.alternateData->body.isSet()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's alternate data body is not set");
            return false;
        }

        if (!enResource.alternateData->size.isSet()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's alternate data size is not set");
            return false;
        }

        if (!enResource.alternateData->bodyHash.isSet())
        {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's alternate data hash is not set");
            return false;
        }
        else
        {
            qint32 hashSize = static_cast<qint32>(enResource.alternateData->bodyHash->size());
            if (hashSize != qevercloud::EDAM_HASH_LEN) {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's alternate data hash has invalid size");
                errorDescription.details() = enResource.alternateData->bodyHash.ref();
                return false;
            }
        }
    }

    if (enResource.mime.isSet())
    {
        int32_t mimeSize = static_cast<int32_t>(enResource.mime->size());
        if ( (mimeSize < qevercloud::EDAM_MIME_LEN_MIN) ||
             (mimeSize > qevercloud::EDAM_MIME_LEN_MAX) )
        {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's mime type has invalid length");
            errorDescription.details() = enResource.mime.ref();
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
                errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's sourceURL attribute has invalid length");
                errorDescription.details() = enResource.attributes->sourceURL.ref();
                return false;
            }
        }

        if (enResource.attributes->cameraMake.isSet())
        {
            int32_t cameraMakeSize = static_cast<int32_t>(enResource.attributes->cameraMake->size());
            if ( (cameraMakeSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) ||
                 (cameraMakeSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's cameraMake attribute has invalid length");
                errorDescription.details() = enResource.attributes->cameraMake.ref();
                return false;
            }
        }

        if (enResource.attributes->cameraModel.isSet())
        {
            int32_t cameraModelSize = static_cast<int32_t>(enResource.attributes->cameraModel->size());
            if ( (cameraModelSize < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) ||
                 (cameraModelSize > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) )
            {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "Resource's cameraModel attribute has invalid length");
                errorDescription.details() = enResource.attributes->cameraModel.ref();
                return false;
            }
        }
    }

    return true;
}

QString Resource::displayName() const
{
    const qevercloud::Resource & enResource = d->m_qecResource;
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

void Resource::setDisplayName(const QString & displayName)
{
    qevercloud::Resource & enResource = d->m_qecResource;
    if (!enResource.attributes.isSet()) {
        enResource.attributes = qevercloud::ResourceAttributes();
    }

    enResource.attributes->fileName = displayName;
}

QString Resource::preferredFileSuffix() const
{
    const qevercloud::Resource & enResource = d->m_qecResource;
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

int Resource::indexInNote() const
{
    return d->m_indexInNote;
}

void Resource::setIndexInNote(const int index)
{
    d->m_indexInNote = index;
}

bool Resource::hasNoteGuid() const
{
    return d->m_qecResource.noteGuid.isSet();
}

const QString & Resource::noteGuid() const
{
    return d->m_qecResource.noteGuid;
}

void Resource::setNoteGuid(const QString & guid)
{
    if (!guid.isEmpty()) {
        d->m_qecResource.noteGuid = guid;
    }
    else {
        d->m_qecResource.noteGuid.clear();
    }
}

bool Resource::hasNoteLocalUid() const
{
    return d->m_noteLocalUid.isSet();
}

const QString & Resource::noteLocalUid() const
{
    return d->m_noteLocalUid.ref();
}

void Resource::setNoteLocalUid(const QString & guid)
{
    if (!guid.isEmpty()) {
        d->m_noteLocalUid = guid;
    }
    else {
        d->m_noteLocalUid.clear();
    }
}

bool Resource::hasData() const
{
    return d->m_qecResource.data.isSet();
}

bool Resource::hasDataHash() const
{
    if (!hasData()) {
        return false;
    }

    return d->m_qecResource.data->bodyHash.isSet();
}

const QByteArray & Resource::dataHash() const
{
    return d->m_qecResource.data->bodyHash;
}

void Resource::setDataHash(const QByteArray & hash)
{
    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.data.isSet())
    {
        if (hash.isEmpty()) {
            return;
        }

        enResource.data = qevercloud::Data();
    }

    if (hash.isEmpty())
    {
        enResource.data->bodyHash.clear();

        if (!enResource.data->body.isSet() && !enResource.data->size.isSet()) {
            enResource.data.clear();
        }

        return;
    }

    enResource.data->bodyHash = hash;
}

bool Resource::hasDataSize() const
{
    if (!hasData()) {
        return false;
    }

    return d->m_qecResource.data->size.isSet();
}

qint32 Resource::dataSize() const
{
    return d->m_qecResource.data->size;
}

void Resource::setDataSize(const qint32 size)
{
    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.data.isSet())
    {
        if (size == 0) {
            return;
        }

        enResource.data = qevercloud::Data();
    }

    if (size == 0)
    {
        enResource.data->size.clear();

        if (!enResource.data->body.isSet() && !enResource.data->bodyHash.isSet()) {
            enResource.data.clear();
        }

        return;
    }

    enResource.data->size = size;
}

bool Resource::hasDataBody() const
{
    if (!hasData()) {
        return false;
    }

    return d->m_qecResource.data->body.isSet();
}

const QByteArray & Resource::dataBody() const
{
    return d->m_qecResource.data->body;
}

void Resource::setDataBody(const QByteArray & body)
{
    QNTRACE(QStringLiteral("Resource::setDataBody: body to set is ")
            << (body.isEmpty() ? QStringLiteral("empty") : QStringLiteral("not empty")));

    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.data.isSet())
    {
        if (body.isEmpty()) {
            return;
        }

        enResource.data = qevercloud::Data();
    }

    if (body.isEmpty())
    {
        enResource.data->body.clear();

        if (!enResource.data->bodyHash.isSet() && !enResource.data->size.isSet()) {
            enResource.data.clear();
        }

        return;
    }

    enResource.data->body = body;
}

bool Resource::hasMime() const
{
    return d->m_qecResource.mime.isSet();
}

const QString & Resource::mime() const
{
    return d->m_qecResource.mime;
}

void Resource::setMime(const QString & mime)
{
    d->m_qecResource.mime = mime;
}

bool Resource::hasWidth() const
{
    return d->m_qecResource.width.isSet();
}

qint16 Resource::width() const
{
    return d->m_qecResource.width;
}

void Resource::setWidth(const qint16 width)
{
    if (width < 0) {
        d->m_qecResource.width.clear();
    }
    else {
        d->m_qecResource.width = width;
    }
}

bool Resource::hasHeight() const
{
    return d->m_qecResource.height.isSet();
}

qint16 Resource::height() const
{
    return d->m_qecResource.height;
}

void Resource::setHeight(const qint16 height)
{
    if (height < 0) {
        d->m_qecResource.height.clear();
    }
    else {
        d->m_qecResource.height = height;
    }
}

bool Resource::hasRecognitionData() const
{
    return d->m_qecResource.recognition.isSet();
}

bool Resource::hasRecognitionDataHash() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return d->m_qecResource.recognition->bodyHash.isSet();
}

const QByteArray & Resource::recognitionDataHash() const
{
    return d->m_qecResource.recognition->bodyHash;
}

void Resource::setRecognitionDataHash(const QByteArray & hash)
{
    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.recognition.isSet())
    {
        if (hash.isEmpty()) {
            return;
        }

        enResource.recognition = qevercloud::Data();
    }

    if (hash.isEmpty())
    {
        enResource.recognition->bodyHash.clear();

        if (!enResource.recognition->body.isSet() && !enResource.recognition->size.isSet()) {
            enResource.recognition.clear();
        }

        return;
    }

    enResource.recognition->bodyHash = hash;
}

bool Resource::hasRecognitionDataSize() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return d->m_qecResource.recognition->size.isSet();
}

qint32 Resource::recognitionDataSize() const
{
    return d->m_qecResource.recognition->size;
}

void Resource::setRecognitionDataSize(const qint32 size)
{
    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.recognition.isSet())
    {
        if (size == 0) {
            return;
        }

        enResource.recognition = qevercloud::Data();
    }

    if (size == 0)
    {
        enResource.recognition->size.clear();

        if (!enResource.recognition->body.isSet() && !enResource.recognition->bodyHash.isSet()) {
            enResource.recognition.clear();
        }

        return;
    }

    enResource.recognition->size = size;
}

bool Resource::hasRecognitionDataBody() const
{
    if (!hasRecognitionData()) {
        return false;
    }

    return d->m_qecResource.recognition->body.isSet();
}

const QByteArray & Resource::recognitionDataBody() const
{
    return d->m_qecResource.recognition->body;
}

void Resource::setRecognitionDataBody(const QByteArray & body)
{
    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.recognition.isSet())
    {
        if (body.isEmpty()) {
            return;
        }

        enResource.recognition = qevercloud::Data();
    }

    if (body.isEmpty())
    {
        enResource.recognition->body.clear();

        if (!enResource.recognition->bodyHash.isSet() && !enResource.recognition->size.isSet()) {
            enResource.recognition.clear();
        }

        return;
    }

    enResource.recognition->body = body;
}

bool Resource::hasAlternateData() const
{
    return d->m_qecResource.alternateData.isSet();
}

bool Resource::hasAlternateDataHash() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return d->m_qecResource.alternateData->bodyHash.isSet();
}

const QByteArray & Resource::alternateDataHash() const
{
    return d->m_qecResource.alternateData->bodyHash;
}

void Resource::setAlternateDataHash(const QByteArray & hash)
{
    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.alternateData.isSet())
    {
        if (hash.isEmpty()) {
            return;
        }

        enResource.alternateData = qevercloud::Data();
    }

    if (hash.isEmpty())
    {
        enResource.alternateData->bodyHash.clear();

        if (!enResource.alternateData->body.isSet() && !enResource.alternateData->size.isSet()) {
            enResource.alternateData.clear();
        }

        return;
    }

    enResource.alternateData->bodyHash = hash;
}

bool Resource::hasAlternateDataSize() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return d->m_qecResource.alternateData->size.isSet();
}

qint32 Resource::alternateDataSize() const
{
    return d->m_qecResource.alternateData->size;
}

void Resource::setAlternateDataSize(const qint32 size)
{
    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.alternateData.isSet())
    {
        if (size == 0) {
            return;
        }

        enResource.alternateData = qevercloud::Data();
    }

    if (size == 0)
    {
        enResource.alternateData->size.clear();

        if (!enResource.alternateData->body.isSet() && !enResource.alternateData->bodyHash.isSet()) {
            enResource.alternateData.clear();
        }

        return;
    }

    enResource.alternateData->size = size;
}

bool Resource::hasAlternateDataBody() const
{
    if (!hasAlternateData()) {
        return false;
    }

    return d->m_qecResource.alternateData->body.isSet();
}

const QByteArray & Resource::alternateDataBody() const
{
    return d->m_qecResource.alternateData->body;
}

void Resource::setAlternateDataBody(const QByteArray & body)
{
    qevercloud::Resource & enResource = d->m_qecResource;

    if (!enResource.alternateData.isSet())
    {
        if (body.isEmpty()) {
            return;
        }

        enResource.alternateData = qevercloud::Data();
    }

    if (body.isEmpty())
    {
        enResource.alternateData->body.clear();

        if (!enResource.alternateData->bodyHash.isSet() && !enResource.alternateData->size.isSet()) {
            enResource.alternateData.clear();
        }

        return;
    }

    enResource.alternateData->body = body;
}

bool Resource::hasResourceAttributes() const
{
    return d->m_qecResource.attributes.isSet();
}

const qevercloud::ResourceAttributes & Resource::resourceAttributes() const
{
    return d->m_qecResource.attributes;
}

qevercloud::ResourceAttributes & Resource::resourceAttributes()
{
    qevercloud::Resource & enResource = d->m_qecResource;
    if (!enResource.attributes.isSet()) {
        enResource.attributes = qevercloud::ResourceAttributes();
    }

    return enResource.attributes;
}

void Resource::setResourceAttributes(const qevercloud::ResourceAttributes & attributes)
{
    d->m_qecResource.attributes = attributes;
}

void Resource::setResourceAttributes(qevercloud::ResourceAttributes && attributes)
{
    d->m_qecResource.attributes = std::move(attributes);
}

QTextStream & Resource::print(QTextStream & strm) const
{
    const qevercloud::Resource & enResource = d->m_qecResource;

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
    strm << indent << QStringLiteral("indexInNote = ") << QString::number(d->m_indexInNote) << QStringLiteral("; \n");
    strm << indent << QStringLiteral("note local uid = ") << (d->m_noteLocalUid.isSet() ? d->m_noteLocalUid.ref() : QStringLiteral("<not set>"))
         << QStringLiteral("; \n");

    strm << indent << enResource;

    strm << QStringLiteral("}; \n");

    return strm;
}

} // namespace quentier
