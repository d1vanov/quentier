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

} // namespace qute_note
