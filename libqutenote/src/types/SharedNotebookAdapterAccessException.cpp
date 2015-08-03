#include <qute_note/types/SharedNotebookAdapterAccessException.h>

namespace qute_note {

SharedNotebookAdapterAccessException::SharedNotebookAdapterAccessException(const QString & message) :
    IQuteNoteException(message)
{}

const QString SharedNotebookAdapterAccessException::exceptionDisplayName() const
{
    return "SharedNotebookAdapterAccessException";
}

} // namespace qute_note
