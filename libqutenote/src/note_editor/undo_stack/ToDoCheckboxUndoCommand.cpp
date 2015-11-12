#include "ToDoCheckboxUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ToDoCheckboxUndoCommand::ToDoCheckboxUndoCommand(const quint64 enToDoIdNumber, NoteEditorPrivate & noteEditorPrivate,
                                                 QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_enToDoIdNumber(enToDoIdNumber)
{}

ToDoCheckboxUndoCommand::ToDoCheckboxUndoCommand(const quint64 enToDoIdNumber, NoteEditorPrivate & noteEditorPrivate,
                                                 const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_enToDoIdNumber(enToDoIdNumber)
{}

ToDoCheckboxUndoCommand::~ToDoCheckboxUndoCommand()
{}

void ToDoCheckboxUndoCommand::redo()
{
    QNDEBUG("ToDoCheckboxUndoCommand::redo");
    m_noteEditorPrivate.flipEnToDoCheckboxState(m_enToDoIdNumber);
}

void ToDoCheckboxUndoCommand::undo()
{
    QNDEBUG("ToDoCheckboxUndoCommand::undo");
    m_noteEditorPrivate.flipEnToDoCheckboxState(m_enToDoIdNumber);
}

} // namespace qute_note
