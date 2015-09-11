#include "IconThemeJavaScriptHandler.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/DesktopServices.h>
#include <QThread>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QBuffer>

namespace qute_note {

IconThemeJavaScriptHandler::IconThemeJavaScriptHandler(const QString & noteEditorPageFolder,
                                                       QThread * ioThread, QObject * parent) :
    QObject(parent),
    m_noteEditorPageFolder(noteEditorPageFolder),
    m_iconFilePathCache(),
    m_iconThemeNameAndLocalFilePathByWriteIconRequestId(),
    m_iconThemeNamesWithIconsWriteInProgress(),
    m_iconWriter(Q_NULLPTR)
{
    Q_ASSERT(ioThread);
    m_iconWriter = new FileIOThreadWorker;
    m_iconWriter->moveToThread(ioThread);

    QObject::connect(this, QNSIGNAL(IconThemeJavaScriptHandler,writeIconToFile,QString,QByteArray,QUuid),
                     m_iconWriter, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));
    QObject::connect(m_iconWriter, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(IconThemeJavaScriptHandler,onWriteFileRequestProcessed,bool,QString,QUuid));

    QNDEBUG("Initialized IconThemeJavaScriptHandler");
}

void IconThemeJavaScriptHandler::onIconFilePathForIconThemeNameRequest(const QString & iconThemeName,
                                                                       const int height, const int width)
{
    QSize size(height, width);
    QString sizeString = sizeToString(size);

    QNDEBUG("IconThemeJavaScriptHandler::onIconFilePathForIconThemeNameRequest: "
            << iconThemeName << ", size: " << sizeString);

    // First try to find it in the cache
    auto it = m_iconFilePathCache.find(iconThemeName);
    if (it != m_iconFilePathCache.end()) {
        QNTRACE("Found cached icon for theme name " << iconThemeName << ": " << it.value());
        emit notifyIconFilePathForIconThemeName(iconThemeName, it.value());
        return;
    }

    // Ok, try to find the already written icon in the appropriate folder
    QString iconsFolderPath = m_noteEditorPageFolder + "/cachedThemeIcons";

    QString iconFilePath = iconsFolderPath + "/" + iconThemeName + "_" + sizeString + ".png";

    QFileInfo iconFileInfo(iconFilePath);
    if (iconFileInfo.exists() && iconFileInfo.isFile() && iconFileInfo.isReadable()) {
        QNTRACE("Found existing icon written to file: " << iconFilePath);
        QString relativeIconFilePath = relativePathFromAbsolutePath(iconFilePath, "NoteEditorPage");
        m_iconFilePathCache[iconThemeName] = relativeIconFilePath;
        emit notifyIconFilePathForIconThemeName(iconThemeName, relativeIconFilePath);
        return;
    }

    // See if the icon for this theme name is still being written to file
    auto iconWriteInProgressIter = m_iconThemeNamesWithIconsWriteInProgress.find(iconThemeName);
    if (iconWriteInProgressIter != m_iconThemeNamesWithIconsWriteInProgress.end()) {
        QNTRACE("Writing icon for theme name " << iconThemeName << " is still in progress");
        return;
    }

    // If it's not there' try to find the icon from theme; if found, schedule the async job
    // of writing that icon to local file
    QIcon icon = QIcon::fromTheme(iconThemeName);
    if (icon.isNull())
    {
        QNDEBUG("Haven't found the icon corresponding to theme name " << iconThemeName);

        QString fallbackIconFilePath = fallbackIconFilePathForThemeName(iconThemeName);
        if (!fallbackIconFilePath.isEmpty()) {
            QNTRACE("Will use fallback icon: " << fallbackIconFilePath);
            m_iconFilePathCache[iconThemeName] = fallbackIconFilePath;
            emit notifyIconFilePathForIconThemeName(iconThemeName, fallbackIconFilePath);
        }

        return;
    }

    QImage iconImage = icon.pixmap(height).toImage();

    QByteArray iconRawData;
    QBuffer buffer(&iconRawData);
    buffer.open(QIODevice::WriteOnly);
    iconImage.save(&buffer, "png");

    QUuid writeIconRequestId = QUuid::createUuid();
    m_iconThemeNameAndLocalFilePathByWriteIconRequestId[writeIconRequestId] = QPair<QString,QString>(iconThemeName, iconFilePath);
    Q_UNUSED(m_iconThemeNamesWithIconsWriteInProgress.insert(iconThemeName));
    emit writeIconToFile(iconFilePath, iconRawData, writeIconRequestId);
    QNTRACE("Emitted a signal to save the icon for theme name " << iconThemeName
            << " to local file with path " << iconFilePath << ", request id = "
            << writeIconRequestId);
}

