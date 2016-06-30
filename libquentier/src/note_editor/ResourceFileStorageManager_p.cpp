#include "ResourceFileStorageManager_p.h"
#include "dialogs/AttachmentStoragePathConfigDialog.h"
#include <quentier/note_editor/ResourceFileStorageManager.h>
#include <quentier/types/Note.h>
#include <quentier/types/ResourceAdapter.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QWidget>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>

namespace quentier {

ResourceFileStorageManagerPrivate::ResourceFileStorageManagerPrivate(const QString & imageResourceFileStorageFolderPath,
                                                                     ResourceFileStorageManager & manager) :
    QObject(&manager),
    m_imageResourceFileStorageLocation(imageResourceFileStorageFolderPath),
    m_resourceFileStorageLocation(),
    m_pCurrentNote(),
    m_resourceLocalUidByFilePath(),
    m_fileSystemWatcher(),
    q_ptr(&manager)
{
    createConnections();
    m_resourceFileStorageLocation = resourceFileStorageLocation(Q_NULLPTR);
}

QString ResourceFileStorageManagerPrivate::resourceFileStorageLocation(QWidget * context)
{
    ApplicationSettings appSettings;
    appSettings.beginGroup("AttachmentSaveLocations");
    QString resourceFileStorageLocation = appSettings.value("OwnFileStorageLocation").toString();
    if (resourceFileStorageLocation.isEmpty()) {
        resourceFileStorageLocation = applicationPersistentStoragePath() + "/" + "attachments";
        appSettings.setValue("OwnFileStorageLocation", QVariant(resourceFileStorageLocation));
    }

    QFileInfo resourceFileStorageLocationInfo(resourceFileStorageLocation);
    QString manualPath;
    if (!resourceFileStorageLocationInfo.exists())
    {
        QDir resourceFileStorageLocationDir(resourceFileStorageLocation);
        bool res = resourceFileStorageLocationDir.mkpath(resourceFileStorageLocation);
        if (!res)
        {
            QNWARNING("Can't create directory for attachment tmp storage location: "
                      << resourceFileStorageLocation);
            manualPath = getAttachmentStoragePath(context);
        }
    }
    else if (!resourceFileStorageLocationInfo.isDir())
    {
        QNWARNING("Can't figure out where to save the temporary copies of attachments: path "
                  << resourceFileStorageLocation << " is not a directory");
        manualPath = getAttachmentStoragePath(context);
    }
    else if (!resourceFileStorageLocationInfo.isWritable())
    {
        QNWARNING("Can't save temporary copies of attachments: the suggested folder is not writable: "
                  << resourceFileStorageLocation);
        manualPath = getAttachmentStoragePath(context);
    }

    if (!manualPath.isEmpty())
    {
        resourceFileStorageLocationInfo.setFile(manualPath);
        if (!resourceFileStorageLocationInfo.exists() || !resourceFileStorageLocationInfo.isDir() ||
            !resourceFileStorageLocationInfo.isWritable())
        {
            QNWARNING("Can't use manually selected attachment storage path");
            return QString();
        }

        appSettings.setValue("OwnFileStorageLocation", QVariant(manualPath));
        resourceFileStorageLocation = manualPath;
    }

    appSettings.endGroup();
    return resourceFileStorageLocation;
}

void ResourceFileStorageManagerPrivate::onWriteResourceToFileRequest(QString noteLocalUid, QString resourceLocalUid, QByteArray data,
                                                                     QByteArray dataHash, QString preferredFileSuffix, QUuid requestId,
                                                                     bool isImage)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onWriteResourceToFileRequest: note local uid = " << noteLocalUid
            << ", resource local uid = " << resourceLocalUid << ", request id = " << requestId
            << ", preferred file suffix = " << preferredFileSuffix << ", data hash = " << dataHash.toHex()
            << ", is image = " << (isImage ? "true" : "false"));

    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write resource data for empty note local uid to local file");
        QNWARNING(errorDescription << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(), ResourceFileStorageManager::EmptyLocalUid, errorDescription);
        return;
    }

    if (Q_UNLIKELY(resourceLocalUid.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write data for empty resource local uid to local file");
        QNWARNING(errorDescription << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(), ResourceFileStorageManager::EmptyLocalUid, errorDescription);
        return;
    }

    if (Q_UNLIKELY(requestId.isNull())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write data for resource to local file with empty request id");
        QNWARNING(errorDescription << ", note local uid = " << noteLocalUid << ", resource local uid = " << resourceLocalUid);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(), ResourceFileStorageManager::EmptyRequestId, errorDescription);
        return;
    }

    if (Q_UNLIKELY(data.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write empty resource data to local file");
        QNWARNING(errorDescription << ", note local uid = " << noteLocalUid << ", resource local uid = " << resourceLocalUid);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(),
                                          ResourceFileStorageManager::EmptyData, errorDescription);
        return;
    }

    if (!isImage && Q_UNLIKELY(m_resourceFileStorageLocation.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Can't automatically choose resource file storage location");
        QNWARNING(errorDescription);
        emit writeResourceToFileCompleted(requestId, dataHash, QString(),
                                          ResourceFileStorageManager::NoResourceFileStorageLocation,
                                          errorDescription);
        return;
    }

    QString fileStoragePath = (isImage
                               ? m_imageResourceFileStorageLocation
                               : m_resourceFileStorageLocation);
    fileStoragePath += "/" + noteLocalUid + "/" + resourceLocalUid;

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
            QString errorDescription = QT_TR_NOOP("Can't create folder to write the resource into");
            QNWARNING(errorDescription << ", note local uid = " << noteLocalUid << ", resource local uid = "
                      << resourceLocalUid << ", request id = " << requestId);
            emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
            return;
        }
    }

    if (dataHash.isEmpty()) {
        dataHash = calculateHash(data);
        QNTRACE("Resource data hash was empty, calculated hash: " << dataHash.toHex());
    }

    bool actual = checkIfResourceFileExistsAndIsActual(noteLocalUid, resourceLocalUid, fileStoragePath, dataHash);
    if (actual) {
        QNTRACE("Skipping writing the resource to file as it is not necessary");
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, 0, QString());
        return;
    }

    QFile file(fileStoragePath);
    bool open = file.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open))
    {
        QString errorDescription = QT_TR_NOOP("Can't open resource file for writing");
        errorDescription += ": " + file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", note local uid = "
                  << noteLocalUid << ", resource local uid = " << resourceLocalUid << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        return;
    }

    qint64 writeRes = file.write(data);
    if (Q_UNLIKELY(writeRes < 0))
    {
        QString errorDescription = QT_TR_NOOP("Can't write data to resource file");
        errorDescription += ": " + file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", note local uid = " << noteLocalUid
                  << ", resource local uid = " << resourceLocalUid << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        return;
    }

    file.close();

    m_resourceLocalUidByFilePath[fileStoragePath] = resourceLocalUid;

    int errorCode = 0;
    QString errorDescription;
    bool res = updateResourceHash(resourceLocalUid, dataHash, fileStoragePathInfo.absolutePath(),
                                  errorCode, errorDescription);
    if (Q_UNLIKELY(!res)) {
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        QNWARNING(errorDescription << ", error code = " << errorCode << ", resource local uid = "
                  << resourceLocalUid << ", request id = " << requestId);
        return;
    }

    QNDEBUG("Successfully wrote resource data to file: resource local uid = " << resourceLocalUid << ", file path = " << fileStoragePath);
    emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, 0, QString());
}

