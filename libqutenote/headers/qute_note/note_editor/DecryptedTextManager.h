#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_MANAGER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_MANAGER_H

#include <qute_note/utility/Linkage.h>
#include <QtGlobal>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManagerPrivate)

class QUTE_NOTE_EXPORT DecryptedTextManager
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

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_MANAGER_H
