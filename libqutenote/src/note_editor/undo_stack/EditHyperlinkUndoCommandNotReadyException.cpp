#include "EditHyperlinkUndoCommandNotReadyException.h"

namespace qute_note {

EditHyperlinkUndoCommandNotReadyException::EditHyperlinkUndoCommandNotReadyException() :
    IQuteNoteException("Can't undo/redo edit hyperlink command: not ready yet")
{}

const QString EditHyperlinkUndoCommandNotReadyException::exceptionDisplayName() const
{
    return QString("EditHyperlinkUndoCommandNotReadyException");
}

} // namespace qute_note
