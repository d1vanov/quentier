#ifndef __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H

#include <QString>

// Evernote service defined constants
#define EN_ITERATIONS (50000)
#define EN_KEYSIZE (16)
#define EN_HMACSIZE (32)
#define EN_IDENT "ENC0"

namespace qute_note {

class EncryptionManagerPrivate
{
public:
    EncryptionManagerPrivate();
    ~EncryptionManagerPrivate();

    bool decrypt(const QString & encryptedText, const QString & passphrase,
                 const QString & cipher, const size_t keyLength,
                 QString & decryptedText, QString & errorDescription);

    bool encrypt(const QString & textToEncrypt, const QString & passphrase,
                 QString & cipher, size_t & keyLength,
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

    bool generateKey(const QString & passphrase, const unsigned char * salt,
                     const size_t numIterations, QString & key, QString & errorDescription);

    bool calculateHmacHash(const QString & passphrase, const unsigned char * salt,
                           const QString & textToEncrypt, const size_t numIterations,
                           QString & hash, QString & errorDescription);

    bool encyptWithAes(const QString & textToEncrypt, const QString & passphrase,
                       QString & encryptedText, QString & errorDescription);

    bool decryptAes(const QString & encryptedText, const QString & passphrase,
                    QString & decryptedText, QString & errorDescription);

    bool decryptPs2(const QString & encryptedText, const QString & passphrase,
                    QString & decryptedText, QString & errorDescription);

    bool splitEncryptedData(const QString & encryptedData, QString & encryptedText,
                            QString & hmac, QString & errorDescription);

private:
    char m_salt[EN_KEYSIZE];
    char m_saltmac[EN_KEYSIZE];
    char m_iv[EN_KEYSIZE];

    unsigned char m_key_buffer[EN_KEYSIZE];
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H