void ResourceFileStorageManagerPrivate::onReadResourceFromFileRequest(QString fileStoragePath, QString resourceLocalUid, QUuid requestId)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onReadResourceFromFileRequest: resource local uid = " << resourceLocalUid
            << ", request id = " << requestId);

    if (Q_UNLIKELY(m_resourceFileStorageLocation.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Resource file storage location is empty");
        QNWARNING(errorDescription << ", resource local uid = " << resourceLocalUid << ", request id = " << requestId);
        emit readResourceFromFileCompleted(requestId, QByteArray(), QByteArray(),
                                           ResourceFileStorageManager::NoResourceFileStorageLocation,
                                           errorDescription);
        return;
    }

    QFile resourceFile(fileStoragePath);
    bool open = resourceFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open)) {
        QString errorDescription = QT_TR_NOOP("Can't open resource file for reading: ");
        errorDescription += ": " + resourceFile.errorString();
        int errorCode = resourceFile.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", resource local uid = "
                  << resourceLocalUid << ", request id = " << requestId);
        emit readResourceFromFileCompleted(requestId, QByteArray(), QByteArray(),
                                           errorCode, errorDescription);
        return;
    }

    QFileInfo resourceFileInfo(fileStoragePath);
    QFile resourceHashFile(resourceFileInfo.absolutePath() + "/" + resourceLocalUid + ".hash");
    open = resourceHashFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open)) {
        QString errorDescription = QT_TR_NOOP("Can't open resource hash file for reading: ");
        errorDescription += ": " + resourceHashFile.errorString();
        int errorCode = resourceHashFile.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", resource local uid = "
                  << resourceLocalUid << ", request id = " << requestId);
        emit readResourceFromFileCompleted(requestId, QByteArray(), QByteArray(),
                                           errorCode, errorDescription);
        return;
    }

    QByteArray data = resourceFile.readAll();
    QByteArray dataHash = resourceHashFile.readAll();

    QNDEBUG("Successfully read resource data and hash from files");
    emit readResourceFromFileCompleted(requestId, data, dataHash, 0, QString());
}

