#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include "ResourceFileStorageManager_p.h"

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

void ResourceFileStorageManager::onReadResourceFromFileRequest(QString localGuid, QUuid requestId)
{
    Q_D(ResourceFileStorageManager);
    d->onReadResourceFromFileRequest(localGuid, requestId);
}

} // namespace qute_note
