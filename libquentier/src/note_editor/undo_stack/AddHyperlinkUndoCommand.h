#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ADD_HYPERLINK_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ADD_HYPERLINK_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace quentier {

class AddHyperlinkUndoCommand: public INoteEditorUndoCommand
{
    typedef NoteEditorPage::Callback Callback;
public:
    AddHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback, QUndoCommand * parent = Q_NULLPTR);
    AddHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~AddHyperlinkUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    Callback    m_callback;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ADD_HYPERLINK_UNDO_COMMAND_H
