#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_FILE_STORAGE_LOCATION_GETTER_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_FILE_STORAGE_LOCATION_GETTER_H

#include <tools/Linkage.h>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace qute_note {

QString QUTE_NOTE_EXPORT getResourceFileStorageLocation(QWidget * parent);

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_FILE_STORAGE_LOCATION_GETTER_H
