#ifndef __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H

#include <QString>

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
                     const size_t numIterations, QString & errorDescription);

    bool calculateHmacHash(const QString & passphrase, const unsigned char * salt,
                           const QString & textToEncrypt, const size_t numIterations,
                           QByteArray & hash, QString & errorDescription);

    bool encyptWithAes(const QString & textToEncrypt, QByteArray & encryptedText,
                       QString & errorDescription);

    bool decryptAes(const QString & encryptedText, const QString & passphrase,
                    QByteArray & decryptedText, QString & errorDescription);

    bool decryptPs2(const QString & encryptedText, const QString & passphrase,
                    QByteArray & decryptedText, QString & errorDescription);

    bool splitEncryptedData(const QString & encryptedData, QByteArray &encryptedText,
                            QByteArray & hmac, QString & errorDescription);

private:
    QByteArray m_salt;
    QByteArray m_saltmac;
    QByteArray m_iv;
    QByteArray m_key;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_PRIVATE_H
