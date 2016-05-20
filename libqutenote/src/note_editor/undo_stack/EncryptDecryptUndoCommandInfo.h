#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ENCRYPT_DECRYPT_UNDO_COMMAND_INFO_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ENCRYPT_DECRYPT_UNDO_COMMAND_INFO_H

#include <QString>

namespace qute_note {

struct EncryptDecryptUndoCommandInfo
{
    QString     m_encryptedText;
    QString     m_decryptedText;
    QString     m_passphrase;
    QString     m_cipher;
    QString     m_hint;
    size_t      m_keyLength;
    bool        m_rememberForSession;
    bool        m_decryptPermanently;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ENCRYPT_DECRYPT_UNDO_COMMAND_INFO_H

