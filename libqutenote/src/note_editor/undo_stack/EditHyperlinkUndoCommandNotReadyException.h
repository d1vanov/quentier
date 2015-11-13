#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_NOT_READY_EXCEPTION_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_NOT_READY_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class EditHyperlinkUndoCommandNotReadyException: public IQuteNoteException
{
public:
    EditHyperlinkUndoCommandNotReadyException();

    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_NOT_READY_EXCEPTION_H
