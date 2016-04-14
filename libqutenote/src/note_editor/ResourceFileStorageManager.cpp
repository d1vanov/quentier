#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include "ResourceFileStorageManager_p.h"
#include <qute_note/types/Note.h>

namespace qute_note {

ResourceFileStorageManager::ResourceFileStorageManager(const QString & imageResourceFileStorageFolderPath, QObject * parent) :
    QObject(parent),
    d_ptr(new ResourceFileStorageManagerPrivate(imageResourceFileStorageFolderPath, *this))
{}

QString ResourceFileStorageManager::resourceFileStorageLocation(QWidget * context)
{
    return ResourceFileStorageManagerPrivate::resourceFileStorageLocation(context);
}

void ResourceFileStorageManager::onWriteResourceToFileRequest(QString noteLocalUid, QString resourceLocalUid, QByteArray data,
                                                              QByteArray dataHash, QString preferredSuffix, QUuid requestId, bool isImage)
{
    Q_D(ResourceFileStorageManager);
    d->onWriteResourceToFileRequest(noteLocalUid, resourceLocalUid, data, dataHash, preferredSuffix, requestId, isImage);
}

void ResourceFileStorageManager::onReadResourceFromFileRequest(QString fileStoragePath, QString resourceLocalUid, QUuid requestId)
{
    Q_D(ResourceFileStorageManager);
    d->onReadResourceFromFileRequest(fileStoragePath, resourceLocalUid, requestId);
}

void ResourceFileStorageManager::onOpenResourceRequest(QString fileStoragePath)
{
    Q_D(ResourceFileStorageManager);
    d->onOpenResourceRequest(fileStoragePath);
}

void ResourceFileStorageManager::onCurrentNoteChanged(Note note)
{
    Q_D(ResourceFileStorageManager);
    d->onCurrentNoteChanged(note);
}

void ResourceFileStorageManager::onRequestDiagnostics(QUuid requestId)
{
    Q_D(ResourceFileStorageManager);
    d->onRequestDiagnostics(requestId);
}

} // namespace qute_note
