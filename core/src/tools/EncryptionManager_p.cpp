#include "EncryptionManager_p.h"
#include <logging/QuteNoteLogger.h>
#include <tools/DesktopServices.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/conf.h>
#include <stdlib.h>

namespace qute_note {

EncryptionManagerPrivate::EncryptionManagerPrivate() :
    m_salt(),
    m_saltmac(),
    m_iv()
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);
}

EncryptionManagerPrivate::~EncryptionManagerPrivate()
{
    EVP_cleanup();
    ERR_free_strings();
}

bool EncryptionManagerPrivate::decrypt(const QString & encryptedText, const QString & passphrase,
                                       const QString & cipher, const size_t keyLength,
                                       QString & decryptedText, QString & errorDescription)
{
    if (cipher == "RC2")
    {
        if (keyLength != 64) {
            errorDescription = QT_TR_NOOP("Invalid key length for PS2 decryption method, "
                                          "should be 64");
            QNWARNING(errorDescription);
            return false;
        }

        QByteArray decryptedByteArray;
        bool res = decryptPs2(encryptedText, passphrase, decryptedByteArray, errorDescription);
        if (!res) {
            return false;
        }

        decryptedText = decryptedByteArray;
        QNWARNING("decrypted text = " << decryptedText);
        return true;
    }
    else if (cipher == "AES")
    {
        if (keyLength != 128) {
            errorDescription = QT_TR_NOOP("Invalid key length for AES decryption method, "
                                          "should be 128");
            QNWARNING(errorDescription);
            return false;
        }

        QByteArray decryptedByteArray;
        bool res = decryptAes(encryptedText, passphrase, decryptedByteArray, errorDescription);
        if (!res) {
            return false;
        }

        decryptedText = decryptedByteArray;
        return true;
    }
    else
    {
        errorDescription = QT_TR_NOOP("Unsupported decryption method");
        QNWARNING(errorDescription);
        return false;
    }
}

bool EncryptionManagerPrivate::encrypt(const QString & textToEncrypt, const QString & passphrase,
                                       QString & cipher, size_t & keyLength,
                                       QString & encryptedText, QString & errorDescription)
{
    QByteArray encryptedTextData(EN_IDENT, 4);

#define GET_OPENSSL_ERROR \
    unsigned long errorCode = ERR_get_error(); \
    const char * errorLib = ERR_lib_error_string(errorCode); \
    const char * errorFunc = ERR_func_error_string(errorCode); \
    const char * errorReason = ERR_reason_error_string(errorCode)

    bool res = generateSalt(SaltKind::SALT, EN_AES_KEYSIZE, errorDescription);
    if (!res) {
        return false;
    }

    res = generateSalt(SaltKind::SALTMAC, EN_AES_KEYSIZE, errorDescription);
    if (!res) {
        return false;
    }

    res = generateSalt(SaltKind::IV, EN_AES_KEYSIZE, errorDescription);
    if (!res) {
        return false;
    }

    encryptedTextData.append(reinterpret_cast<const char*>(m_salt), EN_AES_KEYSIZE);
    encryptedTextData.append(reinterpret_cast<const char*>(m_saltmac), EN_AES_KEYSIZE);
    encryptedTextData.append(reinterpret_cast<const char*>(m_iv), EN_AES_KEYSIZE);

    QByteArray passphraseData = passphrase.toLocal8Bit();

    res = generateKey(passphraseData, m_salt, EN_AES_KEYSIZE, errorDescription);
    if (!res) {
        return false;
    }

    QByteArray textToEncryptData = textToEncrypt.toLocal8Bit();

    res = encyptWithAes(textToEncryptData, encryptedTextData, errorDescription);
    if (!res) {
        return false;
    }

    res = calculateHmac(passphraseData, m_saltmac, textToEncryptData, EN_AES_KEYSIZE, errorDescription);
    if (!res) {
        return false;
    }

    encryptedTextData.append(reinterpret_cast<const char*>(m_hmac), EN_HMACSIZE);

    encryptedText = encryptedTextData.toBase64();
    cipher = "AES";
    keyLength = 128;
    return true;
}

