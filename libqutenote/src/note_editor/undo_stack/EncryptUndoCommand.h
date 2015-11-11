#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class EncryptUndoCommand: public INoteEditorUndoCommand
{
public:
    struct EncryptUndoCommandInfo
    {
        QString     m_encryptedText;
        QString     m_decryptedText;
        QString     m_passphrase;
        QString     m_cipher;
        size_t      m_keyLength;
        bool        m_rememberForSession;
        bool        m_decryptPermanently;
    };

    EncryptUndoCommand(const EncryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    EncryptUndoCommand(const EncryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~EncryptUndoCommand();

    virtual void redo() Q_DECL_OVERRIDE;
    virtual void undo() Q_DECL_OVERRIDE;

private:
    EncryptUndoCommandInfo  m_info;
    DecryptedTextManager &  m_decryptedTextManager;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
