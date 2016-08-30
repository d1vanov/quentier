#ifndef LIB_QUENTIER_SYNCHRONIZATION_NOTE_THUMBNAIL_DOWNLOADER_H
#define LIB_QUENTIER_SYNCHRONIZATION_NOTE_THUMBNAIL_DOWNLOADER_H

#include <QObject>
#include <QRunnable>
#include <QString>
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>

namespace quentier {

class NoteThumbnailDownloader: public QObject,
                               public QRunnable
{
    Q_OBJECT
public:
    explicit NoteThumbnailDownloader(const QString & host, const QString & noteGuid,
                                     const QString & authToken, const QString & shardId,
                                     const bool noteFromPublicLinkedNotebook,
                                     QObject * parent = Q_NULLPTR);

    virtual void run() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void finished(bool success, QString noteGuid, QString downloadedThumbnailImageFilePath, QNLocalizedString errorDescription);

private:
    QString     m_host;
    QString     m_noteGuid;
    QString     m_authToken;
    QString     m_shardId;
    bool        m_noteFromPublicLinkedNotebook;
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_NOTE_THUMBNAIL_DOWNLOADER_H
