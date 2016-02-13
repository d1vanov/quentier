#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H

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

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
