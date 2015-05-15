#ifndef __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H

#include <QString>

namespace qute_note {

class EncryptionManagerPrivate
{
public:
    EncryptionManagerPrivate();

    bool decrypt(const QString & encryptedText, const QString & passphrase,
                 const QString & cipher, const size_t keyLength,
                 QString & decryptedText, QString & errorDescription);

    bool encrypt(const QString & textToEncrypt, const QString & passphrase,
                 const QString & cipher, const size_t keyLength,
                 QString & encryptedText, QString & errorDescription);
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H
