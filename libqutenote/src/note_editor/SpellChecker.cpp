#include "SpellChecker.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/utility/ApplicationSettings.h>
#include <qute_note/utility/DesktopServices.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <hunspell/hunspell.hxx>
#include <QFileInfo>
#include <QDir>
#include <algorithm>

namespace qute_note {

SpellChecker::SpellChecker(FileIOThreadWorker * pFileIOThreadWorker, QObject * parent, const QString & userDictionaryPath) :
    QObject(parent),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_systemDictionaries(),
    m_userDictionary()
{
    initializeUserDictionary(userDictionaryPath);
    scanSystemDictionaries();
}

bool SpellChecker::isValid() const
{
    // TODO: implement
    return true;
}

bool SpellChecker::checkSpell(const QString & word) const
{
    // TODO: implement
    Q_UNUSED(word)
    return true;
}

QStringList SpellChecker::spellCorrectionSuggestions(const QString & misSpelledWord) const
{
    QNDEBUG("SpellChecker::spellCorrectionSuggestions: " << misSpelledWord);

    // TODO: implement
    return QStringList();
}

void SpellChecker::addToUserWordlist(const QString & word)
{
    QNDEBUG("SpellChecker::addToUserWordlist: " << word);

    // TODO: implement
}

void SpellChecker::ignoreWord(const QString & word)
{
    QNDEBUG("SpellChecker::ignoreWord: " << word);

    // TODO: implement
}

void SpellChecker::scanSystemDictionaries()
{
    QNDEBUG("SpellChecker::scanSystemDictionaries");

    // TODO: implement
}

void SpellChecker::initializeUserDictionary(const QString & userDictionaryPath)
{
    QNDEBUG("SpellChecker::initializeUserDictionary: " << (userDictionaryPath.isEmpty() ? QString("<empty>") : userDictionaryPath));

    bool foundValidPath = false;

    if (!userDictionaryPath.isEmpty())
    {
        bool res = checkUserDictionaryPath(userDictionaryPath);
        if (!res) {
            QNINFO("Can't accept the proposed user dictionary path, will use the fallback chain of possible user dictionary paths instead");
        }
        else {
            m_userDictionary.m_dictionaryPath = userDictionaryPath;
            QNDEBUG("Set user dictionary path to " << userDictionaryPath);
            foundValidPath = true;
        }
    }

    if (!foundValidPath)
    {
        ApplicationSettings settings;
        settings.beginGroup("SpellCheck");
        QString userDictionaryPathFromSettings = settings.value("UserDictionaryPath").toString();
        if (!userDictionaryPathFromSettings.isEmpty())
        {
            QNTRACE("Inspecting the user dictionary path found in the application settings");
            bool res = checkUserDictionaryPath(userDictionaryPathFromSettings);
            if (!res) {
                QNINFO("Can't accept the user dictionary path from the application settings: "
                       << userDictionaryPathFromSettings);
            }
            else {
                m_userDictionary.m_dictionaryPath = userDictionaryPathFromSettings;
                QNDEBUG("Set user dictionary path to " << userDictionaryPathFromSettings);
                foundValidPath = true;
            }
        }
    }

    if (!foundValidPath)
    {
        QNTRACE("Haven't found valid user dictionary file path within the app settings, fallback to the default path");

        QString fallbackUserDictionaryPath = applicationPersistentStoragePath() + "/spellcheck/user";
        bool res = checkUserDictionaryPath(fallbackUserDictionaryPath);
        if (!res) {
            QNINFO("Can't accept even the fallback default path");
        }
        else {
            m_userDictionary.m_dictionaryPath = fallbackUserDictionaryPath;
            QNDEBUG("Set user dictionary path to " << fallbackUserDictionaryPath);
            foundValidPath = true;
        }
    }

    if (foundValidPath)
    {
        QString userDictionaryFilePath = m_userDictionary.m_dictionaryPath + ".dic";
        QString userAffixFilePath = m_userDictionary.m_dictionaryPath + ".aff";

        m_userDictionary.m_pHunspell = QSharedPointer<Hunspell>(new Hunspell(userDictionaryFilePath.toLocal8Bit().constData(),
                                                                             userAffixFilePath.toLocal8Bit().constData()));

        // TODO: continue the initialization
    }
    else
    {
        QNINFO("please specify the valid path for the user dictionary "
               "under UserDictionaryPath entry in SpellCheck section of application settings");
    }
}

bool SpellChecker::checkUserDictionaryPath(const QString & userDictionaryPath) const
{
    QFileInfo info(userDictionaryPath);
    if (info.exists())
    {
        if (info.isDir()) {
            return true;
        }
        else {
            QNINFO("User dictionary path candidate " << userDictionaryPath << " is not a directory");
            return false;
        }
    }
    else
    {
        QDir dir;
        bool res = dir.mkpath(userDictionaryPath);
        if (!res) {
            QNWARNING("Can't create not yet existing user dictionary path candidate folder");
            return false;
        }
        else {
            return true;
        }
    }
}

void SpellChecker::onUserDictionaryFileRead(bool success, QString errorDescription, QByteArray data, QUuid requestId)
{
    QNDEBUG("SpellChecker::onUserDictionaryFileRead: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId);

    m_userDictionary.m_dictionaryFileData = data;
    m_userDictionary.m_dictionaryFileRead = true;

    checkAndNotifyDictionariesReadiness();
}

void SpellChecker::onUserAffixFileRead(bool success, QString & errorDescription, QByteArray data, QUuid requestId)
{
    QNDEBUG("SpellChecker::onUserAffixFileRead: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId);

    m_userDictionary.m_affixFileData = data;
    m_userDictionary.m_affixFileRead = true;

    checkAndNotifyDictionariesReadiness();
}

void SpellChecker::onSystemDictionaryFileRead(bool success, QString & errorDescription, QByteArray data, QUuid requestId,
                                              const qint64 dictionaryIndex)
{
    QNDEBUG("SpellChecker::onSystemDictionaryFileRead: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId
            << ", dictionary index = " << dictionaryIndex);

    if (Q_UNLIKELY(m_systemDictionaries.size() <= dictionaryIndex)) {
        QNWARNING("Detected system dictionary file read event but the dictionary index is wrong: " << dictionaryIndex
                  << " while there are only " << m_systemDictionaries.size() << " system dictionaries");
        return;
    }

    Dictionary & dictionary = m_systemDictionaries[static_cast<int>(dictionaryIndex)];
    dictionary.m_dictionaryFileData = data;
    dictionary.m_dictionaryFileRead = true;

    checkAndNotifyDictionariesReadiness();
}

void SpellChecker::onSystemAffixFileRead(bool success, QString & errorDescription, QByteArray data, QUuid requestId,
                                         const qint64 dictionaryIndex)
{
    QNDEBUG("SpellChecker::onSystemAffixFileRead: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId
            << ", dictionary index = " << dictionaryIndex);

    if (Q_UNLIKELY(m_systemDictionaries.size() <= dictionaryIndex)) {
        QNWARNING("Detected system affix file read event but the dictionary index is wrong: " << dictionaryIndex
                  << " while there are only " << m_systemDictionaries.size() << " system dictionaries");
        return;
    }

    Dictionary & dictionary = m_systemDictionaries[static_cast<int>(dictionaryIndex)];
    dictionary.m_affixFileData = data;
    dictionary.m_affixFileRead = true;

    checkAndNotifyDictionariesReadiness();
}

void SpellChecker::checkAndNotifyDictionariesReadiness()
{
    QNDEBUG("SpellChecker::checkAndNotifyDictionariesReadiness");

    if (!m_userDictionary.m_affixFileRead || !m_userDictionary.m_dictionaryFileRead) {
        QNTRACE("User dictionary files are not read yet");
        return;
    }

    for(auto it = m_systemDictionaries.begin(), end = m_systemDictionaries.end(); it != end; ++it)
    {
        const Dictionary & dictionary = *it;

        if (!dictionary.m_affixFileRead || !dictionary.m_dictionaryFileRead) {
            QNTRACE("One of system dictionaries is not ready yet");
            return;
        }
    }

    QNTRACE("All dictionary files are ready");
    emit ready();
}

void SpellChecker::onReadFileRequestProcessed(bool success, QString errorDescription, QByteArray data, QUuid requestId)
{
    if (requestId == m_userDictionary.m_readDictionaryFileRequestId)
    {
        onUserDictionaryFileRead(success, errorDescription, data, requestId);
    }
    else if (requestId == m_userDictionary.m_readAffixFileRequestId)
    {
        onUserAffixFileRead(success, errorDescription, data, requestId);
    }
    else
    {
        auto dictionaryIt = std::find_if(m_systemDictionaries.begin(), m_systemDictionaries.end(),
                                         CompareByReadDictionaryFileRequestId(requestId));
        if (dictionaryIt != m_systemDictionaries.end()) {
            qint64 dictionaryIndex = std::distance(m_systemDictionaries.begin(), dictionaryIt);
            onSystemDictionaryFileRead(success, errorDescription, data, requestId, dictionaryIndex);
        }
        else
        {
            dictionaryIt = std::find_if(m_systemDictionaries.begin(), m_systemDictionaries.end(),
                                        CompareByReadAffixFileRequestId(requestId));
            if (dictionaryIt != m_systemDictionaries.end()) {
                qint64 dictionaryIndex = std::distance(m_systemDictionaries.begin(), dictionaryIt);
                onSystemAffixFileRead(success, errorDescription, data, requestId, dictionaryIndex);
            }
        }
    }
}

void SpellChecker::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(success)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

SpellChecker::Dictionary::Dictionary() :
    m_pHunspell(),
    m_dictionaryPath(),
    m_readDictionaryFileRequestId(),
    m_readAffixFileRequestId(),
    m_dictionaryFileData(),
    m_affixFileData(),
    m_dictionaryFileRead(false),
    m_affixFileRead(false)
{}

bool SpellChecker::Dictionary::isEmpty() const
{
    return m_dictionaryPath.isEmpty() || m_pHunspell.isNull();
}

} // namespace qute_note
