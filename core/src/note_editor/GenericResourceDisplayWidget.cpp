#include "GenericResourceDisplayWidget.h"
#include <logging/QuteNoteLogger.h>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QMimeDatabase>

namespace qute_note {

GenericResourceDisplayWidget::GenericResourceDisplayWidget(const QString & mimeType, const QUrl & url, const QStringList & argumentNames,
                                                           const QStringList & argumentValues, QWidget * parent) :
    QWidget(parent)
{
    QNDEBUG("GenericResourceDisplayWidget::GenericResourceDisplayWidget: mime type = " << mimeType
            << ", url = " << url.toString() << ", argument names = " << argumentNames.join(", ")
            << ", argument values = " << argumentValues.join(", "));

    QString filename;
    int filenameIndex = argumentNames.indexOf("filename");
    if (filenameIndex < 0) {
        QNWARNING("GenericResourceDisplayWidget constructor: can't find filename within argument names: "
                  << argumentNames);
        filename = "Unknown";
    }
    else {
        filename = argumentValues.at(filenameIndex);
    }

    QString filepath;
    int filepathIndex = argumentNames.indexOf("filepath");
    if (filepathIndex < 0) {
        QNWARNING("GenericResourceDisplayWidget constructor: can't find filepath within argument names: "
                  << argumentNames);
    }
    else {
        filepath = argumentValues.at(filepathIndex);
    }

    // TODO: compose the widget

    QFileInfo fileInfo(filepath);
    QIcon icon = getIconForFile(fileInfo);
    if (!icon.isNull())
    {
        // TODO: put it on the widget
    }
    else
    {
        // TODO: put the "unknown type" icon on the widget
    }

    if (fileInfo.exists() && fileInfo.isFile())
    {
        // TODO: put filename to widget

        qint64 bytes = fileInfo.size();
        QString humanReadableSize = humanReadableFileSize(bytes);
        Q_UNUSED(humanReadableSize);  // TODO: put it to widget

        // TODO: create "open with app" and "save as" img links
    }
    else
    {
        // TODO: put "unknown" filename to widget
    }
}

QIcon GenericResourceDisplayWidget::getIconForFile(const QFileInfo & fileInfo) const
{
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);
    if (!mimeType.isValid()) {
        QNINFO("Can't get mime type for file " << fileInfo.canonicalFilePath());
        return QIcon();
    }

    QString iconName = mimeType.iconName();
    if (!QIcon::hasThemeIcon(iconName))
    {
        QNTRACE("Couldn't find icon for mime type " << mimeType.name()
                << ", will try to find the generic icon instead");
        iconName = mimeType.genericIconName();
        if (!QIcon::hasThemeIcon(iconName)) {
            QNTRACE("Unable to find the generic icon for mime type "
                    << mimeType.name() << ", will return the icon returned by QFileIconProvider");
            QFileIconProvider iconProvider;
            return iconProvider.icon(fileInfo);
        }
    }

    return QIcon::fromTheme(iconName);
}

} // namespace qute_note
