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
    for(int i = 0; i < EN_KEYSIZE; ++i) {
        m_salt[i] = '0';
        m_saltmac[i] = '0';
        m_iv[i] = '0';
        m_key_buffer[i] = '0';
    }

    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);
}

EncryptionManagerPrivate::~EncryptionManagerPrivate()
{
    /* Clean up */
    EVP_cleanup();
    ERR_free_strings();
}

bool EncryptionManagerPrivate::decrypt(const QString & encryptedText, const QString & passphrase,
                                       const QString & cipher, const size_t keyLength,
                                       QString & decryptedText, QString & errorDescription)
{
    if (cipher == "PS2")
    {
        if (keyLength != 64) {
            errorDescription = QT_TR_NOOP("Invalid key length for PS2 decryption method, "
                                          "should be 64");
            QNWARNING(errorDescription);
            return false;
        }

        return decryptPs2(encryptedText, passphrase, decryptedText, errorDescription);
    }
    else if (cipher == "AES")
    {
        if (keyLength != 128) {
            errorDescription = QT_TR_NOOP("Invalid key length for AES decryption method, "
                                          "should be 128");
            QNWARNING(errorDescription);
            return false;
        }

        return decryptAes(encryptedText, passphrase, decryptedText, errorDescription);
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
    encryptedText.resize(0);
    encryptedText += EN_IDENT;

#define GET_OPENSSL_ERROR \
    unsigned long errorCode = ERR_get_error(); \
    const char * errorLib = ERR_lib_error_string(errorCode); \
    const char * errorFunc = ERR_func_error_string(errorCode); \
    const char * errorReason = ERR_reason_error_string(errorCode)

    bool res = generateRandomBytes(SaltKind::SALT, errorDescription);
    if (!res) {
        return false;
    }

    res = generateRandomBytes(SaltKind::SALTMAC, errorDescription);
    if (!res) {
        return false;
    }

    res = generateRandomBytes(SaltKind::IV, errorDescription);
    if (!res) {
        return false;
    }

    encryptedText += QString::fromLocal8Bit(m_salt, EN_KEYSIZE);
    encryptedText += QString::fromLocal8Bit(m_saltmac, EN_KEYSIZE);
    encryptedText += QString::fromLocal8Bit(m_iv, EN_KEYSIZE);

    QNWARNING("Encryption: salt = " << QString::fromLocal8Bit(m_salt, EN_KEYSIZE).toUtf8()
              << ", salthmac = " << QString::fromLocal8Bit(m_saltmac, EN_KEYSIZE).toUtf8()
              << ", iv = " << QString::fromLocal8Bit(m_iv, EN_KEYSIZE).toUtf8());

    QString encryptionKey;
    res = generateKey(passphrase, reinterpret_cast<const unsigned char*>(m_salt),
                      EN_ITERATIONS, encryptionKey, errorDescription);
    if (!res) {
        return false;
    }

    QNWARNING("Generated encryption key = " << encryptionKey.toUtf8());

    res = encyptWithAes(textToEncrypt, encryptionKey, encryptedText, errorDescription);
    if (!res) {
        return false;
    }

    QString hmac;
    res = calculateHmacHash(passphrase, reinterpret_cast<const unsigned char*>(m_saltmac),
                            textToEncrypt, EN_ITERATIONS, hmac, errorDescription);
    if (!res) {
        return false;
    }

    encryptedText += hmac;

    QNWARNING("hmac = " << hmac.toUtf8() << ", encrypted text after hmac addition = " << encryptedText.toUtf8());

    QByteArray encryptedTextData = encryptedText.toLocal8Bit();
    encryptedText = encryptedTextData.toBase64();

    QNWARNING("Encrypted text encoded into base64: " << encryptedText);

    cipher = "AES";
    keyLength = 128;

    return true;
}

bool EncryptionManagerPrivate::generateRandomBytes(const EncryptionManagerPrivate::SaltKind::type saltKind,
                                                   QString & errorDescription)
{
    unsigned char * salt = nullptr;
    const char * saltText = nullptr;

    switch (saltKind)
    {
    case SaltKind::SALT:
        salt = reinterpret_cast<unsigned char*>(m_salt);
        saltText = "salt";
        break;
    case SaltKind::SALTMAC:
        salt = reinterpret_cast<unsigned char*>(m_saltmac);
        saltText = "saltmac";
        break;
    case SaltKind::IV:
        salt = reinterpret_cast<unsigned char*>(m_iv);
        saltText = "iv";
        break;
    default:
        errorDescription = QT_TR_NOOP("Internal error, detected incorrect salt kind for cryptographic key generation");
        QNCRITICAL(errorDescription);
        return false;
    }

    int res = RAND_bytes(salt, EN_KEYSIZE);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't generate cryptographically strong bytes for encryption");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << "; " << saltText << ": lib: " << errorLib
                  << "; func: " << errorFunc << ", reason: " << errorReason);
        return false;
    }

    return true;
}

