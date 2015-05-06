#include "GenericResourceDisplayWidget.h"
#include <client/types/IResource.h>
#include <logging/QuteNoteLogger.h>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QDir>

namespace qute_note {

GenericResourceDisplayWidget::GenericResourceDisplayWidget(const IResource & resource,
                                                           const QMimeDatabase & mimeDatabase,
                                                           QCache<QString, QIcon> & iconCache,
                                                           QWidget * parent) :
    QWidget(parent),
    m_resource(nullptr)
{
    QNDEBUG("GenericResourceDisplayWidget::GenericResourceDisplayWidget: resource = " << resource);

    QString mimeTypeName;
    if (resource.hasMime()) {
        mimeTypeName = resource.mime();
    }

    QIcon * pCachedIcon = iconCache.object(mimeTypeName);
    if (!pCachedIcon) {
        QNTRACE("Couldn't find cached icon for mime type " << mimeTypeName);
        QIcon cachedIcon = getIconForMimeType(mimeTypeName, mimeDatabase);
        iconCache.insert(mimeTypeName, &cachedIcon);
        pCachedIcon = &cachedIcon;
    }

    QString resourceSize;
    if (resource.hasDataSize()) {
        resourceSize = humanReadableSize(resource.dataSize());
    }

    QString resourceName;
    if (resource.hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = resource.resourceAttributes();
        if (attributes.fileName.isSet()) {
            resourceName = attributes.fileName;
        }
        else if (attributes.sourceURL.isSet()) {
            resourceName = attributes.sourceURL;
        }
        else {
            resourceName = "Resource";
        }
    }

    m_resource = &resource;

    // TODO: compose the widget
    // TODO: put the icon on the widget
    // TODO: put the resource name and size on the widget
    // TODO: put "open with" link on the widget
    // TODO: put "save as" link on the widget
}

QIcon GenericResourceDisplayWidget::getIconForMimeType(const QString & mimeTypeName,
                                                       const QMimeDatabase & mimeDatabase) const
{
    QNDEBUG("GenericResourceDisplayWidget::getIconForMimeType: mime type = " << mimeTypeName);

    QIcon fallbackUnknownIcon = QIcon::fromTheme("unknown");
    if (fallbackUnknownIcon.isNull()) {
        fallbackUnknownIcon.addFile(":/fallback_icons/png/16x16/unknown.png", QSize(16, 16));
        fallbackUnknownIcon.addFile(":/fallback_icons/png/22x22/unknown.png", QSize(22, 22));
        fallbackUnknownIcon.addFile(":/fallback_icons/png/32x32/unknown.png", QSize(32, 32));
        fallbackUnknownIcon.addFile(":/fallback_icons/png/48x48/unknown.png", QSize(48, 48));
    }

    QMimeType mimeType = mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName
                << ", will use \"unknown\" icon");
        return fallbackUnknownIcon;
    }

    QString iconName = mimeType.iconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE("Found icon from theme, name = " << iconName);
        return QIcon::fromTheme(iconName, fallbackUnknownIcon);
    }

    iconName = mimeType.genericIconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE("Found generic icon from theme, name = " << iconName);
        return QIcon::fromTheme(iconName, fallbackUnknownIcon);
    }

    const QStringList suffixes = mimeType.suffixes();
    const int numSuffixes = suffixes.size();
    if (numSuffixes == 0) {
        QNDEBUG("Can't find any file suffix for mime type " << mimeTypeName << ", will use \"unknown\" icon");
        return fallbackUnknownIcon;
    }

    bool hasNonEmptySuffix = false;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            QNTRACE("Found empty file suffix within suffixes, skipping it");
            continue;
        }

        hasNonEmptySuffix = true;
        break;
    }

    if (!hasNonEmptySuffix) {
        QNDEBUG("All file suffixes for mime type " << mimeTypeName << " are empty, will use \"unknown\" icon");
        return fallbackUnknownIcon;
    }

    QString fakeFilesStoragePath = applicationPersistentStoragePath();
    fakeFilesStoragePath.append("/fake_files");

    QDir fakeFilesDir(fakeFilesStoragePath);
    if (!fakeFilesDir.exists())
    {
        QNDEBUG("Fake files storage path doesn't exist yet, will attempt to create it");
        if (!fakeFilesDir.mkpath(fakeFilesStoragePath)) {
            QNWARNING("Can't create fake files storage path folder");
            return fallbackUnknownIcon;
        }
    }

    QString filename("fake_file");
    QFileInfo fileInfo;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            continue;
        }

        fileInfo.setFile(fakeFilesDir, filename + "." + suffix);
        if (fileInfo.exists() && !fileInfo.isFile())
        {
            if (!fakeFilesDir.rmpath(fakeFilesStoragePath + "/" + filename + "." + suffix)) {
                QNWARNING("Can't remove directory " << fileInfo.absolutePath()
                          << " which should not be here in the first place...");
                continue;
            }
        }

        if (!fileInfo.exists())
        {
            QFile fakeFile(fakeFilesStoragePath + "/" + filename + "." + suffix);
            if (!fakeFile.open(QIODevice::ReadWrite)) {
                QNWARNING("Can't open file " << fakeFilesStoragePath << "/" << filename
                          << "." << suffix << " for writing ");
                continue;
            }
        }

        QFileIconProvider fileIconProvider;
        QIcon icon = fileIconProvider.icon(fileInfo);
        if (icon.isNull()) {
            QNTRACE("File icon provider returned null icon for file with suffix " << suffix);
        }

        QNTRACE("Returning the icon from file icon provider for mime type " << mimeTypeName);
        return icon;
    }

    QNTRACE("Couldn't find appropriate icon from either icon theme or fake file with QFileIconProvider, "
            "using \"unknown\" icon as a last resort");
    return fallbackUnknownIcon;
}

} // namespace qute_note