bool EncryptionManagerPrivate::generateSalt(const EncryptionManagerPrivate::SaltKind::type saltKind,
                                            const size_t saltSize, QString & errorDescription)
{
    unsigned char * salt = nullptr;
    const char * saltText = nullptr;

    switch (saltKind)
    {
    case SaltKind::SALT:
        salt = m_salt;
        saltText = "salt";
        break;
    case SaltKind::SALTMAC:
        salt = m_saltmac;
        saltText = "saltmac";
        break;
    case SaltKind::IV:
        salt = m_iv;
        saltText = "iv";
        break;
    default:
        errorDescription = QT_TR_NOOP("Internal error, detected incorrect salt kind for cryptographic key generation");
        QNCRITICAL(errorDescription);
        return false;
    }

    int res = RAND_bytes(salt, saltSize);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't generate cryptographically strong bytes for encryption");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << "; " << saltText << ": lib: " << errorLib
                  << "; func: " << errorFunc << ", reason: " << errorReason);
        return false;
    }

    return true;
}

bool EncryptionManagerPrivate::generateKey(const QByteArray & passphraseData, const unsigned char * salt,
                                           const size_t keySize, QString & errorDescription)
{
    const char * rawPassphraseData = passphraseData.constData();

    int res = PKCS5_PBKDF2_HMAC(rawPassphraseData, passphraseData.size(), salt, keySize,
                                EN_ITERATIONS, EVP_sha256(), keySize, m_key);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't generate cryptographic key");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl PKCS5_PBKDF2_HMAC failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        return false;
    }

    return true;
}

bool EncryptionManagerPrivate::calculateHmac(const QByteArray & passphraseData, const unsigned char * salt,
                                             const QByteArray & textToEncryptData,
                                             const size_t keySize, QString & errorDescription)
{
    bool res = generateKey(passphraseData, salt, keySize, errorDescription);
    if (!res) {
        return false;
    }

    const unsigned char * data = reinterpret_cast<const unsigned char*>(textToEncryptData.constData());
    unsigned char * digest = HMAC(EVP_sha256(), m_key, keySize, data, textToEncryptData.size(), NULL, NULL);

    for(int i = 0; i < EN_HMACSIZE; ++i) {
        m_hmac[i] = reinterpret_cast<const char*>(digest)[i];
    }
    return true;
}

bool EncryptionManagerPrivate::encyptWithAes(const QByteArray & textToEncryptData,
                                             QByteArray & encryptedTextData,
                                             QString & errorDescription)
{
    const unsigned char * rawTextToEncrypt = reinterpret_cast<const unsigned char*>(textToEncryptData.constData());
    size_t rawTextToEncryptSize = textToEncryptData.size();

    unsigned char * cipherText = reinterpret_cast<unsigned char*>(malloc(rawTextToEncryptSize + MAX_PADDING_LEN));
    int bytesWritten = 0;
    int cipherTextSize = 0;

    EVP_CIPHER_CTX context;
    int res = EVP_EncryptInit(&context, EVP_aes_128_cbc(), m_key, m_iv);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_EnryptInit failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(cipherText);
        return false;
    }

    res = EVP_EncryptUpdate(&context, cipherText, &bytesWritten, rawTextToEncrypt, rawTextToEncryptSize);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherUpdate failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(cipherText);
        return false;
    }

    cipherTextSize += bytesWritten;

    res = EVP_EncryptFinal(&context, cipherText + bytesWritten, &bytesWritten);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherFinal failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(cipherText);
        return false;
    }

    cipherTextSize += bytesWritten;

    encryptedTextData.append(reinterpret_cast<const char*>(cipherText), cipherTextSize);

    Q_UNUSED(EVP_CIPHER_CTX_cleanup(&context));
    free(cipherText);
    return true;
}

bool EncryptionManagerPrivate::decryptAes(const QString & encryptedText, const QString & passphrase,
                                          QByteArray & decryptedText, QString & errorDescription)
{
    return decryptImpl(encryptedText, passphrase, /* use AES = */ true, decryptedText, errorDescription);
}