void ResourceFileStorageManagerPrivate::onOpenResourceRequest(QString fileStoragePath)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onOpenResourceRequest: file path = " << fileStoragePath);

    auto it = m_resourceLocalUidByFilePath.find(fileStoragePath);
    if (Q_UNLIKELY(it == m_resourceLocalUidByFilePath.end())) {
        QNWARNING("Can't set up watching for resource file's changes: can't find resource local uid for file path: " + fileStoragePath);
    }
    else {
        watchResourceFileForChanges(it.value(), fileStoragePath);
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(fileStoragePath));
}

void ResourceFileStorageManagerPrivate::onCurrentNoteChanged(Note note)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onCurrentNoteChanged; new note local uid = " << note.localUid()
            << ", previous note local uid = " << (m_pCurrentNote.isNull() ? QStringLiteral("<null>") : m_pCurrentNote->localUid()));

    if (!m_pCurrentNote.isNull() && (m_pCurrentNote->localUid() == note.localUid())) {
        QNTRACE("The current note is the same, only the note object might have changed");
        *m_pCurrentNote = note;
        removeStaleResourceFilesFromCurrentNote();
        return;
    }

    for(auto it = m_resourceLocalUidByFilePath.begin(), end = m_resourceLocalUidByFilePath.end(); it != end; ++it) {
        m_fileSystemWatcher.removePath(it.key());
        QNTRACE("Stopped watching for file " << it.key());
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
    QNDEBUG("ResourceFileStorageManagerPrivate::onRequestDiagnostics: request id = " << requestId);

    QString diagnostics = "ResourceFileStorageManager diagnostics: {\n";

    diagnostics += "  Resource local uids by file paths: \n";
    for(auto it = m_resourceLocalUidByFilePath.begin(), end = m_resourceLocalUidByFilePath.end(); it != end; ++it) {
        diagnostics += "    [" + it.key() + "]: " + it.value() + "\n";
    }

    diagnostics += "  Watched files: \n";
    QStringList watchedFiles = m_fileSystemWatcher.files();
    const int numWatchedFiles = watchedFiles.size();
    for(int i = 0; i < numWatchedFiles; ++i) {
        diagnostics += "    " + watchedFiles[i] + "\n";
    }

    diagnostics += "}\n";

    emit diagnosticsCollected(requestId, diagnostics);
}

void ResourceFileStorageManagerPrivate::onFileChanged(const QString & path)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onFileChanged: " << path);

    auto it = m_resourceLocalUidByFilePath.find(path);

    QFileInfo resourceFileInfo(path);
    if (!resourceFileInfo.exists())
    {
        if (it != m_resourceLocalUidByFilePath.end()) {
            Q_UNUSED(m_resourceLocalUidByFilePath.erase(it));
        }

        m_fileSystemWatcher.removePath(path);
        QNINFO("Stopped watching for file " << path << " as it was deleted");

        return;
    }

    if (Q_UNLIKELY(it == m_resourceLocalUidByFilePath.end())) {
        QNWARNING("Can't process resource local file change properly: can't find resource local uid by file path: " << path
                  << "; stopped watching for that file's changes");
        m_fileSystemWatcher.removePath(path);
        return;
    }

    QFile file(path);
    bool open = file.QIODevice::open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open)) {
        QNWARNING("Can't process resource local file change properly: can't open resource file for reading: error code = "
                  << file.error() << ", error description: " << file.errorString());
        m_fileSystemWatcher.removePath(path);
        return;
    }

    QByteArray data = file.readAll();
    QByteArray dataHash = calculateHash(data);

    int errorCode = 0;
    QString errorDescription;
    bool res = updateResourceHash(it.value(), dataHash, resourceFileInfo.absolutePath(), errorCode, errorDescription);
    if (Q_UNLIKELY(!res)) {
        QNWARNING("Can't process resource local file change properly: can't update the hash for resource file: error code = "
                  << errorCode << ", error description: " << errorDescription);
        m_fileSystemWatcher.removePath(path);
        return;
    }

    emit resourceFileChanged(it.value(), path);
}

