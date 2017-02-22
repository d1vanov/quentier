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

#include "ResourceFileStorageManager_p.h"
#include <quentier/note_editor/ResourceFileStorageManager.h>
#include <quentier/types/Note.h>
#include <quentier/types/Resource.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/DesktopServices.h>
#include <QWidget>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QCryptographicHash>

namespace quentier {

ResourceFileStorageManagerPrivate::ResourceFileStorageManagerPrivate(const QString & nonImageResourceFileStorageFolderPath,
                                                                     const QString & imageResourceFileStorageFolderPath,
                                                                     ResourceFileStorageManager & manager) :
    QObject(&manager),
    m_nonImageResourceFileStorageLocation(nonImageResourceFileStorageFolderPath),
    m_imageResourceFileStorageLocation(imageResourceFileStorageFolderPath),
    m_pCurrentNote(),
    m_resourceLocalUidByFilePath(),
    m_fileSystemWatcher(),
    q_ptr(&manager)
{
    createConnections();
}

void ResourceFileStorageManagerPrivate::onWriteResourceToFileRequest(QString noteLocalUid, QString resourceLocalUid, QByteArray data,
                                                                     QByteArray dataHash, QString preferredFileSuffix, QUuid requestId,
                                                                     bool isImage)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::onWriteResourceToFileRequest: note local uid = ") << noteLocalUid
            << QStringLiteral(", resource local uid = ") << resourceLocalUid << QStringLiteral(", request id = ") << requestId
            << QStringLiteral(", preferred file suffix = ") << preferredFileSuffix << QStringLiteral(", data hash = ") << dataHash.toHex()
            << QStringLiteral(", is image = ") << (isImage ? QStringLiteral("true") : QStringLiteral("false")));

    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "detected attempt to write resource data for empty note local uid to local file"));
        QNWARNING(errorDescription << QStringLiteral(", request id = ") << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(), ResourceFileStorageManager::Errors::EmptyLocalUid, errorDescription);
        return;
    }

    if (Q_UNLIKELY(resourceLocalUid.isEmpty())) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "detected attempt to write data for empty resource local uid to local file"));
        QNWARNING(errorDescription << QStringLiteral(", request id = ") << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(), ResourceFileStorageManager::Errors::EmptyLocalUid, errorDescription);
        return;
    }

    if (Q_UNLIKELY(requestId.isNull())) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "detected attempt to write data for resource to local file with empty request id"));
        QNWARNING(errorDescription << QStringLiteral(", note local uid = ") << noteLocalUid << QStringLiteral(", resource local uid = ") << resourceLocalUid);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(), ResourceFileStorageManager::Errors::EmptyRequestId, errorDescription);
        return;
    }

    if (Q_UNLIKELY(data.isEmpty())) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "detected attempt to write empty resource data to local file"));
        QNWARNING(errorDescription << QStringLiteral(", note local uid = ") << noteLocalUid << QStringLiteral(", resource local uid = ") << resourceLocalUid);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(), ResourceFileStorageManager::Errors::EmptyData, errorDescription);
        return;
    }

    if (!isImage && Q_UNLIKELY(m_nonImageResourceFileStorageLocation.isEmpty())) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't automatically choose resource file storage location"));
        QNWARNING(errorDescription);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(),
                                          ResourceFileStorageManager::Errors::NoResourceFileStorageLocation,
                                          errorDescription);
        return;
    }

    QString fileStoragePath = (isImage
                               ? m_imageResourceFileStorageLocation
                               : m_nonImageResourceFileStorageLocation);
    fileStoragePath += QStringLiteral("/") + noteLocalUid + QStringLiteral("/") + resourceLocalUid;

    if (!preferredFileSuffix.isEmpty()) {
        fileStoragePath += "." + preferredFileSuffix;
    }

    QFileInfo fileStoragePathInfo(fileStoragePath);
    QDir fileStorageDir(fileStoragePathInfo.absoluteDir());
    if (!fileStorageDir.exists())
    {
        bool createDir = fileStorageDir.mkpath(fileStorageDir.absolutePath());
        if (!createDir)
        {
            int errorCode = -1;
            ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't create folder to write the resource into"));
            QNWARNING(errorDescription << QStringLiteral(", note local uid = ") << noteLocalUid << QStringLiteral(", resource local uid = ")
                      << resourceLocalUid << QStringLiteral(", request id = ") << requestId);
            emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
            return;
        }
    }

    if (dataHash.isEmpty()) {
        dataHash = calculateHash(data);
        QNTRACE(QStringLiteral("Resource data hash was empty, calculated hash: ") << dataHash.toHex());
    }

    bool actual = checkIfResourceFileExistsAndIsActual(noteLocalUid, resourceLocalUid, fileStoragePath, dataHash);
    if (actual) {
        QNTRACE(QStringLiteral("Skipping writing the resource to file as it is not necessary"));
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, 0, ErrorString());
        return;
    }

    QFile file(fileStoragePath);
    bool open = file.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open))
    {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't open resource file for writing"));
        errorDescription.details() = file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << QStringLiteral(", error code = ") << errorCode << QStringLiteral(", note local uid = ")
                  << noteLocalUid << QStringLiteral(", resource local uid = ") << resourceLocalUid << QStringLiteral(", request id = ")
                  << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        return;
    }

    qint64 writeRes = file.write(data);
    if (Q_UNLIKELY(writeRes < 0))
    {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't write data to resource file"));
        errorDescription.details() = file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << QStringLiteral(", error code = ") << errorCode << QStringLiteral(", note local uid = ")
                  << noteLocalUid << QStringLiteral(", resource local uid = ") << resourceLocalUid << QStringLiteral(", request id = ")
                  << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        return;
    }

    file.close();

    m_resourceLocalUidByFilePath[fileStoragePath] = resourceLocalUid;

    int errorCode = 0;
    ErrorString errorDescription;
    bool res = updateResourceHash(resourceLocalUid, dataHash, fileStoragePathInfo.absolutePath(),
                                  errorCode, errorDescription);
    if (Q_UNLIKELY(!res)) {
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        QNWARNING(errorDescription << QStringLiteral(", error code = ") << errorCode << QStringLiteral(", resource local uid = ")
                  << resourceLocalUid << QStringLiteral(", request id = ") << requestId);
        return;
    }

    QNDEBUG(QStringLiteral("Successfully wrote resource data to file: resource local uid = ") << resourceLocalUid
            << QStringLiteral(", file path = ") << fileStoragePath);
    emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, 0, ErrorString());
}

