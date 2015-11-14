#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__DECRYPT_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__DECRYPT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "EncryptDecryptUndoCommandInfo.h"

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class DecryptUndoCommand: public INoteEditorUndoCommand
{
public:
    DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~DecryptUndoCommand();

    virtual void redo() Q_DECL_OVERRIDE;
    virtual void undo() Q_DECL_OVERRIDE;

private:
    void init();

private:
    EncryptDecryptUndoCommandInfo   m_info;
    DecryptedTextManager &          m_decryptedTextManager;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__DECRYPT_UNDO_COMMAND_H
