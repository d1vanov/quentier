#include "NoteEditorContentEditUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

NoteEditorContentEditUndoCommand::NoteEditorContentEditUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent)
{}

NoteEditorContentEditUndoCommand::NoteEditorContentEditUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent)
{}

NoteEditorContentEditUndoCommand::~NoteEditorContentEditUndoCommand()
{}

void NoteEditorContentEditUndoCommand::redo()
{
    QNDEBUG("NoteEditorContentEditUndoCommand::redo (" << text() << ")");
    m_noteEditorPrivate.redoPageAction();
}

void NoteEditorContentEditUndoCommand::undo()
{
    QNDEBUG("NoteEditorContentEditUndoCommand::undo (" << text() << ")");
    m_noteEditorPrivate.undoPageAction();
}

} // namespace qute_note
