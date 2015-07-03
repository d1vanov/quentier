#include "ResourceLocalFileStorageFolderNotFoundException.h"

namespace qute_note {

ResourceLocalFileStorageFolderNotFoundException::ResourceLocalFileStorageFolderNotFoundException(const QString & message) :
    IQuteNoteException(message)
{}

const QString ResourceLocalFileStorageFolderNotFoundException::exceptionDisplayName() const
{
    return "ResourceLocalFileStorageFolderNotFoundException";
}

} // namespace qute_note
