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

private:
    struct SaltKind
    {
        enum type {
            SALT = 0,
            SALTMAC,
            IV
        };
    };

    bool generateRandomBytes(const SaltKind::type saltKind, QString & errorDescription);

private:
    class ErrorStringsHolder
    {
    public:
        ErrorStringsHolder();
        ~ErrorStringsHolder();
    };

private:
    unsigned char m_salt[16];
    unsigned char m_saltmac[16];
    unsigned char m_iv[16];
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H
