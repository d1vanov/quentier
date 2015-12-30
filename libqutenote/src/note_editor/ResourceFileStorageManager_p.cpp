#include "ResourceFileStorageManager_p.h"
#include "dialogs/AttachmentStoragePathConfigDialog.h"
#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/types/Note.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/DesktopServices.h>
#include <qute_note/utility/ApplicationSettings.h>
#include <QWidget>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>

namespace qute_note {

ResourceFileStorageManagerPrivate::ResourceFileStorageManagerPrivate(ResourceFileStorageManager & manager) :
    QObject(&manager),
    m_resourceFileStorageLocation(),
    m_resourceLocalGuidByFilePath(),
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

void ResourceFileStorageManagerPrivate::onWriteResourceToFileRequest(QString localGuid, QByteArray data,
                                                                     QByteArray dataHash, QString fileStoragePath,
                                                                     QUuid requestId)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onWriteResourceToFileRequest: local guid = "
            << localGuid << ", request id = " << requestId << ", data hash = " << dataHash
            << ", file storage path = " << fileStoragePath);

    if (Q_UNLIKELY(localGuid.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write data for empty resource local guid to local file");
        QNWARNING(errorDescription << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath,
                                          ResourceFileStorageManager::EmptyLocalGuid, errorDescription);
        return;
    }

    if (Q_UNLIKELY(requestId.isNull())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write data for resource to local file with empty request id");
        QNWARNING(errorDescription << ", local guid = " << localGuid);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath,
                                          ResourceFileStorageManager::EmptyRequestId, errorDescription);
        return;
    }

    if (Q_UNLIKELY(data.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write empty resource data to local file");
        QNWARNING(errorDescription << ", local guid = " << localGuid);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath,
                                          ResourceFileStorageManager::EmptyData, errorDescription);
        return;
    }

    if (fileStoragePath.isEmpty())
    {
        if (Q_UNLIKELY(m_resourceFileStorageLocation.isEmpty())) {
            QString errorDescription = QT_TR_NOOP("Can't automatically choose resource file storage location");
            QNWARNING(errorDescription);
            emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath,
                                              ResourceFileStorageManager::NoResourceFileStorageLocation,
                                              errorDescription);
            return;
        }

        fileStoragePath = m_resourceFileStorageLocation + "/" + localGuid;
    }
    else
    {
        QFileInfo fileStoragePathInfo(fileStoragePath);
        QDir fileStorageDir(fileStoragePathInfo.absoluteDir());
        if (!fileStorageDir.exists())
        {
            bool createDir = fileStorageDir.mkpath(fileStorageDir.absolutePath());
            if (!createDir) {
                int errorCode = -1;
                QString errorDescription = QT_TR_NOOP("Can't create folder to write the resource into");
                QNWARNING(errorDescription << ", local guid = " << localGuid << ", request id = " << requestId);
                emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
                return;
            }
        }
    }

    if (dataHash.isEmpty()) {
        dataHash = calculateHash(data);
        QNTRACE("Resource data hash was empty, calculated hash: " << dataHash);
    }

    bool actual = checkIfResourceFileExistsAndIsActual(localGuid, fileStoragePath, dataHash);
    if (actual) {
        QNTRACE("Skipping writing the resource to file as it is not necessary");
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, 0, QString());
        return;
    }

    QFile file(fileStoragePath);
    bool open = file.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open)) {
        QString errorDescription = QT_TR_NOOP("Can't open resource file for writing");
        errorDescription += ": " + file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        return;
    }

    qint64 writeRes = file.write(data);
    if (Q_UNLIKELY(writeRes < 0)) {
        QString errorDescription = QT_TR_NOOP("Can't write data to resource file");
        errorDescription += ": " + file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        return;
    }

    file.close();

    m_resourceLocalGuidByFilePath[fileStoragePath] = localGuid;

    int errorCode = 0;
    QString errorDescription;
    bool res = updateResourceHash(localGuid, dataHash, errorCode, errorDescription);
    if (Q_UNLIKELY(!res)) {
        emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, errorCode, errorDescription);
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
        return;
    }

    QNDEBUG("Successfully wrote resource data to file: resource local guid = " << localGuid << ", file path = " << fileStoragePath);
    emit writeResourceToFileCompleted(requestId, dataHash, fileStoragePath, 0, QString());
}

