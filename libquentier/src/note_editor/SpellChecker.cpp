#include "SpellChecker.h"
#include <quentier/utility/FileIOThreadWorker.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/logging/QuentierLogger.h>
#include <hunspell/hunspell.hxx>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QBuffer>
#include <QThreadPool>
#include <algorithm>

#define SPELL_CHECKER_FOUND_DICTIONARIES_GROUP "SpellCheckerFoundDictionaries"
#define SPELL_CHECKER_FOUND_DICTIONARIES_DIC_FILE_ITEM "DicFile"
#define SPELL_CHECKER_FOUND_DICTIONARIES_AFF_FILE_ITEM "AffFile"
#define SPELL_CHECKER_FOUND_DICTIONARIES_ARRAY "Dictionaries"

namespace quentier {

SpellChecker::SpellChecker(FileIOThreadWorker * pFileIOThreadWorker, QObject * parent, const QString & userDictionaryPath) :
    QObject(parent),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_systemDictionaries(),
    m_systemDictionariesReady(false),
    m_readUserDictionaryRequestId(),
    m_userDictionaryPath(),
    m_userDictionary(),
    m_userDictionaryReady(false),
    m_userDictionaryPartPendingWriting(),
    m_appendUserDictionaryPartToFileRequestId(),
    m_updateUserDictionaryFileRequestId()
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
    QNDEBUG("SpellChecker::checkSpell: " << word);

    if (m_userDictionary.contains(word, Qt::CaseInsensitive)) {
        return true;
    }

    QByteArray wordData = word.toUtf8();
    QByteArray lowerWordData = word.toLower().toUtf8();

