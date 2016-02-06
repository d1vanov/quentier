#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__TABLE_ACTION_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__TABLE_ACTION_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace qute_note {

class TableActionUndoCommand: public INoteEditorUndoCommand
{
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

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__TABLE_ACTION_UNDO_COMMAND_H
