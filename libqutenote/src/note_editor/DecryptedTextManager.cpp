#include <qute_note/note_editor/DecryptedTextManager.h>
#include "DecryptedTextManager_p.h"

namespace qute_note {

void DecryptedTextManager::addEntry(const QString & hash, const QString & decryptedText,
                                    const bool rememberForSession, const QString & passphrase,
                                    const QString & cipher, const size_t keyLength)
{
    Q_D(DecryptedTextManager);
    d->addEntry(hash, decryptedText, rememberForSession, passphrase, cipher, keyLength);
}

bool DecryptedTextManager::findDecryptedText(const QString & passphrase, QString & decryptedText,
                                             bool & rememberForSession) const
{
    Q_D(const DecryptedTextManager);
    return d->findDecryptedText(passphrase, decryptedText, rememberForSession);
}

bool DecryptedTextManager::rehashDecryptedText(const QString & originalHash, const QString & newDecryptedText,
                                               QString & newHash)
{
    Q_D(DecryptedTextManager);
    return d->rehashDecryptedText(originalHash, newDecryptedText, newHash);
}

} // namespace qute_note
