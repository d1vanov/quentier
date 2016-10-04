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

#include "ImageResourceRotationDelegate.h"
#include <quentier/note_editor/ResourceFileStorageManager.h>
#include <quentier/types/Note.h>
#include <quentier/types/ResourceAdapter.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/DesktopServices.h>
#include <QDateTime>
#include <QBuffer>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't rotate the image attachment: no note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

ImageResourceRotationDelegate::ImageResourceRotationDelegate(const QByteArray & resourceHashBefore, const INoteEditorBackend::Rotation::type rotationDirection,
                                                             NoteEditorPrivate & noteEditor, ResourceInfo & resourceInfo,
                                                             ResourceFileStorageManager & resourceFileStorageManager,
                                                             QHash<QString, QString> & resourceFileStoragePathsByLocalUid) :
    m_noteEditor(noteEditor),
    m_resourceInfo(resourceInfo),
    m_resourceFileStorageManager(resourceFileStorageManager),
    m_resourceFileStoragePathsByLocalUid(resourceFileStoragePathsByLocalUid),
    m_rotationDirection(rotationDirection),
    m_pNote(Q_NULLPTR),
    m_resourceDataBefore(),
    m_resourceHashBefore(resourceHashBefore),
    m_resourceRecognitionDataBefore(),
    m_resourceRecognitionDataHashBefore(),
    m_resourceFileStoragePathBefore(),
    m_resourceFileStoragePathAfter(),
    m_rotatedResource(),
    m_saveResourceRequestId()
{}

void ImageResourceRotationDelegate::start()
{
    QNDEBUG(QStringLiteral("ImageResourceRotationDelegate::start"));

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(ImageResourceRotationDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        rotateImageResource();
    }
}

void ImageResourceRotationDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG(QStringLiteral("ImageResourceRotationDelegate::onOriginalPageConvertedToNote"));

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(ImageResourceRotationDelegate,onOriginalPageConvertedToNote,Note));

    rotateImageResource();
}

