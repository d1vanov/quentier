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

// Evernote service defined constants
#define EN_ITERATIONS (50000)
#define EN_KEYSIZE (16)
#define EN_HMACSIZE (32)
#define EN_IDENT "ENC0"
#define MAX_PADDING_LEN (16)

namespace qute_note {

EncryptionManagerPrivate::EncryptionManagerPrivate() :
    m_salt(),
    m_saltmac(),
    m_iv()
{
    m_salt.reserve(EN_KEYSIZE);
    m_saltmac.reserve(EN_KEYSIZE);
    m_iv.reserve(EN_KEYSIZE);
    m_key.reserve(EN_KEYSIZE);

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

        QByteArray decryptedByteArray;
        bool res = decryptPs2(encryptedText, passphrase, decryptedByteArray, errorDescription);
        if (!res) {
            return false;
        }

        decryptedText = decryptedByteArray;
        QNWARNING("decrypted text after conversion to string: " << decryptedText);
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
        QNWARNING("decrypted text after conversion to string: " << decryptedText);
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

    encryptedTextData += m_salt;
    encryptedTextData += m_saltmac;
    encryptedTextData += m_iv;

    QNWARNING("Encryption: salt = " << m_salt.toHex() << ", salthmac = " << m_saltmac.toHex()
              << ", iv = " << m_iv.toHex());

    res = generateKey(passphrase, reinterpret_cast<const unsigned char*>(m_salt.constData()),
                      EN_ITERATIONS, errorDescription);
    if (!res) {
        return false;
    }

    res = encyptWithAes(textToEncrypt, encryptedTextData, errorDescription);
    if (!res) {
        return false;
    }

    QByteArray hmac;
    res = calculateHmacHash(passphrase, reinterpret_cast<const unsigned char*>(m_saltmac.constData()),
                            textToEncrypt, EN_ITERATIONS, hmac, errorDescription);
    if (!res) {
        return false;
    }

    encryptedTextData += hmac;

    QNWARNING("hmac = " << hmac.toHex() << ", encrypted text after hmac addition = "
              << encryptedTextData.toHex());

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
        m_salt.resize(EN_KEYSIZE);
        salt = reinterpret_cast<unsigned char*>(m_salt.data());
        saltText = "salt";
        break;
    case SaltKind::SALTMAC:
        m_saltmac.resize(EN_KEYSIZE);
        salt = reinterpret_cast<unsigned char*>(m_saltmac.data());
        saltText = "saltmac";
        break;
    case SaltKind::IV:
        m_iv.resize(EN_KEYSIZE);
        salt = reinterpret_cast<unsigned char*>(m_iv.data());
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
                                           const size_t numIterations, QString & errorDescription)
{
    QByteArray passphraseData = passphrase.toLocal8Bit();
    const char * rawPassphraseData = passphraseData.constData();

    m_key.resize(EN_KEYSIZE);
    unsigned char * key = reinterpret_cast<unsigned char*>(m_key.data());

    int res = PKCS5_PBKDF2_HMAC(rawPassphraseData, passphraseData.size(), salt, EN_KEYSIZE,
                                numIterations, EVP_sha256(), EN_KEYSIZE, key);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't generate cryptographic key");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl PKCS5_PBKDF2_HMAC failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        return false;
    }

    QNWARNING("Generated encryption key: " << m_key.toHex());
    return true;
}

bool EncryptionManagerPrivate::calculateHmacHash(const QString & passphrase, const unsigned char * salt,
                                                 const QString & textToEncrypt, const size_t numIterations,
                                                 QByteArray & hash, QString & errorDescription)
{
    bool res = generateKey(passphrase, salt, numIterations, errorDescription);
    if (!res) {
        return false;
    }

    const char * rawKey = m_key.constData();

    QByteArray textToEncryptData = textToEncrypt.toLocal8Bit();
    const unsigned char * data = reinterpret_cast<const unsigned char*>(textToEncryptData.constData());

    unsigned char * digest = HMAC(EVP_sha256(), rawKey, EN_KEYSIZE, data, textToEncryptData.size(), NULL, NULL);

    hash = QByteArray(reinterpret_cast<const char*>(digest), EN_HMACSIZE);
    return true;
}

