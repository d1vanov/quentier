#include "NoteSearchQuery_p.h"

namespace qute_note {

NoteSearchQueryPrivate::NoteSearchQueryPrivate() :
    m_queryString(),
    m_notebookModifier(),
    m_hasAnyModifier(false),
    m_tagNames(),
    m_negatedTagNames(),
    m_titleNames(),
    m_negatedTitleNames(),
    m_creationTimestamps(),
    m_negatedCreationTimestamps(),
    m_modificationTimestamps(),
    m_negatedModificationTimestamps(),
    m_resourceMimeTypes(),
    m_negatedResourceMimeTypes(),
    m_subjectDateTimestamps(),
    m_negatedSubjectDateTimestamps(),
    m_latitudes(),
    m_negatedLatitudes(),
    m_longitudes(),
    m_negatedLongitudes(),
    m_altitudes(),
    m_negatedAltitudes(),
    m_authors(),
    m_negatedAuthors(),
    m_sources(),
    m_negatedSources(),
    m_sourceApplications(),
    m_negatedSourceApplications(),
    m_contentClasses(),
    m_negatedContentClasses(),
    m_placeNames(),
    m_negatedPlaceNames(),
    m_applicationData(),
    m_negatedApplicationData(),
    m_recognitionTypes(),
    m_negatedRecognitionTypes(),
    m_hasReminderOrder(false),
    m_reminderOrders(),
    m_negatedReminderOrders(),
    m_reminderTimes(),
    m_negatedReminderTimes(),
    m_reminderDoneTimes(),
    m_negatedReminderDoneTimes(),
    m_hasUnfinishedToDo(false),
    m_hasFinishedToDo(false),
    m_hasEncryption(false),
    m_contentSearchTerms(),
    m_negatedContentSearchTerms()
{}

bool NoteSearchQueryPrivate::parseQueryString(const QString & queryString, QString & error)
{
    QStringList words = splitSearchQueryString(queryString);

    // TODO: postprocess words list: convert day, week, month etc. to timestamps

    int notebookScopeModifierPosition = words.indexOf(QRegExp("notebook:*"));
    if (notebookScopeModifierPosition > 0) {
        error = QT_TR_NOOP("Incorrect position of \"notebook:\" scope modified in the search query: "
                           "when present in the query, it should be the first term in the search");
        return false;
    }

    // NOTE: "any:" scope modifier is not position dependent and affects the whole query
    int anyScopeModifierPosition = words.indexOf("any:");
    if (anyScopeModifierPosition >= 0) {
        m_hasAnyModifier = true;
    }
    else {
        m_hasAnyModifier = false;
    }

    parseStringValue("tag", words, m_tagNames, m_negatedTagNames);
    parseStringValue("intitle", words, m_titleNames, m_negatedTitleNames);
    parseStringValue("resource", words, m_resourceMimeTypes, m_negatedResourceMimeTypes);
    parseStringValue("author", words, m_authors, m_negatedAuthors);
    parseStringValue("source", words, m_sources, m_negatedSources);
    parseStringValue("sourceApplication", words, m_sourceApplications, m_negatedSourceApplications);
    parseStringValue("contentClass", words, m_contentClasses, m_negatedContentClasses);;
    parseStringValue("placeName", words, m_placeNames, m_negatedPlaceNames);
    parseStringValue("applicationData", words, m_applicationData, m_negatedApplicationData);
    parseStringValue("recoType", words, m_recognitionTypes, m_negatedRecognitionTypes);

    bool res = parseIntValue("created", words, m_creationTimestamps, m_negatedCreationTimestamps, error);
    if (!res) {
        return false;
    }

    res = parseIntValue("updated", words, m_modificationTimestamps, m_negatedModificationTimestamps, error);
    if (!res) {
        return false;
    }

    res = parseIntValue("subject", words, m_subjectDateTimestamps, m_negatedSubjectDateTimestamps, error);
    if (!res) {
        return false;
    }

    // TODO: must have special treatment for "reminderOrder" key as it is allowed to be asterisk

    res = parseDoubleValue("latitude", words, m_latitudes, m_negatedLatitudes, error);
    if (!res) {
        return false;
    }

    // By now all tagged search terms must have been removed from the list of words
    // so we can extract the actual untagged content search terms here

    // TODO: implement further

    return true;
}

QTextStream & NoteSearchQueryPrivate::Print(QTextStream & strm) const
{
    // TODO: implement
    return strm;
}

QStringList NoteSearchQueryPrivate::splitSearchQueryString(const QString & searchQueryString) const
{
    QStringList words;

    // Retrieving single words from the query string considering any text between quotes a single word
    bool insideQuotedText = false;
    int length = searchQueryString.length();
    const QChar space = ' ';
    const QChar quote = '\"';
    QString currentWord;
    for(int i = 0; i < length; ++i)
    {
        QChar chr = searchQueryString[i];

        if ((chr == space) && !insideQuotedText)
        {
            continue;
        }
        else if (chr == quote)
        {
            if (i == (length - 1))
            {
                // If that was quoted text, it's not any longer
                insideQuotedText = false;
                // can clear the starting quote from the current word right now
                if (currentWord.startsWith(quote)) {
                    currentWord = currentWord.remove(0, 1);
                }
                words << currentWord;
                currentWord.clear();
                break;
            }

            if (insideQuotedText)
            {
                QChar nextChr = searchQueryString[i+1];
                if (nextChr == space)
                {
                    if (i != 0)
                    {
                        QChar prevChr = searchQueryString[i-1];

                        if (prevChr == '\\')
                        {
                            bool backslashEscaped = false;
                            // Looks like this space is escaped. Just in case, let's check whether the backslash is esacped itself
                            if (i != 1) {
                                QChar prevPrevChar = searchQueryString[i-2];
                                if (prevPrevChar == '\\') {
                                    // Yes, backslash is escaped itself, so the quote at i is really the closing one
                                    backslashEscaped = true;
                                }
                            }

                            if (!backslashEscaped) {
                                currentWord += chr;
                                continue;
                            }
                        }
                    }

                    words << currentWord;
                    currentWord.clear();
                    insideQuotedText = false;
                    continue;
                }
            }
            else
            {
                currentWord += chr;
                insideQuotedText = true;
                continue;
            }
        }
        else
        {
            currentWord += chr;
        }
    }

    // Now we can remove any quotes from the words from the splitted query string
    int numWords = words.size();
    for(int i = 0; i < numWords; ++i)
    {
        QString & word = words[i];
        if (word.startsWith('\"') && word.endsWith('\"'))
        {
            // Removing the last character = quote
            int wordLength = word.length();
            word = word.remove(wordLength-1, 1);

            if (word.length() != 0) {
                // Removing the first character = quote
                word = word.remove(0, 1);
            }
        }
    }

    return words;
}

void NoteSearchQueryPrivate::parseStringValue(const QString & key, QStringList & words,
                                              QStringList & container, QStringList & negatedContainer) const
{
    int keyIndex = -1;
    QChar negation('-');
    QStringList processedWords;
    while(keyIndex >= 0)
    {
        keyIndex = words.indexOf(QRegExp(QString("*") + key + QString(":*")),
                                 (keyIndex >= 0 ? keyIndex : 0));
        if (keyIndex < 0) {
            break;
        }
        QString word = words[keyIndex];
        if (processedWords.contains(word)) {
            ++keyIndex;
            continue;
        }
        else {
            processedWords << word;
        }
        int positionInWord = word.indexOf(key + QString(":"));
        if (positionInWord < 0) {
            continue;
        }
        bool isNegated = false;
        if (positionInWord != 0) {
            QChar prevChr = word[positionInWord-1];
            if (prevChr == negation) {
                isNegated = true;
            }
        }

        QString value = word.remove(key + QString(":"));
        if (isNegated) {
            negatedContainer << value;
        }
        else {
            container << value;
        }
    }

    foreach(const QString & word, processedWords) {
        words.removeAll(word);
    }
}

bool NoteSearchQueryPrivate::parseIntValue(const QString & key, QStringList & words,
                                           QVector<qint64> & container, QVector<qint64> & negatedContainer,
                                           QString & error) const
{
    int keyIndex = -1;
    QChar negation('-');
    QStringList processedWords;
    while(keyIndex >= 0)
    {
        keyIndex = words.indexOf(QRegExp(QString("*") + key + QString(":*")),
                                 (keyIndex >= 0 ? keyIndex : 0));
        if (keyIndex < 0) {
            break;
        }
        QString word = words[keyIndex];
        if (processedWords.contains(word)) {
            ++keyIndex;
            continue;
        }
        else {
            processedWords << word;
        }
        int positionInWord = word.indexOf(key + QString(":"));
        if (positionInWord < 0) {
            continue;
        }
        bool isNegated = false;
        if (positionInWord != 0) {
            QChar prevChr = word[positionInWord-1];
            if (prevChr == negation) {
                isNegated = true;
            }
        }
        word = word.remove(key + QString(":"));
        bool conversionResult = false;
        qint64 value = static_cast<qint64>(word.toInt(&conversionResult));
        if (!conversionResult) {
            error = QT_TR_NOOP("Internal error during search query parsing: "
                               "cannot convert parsed value to qint64");
            return false;
        }
        if (isNegated) {
            negatedContainer << value;
        }
        else {
            container << value;
        }
    }

    foreach(const QString & word, processedWords) {
        words.removeAll(word);
    }

    return true;
}

bool NoteSearchQueryPrivate::parseDoubleValue(const QString & key, QStringList & words,
                                              QVector<double> & container, QVector<double> & negatedContainer,
                                              QString & error) const
{
    int keyIndex = -1;
    QChar negation('-');
    QStringList processedWords;
    while(keyIndex >= 0)
    {
        keyIndex = words.indexOf(QRegExp(QString("*") + key + QString(":*")),
                                 (keyIndex >= 0 ? keyIndex : 0));
        if (keyIndex < 0) {
            break;
        }
        QString word = words[keyIndex];
        if (processedWords.contains(word)) {
            ++keyIndex;
            continue;
        }
        else {
            processedWords << word;
        }
        int positionInWord = word.indexOf(key + QString(":"));
        if (positionInWord < 0) {
            continue;
        }
        bool isNegated = false;
        if (positionInWord != 0) {
            QChar prevChr = word[positionInWord-1];
            if (prevChr == negation) {
                isNegated = true;
            }
        }
        word = word.remove(key + QString(":"));
        bool conversionResult = false;
        double value = static_cast<double>(word.toDouble(&conversionResult));
        if (!conversionResult) {
            error = QT_TR_NOOP("Internal error during search query parsing: "
                               "cannot convert parsed value to double");
            return false;
        }
        if (isNegated) {
            negatedContainer << value;
        }
        else {
            container << value;
        }
    }

    foreach(const QString & word, processedWords) {
        words.removeAll(word);
    }

    return true;
}

} // namespace qute_note
