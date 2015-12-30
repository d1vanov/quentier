#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include "ResourceFileStorageManager_p.h"
#include <qute_note/types/Note.h>

namespace qute_note {

ResourceFileStorageManager::ResourceFileStorageManager(QObject * parent) :
    QObject(parent),
    d_ptr(new ResourceFileStorageManagerPrivate(*this))
{}

QString ResourceFileStorageManager::resourceFileStorageLocation(QWidget * context)
{
    return ResourceFileStorageManagerPrivate::resourceFileStorageLocation(context);
}

void ResourceFileStorageManager::onWriteResourceToFileRequest(QString localGuid, QByteArray data,
                                                              QByteArray dataHash, QString fileStoragePath,
                                                              QUuid requestId)
{
    Q_D(ResourceFileStorageManager);
    d->onWriteResourceToFileRequest(localGuid, data, dataHash, fileStoragePath, requestId);
}

void ResourceFileStorageManager::onReadResourceFromFileRequest(QString fileStoragePath, QString localGuid, QUuid requestId)
{
    Q_D(ResourceFileStorageManager);
    d->onReadResourceFromFileRequest(fileStoragePath, localGuid, requestId);
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
