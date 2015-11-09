#include "NoteEditorContentEditUndoCommand.h"
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

NoteEditorContentEditUndoCommand::NoteEditorContentEditUndoCommand(NoteEditor & noteEditor, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent)
{}

NoteEditorContentEditUndoCommand::NoteEditorContentEditUndoCommand(NoteEditor & noteEditor, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent)
{}

NoteEditorContentEditUndoCommand::~NoteEditorContentEditUndoCommand()
{}

void NoteEditorContentEditUndoCommand::redo()
{
    QNDEBUG("NoteEditorContentEditUndoCommand::redo (" << text() << ")");
    m_noteEditor.redo();
}

void NoteEditorContentEditUndoCommand::undo()
{
    QNDEBUG("NoteEditorContentEditUndoCommand::undo (" << text() << ")");
    m_noteEditor.undo();
}

} // namespace qute_note