bool EncryptionManagerPrivate::generateKey(const QString & passphrase, const unsigned char * salt,
                                           const size_t numIterations, QString & key,
                                           QString & errorDescription)
{
    QByteArray passphraseData = passphrase.toLocal8Bit();
    const char * rawPassphraseData = passphraseData.constData();
    int res = PKCS5_PBKDF2_HMAC(rawPassphraseData, passphraseData.size(), salt, EN_KEYSIZE,
                                numIterations, EVP_sha256(), EN_KEYSIZE, m_key_buffer);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't generate cryptographic key");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl PKCS5_PBKDF2_HMAC failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        return false;
    }

    key.resize(0);
    appendUnsignedCharToQString(key, m_key_buffer, EN_KEYSIZE);
    return true;
}

bool EncryptionManagerPrivate::calculateHmacHash(const QString & passphrase, const unsigned char * salt,
                                                 const QString & textToEncrypt, const size_t numIterations,
                                                 QString & hash, QString & errorDescription)
{
    QString key;
    bool res = generateKey(passphrase, salt, numIterations, key, errorDescription);
    if (!res) {
        return false;
    }

    QByteArray keyData = key.toLocal8Bit();
    const char * rawKey = keyData.constData();

    QByteArray textToEncryptData = textToEncrypt.toLocal8Bit();
    const unsigned char * data = reinterpret_cast<const unsigned char*>(textToEncryptData.constData());

    unsigned char * digest = HMAC(EVP_sha256(), rawKey, EN_KEYSIZE, data, textToEncryptData.size(), NULL, NULL);

    hash.resize(0);
    appendUnsignedCharToQString(hash, digest, EN_HMACSIZE);
    return true;
}

bool EncryptionManagerPrivate::encyptWithAes(const QString & textToEncrypt,
                                             const QString & key,
                                             QString & encryptedText,
                                             QString & errorDescription)
{
    QByteArray keyData = key.toLocal8Bit();
    const unsigned char * rawKey = reinterpret_cast<const unsigned char*>(keyData.constData());

    QByteArray textToEncryptData = textToEncrypt.toLocal8Bit();
    const unsigned char * rawTextToEncrypt = reinterpret_cast<const unsigned char*>(textToEncryptData.constData());
    size_t rawTextToEncryptSize = textToEncryptData.size();

    unsigned char * buffer = reinterpret_cast<unsigned char*>(malloc(rawTextToEncryptSize * 2));
    int out_len;

    EVP_CIPHER_CTX context;
    int res = EVP_CipherInit(&context, EVP_aes_128_cbc(), rawKey,
                             reinterpret_cast<const unsigned char*>(m_iv), /* should encrypt = */ 1);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherInit failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(buffer);
        return false;
    }

    res = EVP_CipherUpdate(&context, buffer, &out_len, rawTextToEncrypt, rawTextToEncryptSize);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherUpdate failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(buffer);
        return false;
    }

    res = EVP_CipherFinal(&context, buffer, &out_len);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherFinal failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(buffer);
        return false;
    }

    appendUnsignedCharToQString(encryptedText, buffer, out_len);
    QNWARNING("AES encrypted text = " << encryptedText.toUtf8() << ", out_len = " << out_len);

    free(buffer);
    return true;
}

