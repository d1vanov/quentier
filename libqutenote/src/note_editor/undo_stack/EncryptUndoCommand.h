#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)
QT_FORWARD_DECLARE_CLASS(EncryptionManager)

class EncryptUndoCommand: public INoteEditorUndoCommand
{
public:
    struct EncryptUndoCommandInfo
    {
        QString     m_encryptedText;
        QString     m_cipher;
        size_t      m_keyLength;
        bool        m_rememberForSession;

        QString     m_enmlWithEncryption;   // ENML after the encryption, for "redo" command
    };

    EncryptUndoCommand(const EncryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       EncryptionManager & encryptionManager, NoteEditor & noteEditor, QUndoCommand * parent = Q_NULLPTR);
    EncryptUndoCommand(const EncryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       EncryptionManager & encryptionManager, NoteEditor & noteEditor, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~EncryptUndoCommand();

    virtual void redo() Q_DECL_OVERRIDE;
    virtual void undo() Q_DECL_OVERRIDE;

private:
    EncryptUndoCommandInfo  m_info;
    DecryptedTextManager &  m_decryptedTextManager;
    EncryptionManager &     m_encryptionManager;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
