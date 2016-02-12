#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_HYPERLINK_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_HYPERLINK_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace qute_note {

class RemoveHyperlinkUndoCommand: public INoteEditorUndoCommand
{
    typedef NoteEditorPage::Callback Callback;
public:
    RemoveHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback, QUndoCommand * parent = Q_NULLPTR);
    RemoveHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback, const QString & text,
                               QUndoCommand * parent = Q_NULLPTR);
    virtual ~RemoveHyperlinkUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    Callback    m_callback;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_HYPERLINK_UNDO_COMMAND_H
