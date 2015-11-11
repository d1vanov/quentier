#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_DECRYPT_UNDO_COMMAND_INFO_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_DECRYPT_UNDO_COMMAND_INFO_H

#include <QString>

namespace qute_note {

struct EncryptDecryptUndoCommandInfo
{
    QString     m_encryptedText;
    QString     m_decryptedText;
    QString     m_passphrase;
    QString     m_cipher;
    size_t      m_keyLength;
    bool        m_rememberForSession;
    bool        m_decryptPermanently;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_DECRYPT_UNDO_COMMAND_INFO_H

