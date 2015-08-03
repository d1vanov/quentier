#include <qute_note/types/ResourceAdapterAccessException.h>

namespace qute_note {

ResourceAdapterAccessException::ResourceAdapterAccessException(const QString & message) :
    IQuteNoteException(message)
{}

const QString ResourceAdapterAccessException::exceptionDisplayName() const
{
    return QString("ResourceAdapterAccessException");
}

} // namespace qute_note
