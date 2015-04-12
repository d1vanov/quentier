#ifndef __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H
#define __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H

#include "Linkage.h"
#include <QString>

QT_FORWARD_DECLARE_CLASS(QStyle)

namespace qute_note {

const QString QUTE_NOTE_EXPORT GetApplicationPersistentStoragePath();
const QString QUTE_NOTE_EXPORT GetTemporaryStoragePath();
QStyle * QUTE_NOTE_EXPORT GetApplicationStyle();

} // namespace qute_note

#endif // __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H
