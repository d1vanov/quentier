#include "DecryptedTextManager_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

DecryptedTextManagerPrivate::DecryptedTextManagerPrivate() :
    m_dataHash(),
    m_staleDataHash(),
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

    Data & entry = m_dataHash[hash];
    entry.m_decryptedText = decryptedText;
    entry.m_passphrase = passphrase;
    entry.m_cipher = cipher;
    entry.m_keyLength = keyLength;
    entry.m_rememberForSession = rememberForSession;
}

void DecryptedTextManagerPrivate::removeEntry(const QString & hash)
{
    QNDEBUG("DecryptedTextManagerPrivate::removeEntry: hash = " << hash);

    auto it = m_dataHash.find(hash);
    if (it != m_dataHash.end()) {
        Q_UNUSED(m_dataHash.erase(it));
        return;
    }

    it = m_staleDataHash.find(hash);
    Q_UNUSED(m_staleDataHash.erase(it));
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

    // Also clear the stale data hash here as it shouldn't be needed after this call
    m_staleDataHash.clear();
}

bool DecryptedTextManagerPrivate::findDecryptedTextByEncryptedText(const QString & encryptedText,
                                                                   QString & decryptedText,
                                                                   bool & rememberForSession) const
{
    QNDEBUG("DecryptedTextManagerPrivate::findDecryptedTextByEncryptedText: " << encryptedText);

    DataHash::const_iterator dataIt = m_dataHash.find(encryptedText);
    if (dataIt == m_dataHash.end())
    {
        QNTRACE("Can't find the entry in the up to date data hash, trying the stale hash");

        // Try the stale data hash
        dataIt = m_staleDataHash.find(encryptedText);
        if (dataIt == m_staleDataHash.end()) {
            QNTRACE("Can't find the entry in the stale data hash as well");
            return false;
        }
    }

    const Data & data = dataIt.value();
    decryptedText = data.m_decryptedText;
    rememberForSession = data.m_rememberForSession;
    QNTRACE("Found decrypted text");
    return true;
}

bool DecryptedTextManagerPrivate::modifyDecryptedText(const QString & originalEncryptedText,
                                                      const QString & newDecryptedText,
                                                      QString & newEncryptedText)
{
    QNDEBUG("DecryptedTextManagerPrivate::modifyDecryptedText: original decrypted text = "
            << originalEncryptedText);

    bool foundInDataHash = true;
    DataHash::iterator it = m_dataHash.find(originalEncryptedText);
    if (it == m_dataHash.end())
    {
        foundInDataHash = false;
        // Try the stale data hash instead
        it = m_staleDataHash.find(originalEncryptedText);
        if (it == m_staleDataHash.end()) {
            QNDEBUG("Could not find original hash");
            return false;
        }
    }

    Data & entry = it.value();
    const QString & passphrase = entry.m_passphrase;
    QNLocalizedString errorDescription;
    bool res = m_encryptionManager.encrypt(newDecryptedText, passphrase, entry.m_cipher,
                                           entry.m_keyLength, newEncryptedText, errorDescription);
    if (!res) {
        QNWARNING("Could not re-encrypt the decrypted text: " << errorDescription);
        return false;
    }

    if (foundInDataHash)
    {
        // Copy the previous entry's stale data to the stale data hash
        // in case it would be needed further
        Data & staleEntry = m_staleDataHash[originalEncryptedText];
        staleEntry.m_cipher = entry.m_cipher;
        staleEntry.m_keyLength = entry.m_keyLength;
        staleEntry.m_rememberForSession = entry.m_rememberForSession;
        staleEntry.m_decryptedText = entry.m_decryptedText;
        staleEntry.m_passphrase = entry.m_passphrase;

        m_dataHash.erase(it);
        Data & newEntry = m_dataHash[newEncryptedText];
        newEntry.m_cipher = staleEntry.m_cipher;
        newEntry.m_keyLength = staleEntry.m_keyLength;
        newEntry.m_rememberForSession = staleEntry.m_rememberForSession;
        newEntry.m_decryptedText = newDecryptedText;
        newEntry.m_passphrase = staleEntry.m_passphrase;

        return true;
    }
    else
    {
        Data & dataEntry = m_dataHash[newEncryptedText];
        dataEntry.m_cipher = entry.m_cipher;
        dataEntry.m_keyLength = entry.m_keyLength;
        dataEntry.m_rememberForSession = entry.m_rememberForSession;
        dataEntry.m_decryptedText = newDecryptedText;
        dataEntry.m_passphrase = entry.m_passphrase;

        return true;
    }
}

} // namespace quentier
