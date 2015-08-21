#include <qute_note/note_editor/DecryptedTextManager.h>
#include "DecryptedTextManager_p.h"

namespace qute_note {

DecryptedTextManager::DecryptedTextManager() :
    d_ptr(new DecryptedTextManagerPrivate)
{}

DecryptedTextManager::~DecryptedTextManager()
{
    delete d_ptr;
}

void DecryptedTextManager::addEntry(const QString & hash, const QString & decryptedText,
                                    const bool rememberForSession, const QString & passphrase,
                                    const QString & cipher, const size_t keyLength)
{
    Q_D(DecryptedTextManager);
    d->addEntry(hash, decryptedText, rememberForSession, passphrase, cipher, keyLength);
}

void DecryptedTextManager::clearNonRememberedForSessionEntries()
{
    Q_D(DecryptedTextManager);
    d->clearNonRememberedForSessionEntries();
}

bool DecryptedTextManager::findDecryptedTextByPassphrase(const QString & passphrase, QString & decryptedText,
                                                         bool & rememberForSession) const
{
    Q_D(const DecryptedTextManager);
    return d->findDecryptedTextByPassphrase(passphrase, decryptedText, rememberForSession);
}

bool DecryptedTextManager::findDecryptedTextByEncryptedText(const QString & encryptedText,
                                                            QString & decryptedText,
                                                            bool & rememberForSession) const
{
    Q_D(const DecryptedTextManager);
    return d->findDecryptedTextByEncryptedText(encryptedText, decryptedText, rememberForSession);
}

bool DecryptedTextManager::modifyDecryptedText(const QString & originalEncryptedText,
                                               const QString & newDecryptedText,
                                               QString & newEncryptedText)
{
    Q_D(DecryptedTextManager);
    return d->modifyDecryptedText(originalEncryptedText, newDecryptedText, newEncryptedText);
}

} // namespace qute_note
