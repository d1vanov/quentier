#include "DecryptedTextManager_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

DecryptedTextManagerPrivate::DecryptedTextManagerPrivate() :
    m_dataHash(),
    m_encryptionManager()
{}

void DecryptedTextManagerPrivate::addEntry(const QString & hash, const QString & decryptedText,
                                           const bool rememberForSession, const QString & passphrase,
                                           const QString & cipher, const size_t keyLength)
{
    QNDEBUG("DecryptedTextManagerPrivate::addEntry: hash = " << hash << ", rememberForSession = "
            << (rememberForSession ? "true" : "false"));

    if (passphrase.isEmpty()) {
        QNWARNING("detected attempt to add decrypted text for empty passphrase to decrypted text manager");
        return;
    }

    Data & entry = m_dataHash[passphrase];
    entry.m_decryptedText = decryptedText;
    entry.m_hash = hash;
    entry.m_cipher = cipher;
    entry.m_keyLength = keyLength;
    entry.m_rememberForSession = rememberForSession;
}

bool DecryptedTextManagerPrivate::findDecryptedText(const QString & passphrase, QString & decryptedText,
                                                    bool & rememberForSession) const
{
    QNDEBUG("DecryptedTextManagerPrivate::findDecryptedText");

    DataHash::const_iterator it = m_dataHash.find(passphrase);
    if (it == m_dataHash.end()) {
        QNDEBUG("Could not find decrypted text");
        return false;
    }

    const Data & entry = it.value();
    decryptedText = entry.m_decryptedText;
    rememberForSession = entry.m_rememberForSession;
    return true;
}

bool DecryptedTextManagerPrivate::rehashDecryptedText(const QString & originalHash, const QString & newDecryptedText,
                                                      QString & newHash)
{
    QNDEBUG("DecryptedTextManagerPrivate::rehashDecryptedText: originalHash = " << originalHash
            << ", new decrypted text = " << newDecryptedText);

    typedef DataHash::iterator Iter;
    Iter dataHashEnd = m_dataHash.end();
    Iter itemIt = dataHashEnd;

    for(Iter it = m_dataHash.begin(); it != dataHashEnd; ++it)
    {
        if (it.value().m_hash == originalHash) {
            itemIt = it;
            break;
        }
    }

    if (itemIt == dataHashEnd) {
        QNDEBUG("Could not find original hash");
        return false;
    }

    const QString & passphrase = itemIt.key();
    Data & entry = itemIt.value();
    QString errorDescription;
    bool res = m_encryptionManager.encrypt(newDecryptedText, passphrase, entry.m_cipher, entry.m_keyLength, newHash, errorDescription);
    if (!res) {
        QNWARNING("Could not rehash the decrypted text: " << errorDescription);
        return false;
    }

    entry.m_hash = newHash;
    entry.m_decryptedText = newDecryptedText;
    return true;
}

} // namespace qute_note
