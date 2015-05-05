#ifndef __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H
#define __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H

#include "Linkage.h"
#include <QString>
#include <QStyle>

namespace qute_note {

const QString QUTE_NOTE_EXPORT applicationPersistentStoragePath();
const QString QUTE_NOTE_EXPORT applicationTemporaryStoragePath();
QUTE_NOTE_EXPORT QStyle * applicationStyle();
const QString QUTE_NOTE_EXPORT humanReadableFileSize(const qint64 bytes);

} // namespace qute_note

#endif // __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H