bool EncryptionManagerPrivate::encyptWithAes(const QString & textToEncrypt,
                                             QByteArray & encryptedText,
                                             QString & errorDescription)
{
    const unsigned char * rawKey = reinterpret_cast<const unsigned char*>(m_key.constData());

    QByteArray textToEncryptData = textToEncrypt.toLocal8Bit();
    const unsigned char * rawTextToEncrypt = reinterpret_cast<const unsigned char*>(textToEncryptData.constData());
    size_t rawTextToEncryptSize = textToEncryptData.size();

    unsigned char * ciphertext = reinterpret_cast<unsigned char*>(malloc(rawTextToEncryptSize + MAX_PADDING_LEN));
    int bytes_written = 0;
    int ciphertext_len = 0;

    EVP_CIPHER_CTX context;
    int res = EVP_CipherInit(&context, EVP_aes_128_cbc(), rawKey,
                             reinterpret_cast<const unsigned char*>(m_iv.constData()),
                             /* should encrypt = */ 1);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherInit failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(ciphertext);
        return false;
    }

    res = EVP_CipherUpdate(&context, ciphertext, &bytes_written, rawTextToEncrypt, rawTextToEncryptSize);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherUpdate failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(ciphertext);
        return false;
    }

    ciphertext_len += bytes_written;

    res = EVP_CipherFinal(&context, ciphertext + bytes_written, &bytes_written);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't encrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherFinal failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(ciphertext);
        return false;
    }

    ciphertext_len += bytes_written;

    QByteArray encryptedData(reinterpret_cast<const char*>(ciphertext), ciphertext_len);
    encryptedText += encryptedData;
    QNWARNING("AES encrypted text = " << encryptedData.toHex() << ", out_len = " << ciphertext_len);

    Q_UNUSED(EVP_CIPHER_CTX_cleanup(&context));
    free(ciphertext);
    return true;
}

bool EncryptionManagerPrivate::decryptAes(const QString & encryptedText, const QString & passphrase,
                                          QByteArray & decryptedText, QString & errorDescription)
{
    QByteArray cipherText;
    QByteArray hmac;
    bool bres = splitEncryptedData(encryptedText, cipherText, hmac, errorDescription);
    if (!bres) {
        return false;
    }

    bres = generateKey(passphrase, reinterpret_cast<const unsigned char*>(m_salt.constData()),
                       EN_ITERATIONS, errorDescription);
    if (!bres) {
        return false;
    }

    const unsigned char * rawKeyData = reinterpret_cast<const unsigned char*>(m_key.constData());

    const int rawCipherTextSize = cipherText.size();
    const unsigned char * rawCipherText = reinterpret_cast<const unsigned char*>(cipherText.constData());

    unsigned char * deciphered_text = reinterpret_cast<unsigned char*>(malloc(rawCipherTextSize));
    int bytes_written = 0;
    int deciphered_text_len = 0;

    EVP_CIPHER_CTX context;
    int res = EVP_DecryptInit(&context, EVP_aes_128_cbc(), rawKeyData,
                              reinterpret_cast<const unsigned char*>(m_iv.constData()));
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherInit failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(deciphered_text);
        return false;
    }

    res = EVP_DecryptUpdate(&context, deciphered_text, &bytes_written, rawCipherText, rawCipherTextSize);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherUpdate failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(deciphered_text);
        return false;
    }

    deciphered_text_len += bytes_written;

    res = EVP_DecryptFinal(&context, deciphered_text + bytes_written, &bytes_written);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't decrypt the text using AES algorithm");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << ", openssl EVP_CipherFinal failed: "
                  << ": lib: " << errorLib << "; func: " << errorFunc << ", reason: "
                  << errorReason);
        free(deciphered_text);
        return false;
    }

    deciphered_text_len += bytes_written;

    decryptedText = QByteArray(reinterpret_cast<const char*>(deciphered_text), deciphered_text_len);
    QNWARNING("decrypted text = " << decryptedText << ", out_len = " << deciphered_text_len);

    Q_UNUSED(EVP_CIPHER_CTX_cleanup(&context));
    free(deciphered_text);
    return true;
}

bool EncryptionManagerPrivate::decryptPs2(const QString & encryptedText, const QString & passphrase,
                                          QByteArray & decryptedText, QString & errorDescription)
{
    // TODO: implement
    Q_UNUSED(encryptedText)
    Q_UNUSED(passphrase)
    Q_UNUSED(decryptedText)
    Q_UNUSED(errorDescription)
    return true;
}

bool EncryptionManagerPrivate::splitEncryptedData(const QString & encryptedData,
                                                  QByteArray & encryptedText,
                                                  QByteArray & hmac,
                                                  QString & errorDescription)
{
    QByteArray decodedEncryptedData = QByteArray::fromBase64(encryptedData.toLocal8Bit());

    QNWARNING("Decryption: base64 encoded encrypted data: " << encryptedData
              << ", base64 decoded encrypted data: " << decodedEncryptedData.toHex());

    const int encryptedDataSize = decodedEncryptedData.size();
    if (encryptedDataSize <= 4+3*EN_KEYSIZE+EN_HMACSIZE) {
        errorDescription = QT_TR_NOOP("Encrypted data is too short for being valid: ") +
                           QString::number(encryptedDataSize) + " " + QT_TR_NOOP("while should be") + " " +
                           QString::number(4+3*EN_KEYSIZE+EN_HMACSIZE);
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
    for(int i = encryptedDataSize-EN_HMACSIZE; i < encryptedDataSize; ++i) {
        hmac += decodedEncryptedData.at(i);
    }

    QNWARNING("Decryption, split payload: salt = " << m_salt.toHex()
              << ", saltmac = " << m_saltmac.toHex()
              << ", iv = " << m_iv.toHex()
              << ", encrypted text without hmac = " << encryptedText.toHex()
              << ", hmac = " << hmac.toHex());

    return true;
}

} // namespace qute_note
