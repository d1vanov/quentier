#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__MIME_TYPE_ICON_JAVASCRIPT_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__MIME_TYPE_ICON_JAVASCRIPT_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QString>
#include <QHash>
#include <QSet>

QT_FORWARD_DECLARE_CLASS(QThread)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

/**
 * @brief The MimeTypeIconJavaScriptHandler class is used for communicating the paths
 * to mime type icons saved as local files somewhere in application's persistent storage folder
 * from C++ to JavaScript on requests coming from JavaScript to C++
 */
class MimeTypeIconJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit MimeTypeIconJavaScriptHandler(const QString & noteEditorPageFolderPath,
                                           QThread * ioThread, QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void notifyIconFilePathForMimeType(const QString & mimeType, const QString & filePath);

public Q_SLOTS:
    /**
     * @brief onIconFilePathForMimeTypeRequest - public slot called from JavaScript
     * to determine the path to the icon for given mime type. The method first searches
     * for icon file path in the cache, then in the folder where the icon should reside;
     * if it's not found, the method uses QMimeDatabase to find the icon, if successful,
     * schedules the async job of writing that icon to local file; when the icon is written
     * to file, emits the notification signal for JavaScript to receive and handle
     * @param mimeType - the mime type for which the icon is required
     */
    void onIconFilePathForMimeTypeRequest(const QString & mimeType);

// private signals
Q_SIGNALS:
    void writeIconToFile(QString absoluteFilePath, QByteArray data, QUuid requestId);

private Q_SLOTS:
    void onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

private:
    QString                 m_noteEditorPageFolder;
    QHash<QString, QString> m_iconFilePathCache;
    QHash<QUuid, QPair<QString, QString> >   m_mimeTypeAndLocalFilePathByWriteIconRequestId;
    QSet<QString>           m_mimeTypesWithIconsWriteInProgress;
    FileIOThreadWorker *    m_iconWriter;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__MIME_TYPE_ICON_JAVASCRIPT_HANDLER_H
