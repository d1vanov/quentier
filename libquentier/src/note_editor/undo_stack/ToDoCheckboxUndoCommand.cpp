#include "ToDoCheckboxUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ToDoCheckboxUndoCommand::ToDoCheckboxUndoCommand(const quint64 enToDoCheckboxId, NoteEditorPrivate & noteEditorPrivate,
                                                 QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_enToDoCheckboxId(enToDoCheckboxId)
{
    setText(tr("Change ToDo state"));
}

ToDoCheckboxUndoCommand::ToDoCheckboxUndoCommand(const quint64 enToDoCheckboxId, NoteEditorPrivate & noteEditorPrivate,
                                                 const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_enToDoCheckboxId(enToDoCheckboxId)
{}

ToDoCheckboxUndoCommand::~ToDoCheckboxUndoCommand()
{}

void ToDoCheckboxUndoCommand::redoImpl()
{
    QNDEBUG("ToDoCheckboxUndoCommand::redoImpl");
    m_noteEditorPrivate.flipEnToDoCheckboxState(m_enToDoCheckboxId);
}

void ToDoCheckboxUndoCommand::undoImpl()
{
    QNDEBUG("ToDoCheckboxUndoCommand::undoImpl");
    m_noteEditorPrivate.flipEnToDoCheckboxState(m_enToDoCheckboxId);
}

} // namespace quentier
