#include "RemoveResourceUndoCommandNotReadyException.h"

namespace qute_note {

RemoveResourceUndoCommandNotReadyException::RemoveResourceUndoCommandNotReadyException() :
    IQuteNoteException("Can't undo/redo remove resource command: not ready yet")
{}

const QString RemoveResourceUndoCommandNotReadyException::exceptionDisplayName() const
{
    return QString("RemoveResourceUndoCommandNotReadyException");
}

} // namespace qute_note
