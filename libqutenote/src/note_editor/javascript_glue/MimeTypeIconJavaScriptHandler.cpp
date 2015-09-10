#include "MimeTypeIconJavaScriptHandler.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QFileInfo>
#include <QImage>
#include <QMimeDatabase>
#include <QMimeType>
#include <QBuffer>
#include <QThread>

namespace qute_note {

MimeTypeIconJavaScriptHandler::MimeTypeIconJavaScriptHandler(const QString & noteEditorPageFolderPath,
                                                             QThread * ioThread, QObject * parent) :
    QObject(parent),
    m_noteEditorPageFolder(noteEditorPageFolderPath),
    m_iconFilePathCache(),
    m_mimeTypeAndLocalFilePathByWriteIconRequestId(),
    m_mimeTypesWithIconsWriteInProgress(),
    m_iconWriter(Q_NULLPTR)
{
    Q_ASSERT(ioThread);
    m_iconWriter = new FileIOThreadWorker;
    m_iconWriter->moveToThread(ioThread);

    QObject::connect(this, QNSIGNAL(MimeTypeIconJavaScriptHandler,writeIconToFile,QString,QByteArray,QUuid),
                     m_iconWriter, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));
    QObject::connect(m_iconWriter, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(MimeTypeIconJavaScriptHandler,onWriteFileRequestProcessed,bool,QString,QUuid));

    QNDEBUG("Initialized MimeTypeIconJavaScriptHandler");
}

void MimeTypeIconJavaScriptHandler::iconFilePathForMimeType(const QString & mimeType)
{
    QNDEBUG("MimeTypeIconJavaScriptHandler::iconFilePathForMimeType: " << mimeType);

    // First try to find it in the cache
    auto it = m_iconFilePathCache.find(mimeType);
    if (it != m_iconFilePathCache.end()) {
        QNTRACE("Found cached icon for mime type " << mimeType << ": " << it.value());
        emit gotIconFilePathForMimeType(mimeType, it.value());
        return;
    }

    // Ok, try to find the already written icon in the appropriate folder
    QString iconsFolderPath = m_noteEditorPageFolder + "/mimeTypeIcons";

    QString normalizedMimeType = mimeType;
    normalizedMimeType.replace('/', '_');

    QString iconFilePath = iconsFolderPath + "/" + normalizedMimeType + ".png";
    QFileInfo mimeTypeIconInfo(iconFilePath);
    if (mimeTypeIconInfo.exists() && mimeTypeIconInfo.isFile() && mimeTypeIconInfo.isReadable()) {
        QNTRACE("Found existing icon written to file: " << iconFilePath);
        QString relativeIconFilePath = relativePath(iconFilePath);
        m_iconFilePathCache[mimeType] = relativeIconFilePath;
        emit gotIconFilePathForMimeType(mimeType, relativeIconFilePath);
        return;
    }

    // If we haven't returned just above, we'd most likely need to return this fallback generic resource icon
    // before we get the proper one written to the local file
    QString fallbackIconFilePath = "qrc:/generic_resource_icons/png/attachment.png";

    // If it's not there, see if we are already in the process of writing this icon to local file
    auto mimeTypeIconWriteInProgressIter = m_mimeTypesWithIconsWriteInProgress.find(mimeType);
    if (mimeTypeIconWriteInProgressIter != m_mimeTypesWithIconsWriteInProgress.end()) {
        QNTRACE("Writing icon for mime type " << mimeType << " is still in progress");
        emit gotIconFilePathForMimeType(mimeType, fallbackIconFilePath);
        return;
    }

    // If it's not there, try to find the icon from theme; if found, schedule the async job
    // of writing that icon to local file
    QMimeDatabase mimeDatabase;
    QMimeType type = mimeDatabase.mimeTypeForName(mimeType);
    QString iconName = type.iconName();
    if (iconName.isEmpty()) {
        iconName = type.genericIconName();
    }

    QIcon icon = QIcon::fromTheme(iconName);
    if (icon.isNull()) {
        QNTRACE("Haven't found the icon corresponding to mime type " << mimeType
                << ", will use the default icon instead");
        m_iconFilePathCache[mimeType] = fallbackIconFilePath;
        emit gotIconFilePathForMimeType(mimeType, fallbackIconFilePath);
        return;
    }

    QImage iconImage = icon.pixmap(QSize(24, 24)).toImage();

    QByteArray iconRawData;
    QBuffer buffer(&iconRawData);
    buffer.open(QIODevice::WriteOnly);
    iconImage.save(&buffer, "png");

    QUuid writeIconRequestId = QUuid::createUuid();
    m_mimeTypeAndLocalFilePathByWriteIconRequestId[writeIconRequestId] = QPair<QString,QString>(mimeType, iconFilePath);
    Q_UNUSED(m_mimeTypesWithIconsWriteInProgress.insert(mimeType));
    emit writeIconToFile(iconFilePath, iconRawData, writeIconRequestId);
    QNTRACE("Emitted a signal to save the icon for mime type " << mimeType
            << " to local file with path " << iconFilePath << ", request id = "
            << writeIconRequestId);
}

void MimeTypeIconJavaScriptHandler::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    auto it = m_mimeTypeAndLocalFilePathByWriteIconRequestId.find(requestId);
    if (it == m_mimeTypeAndLocalFilePathByWriteIconRequestId.end()) {
        return;
    }

    QNDEBUG("MimeTypeIconJavaScriptHandler::onWriteFileRequestProcessed: success = "
            << (success ? "true" : "false") << ", error description = " << errorDescription
            << ", request id = " << requestId);

    QPair<QString,QString> mimeTypeAndFilePath = it.value();
    Q_UNUSED(m_mimeTypeAndLocalFilePathByWriteIconRequestId.erase(it));

    auto mimeTypeIconWriteInProgressIter = m_mimeTypesWithIconsWriteInProgress.find(mimeTypeAndFilePath.first);
    if (mimeTypeIconWriteInProgressIter != m_mimeTypesWithIconsWriteInProgress.end()) {
        Q_UNUSED(m_mimeTypesWithIconsWriteInProgress.erase(mimeTypeIconWriteInProgressIter));
    }

    if (!success) {
        QNWARNING("Can't save resource icon for mime type " << mimeTypeAndFilePath.first
                  << " to local file: " << errorDescription);
        QString fallbackIconFilePath = "qrc:/generic_resource_icons/png/attachment.png";
        m_iconFilePathCache[mimeTypeAndFilePath.first] = fallbackIconFilePath;
        emit gotIconFilePathForMimeType(mimeTypeAndFilePath.first, fallbackIconFilePath);
        return;
    }

    QString relativeIconFilePath = relativePath(mimeTypeAndFilePath.second);
    m_iconFilePathCache[mimeTypeAndFilePath.first] = relativeIconFilePath;
    QNTRACE("Emitting the signal to update icon file path for mime type " << mimeTypeAndFilePath.first);
    emit gotIconFilePathForMimeType(mimeTypeAndFilePath.first, relativeIconFilePath);
}

QString MimeTypeIconJavaScriptHandler::relativePath(const QString & absolutePath) const
{
    int position = absolutePath.indexOf("NoteEditorPage", 0, Qt::CaseInsensitive);
    if (position < 0) {
        return QString();
    }

    return absolutePath.mid(position + 15);
}
} // namespace qute_note
