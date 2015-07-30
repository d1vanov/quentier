#ifndef __LIB_QUTE_NOTE__UTILITY__ENCRYPTION_MANAGER_H
#define __LIB_QUTE_NOTE__UTILITY__ENCRYPTION_MANAGER_H

#include <qute_note/utility/Linkage.h>
#include <QObject>
#include <QString>
#include <QUuid>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(EncryptionManagerPrivate)

/**
 * @brief The EncryptionManager class provides both synchronous methods to encrypt or decrypt
 * given text with password, cipher and key length and their signal-slot based potentially asynchronous
 * counterparts
 */
class QUTE_NOTE_EXPORT EncryptionManager: public QObject
{
    Q_OBJECT
public:
    explicit EncryptionManager(QObject * parent = nullptr);
    virtual ~EncryptionManager();

    bool decrypt(const QString & encryptedText, const QString & passphrase,
                 const QString & cipher, const size_t keyLength,
                 QString & decryptedText, QString & errorDescription);

    bool encrypt(const QString & textToEncrypt, const QString & passphrase,
                 QString & cipher, size_t & keyLength,
                 QString & encryptedText, QString & errorDescription);

Q_SIGNALS:
    void decryptedText(QString text, bool success, QString errorDescription, QUuid requestId);
    void encryptedText(QString encryptedText, bool success, QString errorDescription, QUuid requestId);

public Q_SLOTS:
    void onDecryptTextRequest(QString encryptedText, QString passphrase,
                              QString cipher, size_t keyLength, QUuid requestId);
    void onEncryptTextRequest(QString textToEncrypt, QString passphrase,
                              QString cipher, size_t keyLength, QUuid requestId);

private:
    EncryptionManagerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(EncryptionManager)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__ENCRYPTION_MANAGER_H
