#include "SpellChecker.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/utility/ApplicationSettings.h>
#include <qute_note/utility/DesktopServices.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <hunspell/hunspell.hxx>
#include <QFileInfo>
#include <QDir>
#include <QBuffer>
#include <algorithm>

namespace qute_note {

SpellChecker::SpellChecker(FileIOThreadWorker * pFileIOThreadWorker, QObject * parent, const QString & userDictionaryPath) :
    QObject(parent),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_systemDictionaries(),
    m_readUserDictionaryRequestId(),
    m_userDictionaryPath(),
    m_userDictionary(),
    m_userDictionaryReady(false)
{
    initializeUserDictionary(userDictionaryPath);
    scanSystemDictionaries();
}

QVector<QPair<QString,bool> > SpellChecker::listAvailableDictionaries() const
{
    QNDEBUG("SpellChecker::listAvailableDictionaries");

    QVector<QPair<QString,bool> > result;
    result.reserve(m_systemDictionaries.size());

    for(auto it = m_systemDictionaries.begin(), end = m_systemDictionaries.end(); it != end; ++it)
    {
        const QString & language = it.key();
        const Dictionary & dictionary = it.value();

        result << QPair<QString,bool>(language, dictionary.m_enabled);
    }

    return result;
}

void SpellChecker::enableDictionary(const QString & language)
{
    QNDEBUG("SpellChecker::enableDictionary: language = " << language);

    auto it = m_systemDictionaries.find(language);
    if (it == m_systemDictionaries.end()) {
        QNINFO("Can't enable dictionary: no dictionary was found for language " << language);
        return;
    }

    it.value().m_enabled = true;
}

void SpellChecker::disableDictionary(const QString & language)
{
    QNDEBUG("SpellChecker::disableDictionary: language = " << language);

    auto it = m_systemDictionaries.find(language);
    if (it == m_systemDictionaries.end()) {
        QNINFO("Can't disable dictionary: no dictionary was found for language " << language);
        return;
    }

    it.value().m_enabled = false;
}

bool SpellChecker::checkSpell(const QString & word) const
{
    if (m_userDictionary.contains(word, Qt::CaseInsensitive)) {
        return true;
    }

    QByteArray wordData = word.toUtf8();

    for(auto it = m_systemDictionaries.begin(), end = m_systemDictionaries.end(); it != end; ++it)
    {
        const Dictionary & dictionary = it.value();

        if (dictionary.isEmpty() || !dictionary.m_enabled) {
            continue;
        }

        int res = dictionary.m_pHunspell->spell(wordData.constData());
        if (res != 0) {
            return true;
        }
    }

    return false;
}

QStringList SpellChecker::spellCorrectionSuggestions(const QString & misSpelledWord) const
{
    QNDEBUG("SpellChecker::spellCorrectionSuggestions: " << misSpelledWord);

    QByteArray wordData = misSpelledWord.toUtf8();

    QStringList result;
    for(auto it = m_systemDictionaries.begin(), end = m_systemDictionaries.end(); it != end; ++it)
    {
        const Dictionary & dictionary = it.value();

        if (dictionary.isEmpty() || !dictionary.m_enabled) {
            continue;
        }

        char **rawCorrectionSuggestions = Q_NULLPTR;

        int numSuggestions = dictionary.m_pHunspell->suggest(&rawCorrectionSuggestions, wordData.constData());
        for(int i = 0; i < numSuggestions; ++i) {
            result << QString::fromUtf8(rawCorrectionSuggestions[i]);
            free(rawCorrectionSuggestions[i]);
        }
    }

    return result;
}

void SpellChecker::addToUserWordlist(const QString & word)
{
    QNDEBUG("SpellChecker::addToUserWordlist: " << word);

    ignoreWord(word);

    m_userDictionaryPartPendingWriting << word;
    checkUserDictionaryDataPendingWriting();
}

void SpellChecker::ignoreWord(const QString & word)
{
    QNDEBUG("SpellChecker::ignoreWord: " << word);

    QByteArray wordData = word.toUtf8();

    for(auto it = m_systemDictionaries.begin(), end = m_systemDictionaries.end(); it != end; ++it)
    {
        Dictionary & dictionary = it.value();

        if (dictionary.isEmpty() || !dictionary.m_enabled) {
            continue;
        }

        dictionary.m_pHunspell->add(wordData.constData());
    }
}

void SpellChecker::scanSystemDictionaries()
{
    QNDEBUG("SpellChecker::scanSystemDictionaries");

    // First try to look for the paths to dictionaries at the environment variables;
    // probably that is the only way to get path to system wide dictionaries on Windows

    QString envVarSeparator;
#ifdef Q_OS_WIN
    envVarSeparator = ":";
#else
    envVarSeparator = ";";
#endif

    QString ownDictionaryNames = QString::fromLocal8Bit(qgetenv("LIBQUTENOTEDICTNAMES"));
    QString ownDictionaryPaths = QString::fromLocal8Bit(qgetenv("LIBQUTENOTEDICTPATHS"));
    if (!ownDictionaryNames.isEmpty() && !ownDictionaryPaths.isEmpty())
    {
        QStringList ownDictionaryNamesList = ownDictionaryNames.split(envVarSeparator, QString::SkipEmptyParts,
                                                                      Qt::CaseInsensitive);
        QStringList ownDictionaryPathsList = ownDictionaryPaths.split(envVarSeparator, QString::SkipEmptyParts,
                                                                      Qt::CaseInsensitive);
        const int numDictionaries = ownDictionaryNamesList.size();
        if (numDictionaries == ownDictionaryPathsList.size())
        {
            for(int i = 0; i < numDictionaries; ++i)
            {
                QString path = ownDictionaryPathsList[i];
                path = QDir::fromNativeSeparators(path);

                const QString & name = ownDictionaryNamesList[i];

                addSystemDictionary(path, name);
            }
        }
        else
        {
            QNTRACE("Number of found paths to dictionaries doesn't correspond to the number of found dictionary names "
                    "as deduced from libqutenote's own environment variables:\n LIBQUTENOTEDICTNAMES: " << ownDictionaryNames
                    << "; \n LIBQUTENOTEDICTPATHS: " << ownDictionaryPaths);
        }
    }
    else
    {
        QNTRACE("Can't find LIBQUTENOTEDICTNAMES and/or LIBQUTENOTEDICTPATHS within the environment variables");
    }

    // Also see if there's something set in the environment variables for the hunspell executable itself
    QString hunspellDictionaryName = QString::fromLocal8Bit(qgetenv("DICTIONARY"));
    QString hunspellDictionaryPath = QString::fromLocal8Bit(qgetenv("DICPATH"));
    if (!hunspellDictionaryName.isEmpty() && !hunspellDictionaryPath.isEmpty())
    {
        // These environment variables are intended to specify the only one dictionary
        int nameSeparatorIndex = hunspellDictionaryName.indexOf(envVarSeparator);
        if (nameSeparatorIndex >= 0) {
            hunspellDictionaryName = hunspellDictionaryName.left(nameSeparatorIndex);
        }

        int nameColonIndex = hunspellDictionaryName.indexOf(",");
        if (nameColonIndex >= 0) {
            hunspellDictionaryName = hunspellDictionaryName.left(nameColonIndex);
        }

        hunspellDictionaryName = hunspellDictionaryName.trimmed();

        int pathSeparatorIndex = hunspellDictionaryPath.indexOf(envVarSeparator);
        if (pathSeparatorIndex >= 0) {
            hunspellDictionaryPath = hunspellDictionaryPath.left(pathSeparatorIndex);
        }

        hunspellDictionaryPath = hunspellDictionaryPath.trimmed();
        hunspellDictionaryPath = QDir::fromNativeSeparators(hunspellDictionaryPath);

        addSystemDictionary(hunspellDictionaryPath, hunspellDictionaryName);
    }
    else
    {
        QNTRACE("Can't find DICTIONARY and/or DICPATH within the environment variables");
    }

#ifndef Q_OS_WIN
    // Now try to look at standard paths

    QStringList standardPaths;

#ifdef Q_OS_MAC
    standardPaths << "/Library/Spelling";
    standardPaths << "~/Library/Spelling";
#endif

    standardPaths << "/usr/share/hunspell";

    QStringList filter;
    filter << "*.dic";  // NOTE: look only for ".dic" files, ".aff" ones would be checked separately

    for(auto it = standardPaths.begin(), end = standardPaths.end(); it != end; ++it)
    {
        const QString & standardPath = *it;
        QNTRACE("Inspecting standard path " << standardPath);

        QDir dir(standardPath);
        if (!dir.exists()) {
            QNTRACE("Skipping dir " << standardPath << " which doesn't exist");
            continue;
        }

        dir.setNameFilters(filter);
        QFileInfoList fileInfos = dir.entryInfoList(QDir::Files);

        for(auto infoIt = fileInfos.begin(), infoEnd = fileInfos.end(); infoIt != infoEnd; ++infoIt)
        {
            const QFileInfo & fileInfo = *infoIt;
            QString fileName = fileInfo.fileName();
            QNTRACE("Inspecting file name " << fileName);

            if (fileName.endsWith(".dic") || fileName.endsWith(".aff")) {
                fileName.chop(4);   // strip off the ".dic" or ".aff" extension
            }
            addSystemDictionary(standardPath, fileName);
        }
    }

#endif
}

void SpellChecker::addSystemDictionary(const QString & path, const QString & name)
{
    QNDEBUG("SpellChecker::addSystemDictionary: path = " << path << ", name = " << name);

    QFileInfo dictionaryFileInfo(path + "/" + name + ".dic");
    if (!dictionaryFileInfo.exists()) {
        QNTRACE("Dictionary file " << dictionaryFileInfo.absoluteFilePath()
                << " doesn't exist");
        return;
    }

    if (!dictionaryFileInfo.isReadable()) {
        QNTRACE("Dictionary file " << dictionaryFileInfo.absoluteFilePath()
                << " is not readable");
        return;
    }

    QFileInfo affixFileInfo(path + "/" + name + ".aff");
    if (!affixFileInfo.exists()) {
        QNTRACE("Affix file " << affixFileInfo.absoluteFilePath()
                << " does not exist");
        return;
    }

    if (!affixFileInfo.isReadable()) {
        QNTRACE("Affix file " << affixFileInfo.absoluteFilePath()
                << " is not readable");
        return;
    }

    QByteArray dictionaryFileInfoPathData = dictionaryFileInfo.absoluteFilePath().toLocal8Bit();
    QByteArray affixFileInfoPathData = affixFileInfo.absoluteFilePath().toLocal8Bit();

    const char * rawDictionaryFilePath = dictionaryFileInfoPathData.constData();
    const char * rawAffixFilePath = affixFileInfoPathData.constData();
    QNTRACE("Raw dictionary file path = " << rawDictionaryFilePath << ", raw affix file path = " << rawAffixFilePath);

    Dictionary & dictionary = m_systemDictionaries[name];
    dictionary.m_pHunspell = QSharedPointer<Hunspell>(new Hunspell(rawAffixFilePath, rawDictionaryFilePath));
    dictionary.m_enabled = true;
    QNTRACE("Added dictionary for language " << name << "; dictionary file " << dictionaryFileInfo.absoluteFilePath()
            << ", affix file " << affixFileInfo.absoluteFilePath());
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
            m_userDictionaryPath = userDictionaryPath;
            QNDEBUG("Set user dictionary path to " << userDictionaryPath);
            foundValidPath = true;
        }
    }

    if (!foundValidPath)
    {
        ApplicationSettings settings;
        settings.beginGroup("SpellCheck");
        QString userDictionaryPathFromSettings = settings.value("UserDictionaryPath").toString();
        settings.endGroup();

        if (!userDictionaryPathFromSettings.isEmpty())
        {
            QNTRACE("Inspecting the user dictionary path found in the application settings");
            bool res = checkUserDictionaryPath(userDictionaryPathFromSettings);
            if (!res) {
                QNINFO("Can't accept the user dictionary path from the application settings: "
                       << userDictionaryPathFromSettings);
            }
            else {
                m_userDictionaryPath = userDictionaryPathFromSettings;
                QNDEBUG("Set user dictionary path to " << userDictionaryPathFromSettings);
                foundValidPath = true;
            }
        }
    }

    if (!foundValidPath)
    {
        QNTRACE("Haven't found valid user dictionary file path within the app settings, fallback to the default path");

        QString fallbackUserDictionaryPath = applicationPersistentStoragePath() + "/spellcheck/user_dictionary.txt";
        bool res = checkUserDictionaryPath(fallbackUserDictionaryPath);
        if (!res) {
            QNINFO("Can't accept even the fallback default path");
        }
        else {
            m_userDictionaryPath = fallbackUserDictionaryPath;
            QNDEBUG("Set user dictionary path to " << fallbackUserDictionaryPath);
            foundValidPath = true;
        }
    }

    if (foundValidPath)
    {
        ApplicationSettings settings;
        settings.beginGroup("SpellCheck");
        settings.setValue("UserDictionaryPath", m_userDictionaryPath);
        settings.endGroup();

        QObject::connect(this, QNSIGNAL(SpellChecker,readFile,QString,QUuid),
                         m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onReadFileRequest,QString,QUuid));
        QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QString,QByteArray,QUuid),
                         this, QNSLOT(SpellChecker,onReadFileRequestProcessed,bool,QString,QByteArray,QUuid));

        m_readUserDictionaryRequestId = QUuid::createUuid();
        emit readFile(m_userDictionaryPath, m_readUserDictionaryRequestId);
        QNTRACE("Sent the request to read the user dictionary file: id = " << m_readUserDictionaryRequestId);
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
        if (!info.isFile()) {
            QNTRACE("User dictionary path candidate is not a file");
            return false;
        }

        if (!info.isReadable() || !info.isWritable())
        {
            QFile file(userDictionaryPath);
            bool res = file.setPermissions(QFile::WriteUser | QFile::ReadUser);
            if (!res) {
                QNTRACE("User dictionary path candidate is a file with insufficient permissions and attempt to fix that has failed: "
                        "readable = " << (info.isReadable() ? "true" : "false") << ", writable = " << (info.isWritable() ? "true" : "false"));
                return false;
            }
        }

        return true;
    }

    QDir dir = info.absoluteDir();
    if (!dir.exists())
    {
        bool res = dir.mkpath(dir.absolutePath());
        if (!res) {
            QNWARNING("Can't create not yet existing user dictionary path candidate folder");
            return false;
        }
    }

    return true;
}

