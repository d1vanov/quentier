#ifndef LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_H
#define LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_H

#include <quentier/utility/Linkage.h>
#include <QtGlobal>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManagerPrivate)

class QUENTIER_EXPORT DecryptedTextManager
{
public:
    DecryptedTextManager();
    virtual ~DecryptedTextManager();

    void addEntry(const QString & hash, const QString & decryptedText,
                  const bool rememberForSession, const QString & passphrase,
                  const QString & cipher, const size_t keyLength);

    void removeEntry(const QString & hash);

    void clearNonRememberedForSessionEntries();

    bool findDecryptedTextByEncryptedText(const QString & encryptedText, QString & decryptedText,
                                          bool & rememberForSession) const;

    bool modifyDecryptedText(const QString & originalEncryptedText, const QString & newDecryptedText,
                             QString & newEncryptedText);

private:
    Q_DISABLE_COPY(DecryptedTextManager)

    DecryptedTextManagerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(DecryptedTextManager)
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_H
