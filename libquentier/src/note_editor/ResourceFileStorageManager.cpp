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

#include <quentier/note_editor/ResourceFileStorageManager.h>
#include "ResourceFileStorageManager_p.h"
#include <quentier/types/Note.h>

namespace quentier {

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

} // namespace quentier
