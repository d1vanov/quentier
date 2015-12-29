#include "ImageResourceRotationDelegate.h"
#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/types/ResourceAdapter.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/DesktopServices.h>
#include <QDateTime>
#include <QBuffer>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't rotate image: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

ImageResourceRotationDelegate::ImageResourceRotationDelegate(const QString & resourceHashBefore, const INoteEditorBackend::Rotation::type rotationDirection,
                                                             NoteEditorPrivate & noteEditor, ResourceInfo & resourceInfo,
                                                             ResourceFileStorageManager & resourceFileStorageManager,
                                                             QHash<QString, QString> & resourceFileStoragePathsByLocalGuid) :
    m_noteEditor(noteEditor),
    m_resourceInfo(resourceInfo),
    m_resourceFileStorageManager(resourceFileStorageManager),
    m_resourceFileStoragePathsByLocalGuid(resourceFileStoragePathsByLocalGuid),
    m_rotationDirection(rotationDirection),
    m_pNote(Q_NULLPTR),
    m_resourceDataBefore(),
    m_resourceHashBefore(resourceHashBefore),
    m_resourceFileStoragePathBefore(),
    m_resourceFileStoragePathAfter(),
    m_rotatedResource(),
    m_saveResourceRequestId()
{}

void ImageResourceRotationDelegate::start()
{
    QNDEBUG("ImageResourceRotationDelegate::start");

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
    QNDEBUG("ImageResourceRotationDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(ImageResourceRotationDelegate,onOriginalPageConvertedToNote,Note));
}

void ImageResourceRotationDelegate::rotateImageResource()
{
    QNDEBUG("ImageResourceRotationDelegate::rotateImageResource");

    QString errorPrefix = QT_TR_NOOP("Can't rotate image attachment by hash:") + QString(" ");

    m_pNote = m_noteEditor.GetNotePrt();
    if (Q_UNLIKELY(!m_pNote)) {
        QString error = errorPrefix + QT_TR_NOOP("no note is set to the editor");
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
            QString error = errorPrefix + QT_TR_NOOP("resources mime type is not set");
            QNWARNING(error << ", resource: " << resource);
            emit notifyError(error);
            return;
        }

        if (Q_UNLIKELY(!resource.mime().startsWith("image/"))) {
            QString error = errorPrefix + QT_TR_NOOP("resource's mime type indicates it is not an image");
            QNWARNING(error << ", resource: " << resource);
            emit notifyError(error);
            return;
        }

        targetResourceIndex = i;
        break;
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        QString error = errorPrefix + QT_TR_NOOP("can't find the resource in the note");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_rotatedResource = resources[targetResourceIndex];
    if (Q_UNLIKELY(!m_rotatedResource.hasDataBody())) {
        QString error = errorPrefix + QT_TR_NOOP("the resource doesn't have the data body set");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_resourceDataBefore = m_rotatedResource.dataBody();

    QImage resourceImage;
    bool loaded = resourceImage.loadFromData(m_rotatedResource.dataBody());
    if (Q_UNLIKELY(!loaded)) {
        QString error = errorPrefix + QT_TR_NOOP("can't load image from resource data");
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

    m_resourceFileStoragePathAfter = m_noteEditor.imageResourcesStoragePath();
    m_resourceFileStoragePathAfter += "/" + m_rotatedResource.localGuid();
    m_resourceFileStoragePathAfter += ".png";

    m_saveResourceRequestId = QUuid::createUuid();

    QObject::connect(this, QNSIGNAL(ImageResourceRotationDelegate,saveResourceToStorage,QString,QByteArray,QByteArray,QString,QUuid),
                     &m_resourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QByteArray,QByteArray,QString,QUuid));
    QObject::connect(&m_resourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString),
                     this, QNSLOT(ImageResourceRotationDelegate,onResourceSavedToStorage,QUuid,QByteArray,QString,int,QString));

    emit saveResourceToStorage(m_rotatedResource.localGuid(), m_rotatedResource.dataBody(), QByteArray(),
                               m_resourceFileStoragePathAfter, m_saveResourceRequestId);
}

void ImageResourceRotationDelegate::onResourceSavedToStorage(QUuid requestId, QByteArray dataHash, QString fileStoragePath,
                                                             int errorCode, QString errorDescription)
{
    if (requestId != m_saveResourceRequestId) {
        return;
    }

    QNDEBUG("ImageResourceRotationDelegate::onResourceSavedToStorage: hash = " << dataHash << ", file storage path = " << fileStoragePath
            << ", error code = " << errorCode << ", error description = " << errorDescription);

    if (Q_UNLIKELY(errorCode != 0)) {
        errorDescription.prepend(QT_TR_NOOP("Can't rotate image resource: can't write modified resource data to local file: "));
        emit notifyError(errorDescription);
        QNWARNING(errorDescription + ", error code = " << errorCode);
        return;
    }

    Note * pNote = m_noteEditor.GetNotePrt();
    if (Q_UNLIKELY(pNote != m_pNote)) {
        errorDescription = QT_TR_NOOP("Can't rotate image resource: note was changed during the processing of image rotation");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    const QString localGuid = m_rotatedResource.localGuid();
    m_noteEditor.cleanupStaleImageResourceFiles(localGuid);

    QFile rotatedImageResourceFile(fileStoragePath);
    QString linkFileName = fileStoragePath;
    linkFileName.remove(linkFileName.size() - 4, 4);
    linkFileName += "_";
    linkFileName += QString::number(QDateTime::currentMSecsSinceEpoch());
    linkFileName += ".png";

    bool res = rotatedImageResourceFile.link(linkFileName);
    if (Q_UNLIKELY(!res)) {
        errorDescription = QT_TR_NOOP("Can't rotate image resource: can't create a link to the resource file to use within the note editor");
        errorDescription += rotatedImageResourceFile.errorString();
        errorDescription += ", ";
        errorDescription += QT_TR_NOOP("error code = ");
        errorDescription += QString::number(rotatedImageResourceFile.error());
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QNTRACE("Created a link to the original file (" << m_resourceFileStoragePathAfter << "): " << linkFileName);

    m_resourceFileStoragePathAfter = linkFileName;

    auto resourceFileStoragePathIt = m_resourceFileStoragePathsByLocalGuid.find(localGuid);
    if (Q_UNLIKELY(resourceFileStoragePathIt == m_resourceFileStoragePathsByLocalGuid.end())) {
        errorDescription = QT_TR_NOOP("Can't rotate image resource: can't find the path to resource file before the rotation");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    m_resourceFileStoragePathBefore = resourceFileStoragePathIt.value();
    resourceFileStoragePathIt.value() = linkFileName;

    QString resourceDisplayName = m_rotatedResource.displayName();
    QString resourceDisplaySize = humanReadableSize(m_rotatedResource.dataSize());

    QString dataHashStr = QString::fromLocal8Bit(dataHash);
    m_rotatedResource.setDataHash(dataHash);

    m_pNote->updateResource(m_rotatedResource);

    m_resourceInfo.removeResourceInfo(m_resourceHashBefore);
    m_resourceInfo.cacheResourceInfo(dataHashStr, resourceDisplayName,
                                     resourceDisplaySize, linkFileName);

    if (m_resourceFileStoragePathBefore != fileStoragePath)
    {
        QFile oldResourceFile(m_resourceFileStoragePathBefore);
        if (Q_UNLIKELY(!oldResourceFile.remove())) {
            QNINFO("Can't remove stale resource file " + m_resourceFileStoragePathBefore);
        }
    }

    QString javascript = "updateResourceHash('" + m_resourceHashBefore + "', '" + dataHashStr + "');";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &ImageResourceRotationDelegate::onResourceTagHashUpdated));
}

void ImageResourceRotationDelegate::onResourceTagHashUpdated(const QVariant & data)
{
    QNDEBUG("ImageResourceRotationDelegate::onResourceTagHashUpdated");

    Q_UNUSED(data)

    QString javascript = "updateImageResourceSrc('" + m_rotatedResource.dataHash() + "', '" + m_resourceFileStoragePathAfter + "');";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &ImageResourceRotationDelegate::onResourceTagSrcUpdated));
}

void ImageResourceRotationDelegate::onResourceTagSrcUpdated(const QVariant & data)
{
    QNDEBUG("ImageResourceRotationDelegate::onResourceTagSrcUpdated");

    Q_UNUSED(data)

    emit finished(m_resourceDataBefore, m_resourceHashBefore, m_rotatedResource, m_rotationDirection);
}

} // namespace qute_note
