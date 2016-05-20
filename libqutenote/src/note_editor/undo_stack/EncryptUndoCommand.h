#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ENCRYPT_UNDO_COMMAND_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ENCRYPT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace qute_note {

class EncryptUndoCommand: public INoteEditorUndoCommand
{
    typedef NoteEditorPage::Callback Callback;
public:
    EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback,
                       QUndoCommand * parent = Q_NULLPTR);
    EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback,
                       const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~EncryptUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    Callback    m_callback;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ENCRYPT_UNDO_COMMAND_H