void ResourceFileStorageManagerPrivate::onFileRemoved(const QString & path)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onFileRemoved: " << path);

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
    QObject::connect(this, QNSIGNAL(ResourceFileStorageManagerPrivate,readResourceFromFileCompleted,QUuid,QByteArray,QByteArray,int,QString),
                     q, QNSIGNAL(ResourceFileStorageManager,readResourceFromFileCompleted,QUuid,QByteArray,QByteArray,int,QString));
    QObject::connect(this, QNSIGNAL(ResourceFileStorageManagerPrivate,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString),
                     q, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString));
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
    QNDEBUG("ResourceFileStorageManagerPrivate::checkIfResourceFileExistsAndIsActual: note local uid = "
            << noteLocalUid << ", resource local uid = " << resourceLocalUid << ", data hash = " << dataHash.toHex());

    if (Q_UNLIKELY(fileStoragePath.isEmpty())) {
        QNWARNING("Resource file storage location is empty");
        return false;
    }

    QFileInfo resourceFileInfo(fileStoragePath);
    if (!resourceFileInfo.exists()) {
        QNTRACE("Resource file for note local uid " << noteLocalUid << " and resource local uid " << resourceLocalUid << " does not exist");
        return false;
    }

    QFileInfo resourceHashFileInfo(resourceFileInfo.absolutePath() + "/" + resourceFileInfo.baseName() + ".hash");
    if (!resourceHashFileInfo.exists()) {
        QNTRACE("Resource hash file for note local uid " << noteLocalUid << " and resource local uid " << resourceLocalUid << " does not exist");
        return false;
    }

    QFile resourceHashFile(resourceHashFileInfo.absoluteFilePath());
    bool open = resourceHashFile.open(QIODevice::ReadOnly);
    if (!open) {
        QNWARNING("Can't open resource hash file for reading");
        return false;
    }

    QByteArray storedHash = resourceHashFile.readAll();
    if (storedHash != dataHash) {
        QNTRACE("Resource must be stale, the stored hash " << storedHash.toHex()
                << " does not match the actual hash " << dataHash.toHex());
        return false;
    }

    QNDEBUG("Resource file exists and is actual");
    return true;
}

bool ResourceFileStorageManagerPrivate::updateResourceHash(const QString & resourceLocalUid, const QByteArray & dataHash,
                                                           const QString & storageFolderPath, int & errorCode, QString & errorDescription)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::updateResourceHash: resource local uid = " << resourceLocalUid
            << ", data hash = " << dataHash.toHex() << ", storage folder path = " << storageFolderPath);

    QFile file(storageFolderPath + "/" + resourceLocalUid + ".hash");

    bool open = file.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open)) {
        errorDescription = QT_TR_NOOP("Can't open the file with resource's hash for writing");
        errorDescription += ": " + file.errorString();
        errorCode = file.error();
        return false;
    }

    qint64 writeRes = file.write(dataHash);
    if (Q_UNLIKELY(writeRes < 0)) {
        errorDescription = QT_TR_NOOP("Can't write data hash to resource hash file");
        errorDescription += ": " + file.errorString();
        errorCode = file.error();
        return false;
    }

    file.close();
    return true;
}

void ResourceFileStorageManagerPrivate::watchResourceFileForChanges(const QString & resourceLocalUid, const QString & fileStoragePath)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::watchResourceFileForChanges: resource local uid = " << resourceLocalUid
            << ", file storage path = " << fileStoragePath);

    m_fileSystemWatcher.addPath(fileStoragePath);
    QNINFO("Start watching for resource file " << fileStoragePath);
}

