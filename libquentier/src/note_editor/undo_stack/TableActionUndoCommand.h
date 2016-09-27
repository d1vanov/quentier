#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_TABLE_ACTION_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_TABLE_ACTION_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace quentier {

class TableActionUndoCommand: public INoteEditorUndoCommand
{
    Q_OBJECT
    typedef NoteEditorPage::Callback Callback;
public:
    TableActionUndoCommand(NoteEditorPrivate & noteEditorPrivate, Callback callback, QUndoCommand * parent = Q_NULLPTR);
    TableActionUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, Callback callback, QUndoCommand * parent = Q_NULLPTR);
    virtual ~TableActionUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    Callback    m_callback;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_TABLE_ACTION_UNDO_COMMAND_H
