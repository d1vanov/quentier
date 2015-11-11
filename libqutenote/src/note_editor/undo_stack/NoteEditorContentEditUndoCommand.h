#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__NOTE_EDITOR_CONTENT_EDIT_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__NOTE_EDITOR_CONTENT_EDIT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class NoteEditorContentEditUndoCommand: public INoteEditorUndoCommand
{
public:
    NoteEditorContentEditUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    NoteEditorContentEditUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~NoteEditorContentEditUndoCommand();

    virtual void redo() Q_DECL_OVERRIDE;
    virtual void undo() Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__NOTE_EDITOR_CONTENT_EDIT_UNDO_COMMAND_H
