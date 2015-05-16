#include "EncryptionManager_p.h"
#include <logging/QuteNoteLogger.h>
#include <tools/DesktopServices.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

// Evernote service defined constants
#define NUM_ITERATIONS (50000)
#define NUM_SALT_BYTES (16)

namespace qute_note {

EncryptionManagerPrivate::EncryptionManagerPrivate() :
    m_salt(),
    m_saltmac(),
    m_iv()
{
    for(int i = 0; i < NUM_SALT_BYTES; ++i) {
        m_salt[i] = '0';
        m_saltmac[i] = '0';
        m_iv[i] = '0';
    }
}

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
    encryptedText.resize(0);
    encryptedText += "ENCO";

    ErrorStringsHolder errorStringsHolder;
    Q_UNUSED(errorStringsHolder)

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

    appendUnsignedCharToQString(encryptedText, m_salt, NUM_SALT_BYTES);
    appendUnsignedCharToQString(encryptedText, m_saltmac, NUM_SALT_BYTES);
    appendUnsignedCharToQString(encryptedText, m_iv, NUM_SALT_BYTES);

    // TODO: continue from here

    // TODO: implement
    Q_UNUSED(textToEncrypt);
    Q_UNUSED(passphrase);
    Q_UNUSED(cipher);
    Q_UNUSED(keyLength);
    Q_UNUSED(encryptedText);
    Q_UNUSED(errorDescription);
    return false;
}

bool EncryptionManagerPrivate::generateRandomBytes(const EncryptionManagerPrivate::SaltKind::type saltKind,
                                                   QString & errorDescription)
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
    default:
        errorDescription = QT_TR_NOOP("Internal error, detected incorrect salt kind for cryptographic key generation");
        QNCRITICAL(errorDescription);
        return false;
    }

    int res = RAND_bytes(salt, NUM_SALT_BYTES);
    if (res != 1) {
        errorDescription = QT_TR_NOOP("Can't generate cryptographically strong bytes for encryption");
        GET_OPENSSL_ERROR;
        QNWARNING(errorDescription << "; " << saltText << ": lib: " << errorLib
                  << "; func: " << errorFunc << ", reason: " << errorReason);
        return false;
    }

    return true;
}

EncryptionManagerPrivate::ErrorStringsHolder::ErrorStringsHolder()
{
    SSL_load_error_strings();
}

EncryptionManagerPrivate::ErrorStringsHolder::~ErrorStringsHolder()
{
    ERR_free_strings();
}

} // namespace qute_note
