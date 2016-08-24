#ifndef LIB_QUENTIER_SYNCHRONIZATION_INK_NOTE_IMAGE_DOWNLOADER_H
#define LIB_QUENTIER_SYNCHRONIZATION_INK_NOTE_IMAGE_DOWNLOADER_H

#include <QObject>
#include <QRunnable>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/utility/Qt4Helper.h>

namespace quentier {

class InkNoteImageDownloader: public QObject,
                              public QRunnable
{
    Q_OBJECT
public:
    explicit InkNoteImageDownloader(const QString & host, const QString & resourceGuid,
                                    const QString & authToken, const QString & shardId,
                                    const int height, const int width,
                                    const bool noteFromPublicLinkedNotebook,
                                    QObject * parent = Q_NULLPTR);

    virtual void run() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void finished(bool status, QString inkNoteImageFilePath, QNLocalizedString errorDescription);

private:
    QString     m_host;
    QString     m_resourceGuid;
    QString     m_authToken;
    QString     m_shardId;
    int         m_height;
    int         m_width;
    bool        m_noteFromPublicLinkedNotebook;
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_INK_NOTE_IMAGE_DOWNLOADER_H
