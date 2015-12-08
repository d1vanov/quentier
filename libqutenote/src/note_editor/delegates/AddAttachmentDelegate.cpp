#include "AddAttachmentDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/utility/FileIOThreadWorker.h>

#ifdef USE_QT_WEB_ENGINE
#include "../GenericResourceImageWriter.h"
#endif

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
    // TODO: setup all the necessary connections
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
