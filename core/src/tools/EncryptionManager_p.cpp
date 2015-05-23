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
#include <QCryptographicHash>
#include <stdlib.h>

namespace qute_note {

// 256-entry permutation table, probably derived somehow from pi
static const int rc2_permute[256] =
{
      217,120,249,196, 25,221,181,237, 40,233,253,121, 74,160,216,157,
      198,126, 55,131, 43,118, 83,142, 98, 76,100,136, 68,139,251,162,
       23,154, 89,245,135,179, 79, 19, 97, 69,109,141,  9,129,125, 50,
      189,143, 64,235,134,183,123, 11,240,149, 33, 34, 92,107, 78,130,
       84,214,101,147,206, 96,178, 28,115, 86,192, 20,167,140,241,220,
       18,117,202, 31, 59,190,228,209, 66, 61,212, 48,163, 60,182, 38,
      111,191, 14,218, 70,105,  7, 87, 39,242, 29,155,188,148, 67,  3,
      248, 17,199,246,144,239, 62,231,  6,195,213, 47,200,102, 30,215,
        8,232,234,222,128, 82,238,247,132,170,114,172, 53, 77,106, 42,
      150, 26,210,113, 90, 21, 73,116, 75,159,208, 94,  4, 24,164,236,
      194,224, 65,110, 15, 81,203,204, 36,145,175, 80,161,244,112, 57,
      153,124, 58,133, 35,184,180,122,252,  2, 54, 91, 37, 85,151, 49,
       45, 93,250,152,227,138,146,174,  5,223, 41, 16,103,108,186,201,
      211,  0,230,207,225,158,168, 44, 99, 22,  1, 63, 88,226,137,169,
       13, 56, 52, 27,171, 51,255,176,187, 72, 12, 95,185,177,205, 46,
      197,243,219, 71,229,165,156,119, 10,166, 32,104,254,127,193,173
};

EncryptionManagerPrivate::EncryptionManagerPrivate() :
    m_salt(),
    m_saltmac(),
    m_iv(),
    m_key(),
    m_hmac()
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

        bool res = decryptRc2(encryptedText, passphrase, decryptedText, errorDescription);
        if (!res) {
            return false;
        }

        QNWARNING("decrypted text before messing with Utf-8: " << decryptedText);
        QByteArray decryptedByteArray = decryptedText.toLocal8Bit();
        decryptedText = QString::fromUtf8(decryptedByteArray.constData(), decryptedByteArray.size());
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

    encryptedTextData.append(reinterpret_cast<const char*>(m_hmac), EN_AES_HMACSIZE);

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

