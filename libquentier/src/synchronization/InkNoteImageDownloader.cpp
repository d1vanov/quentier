#include "InkNoteImageDownloader.h"
#include <quentier/logging/QuentierLogger.h>
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
    QNDEBUG("InkNoteImageDownloader::run: host = " << m_host << ", note guid = "
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

    QString url = QStringLiteral("https://") + m_host + QStringLiteral("/shardId/") +
                  m_shardId + QStringLiteral("/res/") + m_resourceGuid + QStringLiteral(".ink?slice=");
    int numSlices = (m_height - 1) / 600 + 1;

    // QSize inkNoteImageSize(m_width, m_height);

    QByteArray postData = "";
    if (!m_noteFromPublicLinkedNotebook) {
        postData = QByteArray("auth=")+ QUrl::toPercentEncoding(m_authToken);
    }

    QScopedPointer<QImage> pInkNoteImage;
    for(int i = 0; i < numSlices; ++i)
    {
        // Download each slice of the ink note's resource image

        QNetworkRequest request;
        request.setUrl(QUrl(url + QString::number(i+1)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));

        // TODO: continue
    }

    // TODO: continue
}

} // namespace quentier