void ResourceFileStorageManagerPrivate::onReadResourceFromFileRequest(QString fileStoragePath, QString resourceLocalUid, QUuid requestId)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::onReadResourceFromFileRequest: resource local uid = ")
            << resourceLocalUid << QStringLiteral(", request id = ") << requestId);

    if (Q_UNLIKELY(m_nonImageResourceFileStorageLocation.isEmpty()))
    {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "resource file storage location is empty"));
        QNWARNING(errorDescription << QStringLiteral(", resource local uid = ") << resourceLocalUid
                  << QStringLiteral(", request id = ") << requestId);
        emit readResourceFromFileCompleted(requestId, QByteArray(), QByteArray(),
                                           ResourceFileStorageManager::Errors::NoResourceFileStorageLocation,
                                           errorDescription);
        return;
    }

    QFile resourceFile(fileStoragePath);
    bool open = resourceFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open))
    {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't open resource file for reading"));
        errorDescription.details() = resourceFile.errorString();
        int errorCode = resourceFile.error();
        QNWARNING(errorDescription << QStringLiteral(", error code = ") << errorCode << QStringLiteral(", resource local uid = ")
                  << resourceLocalUid << QStringLiteral(", request id = ") << requestId);
        emit readResourceFromFileCompleted(requestId, QByteArray(), QByteArray(),
                                           errorCode, errorDescription);
        return;
    }

    QFileInfo resourceFileInfo(fileStoragePath);
    QFile resourceHashFile(resourceFileInfo.absolutePath() + QStringLiteral("/") + resourceLocalUid + QStringLiteral(".hash"));
    open = resourceHashFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open))
    {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't open resource hash file for reading"));
        errorDescription.details() = resourceHashFile.errorString();
        int errorCode = resourceHashFile.error();
        QNWARNING(errorDescription << QStringLiteral(", error code = ") << errorCode << QStringLiteral(", resource local uid = ")
                  << resourceLocalUid << QStringLiteral(", request id = ") << requestId);
        emit readResourceFromFileCompleted(requestId, QByteArray(), QByteArray(),
                                           errorCode, errorDescription);
        return;
    }

    QByteArray data = resourceFile.readAll();
    QByteArray dataHash = resourceHashFile.readAll();

    QNDEBUG(QStringLiteral("Successfully read resource data and hash from files"));
    emit readResourceFromFileCompleted(requestId, data, dataHash, 0, ErrorString());
}

