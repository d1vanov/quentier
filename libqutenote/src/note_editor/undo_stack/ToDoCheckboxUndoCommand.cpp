#include "ToDoCheckboxUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ToDoCheckboxUndoCommand::ToDoCheckboxUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                                 QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent)
{}

ToDoCheckboxUndoCommand::ToDoCheckboxUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                                 const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent)
{}

ToDoCheckboxUndoCommand::~ToDoCheckboxUndoCommand()
{}

void ToDoCheckboxUndoCommand::redo()
{
    QNDEBUG("ToDoCheckboxUndoCommand::redo");
    // TODO: implement
}

void ToDoCheckboxUndoCommand::undo()
{
    QNDEBUG("ToDoCheckboxUndoCommand::undo");
    // TODO: implement
}

} // namespace qute_note