void ResourceFileStorageManagerPrivate::stopWatchingResourceFile(const QString & filePath)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::stopWatchingResourceFile: " << filePath);

    auto it = m_resourceLocalUidByFilePath.find(filePath);
    if (it == m_resourceLocalUidByFilePath.end()) {
        QNTRACE("File is not being watched, nothing to do");
        return;
    }

    m_fileSystemWatcher.removePath(filePath);
    QNTRACE("Stopped watching for file");
}

void ResourceFileStorageManagerPrivate::removeStaleResourceFilesFromCurrentNote()
{
    QNDEBUG("ResourceFileStorageManagerPrivate::removeStaleResourceFilesFromCurrentNote");

    if (m_pCurrentNote.isNull()) {
        QNDEBUG("No current note, nothing to do");
        return;
    }

    const QString & noteLocalUid = m_pCurrentNote->localUid();

    QList<ResourceAdapter> resourceAdapters = m_pCurrentNote->resourceAdapters();
    const int numResources = resourceAdapters.size();

    QFileInfoList fileInfoList;
    int numFiles = -1;

    QDir imageResourceFilesFolder(m_imageResourceFileStorageLocation + "/" + m_pCurrentNote->localUid());
    if (imageResourceFilesFolder.exists())
    {
        fileInfoList = imageResourceFilesFolder.entryInfoList(QDir::Files);
        numFiles = fileInfoList.size();
        QNTRACE("Found " << numFiles << " files wihin the image resource files folder for note with local uid " << m_pCurrentNote->localUid());
    }

    QDir genericResourceImagesFolder(m_resourceFileStorageLocation + "/" + m_pCurrentNote->localUid());
    if (genericResourceImagesFolder.exists())
    {
        QFileInfoList genericResourceImageFileInfos = genericResourceImagesFolder.entryInfoList(QDir::Files);
        int numGenericResourceImageFileInfos = genericResourceImageFileInfos.size();
        QNTRACE("Found " << numGenericResourceImageFileInfos << " files within the generic resource files folder for note with local uid "
                << m_pCurrentNote->localUid());

        fileInfoList.append(genericResourceImageFileInfos);
        numFiles = fileInfoList.size();
    }

    QNTRACE("Total " << numFiles << " to check for staleness");

    for(int i = 0; i < numFiles; ++i)
    {
        const QFileInfo & fileInfo = fileInfoList[i];
        QString filePath = fileInfo.absoluteFilePath();

        if (fileInfo.isSymLink()) {
            QNTRACE("Removing symlink file without any checks");
            stopWatchingResourceFile(filePath);
            Q_UNUSED(removeFile(filePath))
            continue;
        }

        QString fullSuffix = fileInfo.completeSuffix();
        if (fullSuffix == QStringLiteral("hash")) {
            QNTRACE("Skipping .hash helper file " << filePath);
            continue;
        }

        QString baseName = fileInfo.baseName();
        QNTRACE("Checking file with base name " << baseName);

        int resourceIndex = -1;
        for(int j = 0; j < numResources; ++j)
        {
            QNTRACE("checking against resource with local uid " << resourceAdapters[j].localUid());
            if (baseName.startsWith(resourceAdapters[j].localUid())) {
                QNTRACE("File " << fileInfo.fileName() << " appears to correspond to resource "
                        << resourceAdapters[j].localUid());
                resourceIndex = j;
                break;
            }
        }

        if (resourceIndex >= 0)
        {
            const ResourceAdapter & resourceAdapter = resourceAdapters[resourceIndex];
            if (resourceAdapter.hasDataHash())
            {
                bool actual = checkIfResourceFileExistsAndIsActual(noteLocalUid, resourceAdapter.localUid(),
                                                                  filePath, resourceAdapter.dataHash());
                if (actual) {
                    QNTRACE("The resource file " << filePath << " is still actual, will keep it");
                    continue;
                }
            }
            else
            {
                QNTRACE("Resource at index " << resourceIndex << " doesn't have the data hash, will remove its resource file just in case");
            }
        }

        QNTRACE("Found stale resource file " << filePath << ", removing it");
        stopWatchingResourceFile(filePath);
        Q_UNUSED(removeFile(filePath))

        // Need to also remove the helper .hash file
        stopWatchingResourceFile(filePath);
        Q_UNUSED(removeFile(fileInfo.absolutePath() + "/" + baseName + ".hash"));
    }
}

} // namespace quentier