bool EncryptionManagerPrivate::decryptPs2(const QString & encryptedText, const QString & passphrase,
                                          QByteArray & decryptedText, QString & errorDescription)
{
    return decryptImpl(encryptedText, passphrase, /* use AES = */ true, decryptedText, errorDescription);
}

bool EncryptionManagerPrivate::decryptImpl(const QString & encryptedText, const QString & passphrase,
                                           const bool useAes, QByteArray & decryptedText,
                                           QString & errorDescription)
{
    const size_t keySize = (useAes ? EN_AES_KEYSIZE : EN_RC2_KEYSIZE);

    QByteArray cipherText;
    bool bres = splitEncryptedData(encryptedText, keySize, cipherText, errorDescription);
    if (!bres) {
        return false;
    }

    QByteArray passphraseData = passphrase.toLocal8Bit();

    bres = generateKey(passphraseData, m_salt, keySize, errorDescription);
    if (!bres) {
        return false;
    }

    const int rawCipherTextSize = cipherText.size();
    const unsigned char * rawCipherText = reinterpret_cast<const unsigned char*>(cipherText.constData());

    unsigned char * decipheredText = reinterpret_cast<unsigned char*>(malloc(rawCipherTextSize));
    int bytesWritten = 0;
    int decipheredTextSize = 0;

    const EVP_CIPHER * cipher = (useAes ? EVP_aes_128_cbc() : EVP_rc2_64_cbc());

    EVP_CIPHER_CTX context;
    int res = EVP_DecryptInit(&context, cipher, m_key, m_iv);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_DecryptInit failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(decipheredText);
        return false;
    }

    res = EVP_DecryptUpdate(&context, decipheredText, &bytesWritten, rawCipherText, rawCipherTextSize);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_DecryptUpdate failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(decipheredText);
        return false;
    }

    decipheredTextSize += bytesWritten;

    res = EVP_DecryptFinal(&context, decipheredText + bytesWritten, &bytesWritten);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_DecryptFinal failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(decipheredText);
        return false;
    }

    decipheredTextSize += bytesWritten;
    decryptedText = QByteArray(reinterpret_cast<const char*>(decipheredText), decipheredTextSize);

    Q_UNUSED(EVP_CIPHER_CTX_cleanup(&context));
    free(decipheredText);
    return true;
}

bool EncryptionManagerPrivate::splitEncryptedData(const QString & encryptedData,
                                                  const size_t saltSize,
                                                  QByteArray & encryptedText,
                                                  QString & errorDescription)
{
    QByteArray decodedEncryptedData = QByteArray::fromBase64(encryptedData.toLocal8Bit());

    const size_t minLength = 4 + 3 * saltSize + EN_HMACSIZE;

    const int encryptedDataSize = decodedEncryptedData.size();
    if (encryptedDataSize <= minLength) {
        errorDescription = QT_TR_NOOP("Encrypted data is too short for being valid: ") +
                           QString::number(encryptedDataSize) + " " + QT_TR_NOOP("while should be") + " " +
                           QString::number(minLength);
        return false;
    }

    const unsigned char * decodedEncryptedDataPtr = reinterpret_cast<const unsigned char*>(decodedEncryptedData.constData());

    size_t cursor = 4;
    for(int i = 0; i < saltSize; ++i) {
        m_salt[i] = decodedEncryptedDataPtr[i+cursor];
    }

    cursor += saltSize;
    for(int i = 0; i < saltSize; ++i) {
        m_saltmac[i] = decodedEncryptedDataPtr[i+cursor];
    }

    cursor += saltSize;
    for(int i = 0; i < saltSize; ++i) {
        m_iv[i] = decodedEncryptedDataPtr[i+cursor];
    }

    cursor += saltSize;
    encryptedText.resize(0);
    for(int i = cursor; i < encryptedDataSize-EN_HMACSIZE; ++i) {
        encryptedText += decodedEncryptedData.at(i);
    }

    cursor = encryptedDataSize-EN_HMACSIZE;
    for(int i = 0; i < EN_HMACSIZE; ++i) {
        m_hmac[i] = decodedEncryptedDataPtr[i+cursor];
    }

    return true;
}

} // namespace qute_note
