#include "InkNoteImageDownloader.h"
#include <quentier/utility/DesktopServices.h>
#include <quentier/logging/QuentierLogger.h>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <qt4qevercloud/thumbnail.h>
#else
#include <qt5qevercloud/thumbnail.h>
#endif

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QNetworkRequest>
#include <QUrl>
#include <QScopedPointer>
#include <QImage>

namespace quentier {

InkNoteImageDownloader::InkNoteImageDownloader(const QString & host, const QString & resourceGuid,
                                               const QString & authToken, const QString & shardId,
                                               const int height, const int width,
                                               const bool noteFromPublicLinkedNotebook,
                                               QObject * parent) :
    QObject(parent),
    m_host(host),
    m_resourceGuid(resourceGuid),
    m_authToken(authToken),
    m_shardId(shardId),
    m_height(height),
    m_width(width),
    m_noteFromPublicLinkedNotebook(noteFromPublicLinkedNotebook)
{}

void InkNoteImageDownloader::run()
{
    QNDEBUG("InkNoteImageDownloader::run: host = " << m_host << ", resource guid = "
            << m_resourceGuid);

#define SET_ERROR(error) \
    QNLocalizedString errorDescription = QT_TR_NOOP(error); \
    emit finished(false, QString(), errorDescription); \
    return

    if (Q_UNLIKELY(m_host.isEmpty())) {
        SET_ERROR("host is empty");
    }

    if (Q_UNLIKELY(m_resourceGuid.isEmpty())) {
        SET_ERROR("resource guid is empty");
    }

    if (Q_UNLIKELY(m_shardId.isEmpty())) {
        SET_ERROR("shard id is empty");
    }

    if (Q_UNLIKELY(!m_noteFromPublicLinkedNotebook && m_authToken.isEmpty())) {
        SET_ERROR("authentication data is incomplete");
    }

    qevercloud::InkNoteImageDownloader downloader(m_host, m_authToken, m_shardId, m_height, m_width);
    QByteArray inkNoteImageData = downloader.download(m_resourceGuid, m_noteFromPublicLinkedNotebook);
    if (Q_UNLIKELY(inkNoteImageData.isEmpty())) {
        SET_ERROR("received empty note thumbnail data");
    }

    QString folderPath = applicationPersistentStoragePath() + "/NoteEditorPage/inkNoteImages";
    QFileInfo folderPathInfo(folderPath);
    if (Q_UNLIKELY(!folderPathInfo.exists()))
    {
        QDir dir(folderPath);
        bool res = dir.mkpath(folderPath);
        if (Q_UNLIKELY(!res)) {
            SET_ERROR("can't create folder to store ink note images in");
        }
    }
    else if (Q_UNLIKELY(!folderPathInfo.isDir())) {
        SET_ERROR("can't create folder to store ink note images in: "
                  "a file with similar name and path exists");
    }
    else if (Q_UNLIKELY(!folderPathInfo.isWritable())) {
        SET_ERROR("folder to store ink note images in is not writable");
    }

    QString filePath = folderPath + "/" + m_resourceGuid;
    QFile file(filePath);
    if (Q_UNLIKELY(!file.open(QIODevice::WriteOnly))) {
        SET_ERROR("can't open file to write the ink note images into");
    }

    file.write(inkNoteImageData);
    file.close();

    emit finished(true, filePath, QNLocalizedString());
}

} // namespace quentier
