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

static const QString crc32table =
    "00000000 77073096 EE0E612C 990951BA 076DC419 "
    "706AF48F E963A535 9E6495A3 0EDB8832 79DCB8A4 "
    "E0D5E91E 97D2D988 09B64C2B 7EB17CBD E7B82D07 "
    "90BF1D91 1DB71064 6AB020F2 F3B97148 84BE41DE "
    "1ADAD47D 6DDDE4EB F4D4B551 83D385C7 136C9856 "
    "646BA8C0 FD62F97A 8A65C9EC 14015C4F 63066CD9 "
    "FA0F3D63 8D080DF5 3B6E20C8 4C69105E D56041E4 "
    "A2677172 3C03E4D1 4B04D447 D20D85FD A50AB56B "
    "35B5A8FA 42B2986C DBBBC9D6 ACBCF940 32D86CE3 "
    "45DF5C75 DCD60DCF ABD13D59 26D930AC 51DE003A "
    "C8D75180 BFD06116 21B4F4B5 56B3C423 CFBA9599 "
    "B8BDA50F 2802B89E 5F058808 C60CD9B2 B10BE924 "
    "2F6F7C87 58684C11 C1611DAB B6662D3D 76DC4190 "
    "01DB7106 98D220BC EFD5102A 71B18589 06B6B51F "
    "9FBFE4A5 E8B8D433 7807C9A2 0F00F934 9609A88E "
    "E10E9818 7F6A0DBB 086D3D2D 91646C97 E6635C01 "
    "6B6B51F4 1C6C6162 856530D8 F262004E 6C0695ED "
    "1B01A57B 8208F4C1 F50FC457 65B0D9C6 12B7E950 "
    "8BBEB8EA FCB9887C 62DD1DDF 15DA2D49 8CD37CF3 "
    "FBD44C65 4DB26158 3AB551CE A3BC0074 D4BB30E2 "
    "4ADFA541 3DD895D7 A4D1C46D D3D6F4FB 4369E96A "
    "346ED9FC AD678846 DA60B8D0 44042D73 33031DE5 "
    "AA0A4C5F DD0D7CC9 5005713C 270241AA BE0B1010 "
    "C90C2086 5768B525 206F85B3 B966D409 CE61E49F "
    "5EDEF90E 29D9C998 B0D09822 C7D7A8B4 59B33D17 "
    "2EB40D81 B7BD5C3B C0BA6CAD EDB88320 9ABFB3B6 "
    "03B6E20C 74B1D29A EAD54739 9DD277AF 04DB2615 "
    "73DC1683 E3630B12 94643B84 0D6D6A3E 7A6A5AA8 "
    "E40ECF0B 9309FF9D 0A00AE27 7D079EB1 F00F9344 "
    "8708A3D2 1E01F268 6906C2FE F762575D 806567CB "
    "196C3671 6E6B06E7 FED41B76 89D32BE0 10DA7A5A "
    "67DD4ACC F9B9DF6F 8EBEEFF9 17B7BE43 60B08ED5 "
    "D6D6A3E8 A1D1937E 38D8C2C4 4FDFF252 D1BB67F1 "
    "A6BC5767 3FB506DD 48B2364B D80D2BDA AF0A1B4C "
    "36034AF6 41047A60 DF60EFC3 A867DF55 316E8EEF "
    "4669BE79 CB61B38C BC66831A 256FD2A0 5268E236 "
    "CC0C7795 BB0B4703 220216B9 5505262F C5BA3BBE "
    "B2BD0B28 2BB45A92 5CB36A04 C2D7FFA7 B5D0CF31 "
    "2CD99E8B 5BDEAE1D 9B64C2B0 EC63F226 756AA39C "
    "026D930A 9C0906A9 EB0E363F 72076785 05005713 "
    "95BF4A82 E2B87A14 7BB12BAE 0CB61B38 92D28E9B "
    "E5D5BE0D 7CDCEFB7 0BDBDF21 86D3D2D4 F1D4E242 "
    "68DDB3F8 1FDA836E 81BE16CD F6B9265B 6FB077E1 "
    "18B74777 88085AE6 FF0F6A70 66063BCA 11010B5C "
    "8F659EFF F862AE69 616BFFD3 166CCF45 A00AE278 "
    "D70DD2EE 4E048354 3903B3C2 A7672661 D06016F7 "
    "4969474D 3E6E77DB AED16A4A D9D65ADC 40DF0B66 "
    "37D83BF0 A9BCAE53 DEBB9EC5 47B2CF7F 30B5FFE9 "
    "BDBDF21C CABAC28A 53B39330 24B4A3A6 BAD03605 "
    "CDD70693 54DE5729 23D967BF B3667A2E C4614AB8 "
    "5D681B02 2A6F2B94 B40BBE37 C30C8EA1 5A05DF1B "
    "2D02EF8D";

