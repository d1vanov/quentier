#include "AddAttachmentDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/types/ResourceWrapper.h>

#ifdef USE_QT_WEB_ENGINE
#include "../GenericResourceImageWriter.h"
#include <QImage>
#include <QBuffer>
#endif

#include <QFileInfo>
#include <QMimeDatabase>

namespace qute_note {

AddAttachmentDelegate::AddAttachmentDelegate(const QString & filePath, NoteEditorPrivate & noteEditor,
                                             ResourceFileStorageManager * pResourceFileStorageManager,
                                             FileIOThreadWorker * pFileIOThreadWorker
#ifdef USE_QT_WEB_ENGINE
                                             , GenericResourceImageWriter * pGenericResourceImageWriter
#endif
                                            ) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_pResourceFileStorageManager(pResourceFileStorageManager),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
#ifdef USE_QT_WEB_ENGINE
    m_pGenericResourceImageWriter(pGenericResourceImageWriter),
    m_saveResourceImageRequestId(),
#endif
    m_filePath(filePath),
    m_resourceFileMimeType(),
    m_resource(),
    m_resourceFileStoragePath(),
    m_readResourceFileRequestId(),
    m_saveResourceToStorageRequestId()
{
}

void AddAttachmentDelegate::start()
{
    QNDEBUG("AddAttachmentDelegate::start");

    QFileInfo fileInfo(m_filePath);
    if (!fileInfo.isFile()) {
        QNINFO("Detected attempt to drop something else rather than file: " << m_filePath);
        return;
    }

    if (!fileInfo.isReadable()) {
        QNINFO("Detected attempt to drop file which is not readable: " << m_filePath);
        return;
    }

    QMimeDatabase mimeDatabase;
    m_resourceFileMimeType = mimeDatabase.mimeTypeForFile(fileInfo);

    if (!m_resourceFileMimeType.isValid()) {
        QNINFO("Detected invalid mime type for file " << m_filePath);
        return;
    }

    m_readResourceFileRequestId = QUuid::createUuid();

    QObject::connect(this, QNSIGNAL(AddAttachmentDelegate,readFileData,QString,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onReadFileRequest,QString,QUuid));
    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QString,QByteArray,QUuid),
                     this, QNSLOT(AddAttachmentDelegate,onResourceFileRead,bool,QString,QByteArray,QUuid));

    emit readFileData(m_filePath, m_readResourceFileRequestId);
}

void AddAttachmentDelegate::onResourceFileRead(bool success, QString errorDescription,
                                               QByteArray data, QUuid requestId)
{
    if (requestId != m_readResourceFileRequestId) {
        return;
    }

    QNDEBUG("AddAttachmentDelegate::onResourceFileRead: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription);

    QObject::disconnect(this, QNSIGNAL(AddAttachmentDelegate,readFileData,QString,QUuid),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onReadFileRequest,QString,QUuid));
    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QString,QByteArray,QUuid),
                        this, QNSLOT(AddAttachmentDelegate,onResourceFileRead,bool,QString,QByteArray,QUuid));

    if (Q_UNLIKELY(!success)) {
        errorDescription = QT_TR_NOOP("Can't read the contents of the attached file: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QFileInfo fileInfo(m_filePath);
    QByteArray dataHash = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    m_resource = m_noteEditor.attachResourceToNote(data, dataHash, m_resourceFileMimeType, fileInfo.fileName());
    QString resourceLocalGuid = m_resource.localGuid();
    if (Q_UNLIKELY(resourceLocalGuid.isEmpty())) {
        return;
    }

    QString resourceHtml = ENMLConverter::resourceHtml(m_resource, errorDescription);
    if (Q_UNLIKELY(resourceHtml.isEmpty())) {
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        m_noteEditor.removeResourceFromNote(m_resource);
        return;
    }

    QNTRACE("Resource html: " << resourceHtml);

    m_noteEditor.skipPushingUndoCommandOnNextContentChange();
    m_noteEditor.execJavascriptCommand("insertHtml", resourceHtml);

    if (m_resourceFileMimeType.name().startsWith("image/")) {
        m_resourceFileStoragePath = m_noteEditor.imageResourcesStoragePath();
    }
    else {
        m_resourceFileStoragePath = m_noteEditor.resourceLocalFileStoragePath();
    }

    m_resourceFileStoragePath += "/" + resourceLocalGuid;

    QString fileInfoSuffix = fileInfo.completeSuffix();
    if (!fileInfoSuffix.isEmpty())
    {
        m_resourceFileStoragePath += "." + fileInfoSuffix;
    }
    else
    {
        const QStringList suffixes = m_resourceFileMimeType.suffixes();
        if (!suffixes.isEmpty()) {
            m_resourceFileStoragePath += "." + suffixes.front();
        }
    }

    m_saveResourceToStorageRequestId = QUuid::createUuid();

    QObject::connect(this, QNSIGNAL(AddAttachmentDelegate,saveResourceToStorage,QString,QByteArray,QByteArray,QString,QUuid),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QByteArray,QByteArray,QString,QUuid));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString),
                     this, QNSLOT(AddAttachmentDelegate,onResourceSavedToStorage,QUuid,QByteArray,QString,int,QString));

    emit saveResourceToStorage(resourceLocalGuid, data, dataHash, m_resourceFileStoragePath,
                               m_saveResourceToStorageRequestId);

    QNTRACE("Emitted request to save the dropped resource to local file storage: generated local guid = "
            << resourceLocalGuid << ", data hash = " << dataHash << ", request id = "
            << m_saveResourceToStorageRequestId << ", mime type name = " << m_resourceFileMimeType.name());
}

