#include "INoteEditorUndoCommand.h"
#include <qute_note/note_editor/NoteEditor.h>

namespace qute_note {

INoteEditorUndoCommand::INoteEditorUndoCommand(NoteEditor & noteEditor, QUndoCommand * parent) :
    QUndoCommand(parent),
    m_noteEditor(noteEditor)
{}

INoteEditorUndoCommand::INoteEditorUndoCommand(NoteEditor & noteEditor, const QString & text, QUndoCommand * parent) :
    QUndoCommand(text, parent),
    m_noteEditor(noteEditor)
{}

INoteEditorUndoCommand::~INoteEditorUndoCommand()
{}

} // namespace qute_note