    for(auto it = m_systemDictionaries.begin(), end = m_systemDictionaries.end(); it != end; ++it)
    {
        const Dictionary & dictionary = it.value();

        if (dictionary.isEmpty() || !dictionary.m_enabled) {
            QNTRACE("Skipping dictionary " << dictionary.m_dictionaryPath);
            continue;
        }

        int res = dictionary.m_pHunspell->spell(wordData.constData());
        if (res != 0) {
            QNTRACE("Found word " << word << " in dictionary " << dictionary.m_dictionaryPath);
            return true;
        }

        res = dictionary.m_pHunspell->spell(lowerWordData.constData());
        if (res != 0) {
            QNTRACE("Found word " << lowerWordData << " in dictionary " << dictionary.m_dictionaryPath);
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
        for(int i = 0; i < numSuggestions; ++i)
        {
            QString suggestion = QString::fromUtf8(rawCorrectionSuggestions[i]);
            if (!result.contains(suggestion)) {
                result << suggestion;
            }

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

void SpellChecker::removeFromUserWordList(const QString & word)
{
    QNDEBUG("SpellChecker::removeFromUserWordList: " << word);

    removeWord(word);

    m_userDictionaryPartPendingWriting.removeAll(word);
    m_userDictionary.removeAll(word);

    QByteArray dataToWrite;
    for(auto it = m_userDictionary.begin(), end = m_userDictionary.end(); it != end; ++it) {
        dataToWrite.append(*it + "\n");
    }

    QObject::connect(this, QNSIGNAL(SpellChecker,writeFile,QString,QByteArray,QUuid,bool),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(SpellChecker,onWriteFileRequestProcessed,bool,QNLocalizedString,QUuid));

    m_updateUserDictionaryFileRequestId = QUuid::createUuid();
    emit writeFile(m_userDictionaryPath, dataToWrite, m_updateUserDictionaryFileRequestId, /* append = */ false);
    QNTRACE("Sent the request to update the user dictionary: "
            << m_updateUserDictionaryFileRequestId);
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

void SpellChecker::removeWord(const QString & word)
{
    QNDEBUG("SpellChecker::removeWord: " << word);

    QByteArray wordData = word.toUtf8();

    for(auto it = m_systemDictionaries.begin(), end = m_systemDictionaries.end(); it != end; ++it)
    {
        Dictionary & dictionary = it.value();

        if (dictionary.isEmpty() || !dictionary.m_enabled) {
            continue;
        }

        dictionary.m_pHunspell->remove(wordData.constData());
    }
}

bool SpellChecker::isReady() const
{
    return m_systemDictionariesReady && m_userDictionaryReady;
}

void SpellChecker::onDictionariesFound(SpellCheckerDictionariesFinder::DicAndAffFilesByDictionaryName files)
{
    QNDEBUG("SpellChecker::onDictionariesFound");

    for(auto it = files.begin(), end = files.end(); it != end; ++it)
    {
        const QPair<QString, QString> & pair = it.value();
        QByteArray dictionaryFilePathData = pair.first.toLocal8Bit();
        QByteArray affixFilePathData = pair.second.toLocal8Bit();

        const char * rawDictionaryFilePath = dictionaryFilePathData.constData();
        const char * rawAffixFilePath = affixFilePathData.constData();
        QNTRACE("Raw dictionary file path = " << rawDictionaryFilePath << ", raw affix file path = " << rawAffixFilePath);

        Dictionary & dictionary = m_systemDictionaries[it.key()];
        dictionary.m_pHunspell = QSharedPointer<Hunspell>(new Hunspell(rawAffixFilePath, rawDictionaryFilePath));
        dictionary.m_dictionaryPath = pair.first;
        dictionary.m_enabled = true;
        QNTRACE("Added dictionary for language " << it.key() << "; dictionary file " << pair.first
                << ", affix file " << pair.second);
    }

    ApplicationSettings settings;
    settings.beginGroup(SPELL_CHECKER_FOUND_DICTIONARIES_GROUP);

    // Removing any previously existing array
    settings.setValue(SPELL_CHECKER_FOUND_DICTIONARIES_ARRAY, QVariant(QStringList()));
    // TODO: verify that actually works as expected

    settings.beginWriteArray(SPELL_CHECKER_FOUND_DICTIONARIES_ARRAY);
    int index = 0;
    for(auto it = files.begin(), end = files.end(); it != end; ++it)
    {
        const QPair<QString, QString> & pair = it.value();
        settings.setArrayIndex(index);
        settings.setValue(SPELL_CHECKER_FOUND_DICTIONARIES_DIC_FILE_ITEM, pair.first);
        settings.setValue(SPELL_CHECKER_FOUND_DICTIONARIES_AFF_FILE_ITEM, pair.second);
        ++index;
    }
    settings.endArray();
    settings.endGroup();

    m_systemDictionariesReady = true;
    if (isReady()) {
        emit ready();
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
                    "as deduced from libquentier's own environment variables:\n LIBQUTENOTEDICTNAMES: " << ownDictionaryNames
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

    if (!m_systemDictionaries.isEmpty())
    {
        QNDEBUG("Found some dictionaries at the expected locations, won't search for dictionaries just everywhere at the system");
        m_systemDictionariesReady = true;

        if (isReady()) {
            emit ready();
        }

        return;
    }

    QNDEBUG("Can't find hunspell dictionaries in any of the expected standard locations, will see if there are some "
            "previously found dictionaries which are still valid");

    SpellCheckerDictionariesFinder::DicAndAffFilesByDictionaryName dicAndAffFiles;
    ApplicationSettings settings;
    QStringList childGroups = settings.childGroups();
    int foundDictionariesGroupIndex = childGroups.indexOf(SPELL_CHECKER_FOUND_DICTIONARIES_GROUP);
    if (foundDictionariesGroupIndex >= 0)
    {
        settings.beginGroup(SPELL_CHECKER_FOUND_DICTIONARIES_GROUP);

        int numDicFiles = settings.beginReadArray(SPELL_CHECKER_FOUND_DICTIONARIES_ARRAY);
        dicAndAffFiles.reserve(numDicFiles);
        for(int i = 0; i < numDicFiles; ++i)
        {
            settings.setArrayIndex(i);
            QString dicFile = settings.value(SPELL_CHECKER_FOUND_DICTIONARIES_DIC_FILE_ITEM).toString();
            QString affFile = settings.value(SPELL_CHECKER_FOUND_DICTIONARIES_AFF_FILE_ITEM).toString();
            if (dicFile.isEmpty() || affFile.isEmpty()) {
                continue;
            }

            QFileInfo dicFileInfo(dicFile);
            if (!dicFileInfo.exists() || !dicFileInfo.isReadable()) {
                continue;
            }

            QFileInfo affFileInfo(affFile);
            if (!affFileInfo.exists() || !affFileInfo.isReadable()) {
                continue;
            }

            dicAndAffFiles[dicFileInfo.baseName()] = QPair<QString, QString>(dicFile, affFile);
        }

        settings.endArray();
        settings.endGroup();
    }

    if (!dicAndAffFiles.isEmpty()) {
        QNDEBUG("Found some previously found dictionary files, will use them instead of running a new search across the system");
        onDictionariesFound(dicAndAffFiles);
        return;
    }

    QNDEBUG("Still can't find any valid hunspell dictionaries, trying the full recursive search across the entire system, just to find something");

    SpellCheckerDictionariesFinder * pFinder = new SpellCheckerDictionariesFinder;
    QThreadPool::globalInstance()->start(pFinder);
    QObject::connect(pFinder, QNSIGNAL(SpellCheckerDictionariesFinder,foundDictionaries,SpellCheckerDictionariesFinder::DicAndAffFilesByDictionaryName),
                     this, QNSLOT(SpellChecker,onDictionariesFound,SpellCheckerDictionariesFinder::DicAndAffFilesByDictionaryName), Qt::QueuedConnection);
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
    dictionary.m_dictionaryPath = dictionaryFileInfo.absoluteFilePath();
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
        QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QNLocalizedString,QByteArray,QUuid),
                         this, QNSLOT(SpellChecker,onReadFileRequestProcessed,bool,QNLocalizedString,QByteArray,QUuid));

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
        dataToWrite.append(*it + "\n");
    }

    if (!dataToWrite.isEmpty())
    {
        QObject::connect(this, QNSIGNAL(SpellChecker,writeFile,QString,QByteArray,QUuid,bool),
                         m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
        QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QNLocalizedString,QUuid),
                         this, QNSLOT(SpellChecker,onWriteFileRequestProcessed,bool,QNLocalizedString,QUuid));

        m_appendUserDictionaryPartToFileRequestId = QUuid::createUuid();
        emit writeFile(m_userDictionaryPath, dataToWrite, m_appendUserDictionaryPartToFileRequestId, /* append = */ true);
        QNTRACE("Sent the request to append the data pending writing to user dictionary, id = "
                << m_appendUserDictionaryPartToFileRequestId);
    }

    m_userDictionaryPartPendingWriting.clear();
}

void SpellChecker::onReadFileRequestProcessed(bool success, QNLocalizedString errorDescription, QByteArray data, QUuid requestId)
{
    Q_UNUSED(errorDescription)

    if (requestId != m_readUserDictionaryRequestId) {
        return;
    }

    QNDEBUG("SpellChecker::onReadFileRequestProcessed: success = " << (success ? "true" : "false")
            << ", request id = " << requestId);

    m_readUserDictionaryRequestId = QUuid();

    QObject::disconnect(this, QNSIGNAL(SpellChecker,readFile,QString,QUuid),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onReadFileRequest,QString,QUuid));
    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QNLocalizedString,QByteArray,QUuid),
                        this, QNSLOT(SpellChecker,onReadFileRequestProcessed,bool,QNLocalizedString,QByteArray,QUuid));

    if (Q_LIKELY(success))
    {
        QBuffer buffer(&data);
        bool res = buffer.open(QIODevice::ReadOnly);
        if (Q_LIKELY(res))
        {
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
        else
        {
            QNWARNING("Can't open the data buffer for reading");
        }
    }
    else
    {
        QNWARNING("Can't read the data from user's dictionary");
    }

    m_userDictionaryReady = true;
    if (isReady()) {
        emit ready();
    }
}

