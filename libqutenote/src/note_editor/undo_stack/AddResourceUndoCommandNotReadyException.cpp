#include "AddResourceUndoCommandNotReadyException.h"

namespace qute_note {

AddResourceUndoCommandNotReadyException::AddResourceUndoCommandNotReadyException() :
    IQuteNoteException("Can't undo/redo add resource command: not ready yet")
{}

const QString AddResourceUndoCommandNotReadyException::exceptionDisplayName() const
{
    return QString("AddResourceUndoCommandNotReadyException");
}

} // namespace qute_note
