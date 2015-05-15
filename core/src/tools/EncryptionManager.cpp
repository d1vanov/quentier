#include "EncryptionManager.h"
#include "EncryptionManager_p.h"

namespace qute_note {

EncryptionManager::EncryptionManager(QObject * parent) :
    QObject(parent),
    d_ptr(new EncryptionManagerPrivate)
{}

EncryptionManager::~EncryptionManager()
{}

bool EncryptionManager::decrypt(const QString & encryptedText, const QString & passphrase,
                                const QString & cipher, const size_t keyLength,
                                QString & decryptedText, QString & errorDescription)
{
    Q_D(EncryptionManager);
    return d->decrypt(encryptedText, passphrase, cipher, keyLength, decryptedText, errorDescription);
}

bool EncryptionManager::encrypt(const QString & textToEncrypt, const QString & passphrase,
                                const QString & cipher, const size_t keyLength,
                                QString & encryptedText, QString & errorDescription)
{
    Q_D(EncryptionManager);
    return d->encrypt(textToEncrypt, passphrase, cipher, keyLength, encryptedText, errorDescription);
}

void EncryptionManager::onDecryptTextRequest(QString encryptedText, QString passphrase,
                                             QString cipher, size_t keyLength, QUuid requestId)
{
    QString decrypted;
    QString errorDescription;
    bool res = decrypt(encryptedText, passphrase, cipher, keyLength, decrypted, errorDescription);
    emit decryptedText(decrypted, res, errorDescription, requestId);
}

void EncryptionManager::onEncryptTextRequest(QString textToEncrypt, QString passphrase,
                                             QString cipher, size_t keyLength, QUuid requestId)
{
    QString encrypted;
    QString errorDescription;
    bool res = encrypt(textToEncrypt, passphrase, cipher, keyLength, encrypted, errorDescription);
    emit encryptedText(encrypted, res, errorDescription, requestId);
}

} // namespace qute_note
