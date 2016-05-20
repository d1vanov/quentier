#ifndef LIB_QUTE_NOTE_EXCEPTION_RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H
#define LIB_QUTE_NOTE_EXCEPTION_RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT ResourceLocalFileStorageFolderNotFoundException: public IQuteNoteException
{
public:
    explicit ResourceLocalFileStorageFolderNotFoundException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_EXCEPTION_RESOURCE_LOCAL_FILE_STORAGE_FOLDER_NOT_FOUND_EXCEPTION_H
