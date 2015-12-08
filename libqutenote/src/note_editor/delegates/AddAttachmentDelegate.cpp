#include "AddAttachmentDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>

#ifdef USE_QT_WEB_ENGINE
#include "../GenericResourceImageWriter.h"
#endif

#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

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
    m_filePath(filePath)
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
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);

    if (!mimeType.isValid()) {
        QNINFO("Detected invalid mime type for file " << m_filePath);
        return;
    }

    QUuid readDroppedFileRequestId = QUuid::createUuid();
    auto & pair = m_droppedFilePathsAndMimeTypesByReadRequestIds[readDroppedFileRequestId];
    pair.first = fileInfo.filePath();
    pair.second = mimeType;
    emit readFileData(m_filePath, readDroppedFileRequestId);
}

void AddAttachmentDelegate::onResourceFileRead(bool success, QString errorDescription,
                                               QByteArray data, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(success);
    Q_UNUSED(errorDescription);
    Q_UNUSED(data);
    Q_UNUSED(requestId);

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
