#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "EncryptDecryptUndoCommandInfo.h"

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class EncryptUndoCommand: public INoteEditorUndoCommand
{
public:
    EncryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    EncryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~EncryptUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    void init();

private:
    EncryptDecryptUndoCommandInfo  m_info;
    DecryptedTextManager &  m_decryptedTextManager;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
