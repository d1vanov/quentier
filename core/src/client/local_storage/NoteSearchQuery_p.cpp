#include "NoteSearchQuery_p.h"
#include <logging/QuteNoteLogger.h>
#include <QDateTime>

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

void NoteSearchQueryPrivate::clear()
{
    // TODO: implement
}

bool NoteSearchQueryPrivate::parseQueryString(const QString & queryString, QString & error)
{
    QStringList words = splitSearchQueryString(queryString);

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

    bool res = convertAbsoluteAndRelativeDateTimesToTimestamps(words, error);
    if (!res) {
        return false;
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

    res = parseIntValue("created", words, m_creationTimestamps, m_negatedCreationTimestamps, error);
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

    res = parseIntValue("reminderTime", words, m_reminderTimes, m_negatedReminderTimes, error);
    if (!res) {
        return false;
    }

    res = parseIntValue("reminderDoneTime", words, m_reminderDoneTimes, m_negatedReminderDoneTimes, error);
    if (!res) {
        return false;
    }

    // Special treatment for reminderOrder key as it is the only integer key which is allowed to be asterisk
    bool foundReminderOrderAsterisk = false;
    res = parseIntValue("reminderOrder", words, m_reminderOrders, m_negatedReminderOrders, error,
                        &foundReminderOrderAsterisk);
    if (!res) {
        return false;
    }

    if (foundReminderOrderAsterisk) {
        m_hasReminderOrder = true;
    }

    res = parseDoubleValue("latitude", words, m_latitudes, m_negatedLatitudes, error);
    if (!res) {
        return false;
    }

    res = parseDoubleValue("longitude", words, m_longitudes, m_negatedLongitudes, error);
    if (!res) {
        return false;
    }

    res = parseDoubleValue("altitude", words, m_altitudes, m_negatedAltitudes, error);
    if (!res) {
        return false;
    }

    // TODO: process "todo:" tags properly

    foreach(const QString & searchTerm, words)
    {
        if (searchTerm.startsWith("encryption:")) {
            m_hasEncryption = true;
            break;
        }
    }

    words.removeAll("encryption:");

    // By now all tagged search terms must have been removed from the list of words
    // so we can extract the actual untagged content search terms here

    foreach(const QString & searchTerm, words)
    {
        if (searchTerm.startsWith("-")) {
            QString localSearchTerm = searchTerm;
            m_negatedContentSearchTerms << localSearchTerm.remove("-");
        }
        else {
            m_contentSearchTerms << searchTerm;
        }
    }

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
                                           QString & error, bool * pFoundAsteriskReminderOrder) const
{
    int keyIndex = -1;
    QChar negation('-');
    QStringList processedWords;

    // reminderOrder is a key with special treatment as it is allowed to be asterisk, not a number
    QString reminderOrderKey("reminderOrder");
    QString asterisk("*");
    if (pFoundAsteriskReminderOrder) {
        *pFoundAsteriskReminderOrder = false;
    }

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
        // Special treatment for "reminderOrder" key as it is allowed to be asterisk
        if ((key == reminderOrderKey) && (word == asterisk))
        {
            if (pFoundAsteriskReminderOrder) {
                *pFoundAsteriskReminderOrder = true;
            }
        }
        else
        {
            bool conversionResult = false;
            qint64 value = static_cast<qint64>(word.toInt(&conversionResult));
            if (!conversionResult) {
                error = QT_TR_NOOP("Internal error during search query parsing: "
                                   "cannot convert parsed value to integer");
                return false;
            }
            if (isNegated) {
                negatedContainer << value;
            }
            else {
                container << value;
            }
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

bool NoteSearchQueryPrivate::dateTimeStringToTimestamp(QString dateTimeString,
                                                       qint64 & timestamp, QString & error) const
{
    QDateTime todayMidnight = QDateTime::currentDateTime();
    todayMidnight.setTime(QTime(0, 0, 0, 1));

    QDateTime dateTime = todayMidnight;
    bool relativeDateArgumentFound = false;

    if (dateTimeString.startsWith("day"))
    {
        relativeDateArgumentFound = true;

        QString offsetSubstr = dateTimeString.remove("day");
        if (!offsetSubstr.isEmpty())
        {
            bool conversionResult = false;
            int daysOffset = offsetSubstr.toInt(&conversionResult);
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid query string: unable to convert days offset to integer");
                return false;
            }

            dateTime.addDays(daysOffset);
        }
    }
    else if (dateTimeString.startsWith("week"))
    {
        relativeDateArgumentFound = true;

        QString offsetSubstr = dateTimeString.remove("week");
        if (!offsetSubstr.isEmpty())
        {
            bool conversionResult = false;
            int weekOffset = offsetSubstr.toInt(&conversionResult);
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid query string: unable to convert weeks offset to integer");
                return false;
            }

            int dayOfWeek = dateTime.date().dayOfWeek();
            dateTime.addDays(-1 * dayOfWeek);   // go to week start and count offset from there

            dateTime.addDays(7 * weekOffset);
        }
    }
    else if (dateTimeString.startsWith("month"))
    {
        relativeDateArgumentFound = true;

        QString offsetSubstr = dateTimeString.remove("month");
        if (!offsetSubstr.isEmpty())
        {
            bool conversionResult = false;
            int monthOffset = offsetSubstr.toInt(&conversionResult);
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid query string: unable to convert months offset to integer");
                return false;
            }

            int dayOfMonth = dateTime.date().day();
            dateTime.addDays(-1 * (dayOfMonth - 1));   // go to month start and count offset from there

            dateTime.addMonths(monthOffset);
        }
    }
    else if (dateTimeString.startsWith("year"))
    {
        relativeDateArgumentFound = true;

        QString offsetSubstr = dateTimeString.remove("year");
        if (!offsetSubstr.isEmpty())
        {
            bool conversionResult = false;
            int yearsOffset = offsetSubstr.toInt(&conversionResult);
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid query string: unable to convert years offset to integer");
                return false;
            }

            int dayOfMonth = dateTime.date().day();
            dateTime.addDays(-1 * (dayOfMonth - 1));    // go to month start

            int currentMonth = dateTime.date().month();
            dateTime.addMonths(-1 * (currentMonth - 1));    // go to year start and count offset from there

            dateTime.addYears(yearsOffset);
        }
    }

    if (relativeDateArgumentFound)
    {
        if (!dateTime.isValid()) {
            error = QT_TR_NOOP("Internal error: datetime processed from query string is invalid: ");
            error += dateTime.toString(Qt::ISODate);
            QNWARNING(error);
            return false;
        }

        timestamp = dateTime.toMSecsSinceEpoch();
        return true;
    }

    // Getting here means the datetime in the string is actually a datetime in ISO 8601 compact profile
    dateTime = QDateTime::fromString(dateTimeString, Qt::ISODate);
    if (!dateTime.isValid()) {
        error = QT_TR_NOOP("Internal error: datetime in query string is invalid with respect to ISO 8601 compact profile: ");
        error += dateTime.toString(Qt::ISODate);
        QNWARNING(error);
        return false;
    }

    timestamp = dateTime.toMSecsSinceEpoch();
    return true;
}

bool NoteSearchQueryPrivate::convertAbsoluteAndRelativeDateTimesToTimestamps(QStringList &words, QString &error) const
{
    QStringList dateTimePrefixes;
    dateTimePrefixes << "created:" << "-created:" << "updated:" << "-updated:"
                     << "subjectDate:" << "-subjectDate:" << "reminderTime:"
                     << "-reminderTime:" << "reminderDoneTime:" << "-reminderDoneTime";

    int numWords = words.size();
    for(int i = 0; i < numWords; ++i)
    {
        QString & word = words[i];

        foreach (const QString & prefix, dateTimePrefixes)
        {
            if (word.startsWith(prefix))
            {
                QString dateTimeString = word.remove(prefix);
                qint64 timestamp;
                bool res = dateTimeStringToTimestamp(dateTimeString, timestamp, error);
                if (!res) {
                    return false;
                }

                word = prefix + QString::number(timestamp);
            }
        }
    }

    return true;
}

} // namespace qute_note