void ResourceFileStorageManagerPrivate::onOpenResourceRequest(QString fileStoragePath)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::onOpenResourceRequest: file path = ") << fileStoragePath);

    auto it = m_resourceLocalUidByFilePath.find(fileStoragePath);
    if (Q_UNLIKELY(it == m_resourceLocalUidByFilePath.end())) {
        QNWARNING(QStringLiteral("Can't set up watching for resource file's changes: can't find resource local uid for file path: ")
                  << fileStoragePath);
    }
    else {
        watchResourceFileForChanges(it.value(), fileStoragePath);
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(fileStoragePath));
}

void ResourceFileStorageManagerPrivate::onCurrentNoteChanged(Note note)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::onCurrentNoteChanged; new note local uid = ") << note.localUid()
            << QStringLiteral(", previous note local uid = ") << (m_pCurrentNote.isNull() ? QStringLiteral("<null>") : m_pCurrentNote->localUid()));

    if (!m_pCurrentNote.isNull() && (m_pCurrentNote->localUid() == note.localUid())) {
        QNTRACE(QStringLiteral("The current note is the same, only the note object might have changed"));
        *m_pCurrentNote = note;
        removeStaleResourceFilesFromCurrentNote();
        return;
    }

    for(auto it = m_resourceLocalUidByFilePath.begin(), end = m_resourceLocalUidByFilePath.end(); it != end; ++it) {
        m_fileSystemWatcher.removePath(it.key());
        QNTRACE(QStringLiteral("Stopped watching for file ") << it.key());
    }
    m_resourceLocalUidByFilePath.clear();

    removeStaleResourceFilesFromCurrentNote();

    if (m_pCurrentNote.isNull()) {
        m_pCurrentNote.reset(new Note(note));
    }
    else {
        *m_pCurrentNote = note;
    }
}

void ResourceFileStorageManagerPrivate::onRequestDiagnostics(QUuid requestId)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::onRequestDiagnostics: request id = ") << requestId);

    QString diagnostics = QStringLiteral("ResourceFileStorageManager diagnostics: {\n");

    diagnostics += QStringLiteral("  Resource local uids by file paths: \n");
    for(auto it = m_resourceLocalUidByFilePath.begin(), end = m_resourceLocalUidByFilePath.end(); it != end; ++it) {
        diagnostics += QStringLiteral("    [") + it.key() + QStringLiteral("]: ") + it.value() + QStringLiteral("\n");
    }

    diagnostics += QStringLiteral("  Watched files: \n");
    QStringList watchedFiles = m_fileSystemWatcher.files();
    const int numWatchedFiles = watchedFiles.size();
    for(int i = 0; i < numWatchedFiles; ++i) {
        diagnostics += QStringLiteral("    ") + watchedFiles[i] + QStringLiteral("\n");
    }

    diagnostics += QStringLiteral("}\n");

    emit diagnosticsCollected(requestId, diagnostics);
}

