#include "NoteThumbnailDownloader.h"
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

namespace quentier {

NoteThumbnailDownloader::NoteThumbnailDownloader(const QString & host, const QString & noteGuid,
                                                 const QString & authToken, const QString & shardId,
                                                 const bool noteFromPublicLinkedNotebook,
                                                 QObject * parent) :
    QObject(parent),
    m_host(host),
    m_noteGuid(noteGuid),
    m_authToken(authToken),
    m_shardId(shardId),
    m_noteFromPublicLinkedNotebook(noteFromPublicLinkedNotebook)
{}

void NoteThumbnailDownloader::run()
{
    QNDEBUG("NoteThumbnailDownloader::run: host = " << m_host << ", note guid = "
            << m_noteGuid << ", is public = " << (m_noteFromPublicLinkedNotebook ? "true" : "false"));

#define SET_ERROR(error) \
    QNLocalizedString errorDescription = QT_TR_NOOP(error); \
    emit finished(false, m_noteGuid, QString(), errorDescription); \
    return

    if (Q_UNLIKELY(m_host.isEmpty())) {
        SET_ERROR("host is empty");
    }

    if (Q_UNLIKELY(m_noteGuid.isEmpty())) {
        SET_ERROR("note guid is empty");
    }

    if (Q_UNLIKELY(m_shardId.isEmpty())) {
        SET_ERROR("shard id is empty");
    }

    if (Q_UNLIKELY(!m_noteFromPublicLinkedNotebook && m_authToken.isEmpty())) {
        SET_ERROR("authentication data is incomplete");
    }

    qevercloud::Thumbnail thumbnail(m_host, m_shardId, m_authToken);
    QByteArray thumbnailImageData = thumbnail.download(m_noteGuid, m_noteFromPublicLinkedNotebook,
                                                       /* is resource guid = */ false);
    if (Q_UNLIKELY(thumbnailImageData.isEmpty())) {
        SET_ERROR("received empty note thumbnail data");
    }

    QString folderPath = applicationPersistentStoragePath() + "/NoteEditorPage/thumbnails";
    QFileInfo folderPathInfo(folderPath);
    if (Q_UNLIKELY(!folderPathInfo.exists()))
    {
        QDir dir(folderPath);
        bool res = dir.mkpath(folderPath);
        if (Q_UNLIKELY(!res)) {
            SET_ERROR("can't create folder to store note thumbnails in");
        }
    }
    else if (Q_UNLIKELY(!folderPathInfo.isDir())) {
        SET_ERROR("can't create folder to store note thumbnails in: "
                  "a file with similar name and path exists");
    }
    else if (Q_UNLIKELY(!folderPathInfo.isWritable())) {
        SET_ERROR("folder to store note thumbnails in is not writable");
    }

    QString filePath = folderPath + "/" + m_noteGuid;
    QFile file(filePath);
    if (Q_UNLIKELY(!file.open(QIODevice::WriteOnly))) {
        SET_ERROR("can't open file to write the note thumbnail into");
    }

    file.write(thumbnailImageData);
    file.close();

    emit finished(true, m_noteGuid, filePath, QNLocalizedString());
}

} // namespace quentier
