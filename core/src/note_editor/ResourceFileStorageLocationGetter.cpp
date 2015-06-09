#include "ResourceFileStorageLocationGetter.h"
#include "AttachmentStoragePathConfigDialog.h"
#include <logging/QuteNoteLogger.h>
#include <tools/DesktopServices.h>
#include <QWidget>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

namespace qute_note {

QString getResourceFileStorageLocation(QWidget * parent)
{
    QSettings settings;
    settings.beginGroup("AttachmentSaveLocations");
    QString resourceFileStorageLocation = settings.value("OwnFileStorageLocation").toString();
    if (resourceFileStorageLocation.isEmpty()) {
        resourceFileStorageLocation = applicationPersistentStoragePath() + "/" + "attachments";
        settings.setValue("OwnFileStorageLocation", QVariant(resourceFileStorageLocation));
    }

    QFileInfo resourceFileStorageLocationInfo(resourceFileStorageLocation);
    QString manualPath;
    if (!resourceFileStorageLocationInfo.exists())
    {
        QDir resourceFileStorageLocationDir(resourceFileStorageLocation);
        bool res = resourceFileStorageLocationDir.mkpath(resourceFileStorageLocation);
        if (!res)
        {
            QNWARNING("Can't create directory for attachment tmp storage location: "
                      << resourceFileStorageLocation);
            manualPath = getAttachmentStoragePath(parent);
        }
    }
    else if (!resourceFileStorageLocationInfo.isDir())
    {
        QNWARNING("Can't figure out where to save the temporary copies of attachments: path "
                  << resourceFileStorageLocation << " is not a directory");
        manualPath = getAttachmentStoragePath(parent);
    }
    else if (resourceFileStorageLocationInfo.isWritable())
    {
        QNWARNING("Can't save temporary copies of attachments: the suggested folder is not writable: "
                  << resourceFileStorageLocation);
        manualPath = getAttachmentStoragePath(parent);
    }

    if (!manualPath.isEmpty())
    {
        resourceFileStorageLocationInfo.setFile(manualPath);
        if (!resourceFileStorageLocationInfo.exists() || !resourceFileStorageLocationInfo.isDir() ||
                !resourceFileStorageLocationInfo.isWritable())
        {
            QNWARNING("Can't use manually selected attachment storage path");
            return QString();
        }

        settings.setValue("OwnFileStorageLocation", QVariant(manualPath));
        resourceFileStorageLocation = manualPath;
    }

    return resourceFileStorageLocation;
}

} // namespace qute_note