bool EncryptionManagerPrivate::decryptAes(const QString & encryptedText, const QString & passphrase,
                                          QString & decryptedText, QString & errorDescription)
{
    QString cipherText;
    QString hmac;
    bool bres = splitEncryptedData(encryptedText, cipherText, hmac, errorDescription);
    if (!bres) {
        return false;
    }

    QString key;
    bres = generateKey(passphrase, reinterpret_cast<const unsigned char*>(m_salt),
                       EN_ITERATIONS, key, errorDescription);
    if (!bres) {
        return false;
    }

    QNWARNING("Key for decryption generated from passphrase and salt: " << key);

    QByteArray keyData = key.toLocal8Bit();
    const unsigned char * rawKeyData = reinterpret_cast<const unsigned char*>(keyData.constData());

    QByteArray cipherTextData = cipherText.toLocal8Bit();
    const int rawCipherTextDataSize = cipherTextData.size();
    const unsigned char * rawCipherTextData = reinterpret_cast<const unsigned char*>(cipherTextData.constData());

    unsigned char * buffer = reinterpret_cast<unsigned char*>(malloc(rawCipherTextDataSize * 2));
    int out_len;

    EVP_CIPHER_CTX context;
    int res = EVP_DecryptInit(&context, EVP_aes_128_cbc(), rawKeyData, reinterpret_cast<const unsigned char*>(m_iv));
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherInit failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(buffer);
        return false;
    }

    res = EVP_DecryptUpdate(&context, buffer, &out_len, rawCipherTextData, rawCipherTextDataSize);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherUpdate failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(buffer);
        return false;
    }

    res = EVP_DecryptFinal(&context, buffer, &out_len);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherFinal failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(buffer);
        return false;
    }

    appendUnsignedCharToQString(decryptedText, buffer, out_len);
    free(buffer);
    return true;
}

bool EncryptionManagerPrivate::decryptPs2(const QString & encryptedText, const QString & passphrase,
                                          QString & decryptedText, QString & errorDescription)
{
    // TODO: implement
    Q_UNUSED(encryptedText)
    Q_UNUSED(passphrase)
    Q_UNUSED(decryptedText)
    Q_UNUSED(errorDescription)
    return true;
}

bool EncryptionManagerPrivate::splitEncryptedData(const QString & encryptedData,
                                                  QString & encryptedText,
                                                  QString & hmac,
                                                  QString & errorDescription)
{
    QByteArray decodedEncryptedData = QByteArray::fromBase64(encryptedData.toLocal8Bit());

    QNWARNING("Decryption: base64 encoded encrypted data: " << encryptedData
              << ", base64 decoded encrypted data: " << decodedEncryptedData);

    const int encryptedDataSize = decodedEncryptedData.size();
    if (encryptedDataSize <= 4+3*EN_KEYSIZE+EN_HMACSIZE) {
        errorDescription = QT_TR_NOOP("Encrypted data is too short for being valid");
        return false;
    }

    for(int i = 4; i < 4+EN_KEYSIZE; ++i) {
        m_salt[i-4] = decodedEncryptedData.at(i);
    }

    for(int i = 4+EN_KEYSIZE; i < 4+2*EN_KEYSIZE; ++i) {
        m_saltmac[i-4-EN_KEYSIZE] = decodedEncryptedData.at(i);
    }

    for(int i = 4+2*EN_KEYSIZE; i < 4+3*EN_KEYSIZE; ++i) {
        m_iv[i-4-2*EN_KEYSIZE] = decodedEncryptedData.at(i);
    }

    encryptedText.resize(0);
    for(int i = 4+3*EN_KEYSIZE; i < encryptedDataSize-EN_HMACSIZE; ++i) {
        encryptedText += decodedEncryptedData.at(i);
    }

    hmac.resize(0);
    for(int i = encryptedDataSize-EN_KEYSIZE; i < encryptedDataSize; ++i) {
        hmac += decodedEncryptedData.at(i);
    }

    QNWARNING("Decryption, split payload: salt = " << QString::fromLocal8Bit(m_salt, EN_KEYSIZE).toUtf8()
              << ", saltmac = " << QString::fromLocal8Bit(m_saltmac, EN_KEYSIZE).toUtf8()
              << ", iv = " << QString::fromLocal8Bit(m_iv, EN_KEYSIZE).toUtf8()
              << ", encrypted text without hmac = " << encryptedText.toUtf8()
              << ", hmac = " << hmac.toUtf8());

    return true;
}

} // namespace qute_note