void ResourceFileStorageManagerPrivate::onFileChanged(const QString & path)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::onFileChanged: ") << path);

    auto it = m_resourceLocalUidByFilePath.find(path);

    QFileInfo resourceFileInfo(path);
    if (!resourceFileInfo.exists())
    {
        if (it != m_resourceLocalUidByFilePath.end()) {
            Q_UNUSED(m_resourceLocalUidByFilePath.erase(it));
        }

        m_fileSystemWatcher.removePath(path);
        QNINFO(QStringLiteral("Stopped watching for file ") << path << QStringLiteral(" as it was deleted"));

        return;
    }

    if (Q_UNLIKELY(it == m_resourceLocalUidByFilePath.end())) {
        QNWARNING(QStringLiteral("Can't process resource local file change properly: can't find resource local uid by file path: ")
                  << path << QStringLiteral("; stopped watching for that file's changes"));
        m_fileSystemWatcher.removePath(path);
        return;
    }

    QFile file(path);
    bool open = file.QIODevice::open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open)) {
        QNWARNING(QStringLiteral("Can't process resource local file change properly: can't open resource file for reading: error code = ")
                  << file.error() << QStringLiteral(", error description: ") << file.errorString());
        m_fileSystemWatcher.removePath(path);
        return;
    }

    QByteArray data = file.readAll();
    QByteArray dataHash = calculateHash(data);

    int errorCode = 0;
    ErrorString errorDescription;
    bool res = updateResourceHash(it.value(), dataHash, resourceFileInfo.absolutePath(), errorCode, errorDescription);
    if (Q_UNLIKELY(!res)) {
        QNWARNING(QStringLiteral("Can't process resource local file change properly: can't update the hash for resource file: error code = ")
                  << errorCode << QStringLiteral(", error description: ") << errorDescription);
        m_fileSystemWatcher.removePath(path);
        return;
    }

    emit resourceFileChanged(it.value(), path);
}

void ResourceFileStorageManagerPrivate::onFileRemoved(const QString & path)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::onFileRemoved: ") << path);

    auto it = m_resourceLocalUidByFilePath.find(path);
    if (it != m_resourceLocalUidByFilePath.end()) {
        Q_UNUSED(m_resourceLocalUidByFilePath.erase(it));
    }
}

void ResourceFileStorageManagerPrivate::createConnections()
{
    QObject::connect(&m_fileSystemWatcher, QNSIGNAL(FileSystemWatcher,fileChanged,QString),
                     this, QNSLOT(ResourceFileStorageManagerPrivate,onFileChanged,QString));

    Q_Q(ResourceFileStorageManager);

    QObject::connect(this, QNSIGNAL(ResourceFileStorageManagerPrivate,resourceFileChanged,QString,QString),
                     q, QNSIGNAL(ResourceFileStorageManager,resourceFileChanged,QString,QString));
    QObject::connect(this, QNSIGNAL(ResourceFileStorageManagerPrivate,readResourceFromFileCompleted,QUuid,QByteArray,QByteArray,int,ErrorString),
                     q, QNSIGNAL(ResourceFileStorageManager,readResourceFromFileCompleted,QUuid,QByteArray,QByteArray,int,ErrorString));
    QObject::connect(this, QNSIGNAL(ResourceFileStorageManagerPrivate,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,ErrorString),
                     q, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,ErrorString));
    QObject::connect(this, QNSIGNAL(ResourceFileStorageManagerPrivate,diagnosticsCollected,QUuid,QString),
                     q, QNSIGNAL(ResourceFileStorageManager,diagnosticsCollected,QUuid,QString));
}

QByteArray ResourceFileStorageManagerPrivate::calculateHash(const QByteArray & data) const
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
}

