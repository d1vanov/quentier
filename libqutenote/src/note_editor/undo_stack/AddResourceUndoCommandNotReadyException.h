#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ADD_RESOURCE_UNDO_COMMAND_NOT_READY_EXCEPTION_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ADD_RESOURCE_UNDO_COMMAND_NOT_READY_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class AddResourceUndoCommandNotReadyException: public IQuteNoteException
{
public:
    AddResourceUndoCommandNotReadyException();

    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ADD_RESOURCE_UNDO_COMMAND_NOT_READY_EXCEPTION_H