void ImageResourceRotationDelegate::rotateImageResource()
{
    QNDEBUG(QStringLiteral("ImageResourceRotationDelegate::rotateImageResource"));

    QNLocalizedString error = QT_TR_NOOP("can't rotate the image attachment");

    m_pNote = m_noteEditor.notePtr();
    if (Q_UNLIKELY(!m_pNote)) {
        error += QStringLiteral(": ");
        error += QT_TR_NOOP("no note is set to the editor");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int targetResourceIndex = -1;
    QList<ResourceAdapter> resources = m_pNote->resourceAdapters();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resource = resources[i];
        if (!resource.hasDataHash() || (resource.dataHash() != m_resourceHashBefore)) {
            continue;
        }

        if (Q_UNLIKELY(!resource.hasMime())) {
            error += QStringLiteral(": ");
            error += QT_TR_NOOP("the mime type is missing");;
            QNWARNING(error << QStringLiteral(", resource: ") << resource);
            emit notifyError(error);
            return;
        }

        if (Q_UNLIKELY(!resource.mime().startsWith(QStringLiteral("image/")))) {
            error += QStringLiteral(": ");
            error += QT_TR_NOOP("the mime type indicates the attachment is not an image");
            QNWARNING(error << QStringLiteral(", resource: ") << resource);
            emit notifyError(error);
            return;
        }

        targetResourceIndex = i;
        break;
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        error += QStringLiteral(": ");
        error += QT_TR_NOOP("can't find the attachment within the note");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_rotatedResource = resources[targetResourceIndex];
    if (Q_UNLIKELY(!m_rotatedResource.hasDataBody())) {
        error += QStringLiteral(": ");
        error += QT_TR_NOOP("the data body is missing");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_resourceDataBefore = m_rotatedResource.dataBody();

    if (m_rotatedResource.hasRecognitionDataBody()) {
        m_resourceRecognitionDataBefore = m_rotatedResource.recognitionDataBody();
    }

    if (m_rotatedResource.hasRecognitionDataHash()) {
        m_resourceRecognitionDataHashBefore = m_rotatedResource.recognitionDataHash();
    }

    QImage resourceImage;
    bool loaded = resourceImage.loadFromData(m_rotatedResource.dataBody());
    if (Q_UNLIKELY(!loaded)) {
        error += QStringLiteral(": ");
        error += QT_TR_NOOP("can't load image from resource data");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    qreal angle = ((m_rotationDirection == INoteEditorBackend::Rotation::Clockwise) ? 90.0 : -90.0);
    QTransform transform;
    transform.rotate(angle);
    resourceImage = resourceImage.transformed(transform);

    QByteArray rotatedResourceData;
    QBuffer rotatedResourceDataBuffer(&rotatedResourceData);
    rotatedResourceDataBuffer.open(QIODevice::WriteOnly);
    resourceImage.save(&rotatedResourceDataBuffer, "PNG");

    m_rotatedResource.setDataBody(rotatedResourceData);
    m_rotatedResource.setDataSize(rotatedResourceData.size());
    m_rotatedResource.setDataHash(QByteArray());

    // Need to destroy the recognition data (if any) because it would no longer correspond to the rotated image
    m_rotatedResource.setRecognitionDataBody(QByteArray());
    m_rotatedResource.setRecognitionDataSize(0);
    m_rotatedResource.setRecognitionDataHash(QByteArray());

    m_saveResourceRequestId = QUuid::createUuid();

    QObject::connect(this, QNSIGNAL(ImageResourceRotationDelegate,saveResourceToStorage,QString,QString,QByteArray,QByteArray,QString,QUuid,bool),
                     &m_resourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QString,QByteArray,QByteArray,QString,QUuid,bool));
    QObject::connect(&m_resourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QNLocalizedString),
                     this, QNSLOT(ImageResourceRotationDelegate,onResourceSavedToStorage,QUuid,QByteArray,QString,int,QNLocalizedString));

    emit saveResourceToStorage(m_rotatedResource.noteLocalUid(), m_rotatedResource.localUid(), m_rotatedResource.dataBody(), QByteArray(),
                               QStringLiteral("png"), m_saveResourceRequestId, /* is image = */ true);
}

void ImageResourceRotationDelegate::onResourceSavedToStorage(QUuid requestId, QByteArray dataHash, QString fileStoragePath,
                                                             int errorCode, QNLocalizedString errorDescription)
{
    if (requestId != m_saveResourceRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("ImageResourceRotationDelegate::onResourceSavedToStorage: hash = ") << dataHash.toHex()
            << QStringLiteral(", file storage path = ") << fileStoragePath << QStringLiteral(", error code = ")
            << errorCode << QStringLiteral(", error description = ") << errorDescription);

    if (Q_UNLIKELY(errorCode != 0)) {
        QNLocalizedString error = QT_TR_NOOP("can't rotate the image attachment: can't write modified resource data to local file");
        error += QStringLiteral(": ");
        error += errorDescription;
        emit notifyError(error);
        QNWARNING(error << QStringLiteral(", error code = ") << errorCode);
        return;
    }

    Note * pNote = m_noteEditor.notePtr();
    if (Q_UNLIKELY(pNote != m_pNote)) {
        errorDescription = QT_TR_NOOP("can't rotate the image attachment: note was changed during the processing of image rotation");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    const QString localUid = m_rotatedResource.localUid();
    m_noteEditor.removeSymlinksToImageResourceFile(localUid);

    QFile rotatedImageResourceFile(fileStoragePath);
    QString linkFileName = fileStoragePath;
    linkFileName.remove(linkFileName.size() - 4, 4);
    linkFileName += QStringLiteral("_");
    linkFileName += QString::number(QDateTime::currentMSecsSinceEpoch());

#ifdef Q_OS_WIN
    linkFileName += QStringLiteral(".lnk");
#else
    linkFileName += QStringLiteral(".png");
#endif

    bool res = rotatedImageResourceFile.link(linkFileName);
    if (Q_UNLIKELY(!res)) {
        errorDescription = QT_TR_NOOP("can't rotate the image attachment: can't create a link to the resource file "
                                      "to use within the note editor");
        errorDescription += QStringLiteral(": ");
        errorDescription += rotatedImageResourceFile.errorString();
        errorDescription += QStringLiteral(", ");
        errorDescription += QT_TR_NOOP("error code");
        errorDescription += QStringLiteral(": ");
        errorDescription += QString::number(rotatedImageResourceFile.error());
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QNTRACE(QStringLiteral("Created a link to the original file (") << m_resourceFileStoragePathAfter
            << QStringLiteral("): ") << linkFileName);

    m_resourceFileStoragePathAfter = linkFileName;

    auto resourceFileStoragePathIt = m_resourceFileStoragePathsByLocalUid.find(localUid);
    if (Q_UNLIKELY(resourceFileStoragePathIt == m_resourceFileStoragePathsByLocalUid.end())) {
        errorDescription = QT_TR_NOOP("can't rotate the image attachment: can't find path to the attachment file before the rotation");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    m_resourceFileStoragePathBefore = resourceFileStoragePathIt.value();
    resourceFileStoragePathIt.value() = linkFileName;

    QString resourceDisplayName = m_rotatedResource.displayName();
    QString resourceDisplaySize = humanReadableSize(static_cast<quint64>(m_rotatedResource.dataSize()));

    m_rotatedResource.setDataHash(dataHash);

    m_pNote->updateResource(m_rotatedResource);

    m_resourceInfo.removeResourceInfo(m_resourceHashBefore);
    m_resourceInfo.cacheResourceInfo(dataHash, resourceDisplayName,
                                     resourceDisplaySize, linkFileName);

    if (m_resourceFileStoragePathBefore != fileStoragePath)
    {
        QFile oldResourceFile(m_resourceFileStoragePathBefore);
        if (Q_UNLIKELY(!oldResourceFile.remove()))
        {
#ifdef Q_OS_WIN
            if (m_resourceFileStoragePathBefore.endsWith(QStringLiteral(".lnk"))) {
                // NOTE: there appears to be a bug in Qt for Windows, QFile::remove returns false
                // for any *.lnk files even though the files are actually getting removed
                QNTRACE(QStringLiteral("Skipping the reported failure at removing the .lnk file"));
            }
            else {
#endif

            QNINFO(QStringLiteral("Can't remove stale resource file ") + m_resourceFileStoragePathBefore);

#ifdef Q_OS_WIN
            }
#endif
        }
    }

    QString javascript = QStringLiteral("updateResourceHash('") + QString::fromLocal8Bit(m_resourceHashBefore.toHex()) +
                         QStringLiteral("', '") + QString::fromLocal8Bit(dataHash.toHex()) + QStringLiteral("');");

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &ImageResourceRotationDelegate::onResourceTagHashUpdated));
}

void ImageResourceRotationDelegate::onResourceTagHashUpdated(const QVariant & data)
{
    QNDEBUG(QStringLiteral("ImageResourceRotationDelegate::onResourceTagHashUpdated"));

    Q_UNUSED(data)

    QString javascript = QStringLiteral("updateImageResourceSrc('") + m_rotatedResource.dataHash().toHex() +
                         QStringLiteral("', '") + m_resourceFileStoragePathAfter + QStringLiteral("');");

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &ImageResourceRotationDelegate::onResourceTagSrcUpdated));
}

void ImageResourceRotationDelegate::onResourceTagSrcUpdated(const QVariant & data)
{
    QNDEBUG(QStringLiteral("ImageResourceRotationDelegate::onResourceTagSrcUpdated"));

    Q_UNUSED(data)

    emit finished(m_resourceDataBefore, m_resourceHashBefore, m_resourceRecognitionDataBefore,
                  m_resourceRecognitionDataHashBefore, m_rotatedResource, m_rotationDirection);
}

} // namespace quentier