bool ResourceFileStorageManagerPrivate::checkIfResourceFileExistsAndIsActual(const QString & noteLocalUid, const QString & resourceLocalUid,
                                                                             const QString & fileStoragePath, const QByteArray & dataHash) const
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::checkIfResourceFileExistsAndIsActual: note local uid = ")
            << noteLocalUid << QStringLiteral(", resource local uid = ") << resourceLocalUid << QStringLiteral(", data hash = ")
            << dataHash.toHex());

    if (Q_UNLIKELY(fileStoragePath.isEmpty())) {
        QNWARNING(QStringLiteral("Resource file storage location is empty"));
        return false;
    }

    QFileInfo resourceFileInfo(fileStoragePath);
    if (!resourceFileInfo.exists()) {
        QNTRACE(QStringLiteral("Resource file for note local uid ") << noteLocalUid << QStringLiteral(" and resource local uid ")
                << resourceLocalUid << QStringLiteral(" does not exist"));
        return false;
    }

    QFileInfo resourceHashFileInfo(resourceFileInfo.absolutePath() + "/" + resourceFileInfo.baseName() + ".hash");
    if (!resourceHashFileInfo.exists()) {
        QNTRACE(QStringLiteral("Resource hash file for note local uid ") << noteLocalUid << QStringLiteral(" and resource local uid ")
                << resourceLocalUid << QStringLiteral(" does not exist"));
        return false;
    }

    QFile resourceHashFile(resourceHashFileInfo.absoluteFilePath());
    bool open = resourceHashFile.open(QIODevice::ReadOnly);
    if (!open) {
        QNWARNING(QStringLiteral("Can't open resource hash file for reading"));
        return false;
    }

    QByteArray storedHash = resourceHashFile.readAll();
    if (storedHash != dataHash) {
        QNTRACE(QStringLiteral("Resource must be stale, the stored hash ") << storedHash.toHex()
                << QStringLiteral(" does not match the actual hash ") << dataHash.toHex());
        return false;
    }

    QNDEBUG(QStringLiteral("Resource file exists and is actual"));
    return true;
}

bool ResourceFileStorageManagerPrivate::updateResourceHash(const QString & resourceLocalUid, const QByteArray & dataHash,
                                                           const QString & storageFolderPath, int & errorCode,
                                                           ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::updateResourceHash: resource local uid = ") << resourceLocalUid
            << QStringLiteral(", data hash = ") << dataHash.toHex() << QStringLiteral(", storage folder path = ") << storageFolderPath);

    QFile file(storageFolderPath + QStringLiteral("/") + resourceLocalUid + QStringLiteral(".hash"));

    bool open = file.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "can't open the file with resource's hash for writing");
        errorDescription.details() = file.errorString();
        errorCode = file.error();
        return false;
    }

    qint64 writeRes = file.write(dataHash);
    if (Q_UNLIKELY(writeRes < 0)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "can't write resource data hash to the separate file");
        errorDescription.details() = file.errorString();
        errorCode = file.error();
        return false;
    }

    file.close();
    return true;
}

void ResourceFileStorageManagerPrivate::watchResourceFileForChanges(const QString & resourceLocalUid, const QString & fileStoragePath)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::watchResourceFileForChanges: resource local uid = ")
            << resourceLocalUid << QStringLiteral(", file storage path = ") << fileStoragePath);

    m_fileSystemWatcher.addPath(fileStoragePath);
    QNINFO(QStringLiteral("Start watching for resource file ") << fileStoragePath);
}

void ResourceFileStorageManagerPrivate::stopWatchingResourceFile(const QString & filePath)
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::stopWatchingResourceFile: ") << filePath);

    auto it = m_resourceLocalUidByFilePath.find(filePath);
    if (it == m_resourceLocalUidByFilePath.end()) {
        QNTRACE(QStringLiteral("File is not being watched, nothing to do"));
        return;
    }

    m_fileSystemWatcher.removePath(filePath);
    QNTRACE(QStringLiteral("Stopped watching for file"));
}

