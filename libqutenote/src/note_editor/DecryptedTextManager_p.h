#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_MANAGER_P_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_MANAGER_P_H

#include <qute_note/utility/EncryptionManager.h>
#include <QtGlobal>
#include <QHash>

namespace qute_note {

class DecryptedTextManagerPrivate
{
public:
    DecryptedTextManagerPrivate();

    void addEntry(const QString & hash, const QString & decryptedText,
                  const bool rememberForSession, const QString & passphrase,
                  const QString & cipher, const size_t keyLength);

    void removeEntry(const QString & hash);

    void clearNonRememberedForSessionEntries();

    bool findDecryptedTextByEncryptedText(const QString & encryptedText, QString & decryptedText,
                                          bool & rememberForSession) const;

    bool modifyDecryptedText(const QString & originalHash, const QString & newDecryptedText,
                             QString & newEncryptedText);

private:
    Q_DISABLE_COPY(DecryptedTextManagerPrivate)

private:
    class Data
    {
    public:
        Data() :
            m_decryptedText(),
            m_passphrase(),
            m_cipher(),
            m_keyLength(0),
            m_rememberForSession(false)
        {}

        QString m_decryptedText;
        QString m_passphrase;
        QString m_cipher;
        size_t  m_keyLength;
        bool    m_rememberForSession;
    };

    typedef QHash<QString, Data> DataHash;
    DataHash    m_dataHash;

    DataHash    m_staleDataHash;

    EncryptionManager   m_encryptionManager;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_MANAGER_P_H