void SpellChecker::checkUserDictionaryDataPendingWriting()
{
    QNDEBUG("SpellChecker::checkUserDictionaryDataPendingWriting");

    if (m_userDictionaryPartPendingWriting.isEmpty()) {
        QNTRACE("Nothing is pending writing");
        return;
    }

    QByteArray dataToWrite;
    for(auto it = m_userDictionaryPartPendingWriting.begin(), end = m_userDictionaryPartPendingWriting.end(); it != end; ++it) {
        m_userDictionary << *it;
        dataToWrite.append(*it);
    }

    if (!dataToWrite.isEmpty())
    {
        QObject::connect(this, QNSIGNAL(SpellChecker,writeFile,QString,QByteArray,QUuid,bool),
                         m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
        QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                         this, QNSLOT(SpellChecker,onWriteFileRequestProcessed,bool,QString,QUuid));

        m_appendUserDictionaryPartToFileRequestId = QUuid::createUuid();
        emit writeFile(m_userDictionaryPath, dataToWrite, m_appendUserDictionaryPartToFileRequestId, /* append = */ true);
        QNTRACE("Sent the request to append the data pending writing to user dictionary, id = "
                << m_appendUserDictionaryPartToFileRequestId);
    }

    m_userDictionaryPartPendingWriting.clear();
}

