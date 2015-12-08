#include "AddAttachmentDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/types/ResourceWrapper.h>

#ifdef USE_QT_WEB_ENGINE
#include "../GenericResourceImageWriter.h"
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
#endif
    m_filePath(filePath),
    m_resourceFileMimeType(),
    m_readResourceFileRequestId(),
    m_resourceLocalGuid(),
    m_saveResourceToStorageRequestId()
{
    QObject::connect(this, QNSIGNAL(AddAttachmentDelegate,readFileData,QString,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onReadFileRequest,QString,QUuid));
    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QString,QByteArray,QUuid),
                     this, QNSLOT(AddAttachmentDelegate,onResourceFileRead,bool,QString,QByteArray,QUuid));

    QObject::connect(this, QNSIGNAL(AddAttachmentDelegate,saveResourceToStorage,QString,QByteArray,QByteArray,QString,QUuid),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QByteArray,QByteArray,QString,QUuid));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString),
                     this, QNSLOT(AddAttachmentDelegate,onResourceSavedToStorage,QUuid,QByteArray,QString,int,QString));
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
    emit readFileData(m_filePath, m_readResourceFileRequestId);
}

void AddAttachmentDelegate::onResourceFileRead(bool success, QString errorDescription,
                                               QByteArray data, QUuid requestId)
{
    if (requestId != m_readResourceFileRequestId) {
        return;
    }

    QFileInfo fileInfo(m_filePath);

    if (Q_UNLIKELY(!success)) {
        QNDEBUG("Could not read the content of the dropped file for request id " << requestId
                << ": " << errorDescription);
        return;
    }

    QNDEBUG("Successfully read the content of the dropped file for request id " << requestId);

    QByteArray dataHash = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    ResourceWrapper newResource = m_noteEditor.attachResourceToNote(data, dataHash, m_resourceFileMimeType, fileInfo.fileName());
    m_resourceLocalGuid = newResource.localGuid();
    if (Q_UNLIKELY(m_resourceLocalGuid.isEmpty())) {
        return;
    }

    QString resourceHtml = ENMLConverter::resourceHtml(newResource, errorDescription);
    if (Q_UNLIKELY(resourceHtml.isEmpty())) {
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        m_noteEditor.removeResourceFromNote(newResource);
        return;
    }

    QNTRACE("Resource html: " << resourceHtml);

    m_noteEditor.skipPushingUndoCommandOnNextContentChange();
    m_noteEditor.execJavascriptCommand("insertHtml", resourceHtml);

    QString fileStoragePath;
    if (m_resourceFileMimeType.name().startsWith("image/")) {
        fileStoragePath = m_noteEditor.imageResourcesStoragePath();
    }
    else {
        fileStoragePath = m_noteEditor.resourceLocalFileStoragePath();
    }

    fileStoragePath += "/" + m_resourceLocalGuid;

    QString fileInfoSuffix = fileInfo.completeSuffix();
    if (!fileInfoSuffix.isEmpty())
    {
        fileStoragePath += "." + fileInfoSuffix;
    }
    else
    {
        const QStringList suffixes = m_resourceFileMimeType.suffixes();
        if (!suffixes.isEmpty()) {
            fileStoragePath += "." + suffixes.front();
        }
    }

    m_saveResourceToStorageRequestId = QUuid::createUuid();
    emit saveResourceToStorage(m_resourceLocalGuid, data, dataHash, fileStoragePath, m_saveResourceToStorageRequestId);

    QNTRACE("Emitted request to save the dropped resource to local file storage: generated local guid = "
            << m_resourceLocalGuid << ", data hash = " << dataHash << ", request id = "
            << m_saveResourceToStorageRequestId << ", mime type name = " << m_resourceFileMimeType.name());
}

void AddAttachmentDelegate::onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                                     QString fileStoragePath, int errorCode,
                                                     QString errorDescription)
{
    // TODO: implement
    Q_UNUSED(requestId);
    Q_UNUSED(dataHash);
    Q_UNUSED(fileStoragePath);
    Q_UNUSED(errorCode);
    Q_UNUSED(errorDescription);
}

#ifdef USE_QT_WEB_ENGINE
void AddAttachmentDelegate::onGenericResourceImageSaved(const bool success, const QByteArray resourceActualHash,
                                                        const QString filePath, const QString errorDescription,
                                                        const QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(success);
    Q_UNUSED(resourceActualHash);
    Q_UNUSED(filePath);
    Q_UNUSED(errorDescription);
    Q_UNUSED(requestId);
}
#endif

} // namespace qute_note
