#include "INoteEditorUndoCommand.h"
#include "../NoteEditor_p.h"

namespace quentier {

INoteEditorUndoCommand::INoteEditorUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    QuentierUndoCommand(parent),
    m_noteEditorPrivate(noteEditorPrivate)
{}

INoteEditorUndoCommand::INoteEditorUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    QuentierUndoCommand(text, parent),
    m_noteEditorPrivate(noteEditorPrivate)
{}

INoteEditorUndoCommand::~INoteEditorUndoCommand()
{}

} // namespace quentier