void SpellChecker::onWriteFileRequestProcessed(bool success, QNLocalizedString errorDescription, QUuid requestId)
{
    if (requestId == m_appendUserDictionaryPartToFileRequestId) {
        onAppendUserDictionaryPartDone(success, errorDescription);
    }
    else if (requestId == m_updateUserDictionaryFileRequestId) {
        onUpdateUserDictionaryDone(success, errorDescription);
    }
    else {
        return;
    }

    if (m_appendUserDictionaryPartToFileRequestId.isNull() &&
        m_updateUserDictionaryFileRequestId.isNull())
    {
        QObject::disconnect(this, QNSIGNAL(SpellChecker,writeFile,QString,QByteArray,QUuid,bool),
                            m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
        QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QNLocalizedString,QUuid),
                            this, QNSLOT(SpellChecker,onWriteFileRequestProcessed,bool,QNLocalizedString,QUuid));
    }
}

void SpellChecker::onAppendUserDictionaryPartDone(bool success, QNLocalizedString errorDescription)
{
    QNDEBUG("SpellChecker::onAppendUserDictionaryPartDone: success = " << (success ? "true" : "false"));

    Q_UNUSED(errorDescription)
    m_appendUserDictionaryPartToFileRequestId = QUuid();

    if (Q_UNLIKELY(!success)) {
        QNWARNING("Can't append word to the user dictionary file");
        return;
    }

    checkUserDictionaryDataPendingWriting();
}

void SpellChecker::onUpdateUserDictionaryDone(bool success, QNLocalizedString errorDescription)
{
    QNDEBUG("SpellChecker::onUpdateUserDictionaryDone: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription);

    m_updateUserDictionaryFileRequestId = QUuid();

    if (Q_UNLIKELY(!success)) {
        QNWARNING("Can't update the user dictionary file");
        return;
    }
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

} // namespace quentier
