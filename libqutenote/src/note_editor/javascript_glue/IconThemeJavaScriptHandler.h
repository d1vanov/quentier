#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__ICON_THEME_JAVASCRIPT_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__ICON_THEME_JAVASCRIPT_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QString>
#include <QHash>
#include <QSet>
#include <QSize>

QT_FORWARD_DECLARE_CLASS(QThread)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

class IconThemeJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit IconThemeJavaScriptHandler(const QString & noteEditorPageFolder,
                                        QThread * ioThread, QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void notifyIconFilePathForIconThemeName(const QString & iconThemeName, const QString & filePath);

public Q_SLOTS:
    /**
     * @brief onIconFilePathForIconThemeNameRequest - public slot called from JavaScript
     * to determine the path to icon identified by its name in the icon theme and the desired size;
     * the method first looks up in the cache for icon with this theme name and size, then
     * in the folder where the icon should reside; if the path to the icon file is still not found,
     * the method schedules the async job of writing the icon to local file
     * @param iconThemeName - the name of the icon within the icon theme, for example, document-open
     * @param height - the desired height of the icon
     * @param width - the desired width of the icon
     */
    void onIconFilePathForIconThemeNameRequest(const QString & iconThemeName, const int height, const int width);

// private signals
Q_SIGNALS:
    void writeIconToFile(QString absoluteFilePath, QByteArray data, QUuid requestId);

private Q_SLOTS:
    void onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

private:
    const QString fallbackIconFilePathForThemeName(const QString & iconThemeName) const;
    const QString sizeToString(const QSize & size) const;

private:
    QString                 m_noteEditorPageFolder;
    QHash<QString, QString> m_iconFilePathCache;
    QHash<QUuid, QPair<QString, QString> >   m_iconThemeNameAndLocalFilePathByWriteIconRequestId;
    QSet<QString>           m_iconThemeNamesWithIconsWriteInProgress;
    FileIOThreadWorker *    m_iconWriter;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__ICON_THEME_JAVASCRIPT_HANDLER_H