void IconThemeJavaScriptHandler::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    auto it = m_iconThemeNameAndLocalFilePathByWriteIconRequestId.find(requestId);
    if (it == m_iconThemeNameAndLocalFilePathByWriteIconRequestId.end()) {
        return;
    }

    QNDEBUG("IconThemeJavaScriptHandler::onWriteFileRequestProcessed: success = "
            << (success ? "true" : "false") << ", error description = " << errorDescription
            << ", request id = " << requestId);

    QPair<QString,QString> iconThemeNameAndFilePath = it.value();
    Q_UNUSED(m_iconThemeNameAndLocalFilePathByWriteIconRequestId.erase(it));

    auto iconWriteInProgressIter = m_iconThemeNamesWithIconsWriteInProgress.find(iconThemeNameAndFilePath.first);
    if (iconWriteInProgressIter != m_iconThemeNamesWithIconsWriteInProgress.end()) {
        Q_UNUSED(m_iconThemeNamesWithIconsWriteInProgress.erase(iconWriteInProgressIter));
    }

    if (!success)
    {
        QNWARNING("Can't save icon for theme name " << iconThemeNameAndFilePath.first
                  << " to local file: " << errorDescription);

        QString fallbackIconFilePath = fallbackIconFilePathForThemeName(iconThemeNameAndFilePath.first);
        if (!fallbackIconFilePath.isEmpty()) {
            QNTRACE("Will use fallback icon: " << fallbackIconFilePath);
            m_iconFilePathCache[iconThemeNameAndFilePath.first] = fallbackIconFilePath;
            emit notifyIconFilePathForIconThemeName(iconThemeNameAndFilePath.first, fallbackIconFilePath);
        }

        return;
    }

    QString relativeIconFilePath = relativePathFromAbsolutePath(iconThemeNameAndFilePath.second, "NoteEditorPage");
    m_iconFilePathCache[iconThemeNameAndFilePath.first] = relativeIconFilePath;
    QNTRACE("Emitting the signal to update the icon file path for icon theme name " << iconThemeNameAndFilePath.first);
    emit notifyIconFilePathForIconThemeName(iconThemeNameAndFilePath.first, relativeIconFilePath);
}

const QString IconThemeJavaScriptHandler::fallbackIconFilePathForThemeName(const QString & iconThemeName) const
{
    QString fallbackIconFilePath;

    // See if there are any hardcoded fallback alternatives
    if (iconThemeName == "document-open") {
        fallbackIconFilePath = "qrc:/generic_resource_icons/png/open_with.png";
    }
    else if (iconThemeName == "document-save-as") {
        fallbackIconFilePath = "qrc:/generic_resource_icons/png/save.png";
    }

    return fallbackIconFilePath;
}

const QString IconThemeJavaScriptHandler::sizeToString(const QSize & size) const
{
    if (size.isEmpty()) {
        return "<empty>";
    }

    if (size.isNull()) {
        return "<null>";
    }

    if (!size.isValid()) {
        return "<invalid>";
    }

    return QString::number(size.height()) + "x" + QString::number(size.width());
}

} // namespace qute_note