void ResourceFileStorageManagerPrivate::removeStaleResourceFilesFromCurrentNote()
{
    QNDEBUG(QStringLiteral("ResourceFileStorageManagerPrivate::removeStaleResourceFilesFromCurrentNote"));

    if (m_pCurrentNote.isNull()) {
        QNDEBUG(QStringLiteral("No current note, nothing to do"));
        return;
    }

    const QString & noteLocalUid = m_pCurrentNote->localUid();

    QList<Resource> resources = m_pCurrentNote->resources();
    const int numResources = resources.size();

    QFileInfoList fileInfoList;
    int numFiles = -1;

    QDir imageResourceFilesFolder(m_imageResourceFileStorageLocation + QStringLiteral("/") + m_pCurrentNote->localUid());
    if (imageResourceFilesFolder.exists())
    {
        fileInfoList = imageResourceFilesFolder.entryInfoList(QDir::Files);
        numFiles = fileInfoList.size();
        QNTRACE(QStringLiteral("Found ") << numFiles << QStringLiteral(" files wihin the image resource files folder for note with local uid ")
                << m_pCurrentNote->localUid());
    }

    QDir genericResourceImagesFolder(m_nonImageResourceFileStorageLocation + QStringLiteral("/") + m_pCurrentNote->localUid());
    if (genericResourceImagesFolder.exists())
    {
        QFileInfoList genericResourceImageFileInfos = genericResourceImagesFolder.entryInfoList(QDir::Files);
        int numGenericResourceImageFileInfos = genericResourceImageFileInfos.size();
        QNTRACE(QStringLiteral("Found ") << numGenericResourceImageFileInfos
                << QStringLiteral(" files within the generic resource files folder for note with local uid ")
                << m_pCurrentNote->localUid());

        fileInfoList.append(genericResourceImageFileInfos);
        numFiles = fileInfoList.size();
    }

    QNTRACE(QStringLiteral("Total ") << numFiles << QStringLiteral(" to check for staleness"));

    for(int i = 0; i < numFiles; ++i)
    {
        const QFileInfo & fileInfo = fileInfoList[i];
        QString filePath = fileInfo.absoluteFilePath();

        if (fileInfo.isSymLink()) {
            QNTRACE(QStringLiteral("Removing symlink file without any checks"));
            stopWatchingResourceFile(filePath);
            Q_UNUSED(removeFile(filePath))
            continue;
        }

        QString fullSuffix = fileInfo.completeSuffix();
        if (fullSuffix == QStringLiteral("hash")) {
            QNTRACE(QStringLiteral("Skipping .hash helper file ") << filePath);
            continue;
        }

        QString baseName = fileInfo.baseName();
        QNTRACE(QStringLiteral("Checking file with base name ") << baseName);

        int resourceIndex = -1;
        for(int j = 0; j < numResources; ++j)
        {
            QNTRACE(QStringLiteral("checking against resource with local uid ") << resources[j].localUid());
            if (baseName.startsWith(resources[j].localUid()))
            {
                QNTRACE(QStringLiteral("File ") << fileInfo.fileName() << QStringLiteral(" appears to correspond to resource ")
                        << resources[j].localUid());
                resourceIndex = j;
                break;
            }
        }

        if (resourceIndex >= 0)
        {
            const Resource & resource = resources[resourceIndex];
            if (resource.hasDataHash())
            {
                bool actual = checkIfResourceFileExistsAndIsActual(noteLocalUid, resource.localUid(), filePath, resource.dataHash());
                if (actual) {
                    QNTRACE(QStringLiteral("The resource file ") << filePath << QStringLiteral(" is still actual, will keep it"));
                    continue;
                }
            }
            else
            {
                QNTRACE(QStringLiteral("Resource at index ") << resourceIndex
                        << QStringLiteral(" doesn't have the data hash, will remove its resource file just in case"));
            }
        }

        QNTRACE(QStringLiteral("Found stale resource file ") << filePath << QStringLiteral(", removing it"));
        stopWatchingResourceFile(filePath);
        Q_UNUSED(removeFile(filePath))

        // Need to also remove the helper .hash file
        stopWatchingResourceFile(filePath);
        Q_UNUSED(removeFile(fileInfo.absolutePath() + QStringLiteral("/") + baseName + QStringLiteral(".hash")));
    }
}

} // namespace quentier