void SpellChecker::onReadFileRequestProcessed(bool success, QString errorDescription, QByteArray data, QUuid requestId)
{
    if (requestId != m_readUserDictionaryRequestId) {
        return;
    }

    QNDEBUG("SpellChecker::onReadFileRequestProcessed: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId);

    m_readUserDictionaryRequestId = QUuid();

    QObject::disconnect(this, QNSIGNAL(SpellChecker,readFile,QString,QUuid),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onReadFileRequest,QString,QUuid));
    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QString,QByteArray,QUuid),
                        this, QNSLOT(SpellChecker,onReadFileRequestProcessed,bool,QString,QByteArray,QUuid));

    if (Q_UNLIKELY(!success)) {
        QNWARNING("Can't read the data from user's dictionary");
        return;
    }

    QBuffer buffer(&data);
    bool res = buffer.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!res)) {
        QNWARNING("Can't open the data buffer for reading");
        return;
    }

    QTextStream stream(&buffer);
    QString word;
    while(true)
    {
        word = stream.readLine();
        if (word.isEmpty()) {
            break;
        }

        m_userDictionary << word;
    }
    buffer.close();

    checkUserDictionaryDataPendingWriting();
}

void SpellChecker::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    if (requestId != m_appendUserDictionaryPartToFileRequestId) {
        return;
    }

    QNDEBUG("SpellChecker::onWriteFileRequestProcessed: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId);

    m_appendUserDictionaryPartToFileRequestId = QUuid();

    QObject::disconnect(this, QNSIGNAL(SpellChecker,writeFile,QString,QByteArray,QUuid,bool),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                        this, QNSLOT(SpellChecker,onWriteFileRequestProcessed,bool,QString,QUuid));

    if (Q_UNLIKELY(!success)) {
        QNWARNING("Can't update user dictionary file: " << errorDescription);
        return;
    }

    checkUserDictionaryDataPendingWriting();
}

SpellChecker::Dictionary::Dictionary() :
    m_pHunspell(),
    m_dictionaryPath(),
    m_enabled(true)
{}

bool SpellChecker::Dictionary::isEmpty() const
{
    return m_dictionaryPath.isEmpty() || m_pHunspell.isNull();
}

} // namespace qute_note
