#ifndef __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H
#define __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H

#include "Linkage.h"
#include <QString>
#include <QStyle>

namespace qute_note {

const QString QUTE_NOTE_EXPORT GetApplicationPersistentStoragePath();
const QString QUTE_NOTE_EXPORT GetTemporaryStoragePath();
QUTE_NOTE_EXPORT QStyle * GetApplicationStyle();

} // namespace qute_note

#endif // __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H
