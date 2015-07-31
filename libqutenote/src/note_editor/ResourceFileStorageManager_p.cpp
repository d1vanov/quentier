#include "ResourceFileStorageManager_p.h"
#include "ResourceFileStorageManager.h"
#include <qute_note/note_editor/AttachmentStoragePathConfigDialog.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/DesktopServices.h>
#include <QWidget>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

namespace qute_note {

ResourceFileStorageManagerPrivate::ResourceFileStorageManagerPrivate(ResourceFileStorageManager & manager) :
    QObject(&manager),
    m_resourceFileStorageLocation(),
    q_ptr(&manager)
{
    createConnections();
    m_resourceFileStorageLocation = resourceFileStorageLocation(nullptr);
}

QString ResourceFileStorageManagerPrivate::resourceFileStorageLocation(QWidget * context)
{
    QSettings settings;
    settings.beginGroup("AttachmentSaveLocations");
    QString resourceFileStorageLocation = settings.value("OwnFileStorageLocation").toString();
    if (resourceFileStorageLocation.isEmpty()) {
        resourceFileStorageLocation = applicationPersistentStoragePath() + "/" + "attachments";
        settings.setValue("OwnFileStorageLocation", QVariant(resourceFileStorageLocation));
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

        settings.setValue("OwnFileStorageLocation", QVariant(manualPath));
        resourceFileStorageLocation = manualPath;
    }

    return resourceFileStorageLocation;
}

void ResourceFileStorageManagerPrivate::onWriteResourceToFileRequest(QString localGuid,
                                                                     QByteArray data,
                                                                     QByteArray dataHash,
                                                                     QUuid requestId)
{
    QNDEBUG("ResourceFileStorageManagerPrivate::onWriteResourceToFileRequest: local guid = "
            << localGuid << ", request id = " << requestId << ", data hash = " << dataHash);

    if (Q_UNLIKELY(localGuid.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write data for empty resource local guid to local file");
        QNWARNING(errorDescription << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, ResourceFileStorageManager::EmptyLocalGuid, errorDescription);
        return;
    }

    if (Q_UNLIKELY(requestId.isNull())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write data for resource to local file with empty request id");
        QNWARNING(errorDescription << ", local guid = " << localGuid);
        emit writeResourceToFileCompleted(requestId, dataHash, ResourceFileStorageManager::EmptyRequestId, errorDescription);
        return;
    }

    if (Q_UNLIKELY(data.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to write empty resource data to local file");
        QNWARNING(errorDescription << ", local guid = " << localGuid);
        emit writeResourceToFileCompleted(requestId, dataHash, ResourceFileStorageManager::EmptyData, errorDescription);
        return;
    }

    if (Q_UNLIKELY(m_resourceFileStorageLocation.isEmpty())) {
        QString errorDescription = QT_TR_NOOP("Resource file storage location is empty");
        QNWARNING(errorDescription);
        emit writeResourceToFileCompleted(requestId, dataHash, ResourceFileStorageManager::NoResourceFileStorageLocation,
                                          errorDescription);
        return;
    }

    if (dataHash.isEmpty()) {
        dataHash = calculateHash(data);
        QNTRACE("Resource data hash was empty, calculated hash: " << dataHash);
    }

    bool actual = checkIfResourceFileExistsAndIsActual(localGuid, dataHash);
    if (actual) {
        QNTRACE("Skipping writing the resource to file as it is not necessary");
        emit writeResourceToFileCompleted(requestId, dataHash, 0, QString());
        return;
    }

    QFile file(m_resourceFileStorageLocation + "/" + localGuid);
    bool open = file.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open)) {
        QString errorDescription = QT_TR_NOOP("Can't open resource file for writing");
        errorDescription += ": " + file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, errorCode, errorDescription);
        return;
    }

    qint64 writeRes = file.write(data);
    if (Q_UNLIKELY(writeRes < 0)) {
        QString errorDescription = QT_TR_NOOP("Can't write data to resource file");
        errorDescription += ": " + file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, errorCode, errorDescription);
        return;
    }

    file.close();

    file.setFileName(m_resourceFileStorageLocation + "/" + localGuid + ".hash");

    open = file.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open)) {
        QString errorDescription = QT_TR_NOOP("Can't open the file with resource's hash for writing");
        errorDescription += ": " + file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, errorCode, errorDescription);
        return;
    }

    writeRes = file.write(dataHash);
    if (Q_UNLIKELY(writeRes < 0)) {
        QString errorDescription = QT_TR_NOOP("Can't write data hash to resource hash file");
        errorDescription += ": " + file.errorString();
        int errorCode = file.error();
        QNWARNING(errorDescription << ", error code = " << errorCode << ", local guid = "
                  << localGuid << ", request id = " << requestId);
        emit writeResourceToFileCompleted(requestId, dataHash, errorCode, errorDescription);
        return;
    }

    file.close();

    QNDEBUG("Successfully wrote resource data to file");
    emit writeResourceToFileCompleted(requestId, dataHash, 0, QString());
}

void ResourceFileStorageManagerPrivate::onReadResourceFromFileRequest(QString localGuid, QUuid requestId)
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

    QFile resourceFile(m_resourceFileStorageLocation + "/" + localGuid);
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
    QByteArray dataHash = resourceFile.readAll();

    QNDEBUG("Successfully read resource data and hash from files");
    emit readResourceFromFileCompleted(requestId, data, dataHash, 0, QString());
}

void ResourceFileStorageManagerPrivate::createConnections()
{
    Q_Q(ResourceFileStorageManager);

    QObject::connect(this, SIGNAL(readResourceFromFileCompleted(QUuid,QByteArray,QByteArray,int,QString)),
                     q, SIGNAL(readResourceFromFileCompleted(QUuid,QByteArray,QByteArray,int,QString)));
    QObject::connect(this, SIGNAL(writeResourceToFileCompleted(QUuid,QByteArray,int,QString)),
                     q, SIGNAL(writeResourceToFileCompleted(QUuid,QByteArray,int,QString)));
}

QByteArray ResourceFileStorageManagerPrivate::calculateHash(const QByteArray & data) const
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

bool ResourceFileStorageManagerPrivate::checkIfResourceFileExistsAndIsActual(const QString & localGuid,
                                                                             const QByteArray & dataHash) const
{
    QNDEBUG("ResourceFileStorageManagerPrivate::checkIfResourceFileExistsAndIsActual: local guid = "
            << localGuid << ", data hash = " << dataHash);

    if (Q_UNLIKELY(m_resourceFileStorageLocation.isEmpty())) {
        QNWARNING("Resource file storage location is empty");
        return false;
    }

    QFileInfo resourceFileInfo(m_resourceFileStorageLocation + "/" + localGuid);
    if (!resourceFileInfo.exists()) {
        QNTRACE("Resource file for local guid " << localGuid << " does not exist");
        return false;
    }

    QFileInfo resourceHashFileInfo(m_resourceFileStorageLocation + "/" + localGuid + ".hash");
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

} // namespace qute_note
