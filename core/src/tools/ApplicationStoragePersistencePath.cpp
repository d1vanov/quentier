#include "ApplicationStoragePersistencePath.h"

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace qute_note {

const QString GetApplicationPersistentStoragePath()
{
#if QT_VERSION >= 0x050000
    return std::move(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
#else
    return std::move(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
#endif
}

}