    for(int i = 0; i < EN_AES_HMACSIZE; ++i) {
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
    QByteArray cipherText;
    bool bres = splitEncryptedData(encryptedText, EN_AES_KEYSIZE, EN_AES_HMACSIZE, cipherText, errorDescription);
    if (!bres) {
        return false;
    }

    QByteArray passphraseData = passphrase.toLocal8Bit();

    bres = generateKey(passphraseData, m_salt, EN_AES_KEYSIZE, errorDescription);
    if (!bres) {
        return false;
    }

    const int rawCipherTextSize = cipherText.size();
    const unsigned char * rawCipherText = reinterpret_cast<const unsigned char*>(cipherText.constData());

    unsigned char * decipheredText = reinterpret_cast<unsigned char*>(malloc(rawCipherTextSize));
    int bytesWritten = 0;
    int decipheredTextSize = 0;

    EVP_CIPHER_CTX context;
    int res = EVP_DecryptInit(&context, EVP_aes_128_cbc(), m_key, m_iv);
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
                                                  const size_t hmacSize,
                                                  QByteArray & encryptedText,
                                                  QString & errorDescription)
{
    QByteArray decodedEncryptedData = QByteArray::fromBase64(encryptedData.toLocal8Bit());

    const size_t minLength = 4 + 3 * saltSize + hmacSize;

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
    for(int i = cursor; i < encryptedDataSize-hmacSize; ++i) {
        encryptedText += decodedEncryptedData.at(i);
    }

    cursor = encryptedDataSize-hmacSize;
    for(int i = 0; i < hmacSize; ++i) {
        m_hmac[i] = decodedEncryptedDataPtr[i+cursor];
    }

    return true;
}

bool EncryptionManagerPrivate::decryptRc2(const QString & encryptedText, const QString & passphrase,
                                          QString & decryptedText, QString & errorDescription)
{
    QByteArray encryptedTextData = QByteArray::fromBase64(encryptedText.toLocal8Bit());
    decryptedText.resize(0);

    QVector<int> keyCodes = rc2KeyCodesFromPassphrase(passphrase);

    while(encryptedTextData.size() > 0)
    {
        QByteArray chunk;
        chunk.reserve(8);
        for(int i = 0; i < 8; ++i) {
            chunk.push_back(encryptedTextData[i]);
        }
        Q_UNUSED(encryptedTextData.remove(0, 8));
        QString decryptedChunk = decryptRc2Chunk(chunk, keyCodes);
        decryptedText += decryptedChunk;
    }

    // Get rid of zero symbols at the end of the string, if any
    while((decryptedText.size() > 0) && (decryptedText.at(decryptedText.size() - 1) == char(0))) {
        decryptedText.remove(decryptedText.size() - 1, 1);
    }

    QNWARNING("decrypted text before cutting off four characters at the start: " << decryptedText);

    // FIXME: temporarily temove the leading 4 characters while in reality they should be used for CRC validation
    decryptedText.remove(0, 4);

    return true;
}

QVector<int> EncryptionManagerPrivate::rc2KeyCodesFromPassphrase(const QString & passphrase) const
{
    QByteArray keyData = QCryptographicHash::hash(passphrase.toUtf8(), QCryptographicHash::Md5);
    const int keyDataSize = keyData.size();

    // Convert the input data into the array
    QVector<int> xkey(keyDataSize);
    for(int i = 0; i < keyDataSize; ++i) {
        xkey[i] = static_cast<int>(keyData[i]);
    }

    // Phase 1: Expand input key to 128 bytes
    int len = xkey.size();
    xkey.resize(128);
    for(int i = len; i < 128; ++i) {
        xkey[i] = rc2_permute[(xkey[i - 1] + xkey[i - len]) & 255];
    }

    // Phase 2: Reduce effective key size to 64 bits
    const int bits = 64;

    len = (bits + 7) >> 3;
    int i = 128 - len;
    int x = rc2_permute[xkey[i] & (255 >> (7 & -bits))];
    xkey[i] = x;
    while (i--) {
      x = rc2_permute[x ^ xkey[i + len]];
      xkey[i] = x;
    }

    // Phase 3: copy to key array of words in little-endian order
    QVector<int> key;
    key.resize(64);
    i = 63;
    do {
      key[i] = (xkey[2 * i] & 255) + (xkey[2 * i + 1] << 8);
    } while (i--);

    return key;
}

QString EncryptionManagerPrivate::decryptRc2Chunk(const QByteArray & inputCharCodes, const QVector<int> & xkey) const
{
    int x76, x54, x32, x10, i;

    QVector<int> convertedCharCodes;
    convertedCharCodes.resize(8);
    for(int i = 0; i < 8; ++i)
    {
        int & code = convertedCharCodes[i];
        code = static_cast<int>(inputCharCodes.at(i));
        if (code < 0) {
            code += 256;
        }
    }

    x76 = (convertedCharCodes[7] << 8) + convertedCharCodes[6];
    x54 = (convertedCharCodes[5] << 8) + convertedCharCodes[4];
    x32 = (convertedCharCodes[3] << 8) + convertedCharCodes[2];
    x10 = (convertedCharCodes[1] << 8) + convertedCharCodes[0];

    i = 15;
    do {
      x76 &= 65535;
      x76 = (x76 << 11) + (x76 >> 5);
      x76 -= (x10 & ~x54) + (x32 & x54) + xkey[4*i+3];

      x54 &= 65535;
      x54 = (x54 << 13) + (x54 >> 3);
      x54 -= (x76 & ~x32) + (x10 & x32) + xkey[4*i+2];

      x32 &= 65535;
      x32 = (x32 << 14) + (x32 >> 2);
      x32 -= (x54 & ~x10) + (x76 & x10) + xkey[4*i+1];

      x10 &= 65535;
      x10 = (x10 << 15) + (x10 >> 1);
      x10 -= (x32 & ~x76) + (x54 & x76) + xkey[4*i+0];

      if (i == 5 || i == 11) {
        x76 -= xkey[x54 & 63];
        x54 -= xkey[x32 & 63];
        x32 -= xkey[x10 & 63];
        x10 -= xkey[x76 & 63];
      }
    } while (i--);

    QString out;
    out.reserve(8);

#define APPEND_UNICODE_CHAR(code) \
    out += QChar(code)

    APPEND_UNICODE_CHAR(x10 & 255);
    APPEND_UNICODE_CHAR((x10 >> 8) & 255);
    APPEND_UNICODE_CHAR(x32 & 255);
    APPEND_UNICODE_CHAR((x32 >> 8) & 255);
    APPEND_UNICODE_CHAR(x54 & 255);
    APPEND_UNICODE_CHAR((x54 >> 8) & 255);
    APPEND_UNICODE_CHAR(x76 & 255);
    APPEND_UNICODE_CHAR((x76 >> 8) & 255);

#undef APPEND_UNICODE_CHAR

    QByteArray outData = out.toLocal8Bit();
    out = QString::fromUtf8(outData.constData(), outData.size());

    return out;
}

} // namespace qute_note