#ifdef _MSC_VER
#pragma warning(disable:4351)
#endif

EncryptionManagerPrivate::EncryptionManagerPrivate() :
    m_salt(),
    m_saltmac(),
    m_iv(),
    m_key(),
    m_hmac(),
    m_cached_xkey(),
    m_cached_key(),
    m_decrypt_rc2_chunk_key_codes(),
    m_rc2_chunk_out()
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

        QByteArray decryptedByteArray = decryptedText.toLocal8Bit();
        decryptedText = QString::fromUtf8(decryptedByteArray.constData(), decryptedByteArray.size());
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

    res = calculateHmac(passphraseData, m_saltmac, encryptedTextData, EN_AES_KEYSIZE, errorDescription);
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
                                             const QByteArray & encryptedTextData,
                                             const size_t keySize, QString & errorDescription)
{
    bool res = generateKey(passphraseData, salt, keySize, errorDescription);
    if (!res) {
        return false;
    }

    const unsigned char * data = reinterpret_cast<const unsigned char*>(encryptedTextData.constData());
    unsigned char * digest = HMAC(EVP_sha256(), m_key, keySize, data, encryptedTextData.size(), NULL, NULL);

    for(int i = 0; i < EN_AES_HMACSIZE; ++i) {
        m_hmac[i] = digest[i];
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
    QNDEBUG("EncryptionManagerPrivate::decryptAes");

    QByteArray cipherText;
    bool bres = splitEncryptedData(encryptedText, EN_AES_KEYSIZE, EN_AES_HMACSIZE, cipherText, errorDescription);
    if (!bres) {
        return false;
    }

    QByteArray passphraseData = passphrase.toLocal8Bit();

    // Validate HMAC
    QNTRACE("Validating hmac...");

    unsigned char parsedHmac[EN_AES_HMACSIZE];
    for(int i = 0; i < EN_AES_HMACSIZE; ++i) {
        parsedHmac[i] = m_hmac[i];
    }

    QByteArray saltWithCipherText = QByteArray::fromBase64(encryptedText.toLocal8Bit());
    saltWithCipherText.remove(saltWithCipherText.size() - EN_AES_HMACSIZE, EN_AES_HMACSIZE);

    bres = calculateHmac(passphraseData, m_saltmac, saltWithCipherText, EN_AES_KEYSIZE, errorDescription);
    if (!bres) {
        return false;
    }

    for(int i = 0; i < EN_AES_HMACSIZE; ++i)
    {
        if (parsedHmac[i] != m_hmac[i]) {
            errorDescription = QT_TR_NOOP("Can't decrypt text: invalid checksum");
            QNWARNING(errorDescription << ", parsed hmac: "
                      << QByteArray(reinterpret_cast<const char*>(parsedHmac), EN_AES_HMACSIZE).toHex()
                      << ", expected hmac: "
                      << QByteArray(reinterpret_cast<const char*>(m_hmac), EN_AES_HMACSIZE).toHex());
            return false;
        }
    }

    QNTRACE("Successfully validated hmac");

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

    const int minLength = 4 + 3 * saltSize + hmacSize;

    const int encryptedDataSize = decodedEncryptedData.size();
    if (encryptedDataSize <= minLength) {
        errorDescription = QT_TR_NOOP("Encrypted data is too short for being valid: ") +
                           QString::number(encryptedDataSize) + " " + QT_TR_NOOP("while should be") + " " +
                           QString::number(minLength);
        return false;
    }

    const unsigned char * decodedEncryptedDataPtr = reinterpret_cast<const unsigned char*>(decodedEncryptedData.constData());

    size_t cursor = 4;
    for(size_t i = 0; i < saltSize; ++i) {
        m_salt[i] = decodedEncryptedDataPtr[i+cursor];
    }

    cursor += saltSize;
    for(size_t i = 0; i < saltSize; ++i) {
        m_saltmac[i] = decodedEncryptedDataPtr[i+cursor];
    }

    cursor += saltSize;
    for(size_t i = 0; i < saltSize; ++i) {
        m_iv[i] = decodedEncryptedDataPtr[i+cursor];
    }

    cursor += saltSize;
    encryptedText.resize(0);
    for(size_t i = cursor; i < encryptedDataSize-hmacSize; ++i) {
        encryptedText += decodedEncryptedData.at(i);
    }

    cursor = encryptedDataSize-hmacSize;
    for(size_t i = 0; i < hmacSize; ++i) {
        m_hmac[i] = decodedEncryptedDataPtr[i+cursor];
    }

    return true;
}

// WARNING: the implementation was ported from JavaScript taken from Evernote site
// so it contains dangerous magic. Don't touch that unless you know what you're doing!
bool EncryptionManagerPrivate::decryptRc2(const QString & encryptedText, const QString & passphrase,
                                          QString & decryptedText, QString & errorDescription)
{
    QByteArray encryptedTextData = QByteArray::fromBase64(encryptedText.toLocal8Bit());
    decryptedText.resize(0);

    rc2KeyCodesFromPassphrase(passphrase);

    while(encryptedTextData.size() > 0)
    {
        QByteArray chunk;
        chunk.reserve(8);
        for(int i = 0; i < 8; ++i) {
            chunk.push_back(encryptedTextData[i]);
        }
        Q_UNUSED(encryptedTextData.remove(0, 8));
        QString decryptedChunk = decryptRc2Chunk(chunk, m_cached_key);
        decryptedText += decryptedChunk;
    }

    // First 4 chars of the string is the HEX-representation of the upper-byte of the CRC32 of the string.
    // If CRC32 is valid, we return the decoded string, otherwise return with error
    QString crc = decryptedText.left(4);
    decryptedText.remove(0, 4);

    qint32 realCrc = crc32(decryptedText);
    realCrc ^= (-1);

    quint32 unsignedRealCrc = static_cast<quint32>(realCrc);
    unsignedRealCrc >>= 0;

    QString realCrcStr = QString::number(unsignedRealCrc, 16);
    realCrcStr = realCrcStr.left(4);
    realCrcStr = realCrcStr.toUpper();

    if (realCrcStr != crc) {
        QNWARNING("CRC32 checksum mismatch! Decrypted string has " << crc << ", calculated CRC32 is " << realCrcStr);
        return false;
    }

    // Get rid of zero symbols at the end of the string, if any
    while((decryptedText.size() > 0) && (decryptedText.at(decryptedText.size() - 1) == char(0))) {
        decryptedText.remove(decryptedText.size() - 1, 1);
    }

    return true;
}

void EncryptionManagerPrivate::rc2KeyCodesFromPassphrase(const QString & passphrase) const
{
    QByteArray keyData = QCryptographicHash::hash(passphrase.toUtf8(), QCryptographicHash::Md5);
    const int keyDataSize = keyData.size();

    // Convert the input data into the array
    m_cached_xkey.resize(keyDataSize);
    for(int i = 0; i < keyDataSize; ++i) {
        m_cached_xkey[i] = static_cast<int>(keyData[i]);
    }

    // Phase 1: Expand input key to 128 bytes
    int len = m_cached_xkey.size();
    m_cached_xkey.resize(128);
    for(int i = len; i < 128; ++i) {
        m_cached_xkey[i] = rc2_permute[(m_cached_xkey[i - 1] + m_cached_xkey[i - len]) & 255];
    }

    // Phase 2: Reduce effective key size to 64 bits
    const int bits = 64;

    len = (bits + 7) >> 3;
    int i = 128 - len;
    int x = rc2_permute[m_cached_xkey[i] & (255 >> (7 & -bits))];
    m_cached_xkey[i] = x;
    while (i--) {
      x = rc2_permute[x ^ m_cached_xkey[i + len]];
      m_cached_xkey[i] = x;
    }

    // Phase 3: copy to key array of words in little-endian order
    m_cached_key.resize(64);
    i = 63;
    do {
      m_cached_key[i] = (m_cached_xkey[2 * i] & 255) + (m_cached_xkey[2 * i + 1] << 8);
    } while (i--);
}

QString EncryptionManagerPrivate::decryptRc2Chunk(const QByteArray & inputCharCodes, const std::vector<int> & key) const
{
    int x76, x54, x32, x10, i;

    for(int i = 0; i < 8; ++i)
    {
        int & code = m_decrypt_rc2_chunk_key_codes[i];
        code = static_cast<int>(inputCharCodes.at(i));
        if (code < 0) {
            code += 256;
        }
    }

    x76 = (m_decrypt_rc2_chunk_key_codes[7] << 8) + m_decrypt_rc2_chunk_key_codes[6];
    x54 = (m_decrypt_rc2_chunk_key_codes[5] << 8) + m_decrypt_rc2_chunk_key_codes[4];
    x32 = (m_decrypt_rc2_chunk_key_codes[3] << 8) + m_decrypt_rc2_chunk_key_codes[2];
    x10 = (m_decrypt_rc2_chunk_key_codes[1] << 8) + m_decrypt_rc2_chunk_key_codes[0];

    i = 15;
    do {
      x76 &= 65535;
      x76 = (x76 << 11) + (x76 >> 5);
      x76 -= (x10 & ~x54) + (x32 & x54) + key[4*i+3];

      x54 &= 65535;
      x54 = (x54 << 13) + (x54 >> 3);
      x54 -= (x76 & ~x32) + (x10 & x32) + key[4*i+2];

      x32 &= 65535;
      x32 = (x32 << 14) + (x32 >> 2);
      x32 -= (x54 & ~x10) + (x76 & x10) + key[4*i+1];

      x10 &= 65535;
      x10 = (x10 << 15) + (x10 >> 1);
      x10 -= (x32 & ~x76) + (x54 & x76) + key[4*i+0];

      if (i == 5 || i == 11) {
        x76 -= key[x54 & 63];
        x54 -= key[x32 & 63];
        x32 -= key[x10 & 63];
        x10 -= key[x76 & 63];
      }
    } while (i--);

    m_rc2_chunk_out.resize(8);

#define APPEND_UNICODE_CHAR(code, i) \
    m_rc2_chunk_out[i] = QChar(code)

    APPEND_UNICODE_CHAR(x10 & 255, 0);
    APPEND_UNICODE_CHAR((x10 >> 8) & 255, 1);
    APPEND_UNICODE_CHAR(x32 & 255, 2);
    APPEND_UNICODE_CHAR((x32 >> 8) & 255, 3);
    APPEND_UNICODE_CHAR(x54 & 255, 4);
    APPEND_UNICODE_CHAR((x54 >> 8) & 255, 5);
    APPEND_UNICODE_CHAR(x76 & 255, 6);
    APPEND_UNICODE_CHAR((x76 >> 8) & 255, 7);

#undef APPEND_UNICODE_CHAR

    QByteArray outData = m_rc2_chunk_out.toLocal8Bit();
    QString out = QString::fromUtf8(outData.constData(), outData.size());
    return out;
}

// WARNING: the implementation was ported from JavaScript taken from Evernote site
// so it contains dangerous magic. Don't touch that unless you know what you're doing!
qint32 EncryptionManagerPrivate::crc32(const QString & str) const
{
    qint32 crc = 0;
    qint32 x = 0;
    qint32 y = 0;

    crc ^= (-1);

    const QByteArray strData = str.toLocal8Bit();
    const int size = strData.size();

    QVector<int> convertedCharCodes;
    convertedCharCodes.resize(size);
    for(int i = 0; i < size; ++i)
    {
        int & code = convertedCharCodes[i];
        code = static_cast<int>(strData[i]);
        if (code < 0) {
            code += 256;
        }
    }

    QString x_str;
    for(int i = 0; i < size; ++i)
    {
        y = (crc ^ convertedCharCodes[i]) & 0xFF;
        x_str = crc32table.mid(y * 9, 8);
        bool conversionResult = false;
        x = x_str.toUInt(&conversionResult, 16);
        if (Q_UNLIKELY(!conversionResult)) {
            QNCRITICAL("Can't convert string representation of hex number "
                       << x_str << " to unsigned int!");
            crc = 0;
            return crc;
        }

        quint32 unsignedCrc = static_cast<quint32>(crc);
        unsignedCrc >>= 8;
        crc = static_cast<qint32>(unsignedCrc);
        crc ^= x;
    }

    return (crc ^ (-1));
}

} // namespace qute_note