void AddAttachmentDelegate::onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                                     QString fileStoragePath, int errorCode,
                                                     QString errorDescription)
{
    if (requestId != m_saveResourceToStorageRequestId) {
        return;
    }

    QNDEBUG("AddAttachmentDelegate::onResourceSavedToStorage: error code = " << errorCode
            << ", file storage path = " << fileStoragePath << ", error description = "
            << errorDescription);

    QObject::disconnect(this, QNSIGNAL(AddAttachmentDelegate,saveResourceToStorage,QString,QByteArray,QByteArray,QString,QUuid),
                        m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QByteArray,QByteArray,QString,QUuid));
    QObject::disconnect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString),
                        this, QNSLOT(AddAttachmentDelegate,onResourceSavedToStorage,QUuid,QByteArray,QString,int,QString));

    if (Q_UNLIKELY(errorCode != 0)) {
        errorDescription = QT_TR_NOOP("Can't write the resource to local file: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        m_noteEditor.removeResourceFromNote(m_resource);
        return;
    }

    if (!m_resource.hasDataHash()) {
        m_resource.setDataHash(dataHash);
        m_noteEditor.replaceResourceInNote(m_resource);
    }

#ifndef USE_QT_WEB_ENGINE
    emit finished(m_resource, m_resourceFileStoragePath);
#else
    if (m_resourceFileMimeType.name().startsWith("image/")) {
        QNTRACE("Done adding the image resource");
        emit finished(m_resource, m_resourceFileStoragePath, QString());
        return;
    }

    // Otherwise need to build the image for the generic resource
    QImage resourceImage = m_noteEditor.buildGenericResourceImage(m_resource);

    QByteArray resourceImageData;
    QBuffer buffer(&resourceImageData);
    Q_UNUSED(buffer.open(QIODevice::WriteOnly));
    resourceImage.save(&buffer, "PNG");

    m_saveResourceImageRequestId = QUuid::createUuid();

    QObject::connect(this, QNSIGNAL(AddAttachmentDelegate,saveGenericResourceImageToFile,QString,QByteArray,QString,QByteArray,QString,QUuid),
                     m_pGenericResourceImageWriter, QNSLOT(GenericResourceImageWriter,onGenericResourceImageWriteRequest,QString,QByteArray,QString,QByteArray,QString,QUuid));
    QObject::connect(m_pGenericResourceImageWriter, QNSIGNAL(GenericResourceImageWriter,genericResourceImageWriteReply,bool,QByteArray,QString,QString,QUuid),
                     this, QNSLOT(AddAttachmentDelegate,onGenericResourceImageSaved,bool,QByteArray,QString,QString,QUuid));

    QNDEBUG("Emitting request to write generic resource image for new resource with local guid "
            << m_resource.localGuid() << ", request id " << m_saveResourceImageRequestId);
    emit saveGenericResourceImageToFile(m_resource.localGuid(), resourceImageData, "PNG", dataHash,
                                        m_resourceFileStoragePath, m_saveResourceImageRequestId);
#endif
}

#ifdef USE_QT_WEB_ENGINE
void AddAttachmentDelegate::onGenericResourceImageSaved(bool success, QByteArray resourceImageDataHash,
                                                        QString filePath, QString errorDescription,
                                                        QUuid requestId)
{
    if (requestId != m_saveResourceImageRequestId) {
        return;
    }

    QObject::disconnect(this, QNSIGNAL(AddAttachmentDelegate,saveGenericResourceImageToFile,QString,QByteArray,QByteArray,QString,QUuid),
                        m_pGenericResourceImageWriter, QNSLOT(GenericResourceImageWriter,onGenericResourceImageWriteRequest,QString,QByteArray,QByteArray,QString,QUuid));
    QObject::disconnect(m_pGenericResourceImageWriter, QNSIGNAL(GenericResourceImageWriter,genericResourceImageWriteReply,bool,QByteArray,QString,QString,QUuid),
                        this, QNSLOT(AddAttachmentDelegate,onGenericResourceImageSaved,bool,QByteArray,QString,QString,QUuid));

    QNDEBUG("AddAttachmentDelegate::onGenericResourceImageSaved: success = " << (success ? "true" : "false")
            << ", file path = " << filePath);

    Q_UNUSED(resourceImageDataHash);

    if (Q_UNLIKELY(!success)) {
        errorDescription = QT_TR_NOOP("Can't write resource representing image to file: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        m_noteEditor.removeResourceFromNote(m_resource);
        return;
    }

    m_noteEditor.provideSrcForGenericResourceImages();
    m_noteEditor.setupGenericResourceOnClickHandler();

    emit finished(m_resource, m_resourceFileStoragePath, filePath);
}
#endif

} // namespace qute_note
