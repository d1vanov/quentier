#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_TO_DO_CHECKBOX_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_TO_DO_CHECKBOX_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace quentier {

class ToDoCheckboxUndoCommand: public INoteEditorUndoCommand
{
    Q_OBJECT
public:
    ToDoCheckboxUndoCommand(const quint64 enToDoCheckboxId, NoteEditorPrivate & noteEditorPrivate,
                            QUndoCommand * parent = Q_NULLPTR);
    ToDoCheckboxUndoCommand(const quint64 enToDoCheckboxId, NoteEditorPrivate & noteEditorPrivate, const QString & text,
                            QUndoCommand * parent = Q_NULLPTR);
    virtual ~ToDoCheckboxUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    quint64     m_enToDoCheckboxId;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_TO_DO_CHECKBOX_UNDO_COMMAND_H
