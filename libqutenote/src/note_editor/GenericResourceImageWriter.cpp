#include "GenericResourceImageWriter.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

namespace qute_note {

GenericResourceImageWriter::GenericResourceImageWriter(QObject * parent) :
    QObject(parent),
    m_storageFolderPath()
{}

void GenericResourceImageWriter::setStorageFolderPath(const QString & storageFolderPath)
{
    QNDEBUG("GenericResourceImageWriter::setStorageFolderPath: " << storageFolderPath);
    m_storageFolderPath = storageFolderPath;
}

void GenericResourceImageWriter::onGenericResourceImageWriteRequest(const QString resourceLocalGuid, const QByteArray resourceImageData,
                                                                    const QString resourceFileSuffix, const QByteArray resourceActualHash,
                                                                    const QString resourceDisplayName, const QUuid requestId)
{
    QNDEBUG("GenericResourceImageWriter::onGenericResourceImageWriteRequest: resource local guid = " << resourceLocalGuid
            << ", resource actual hash = " << resourceActualHash << ", request id = " << requestId);

#define RETURN_WITH_ERROR(message) \
    QString errorDescription = QT_TR_NOOP(message); \
    QNWARNING(errorDescription); \
    emit genericResourceImageWriteReply(/* success = */ false, QByteArray(), QString(), \
                                        errorDescription, requestId); \
    return

    if (Q_UNLIKELY(m_storageFolderPath.isEmpty())) {
        RETURN_WITH_ERROR("storage folder path is empty");
    }

    if (Q_UNLIKELY(resourceLocalGuid.isEmpty())) {
        RETURN_WITH_ERROR("resource local guid is empty");
    }

    if (Q_UNLIKELY(resourceActualHash.isEmpty())) {
        RETURN_WITH_ERROR("resource hash is empty");
    }

    if (Q_UNLIKELY(resourceFileSuffix.isEmpty())) {
        RETURN_WITH_ERROR("resource image file suffix is empty");
    }

    QString resourceFileNameMask = resourceLocalGuid + "*." + resourceFileSuffix;
    QDir storageDir(m_storageFolderPath);
    QStringList nameFilter = QStringList() << resourceFileNameMask;
    QFileInfoList existingResourceImageFileInfos = storageDir.entryInfoList(nameFilter, QDir::Files | QDir::Readable);

    bool resourceHashChanged = true;
    QFileInfo resourceHashFileInfo(m_storageFolderPath + "/" + resourceLocalGuid + ".hash");
    bool resourceHashFileExists = resourceHashFileInfo.exists();
    if (resourceHashFileExists)
    {
        if (Q_UNLIKELY(!resourceHashFileInfo.isWritable())) {
            RETURN_WITH_ERROR("resource hash file is not writable");
        }

        if (resourceHashFileInfo.isReadable())
        {
            QFile resourceHashFile(resourceHashFileInfo.absoluteFilePath());
            Q_UNUSED(resourceHashFile.open(QIODevice::ReadOnly));
            QByteArray previousResourceHash = resourceHashFile.readAll();

            if (resourceActualHash == previousResourceHash) {
                QNTRACE("Resource hash hasn't changed");
                resourceHashChanged = false;
            }
        }
    }

    bool resourceDisplayNameChanged = false;
    QFileInfo resourceNameFileInfo(m_storageFolderPath + "/" + resourceLocalGuid + ".name");
    if (!resourceHashChanged)
    {
        bool resourceNameFileExists = resourceNameFileInfo.exists();
        if (resourceNameFileExists)
        {
            if (Q_UNLIKELY(!resourceNameFileInfo.isWritable())) {
                RETURN_WITH_ERROR("resource name file is not writable");
            }

            if (Q_UNLIKELY(!resourceNameFileInfo.isReadable()))
            {
                QNINFO("Helper file with resource name for generic resource image is not readable: " +
                       resourceNameFileInfo.absoluteFilePath() + " which is quite strange...");
                resourceDisplayNameChanged = true;
            }
            else
            {
                QFile resourceNameFile(resourceNameFileInfo.absoluteFilePath());
                Q_UNUSED(resourceNameFile.open(QIODevice::ReadOnly));
                QString previousResourceName = resourceNameFile.readAll();

                if (resourceDisplayName != previousResourceName) {
                    QNTRACE("Resource display name has changed from " << previousResourceName
                            << " to " << resourceDisplayName);
                    resourceDisplayNameChanged = true;
                }
            }
        }
    }

    if (!resourceHashChanged && !resourceDisplayNameChanged && !existingResourceImageFileInfos.isEmpty())
    {
        QNDEBUG("resource hash and display name haven't changed, won't rewrite the resource's image");
        emit genericResourceImageWriteReply(/* success = */ true, resourceActualHash,
                                            existingResourceImageFileInfos.front().absoluteFilePath(),
                                            QString(), requestId);
        return;
    }

    QNTRACE("Writing resource image file and helper files with hash and display name");

    QDir resourceImageDir(m_storageFolderPath);
    if (Q_UNLIKELY(!resourceImageDir.exists()))
    {
        bool res = resourceImageDir.mkpath(m_storageFolderPath);
        if (!res) {
            RETURN_WITH_ERROR("can't create folder to store generic resource image files");
        }
    }

    QString resourceImageFilePath = m_storageFolderPath + "/" + resourceLocalGuid + "_" +
                                    QString::number(QDateTime::currentMSecsSinceEpoch()) + "." + resourceFileSuffix;

    QFile resourceImageFile(resourceImageFilePath);
    if (Q_UNLIKELY(!resourceImageFile.open(QIODevice::ReadWrite))) {
        RETURN_WITH_ERROR("can't open resource image file for writing");
    }
    resourceImageFile.write(resourceImageData);
    resourceImageFile.close();

    QFile resourceHashFile(resourceHashFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(!resourceHashFile.open(QIODevice::ReadWrite))) {
        RETURN_WITH_ERROR("can't open resource hash file for writing");
    }
    resourceHashFile.write(resourceActualHash);
    resourceHashFile.close();

    QFile resourceNameFile(resourceNameFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(!resourceNameFile.open(QIODevice::ReadWrite))) {
        RETURN_WITH_ERROR("can't open resource name file for writing");
    }
    resourceNameFile.write(resourceDisplayName.toLocal8Bit());
    resourceNameFile.close();

    QNTRACE("Successfully wrote resource image file and helper files with hash and display name for request " << requestId);
    emit genericResourceImageWriteReply(/* success = */ true, resourceActualHash,
                                        resourceImageFilePath, QString(), requestId);

    if (!existingResourceImageFileInfos.isEmpty())
    {
        const int numStaleResourceImageFiles = existingResourceImageFileInfos.size();
        for(int i = 0; i < numStaleResourceImageFiles; ++i)
        {
            QFileInfo & staleResourceImageFileInfo = existingResourceImageFileInfos[i];
            QFile staleResourceImageFile(staleResourceImageFileInfo.absoluteFilePath());
            bool res = staleResourceImageFile.remove();
            if (Q_UNLIKELY(!res)) {
                QNINFO("Can't remove stale generic resource image file: " << staleResourceImageFile.errorString()
                       << " (error code = " << staleResourceImageFile.error() << ")");
            }
        }
    }
}

} // namespace qute_note