void ResourceFileStorageManagerPrivate::onReadResourceFromFileRequest(QString fileStoragePath, QString localGuid, QUuid requestId)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onReadResourceFromFileRequest: local guid = " << localGuid
            << ", request id = " << requestId);

    if (Q_UNLIKELY(m_resourceFileStorageLocation.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Resource file storage location is empty");
        QNWARNING(errorDescription << ", local guid = " << localGuid << ", request id = " << requestId);
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
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
        emit readResourceFromFileCompleted(requestId, QByteArray(), QByteArray(),
                                           errorCode, errorDescription);
        return;
    }

    QFile resourceHashFile(m_resourceFileStorageLocation + "/" + localGuid + ".hash");
    open = resourceHashFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open)) {
        QString errorDescription = QT_TR_NOOP("Can't open resource hash file for reading: ");
        errorDescription += ": " + resourceHashFile.errorString();
        int errorCode = resourceHashFile.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
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

    auto it = m_resourceLocalGuidByFilePath.find(fileStoragePath);
    if (Q_UNLIKELY(it == m_resourceLocalGuidByFilePath.end())) {
        QNWARNING("Can't set up watching for resource file's changes: can't find resource local guid for file path: " + fileStoragePath);
    }
    else {
        watchResourceFileForChanges(it.value(), fileStoragePath);
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(fileStoragePath));
}

void ResourceFileStorageManagerPrivate::onCurrentNoteChanged(Note note)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onCurrentNoteChanged");

    Q_UNUSED(note)

    for(auto it = m_resourceLocalGuidByFilePath.begin(), end = m_resourceLocalGuidByFilePath.end(); it != end; ++it) {
        m_fileSystemWatcher.removePath(it.key());
        QNTRACE("Stopped watching for file " << it.key());
    }

    m_resourceLocalGuidByFilePath.clear();
}

void ResourceFileStorageManagerPrivate::onRequestDiagnostics(QUuid requestId)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onRequestDiagnostics: request id = " << requestId);

    QString diagnostics = "ResourceFileStorageManager diagnostics: {\n";

    diagnostics += "  Resource local guids by file paths: \n";
    for(auto it = m_resourceLocalGuidByFilePath.begin(), end = m_resourceLocalGuidByFilePath.end(); it != end; ++it) {
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

    auto it = m_resourceLocalGuidByFilePath.find(path);

    QFileInfo resourceFileInfo(path);
    if (!resourceFileInfo.exists())
    {
        if (it != m_resourceLocalGuidByFilePath.end()) {
            Q_UNUSED(m_resourceLocalGuidByFilePath.erase(it));
        }

        m_fileSystemWatcher.removePath(path);
        QNINFO("Stopped watching for file " << path << " as it was deleted");

        return;
    }

    if (Q_UNLIKELY(it == m_resourceLocalGuidByFilePath.end())) {
        QNWARNING("Can't process resource local file change properly: can't find resource local guid by file path: " << path
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
    bool res = updateResourceHash(it.value(), dataHash, errorCode, errorDescription);
    if (Q_UNLIKELY(!res)) {
        QNWARNING("Can't process resource local file change properly: can't update the hash for resource file: error code = "
                  << errorCode << ", error description: " << errorDescription);
        m_fileSystemWatcher.removePath(path);
        return;
    }

    emit resourceFileChanged(it.value(), path);
}

void ResourceFileStorageManagerPrivate::createConnections()
{
    QObject::connect(&m_fileSystemWatcher, QNSIGNAL(QFileSystemWatcher,fileChanged,QString),
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
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

bool ResourceFileStorageManagerPrivate::checkIfResourceFileExistsAndIsActual(const QString & localGuid,
                                                                             const QString & fileStoragePath,
                                                                             const QByteArray & dataHash) const
{
    QNDEBUG("ResourceFileStorageManagerPrivate::checkIfResourceFileExistsAndIsActual: local guid = "
            << localGuid << ", data hash = " << dataHash);

    if (Q_UNLIKELY(fileStoragePath.isEmpty())) {
        QNWARNING("Resource file storage location is empty");
        return false;
    }

    QFileInfo resourceFileInfo(fileStoragePath);
    if (!resourceFileInfo.exists()) {
        QNTRACE("Resource file for local guid " << localGuid << " does not exist");
        return false;
    }

    QFileInfo resourceHashFileInfo(fileStoragePath + ".hash");
    if (!resourceHashFileInfo.exists()) {
        QNTRACE("Resource hash file for local guid " << localGuid << " does not exist");
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
        QNTRACE("Resource must be stale, the stored hash " << storedHash
                << " does not match the actual hash " << dataHash);
        return false;
    }

    QNDEBUG("Resource file exists and is actual");
    return true;
}

bool ResourceFileStorageManagerPrivate::updateResourceHash(const QString & resourceLocalGuid, const QByteArray & dataHash,
                                                           int & errorCode, QString & errorDescription)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::updateResourceHash: resource local guid = " << resourceLocalGuid
            << ", data hash = " << dataHash);

    QFile file(m_resourceFileStorageLocation + "/" + resourceLocalGuid + ".hash");

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

void ResourceFileStorageManagerPrivate::watchResourceFileForChanges(const QString & resourceLocalGuid, const QString & fileStoragePath)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::watchResourceFileForChanges: resource local guid = " << resourceLocalGuid
            << ", file storage path = " << fileStoragePath);

    m_fileSystemWatcher.addPath(fileStoragePath);
    QNINFO("Start watching for resource file " << fileStoragePath);
}

} // namespace qute_note
