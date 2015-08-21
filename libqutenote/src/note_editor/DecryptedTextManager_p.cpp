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
    entry.m_encryptedText = hash;
    entry.m_cipher = cipher;
    entry.m_keyLength = keyLength;
    entry.m_rememberForSession = rememberForSession;
}

void DecryptedTextManagerPrivate::clearNonRememberedForSessionEntries()
{
    QNDEBUG("DecryptedTextManagerPrivate::clearNonRememberedForSessionEntries");

    for(auto it = m_dataHash.begin(); it != m_dataHash.end();)
    {
        const Data & data = it.value();
        if (!data.m_rememberForSession) {
            it = m_dataHash.erase(it);
        }
        else {
            ++it;
        }
    }
}

bool DecryptedTextManagerPrivate::findDecryptedTextByPassphrase(const QString & passphrase, QString & decryptedText,
                                                                bool & rememberForSession) const
{
    QNDEBUG("DecryptedTextManagerPrivate::findDecryptedTextByPassphrase");

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

bool DecryptedTextManagerPrivate::findDecryptedTextByEncryptedText(const QString & encryptedText,
                                                                   QString & decryptedText,
                                                                   bool & rememberForSession) const
{
    QNDEBUG("DecryptedTextManagerPrivate::findDecryptedTextByEncryptedText: " << encryptedText);

    typedef DataHash::const_iterator CIter;
    CIter dataHashEnd = m_dataHash.end();

    for(CIter it = m_dataHash.begin(); it != dataHashEnd; ++it)
    {
        const Data & data = it.value();
        if (data.m_encryptedText == encryptedText) {
            decryptedText = data.m_decryptedText;
            rememberForSession = data.m_rememberForSession;
            return true;
        }
    }

    return false;
}

bool DecryptedTextManagerPrivate::modifyDecryptedText(const QString & originalEncryptedText,
                                                      const QString & newDecryptedText,
                                                      QString & newEncryptedText)
{
    QNDEBUG("DecryptedTextManagerPrivate::modifyDecryptedText: original decrypted text = "
            << originalEncryptedText << ", new decrypted text = " << newDecryptedText);

    typedef DataHash::iterator Iter;
    Iter dataHashEnd = m_dataHash.end();
    Iter itemIt = dataHashEnd;

    for(Iter it = m_dataHash.begin(); it != dataHashEnd; ++it)
    {
        if (it.value().m_encryptedText == originalEncryptedText) {
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
    bool res = m_encryptionManager.encrypt(newDecryptedText, passphrase, entry.m_cipher, entry.m_keyLength, newEncryptedText, errorDescription);
    if (!res) {
        QNWARNING("Could not rehash the decrypted text: " << errorDescription);
        return false;
    }

    entry.m_encryptedText = newEncryptedText;
    entry.m_decryptedText = newDecryptedText;
    return true;
}

} // namespace qute_note
