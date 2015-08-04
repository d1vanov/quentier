#ifndef __LIB_QUTE_NOTE__EXCEPTION__RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT ResourceLocalFileStorageFolderNotFoundException: public IQuteNoteException
{
public:
    explicit ResourceLocalFileStorageFolderNotFoundException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__EXCEPTION__RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H
