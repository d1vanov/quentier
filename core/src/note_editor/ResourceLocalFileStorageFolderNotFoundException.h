#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H

#include <tools/IQuteNoteException.h>
#include <tools/Linkage.h>

namespace qute_note {

class QUTE_NOTE_EXPORT ResourceLocalFileStorageFolderNotFoundException: public IQuteNoteException
{
public:
    explicit ResourceLocalFileStorageFolderNotFoundException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H
