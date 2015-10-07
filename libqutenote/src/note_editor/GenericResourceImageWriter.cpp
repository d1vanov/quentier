#include "GenericResourceImageWriter.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QFile>
#include <QFileInfo>

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
                                                                    const QString resourceFileSuffix, const QString resourceActualHash,
                                                                    const QUuid requestId)
{
    QNDEBUG("GenericResourceImageWriter::onGenericResourceImageWriteRequest: resource local guid = " << resourceLocalGuid
            << ", resource actual hash = " << resourceActualHash << ", request id = " << requestId);

#define RETURN_WITH_ERROR(message) \
    QString errorDescription = QT_TR_NOOP(message); \
    QNWARNING(errorDescription); \
    emit genericResourceImageWriteReply(/* success = */ false, errorDescription, requestId); \
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

    QFileInfo resourceImageFileInfo(m_storageFolderPath + "/" + resourceLocalGuid + "." + resourceFileSuffix);
    bool resourceImageFileExists = resourceImageFileInfo.exists();
    if (resourceImageFileExists && Q_UNLIKELY(!resourceImageFileInfo.isWritable())) {
        RETURN_WITH_ERROR("resource image file is not writable");
    }

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
            QString previousResourceHash = resourceHashFile.readAll();

            if (resourceActualHash == previousResourceHash) {
                QNDEBUG("resource hash hasn't changed, won't rewrite the resource's image");
                emit genericResourceImageWriteReply(/* success = */ true, QString(), requestId);
                return;
            }
        }
    }

    QNTRACE("Writing both resource image file and resource hash file");

    QFile resourceImageFile(resourceImageFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(!resourceImageFile.open(QIODevice::ReadWrite))) {
        RETURN_WITH_ERROR("can't open resource image file for writing");
    }
    resourceImageFile.write(resourceImageData);
    resourceImageFile.close();

    QFile resourceHashFile(resourceHashFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(!resourceHashFile.open(QIODevice::ReadWrite))) {
        RETURN_WITH_ERROR("can't open resource hash file for writing");
    }
    resourceHashFile.write(resourceActualHash.toLocal8Bit());
    resourceHashFile.close();

    QNTRACE("Successfully wrote both resource image file and resource hash file for request " << requestId);
    emit genericResourceImageWriteReply(/* success = */ true, QString(), requestId);
}

} // namespace qute_note
