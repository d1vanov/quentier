#include "INoteEditorUndoCommand.h"
#include "../NoteEditor_p.h"

namespace qute_note {

INoteEditorUndoCommand::INoteEditorUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    QuteNoteUndoCommand(parent),
    m_noteEditorPrivate(noteEditorPrivate),
    m_ready(true)
{}

INoteEditorUndoCommand::INoteEditorUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    QuteNoteUndoCommand(text, parent),
    m_noteEditorPrivate(noteEditorPrivate),
    m_ready(true)
{}

INoteEditorUndoCommand::~INoteEditorUndoCommand()
{}

} // namespace qute_note
