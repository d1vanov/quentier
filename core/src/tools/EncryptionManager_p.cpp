#include "EncryptionManager_p.h"

namespace qute_note {

EncryptionManagerPrivate::EncryptionManagerPrivate()
{}

bool EncryptionManagerPrivate::decrypt(const QString & encryptedText, const QString & passphrase,
                                       const QString & cipher, const size_t keyLength,
                                       QString & decryptedText, QString & errorDescription)
{
    // TODO: implement
    Q_UNUSED(encryptedText);
    Q_UNUSED(passphrase);
    Q_UNUSED(cipher);
    Q_UNUSED(keyLength);
    Q_UNUSED(decryptedText);
    Q_UNUSED(errorDescription);
    return false;
}

bool EncryptionManagerPrivate::encrypt(const QString & textToEncrypt, const QString & passphrase,
                                       const QString & cipher, const size_t keyLength,
                                       QString & encryptedText, QString & errorDescription)
{
    // TODO: implement
    Q_UNUSED(textToEncrypt);
    Q_UNUSED(passphrase);
    Q_UNUSED(cipher);
    Q_UNUSED(keyLength);
    Q_UNUSED(encryptedText);
    Q_UNUSED(errorDescription);
    return false;
}

} // namespace qute_note
