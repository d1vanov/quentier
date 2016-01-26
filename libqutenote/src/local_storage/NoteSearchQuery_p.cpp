#include "NoteSearchQuery_p.h"
#include <qute_note/utility/StringUtils.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QDateTime>

namespace qute_note {

NoteSearchQueryPrivate::NoteSearchQueryPrivate() :
    m_queryString(),
    m_notebookModifier(),
    m_hasAnyModifier(false),
    m_tagNames(),
    m_negatedTagNames(),
    m_hasAnyTag(false),
    m_hasNegatedAnyTag(false),
    m_titleNames(),
    m_negatedTitleNames(),
    m_hasAnyTitleName(false),
    m_hasNegatedAnyTitleName(false),
    m_creationTimestamps(),
    m_negatedCreationTimestamps(),
    m_hasAnyCreationTimestamp(false),
    m_hasNegatedAnyCreationTimestamp(false),
    m_modificationTimestamps(),
    m_negatedModificationTimestamps(),
    m_hasAnyModificationTimestamp(false),
    m_hasNegatedAnyModificationTimestamp(false),
    m_resourceMimeTypes(),
    m_negatedResourceMimeTypes(),
    m_hasAnyResourceMimeType(false),
    m_hasNegatedAnyResourceMimeType(false),
    m_subjectDateTimestamps(),
    m_negatedSubjectDateTimestamps(),
    m_hasAnySubjectDateTimestamp(false),
    m_hasNegatedAnySubjectDateTimestamp(false),
    m_latitudes(),
    m_negatedLatitudes(),
    m_hasAnyLatitude(false),
    m_hasNegatedAnyLatitude(false),
    m_longitudes(),
    m_negatedLongitudes(),
    m_hasAnyLongitude(false),
    m_hasNegatedAnyLongitude(false),
    m_altitudes(),
    m_negatedAltitudes(),
    m_hasAnyAltitude(false),
    m_hasNegatedAnyAltitude(false),
    m_authors(),
    m_negatedAuthors(),
    m_hasAnyAuthor(false),
    m_hasNegatedAnyAuthor(false),
    m_sources(),
    m_negatedSources(),
    m_hasAnySource(false),
    m_hasNegatedAnySource(false),
    m_sourceApplications(),
    m_negatedSourceApplications(),
    m_hasAnySourceApplication(false),
    m_hasNegatedAnySourceApplication(false),
    m_contentClasses(),
    m_negatedContentClasses(),
    m_hasAnyContentClass(false),
    m_hasNegatedAnyContentClass(false),
    m_placeNames(),
    m_negatedPlaceNames(),
    m_hasAnyPlaceName(false),
    m_hasNegatedAnyPlaceName(false),
    m_applicationData(),
    m_negatedApplicationData(),
    m_hasAnyApplicationData(false),
    m_hasNegatedAnyApplicationData(false),
    m_reminderOrders(),
    m_negatedReminderOrders(),
    m_hasAnyReminderOrder(false),
    m_hasNegatedAnyReminderOrder(false),
    m_reminderTimes(),
    m_negatedReminderTimes(),
    m_hasAnyReminderTime(false),
    m_hasNegatedAnyReminderTime(false),
    m_reminderDoneTimes(),
    m_negatedReminderDoneTimes(),
    m_hasAnyReminderDoneTime(false),
    m_hasNegatedAnyReminderDoneTime(false),
    m_hasUnfinishedToDo(false),
    m_hasNegatedUnfinishedToDo(false),
    m_hasFinishedToDo(false),
    m_hasNegatedFinishedToDo(false),
    m_hasAnyToDo(false),
    m_hasNegatedAnyToDo(false),
    m_hasEncryption(false),
    m_hasNegatedEncryption(false),
    m_contentSearchTerms(),
    m_negatedContentSearchTerms()
{}

void NoteSearchQueryPrivate::clear()
{
    m_queryString.resize(0);
    m_notebookModifier.resize(0);
    m_hasAnyModifier = false;
    m_tagNames.clear();
    m_negatedTagNames.clear();
    m_hasAnyTag = false;
    m_hasNegatedAnyTag = false;
    m_titleNames.clear();
    m_negatedTitleNames.clear();
    m_hasAnyTitleName = false;
    m_hasNegatedAnyTitleName = false;
    m_creationTimestamps.clear();
    m_negatedCreationTimestamps.clear();
    m_hasAnyCreationTimestamp = false;
    m_hasNegatedAnyCreationTimestamp = false;
    m_modificationTimestamps.clear();
    m_negatedModificationTimestamps.clear();
    m_hasAnyModificationTimestamp = false;
    m_hasNegatedAnyModificationTimestamp = false;
    m_resourceMimeTypes.clear();
    m_negatedResourceMimeTypes.clear();
    m_hasAnyResourceMimeType = false;
    m_hasNegatedAnyResourceMimeType = false;
    m_subjectDateTimestamps.clear();
    m_negatedSubjectDateTimestamps.clear();
    m_hasAnySubjectDateTimestamp = false;
    m_hasNegatedAnySubjectDateTimestamp = false;
    m_latitudes.clear();
    m_negatedLatitudes.clear();
    m_hasAnyLatitude = false;
    m_hasNegatedAnyLatitude = false;
    m_longitudes.clear();
    m_negatedLongitudes.clear();
    m_hasAnyLongitude = false;
    m_hasNegatedAnyLongitude = false;
    m_altitudes.clear();
    m_negatedAltitudes.clear();
    m_hasAnyAltitude = false;
    m_hasNegatedAnyAltitude = false;
    m_authors.clear();
    m_negatedAuthors.clear();
    m_hasAnyAuthor = false;
    m_hasNegatedAnyAuthor = false;
    m_sources.clear();
    m_negatedSources.clear();
    m_hasAnySource = false;
    m_hasNegatedAnySource = false;
    m_sourceApplications.clear();
    m_negatedSourceApplications.clear();
    m_hasAnySourceApplication = false;
    m_hasNegatedAnySourceApplication = false;
    m_contentClasses.clear();
    m_negatedContentClasses.clear();
    m_hasAnyContentClass = false;
    m_hasNegatedAnyContentClass = false;
    m_placeNames.clear();
    m_negatedPlaceNames.clear();
    m_hasAnyPlaceName = false;
    m_hasNegatedAnyPlaceName = false;
    m_applicationData.clear();
    m_negatedApplicationData.clear();
    m_hasAnyApplicationData = false;
    m_hasNegatedAnyApplicationData = false;
    m_reminderOrders.clear();
    m_negatedReminderOrders.clear();
    m_hasAnyReminderOrder = false;
    m_hasNegatedAnyReminderOrder = false;
    m_reminderTimes.clear();
    m_negatedReminderTimes.clear();
    m_hasAnyReminderTime = false;
    m_hasNegatedAnyReminderTime = false;
    m_reminderDoneTimes.clear();
    m_negatedReminderDoneTimes.clear();
    m_hasAnyReminderDoneTime = false;
    m_hasNegatedAnyReminderDoneTime = false;
    m_hasUnfinishedToDo = false;
    m_hasNegatedUnfinishedToDo = false;
    m_hasFinishedToDo = false;
    m_hasNegatedFinishedToDo = false;
    m_hasAnyToDo = false;
    m_hasNegatedAnyToDo = false;
    m_hasEncryption = false;
    m_hasNegatedEncryption = false;
    m_contentSearchTerms.clear();
    m_negatedContentSearchTerms.clear();
}

bool NoteSearchQueryPrivate::parseQueryString(const QString & queryString, QString & error)
{
    m_queryString = queryString;

    QStringList words = splitSearchQueryString(queryString);

    QRegExp notebookModifierRegex("notebook:*");
    notebookModifierRegex.setPatternSyntax(QRegExp::Wildcard);

    int notebookScopeModifierPosition = words.indexOf(notebookModifierRegex);
    if (notebookScopeModifierPosition > 0) {
        error = QT_TR_NOOP("Incorrect position of \"notebook:\" scope modified in the search query: "
                           "when present in the query, it should be the first term in the search");
        return false;
    }
    else if (notebookScopeModifierPosition == 0)
    {
        m_notebookModifier = words[notebookScopeModifierPosition];
        m_notebookModifier = m_notebookModifier.remove("notebook:");
        removeBoundaryQuotesFromWord(m_notebookModifier);
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

    parseStringValue("tag", words, m_tagNames, m_negatedTagNames, m_hasAnyTag, m_hasNegatedAnyTag);
    parseStringValue("intitle", words, m_titleNames, m_negatedTitleNames, m_hasAnyTitleName, m_hasNegatedAnyTitleName);
    parseStringValue("resource", words, m_resourceMimeTypes, m_negatedResourceMimeTypes,
                     m_hasAnyResourceMimeType, m_hasNegatedAnyResourceMimeType);
    parseStringValue("author", words, m_authors, m_negatedAuthors, m_hasAnyAuthor, m_hasNegatedAnyAuthor);
    parseStringValue("source", words, m_sources, m_negatedSources, m_hasAnySource, m_hasNegatedAnySource);
    parseStringValue("sourceApplication", words, m_sourceApplications, m_negatedSourceApplications,
                     m_hasAnySourceApplication, m_hasNegatedAnySourceApplication);
    parseStringValue("contentClass", words, m_contentClasses, m_negatedContentClasses,
                     m_hasAnyContentClass, m_hasNegatedAnyContentClass);
    parseStringValue("placeName", words, m_placeNames, m_negatedPlaceNames,
                     m_hasAnyPlaceName, m_hasNegatedAnyPlaceName);
    parseStringValue("applicationData", words, m_applicationData, m_negatedApplicationData,
                     m_hasAnyApplicationData, m_hasNegatedAnyApplicationData);

    res = parseIntValue("created", words, m_creationTimestamps, m_negatedCreationTimestamps,
                        m_hasAnyCreationTimestamp, m_hasNegatedAnyCreationTimestamp, error);
    if (!res) {
        return false;
    }

    res = parseIntValue("updated", words, m_modificationTimestamps, m_negatedModificationTimestamps,
                        m_hasAnyModificationTimestamp, m_hasNegatedAnyModificationTimestamp, error);
    if (!res) {
        return false;
    }

    res = parseIntValue("subjectDate", words, m_subjectDateTimestamps, m_negatedSubjectDateTimestamps,
                        m_hasAnySubjectDateTimestamp, m_hasNegatedAnySubjectDateTimestamp, error);
    if (!res) {
        return false;
    }

    res = parseIntValue("reminderTime", words, m_reminderTimes, m_negatedReminderTimes,
                        m_hasAnyReminderTime, m_hasNegatedAnyReminderTime, error);
    if (!res) {
        return false;
    }

    res = parseIntValue("reminderDoneTime", words, m_reminderDoneTimes, m_negatedReminderDoneTimes,
                        m_hasAnyReminderDoneTime, m_hasNegatedAnyReminderDoneTime, error);
    if (!res) {
        return false;
    }

    res = parseIntValue("reminderOrder", words, m_reminderOrders, m_negatedReminderOrders,
                        m_hasAnyReminderOrder, m_hasNegatedAnyReminderOrder, error);
    if (!res) {
        return false;
    }

    res = parseDoubleValue("latitude", words, m_latitudes, m_negatedLatitudes,
                           m_hasAnyLatitude, m_hasNegatedAnyLatitude, error);
    if (!res) {
        return false;
    }

    res = parseDoubleValue("longitude", words, m_longitudes, m_negatedLongitudes,
                           m_hasAnyLongitude, m_hasNegatedAnyLongitude, error);
    if (!res) {
        return false;
    }

    res = parseDoubleValue("altitude", words, m_altitudes, m_negatedAltitudes,
                           m_hasAnyAltitude, m_hasNegatedAnyAltitude, error);
    if (!res) {
        return false;
    }

    // Processing all possible variants of "todo:[true|false|*]" tags

    QString negatedFinishedToDo = "-todo:true";
    QString finishedToDo = "todo:true";
    QString negatedUnfinishedToDo = "-todo:false";
    QString unfinishedToDo = "todo:false";
    QString negatedAnyToDo = "-todo:*";
    QString anyToDo = "todo:*";

    foreach(const QString & searchTerm, words)
    {
        if (searchTerm == negatedFinishedToDo)
        {
            if (m_hasFinishedToDo) {
                error = QT_TR_NOOP("Incorrect search query: both finished todo and negated finished todo tags were found");
                return false;
            }
            m_hasNegatedFinishedToDo = true;
        }
        else if (searchTerm == finishedToDo)
        {
            if (m_hasNegatedFinishedToDo) {
                error = QT_TR_NOOP("Incorrect search query: both negated finished todo and finished todo tags were found");
                return false;
            }
            m_hasFinishedToDo = true;
        }
        else if (searchTerm == negatedUnfinishedToDo) {
            if (m_hasUnfinishedToDo) {
                error = QT_TR_NOOP("Incorrect search query: both unfinished todo and negated unfinished todo tags were found");
                return false;
            }
            m_hasNegatedUnfinishedToDo = true;
        }
        else if (searchTerm == unfinishedToDo) {
            if (m_hasNegatedUnfinishedToDo) {
                error = QT_TR_NOOP("Incorrect search query: both negated unfinished todo and unfinished todo tags were found");
                return false;
            }
            m_hasUnfinishedToDo = true;
        }
        else if (searchTerm == negatedAnyToDo) {
            m_hasNegatedAnyToDo = true;
        }
        else if (searchTerm == anyToDo) {
            m_hasAnyToDo = true;
        }
    }

    words.removeAll(negatedFinishedToDo);
    words.removeAll(finishedToDo);
    words.removeAll(negatedUnfinishedToDo);
    words.removeAll(unfinishedToDo);
    words.removeAll(negatedAnyToDo);
    words.removeAll(anyToDo);

    // Processing encryption tag

    QString negatedEncryption = "-encryption:";
    QString encryption = "encryption:";
    foreach(const QString & searchTerm, words)
    {
        if (searchTerm == negatedEncryption) {
            m_hasNegatedEncryption = true;
        }
        else if (searchTerm == encryption) {
            m_hasEncryption = true;
        }
    }

    words.removeAll(negatedEncryption);
    words.removeAll(encryption);

    // By now most of tagged search terms must have been removed from the list of words
    // so we can extract the actual untagged content search terms;
    // In the Evernote search grammar the searches are case insensitive so forcing all words
    // to the lower case

    QRegExp asteriskFilter("[*]");

    foreach(QString searchTerm, words)
    {
        if (searchTerm.startsWith("notebook:")) {
            continue;
        }

        if (searchTerm.startsWith("any:")) {
            continue;
        }

        bool negated = searchTerm.startsWith("-");
        if (negated) {
            searchTerm.remove(0, 1);
        }

        removePunctuation(searchTerm);
        if (searchTerm.isEmpty()) {
            continue;
        }

        // Don't accept search terms consisting only of asterisks
        QString searchTermWithoutAsterisks = searchTerm;
        searchTermWithoutAsterisks.remove(asteriskFilter);
        if (searchTermWithoutAsterisks.isEmpty()) {
            continue;
        }

        // Only accept asterisk if there's a single one in the end of the search term
        int indexOfAsterisk = searchTerm.indexOf("*");
        int lastIndexOfAsterisk = searchTerm.lastIndexOf("*");
        if (indexOfAsterisk != lastIndexOfAsterisk) {
            continue;
        }

        if ((indexOfAsterisk >= 0) && (indexOfAsterisk != (searchTerm.size() - 1))) {
            continue;
        }

        // Normalize the search term to use the lower case
        if (negated) {
            m_negatedContentSearchTerms << searchTerm.simplified().toLower();
        }
        else {
            m_contentSearchTerms << searchTerm.simplified().toLower();
        }
    }

    return true;
}

QTextStream & NoteSearchQueryPrivate::Print(QTextStream & strm) const
{
    QString indent = "  ";

    strm << "NoteSearchQuery: { \n";
    strm << indent << "query string: " << (m_queryString.isEmpty() ? "<empty>" : m_queryString) << "; \n";
    strm << indent << "notebookModifier: " << (m_notebookModifier.isEmpty() ? "<empty>" : m_notebookModifier) << "; \n";
    strm << indent << "hasAnyModifier: " << (m_hasAnyModifier ? "true" : "false")   << "\n";

#define CHECK_AND_PRINT_ANY_ITEM(name) \
    if (m_hasAny##name) { \
        strm << indent << "hasAny" #name " is true; \n"; \
    } \
    if (m_hasNegatedAny##name) { \
        strm << indent << "hasNegatedAny" #name "is true; \n"; \
    }

#define CHECK_AND_PRINT_LIST(name, type, ...) \
    if (m_##name.isEmpty()) { \
        strm << indent << #name " is empty; \n"; \
    } \
    else { \
        strm << indent << #name ": { \n"; \
        foreach(const type & word, m_##name) { \
            strm << indent << indent << __VA_ARGS__ (word) << "; \n"; \
        } \
        strm << indent << "}; \n"; \
    }

    CHECK_AND_PRINT_ANY_ITEM(Tag);
    CHECK_AND_PRINT_LIST(tagNames, QString);
    CHECK_AND_PRINT_LIST(negatedTagNames, QString);

    CHECK_AND_PRINT_ANY_ITEM(TitleName);
    CHECK_AND_PRINT_LIST(titleNames, QString);
    CHECK_AND_PRINT_LIST(negatedTitleNames, QString);

    CHECK_AND_PRINT_ANY_ITEM(CreationTimestamp);
    CHECK_AND_PRINT_LIST(creationTimestamps, qint64, QString::number);
    CHECK_AND_PRINT_LIST(negatedCreationTimestamps, qint64, QString::number);

    CHECK_AND_PRINT_ANY_ITEM(ModificationTimestamp);
    CHECK_AND_PRINT_LIST(modificationTimestamps, qint64, QString::number);
    CHECK_AND_PRINT_LIST(negatedModificationTimestamps, qint64, QString::number);

    CHECK_AND_PRINT_ANY_ITEM(ResourceMimeType);
    CHECK_AND_PRINT_LIST(resourceMimeTypes, QString);
    CHECK_AND_PRINT_LIST(negatedResourceMimeTypes, QString);

    CHECK_AND_PRINT_ANY_ITEM(SubjectDateTimestamp);
    CHECK_AND_PRINT_LIST(subjectDateTimestamps, qint64, QString::number);
    CHECK_AND_PRINT_LIST(negatedSubjectDateTimestamps, qint64, QString::number);

    CHECK_AND_PRINT_ANY_ITEM(Latitude);
    CHECK_AND_PRINT_LIST(latitudes, double, QString::number);
    CHECK_AND_PRINT_LIST(negatedLatitudes, double, QString::number);

    CHECK_AND_PRINT_ANY_ITEM(Longitude);
    CHECK_AND_PRINT_LIST(longitudes, double, QString::number);
    CHECK_AND_PRINT_LIST(negatedLongitudes, double, QString::number);

    CHECK_AND_PRINT_ANY_ITEM(Altitude);
    CHECK_AND_PRINT_LIST(altitudes, double, QString::number);
    CHECK_AND_PRINT_LIST(negatedAltitudes, double, QString::number);

    CHECK_AND_PRINT_ANY_ITEM(Author);
    CHECK_AND_PRINT_LIST(authors, QString);
    CHECK_AND_PRINT_LIST(negatedAuthors, QString);

    CHECK_AND_PRINT_ANY_ITEM(Source);
    CHECK_AND_PRINT_LIST(sources, QString);
    CHECK_AND_PRINT_LIST(negatedSources, QString);

    CHECK_AND_PRINT_ANY_ITEM(SourceApplication);
    CHECK_AND_PRINT_LIST(sourceApplications, QString);
    CHECK_AND_PRINT_LIST(negatedSourceApplications, QString);

    CHECK_AND_PRINT_ANY_ITEM(ContentClass);
    CHECK_AND_PRINT_LIST(contentClasses, QString);
    CHECK_AND_PRINT_LIST(negatedContentClasses, QString);

    CHECK_AND_PRINT_ANY_ITEM(PlaceName);
    CHECK_AND_PRINT_LIST(placeNames, QString);
    CHECK_AND_PRINT_LIST(negatedPlaceNames, QString);

    CHECK_AND_PRINT_ANY_ITEM(ApplicationData);
    CHECK_AND_PRINT_LIST(applicationData, QString);
    CHECK_AND_PRINT_LIST(negatedApplicationData, QString);

    CHECK_AND_PRINT_ANY_ITEM(ReminderOrder);
    CHECK_AND_PRINT_LIST(reminderOrders, qint64, QString::number);
    CHECK_AND_PRINT_LIST(negatedReminderOrders, qint64, QString::number);

    CHECK_AND_PRINT_ANY_ITEM(ReminderTime);
    CHECK_AND_PRINT_LIST(reminderTimes, qint64, QString::number);
    CHECK_AND_PRINT_LIST(negatedReminderTimes, qint64, QString::number);

    CHECK_AND_PRINT_ANY_ITEM(ReminderDoneTime);
    CHECK_AND_PRINT_LIST(reminderDoneTimes, qint64, QString::number);
    CHECK_AND_PRINT_LIST(negatedReminderDoneTimes, qint64, QString::number);

    strm << indent << "hasUnfinishedToDo: " << (m_hasUnfinishedToDo ? "true" : "false") << "; \n";
    strm << indent << "hasNegatedUnfinishedToDo: " << (m_hasNegatedUnfinishedToDo ? "true" : "false") << "; \n";
    strm << indent << "hasFinishedToDo: " << (m_hasFinishedToDo ? "true" : "false") << "; \n";
    strm << indent << "hasNegatedFinishedToDo: " << (m_hasNegatedFinishedToDo ? "true" : "false") << "; \n";
    strm << indent << "hasAnyToDo: " << (m_hasAnyToDo ? "true" : "false") << "; \n";
    strm << indent << "hasNegatedAnyTodo: " << (m_hasNegatedAnyToDo ? "true" : "false") << " \n";
    strm << indent << "hasEncryption: " << (m_hasEncryption ? "true" : "false") << "\n";
    strm << indent << "hasNegatedEncryption: " << (m_hasNegatedEncryption ? "true" : "false") << " \n";

    CHECK_AND_PRINT_LIST(contentSearchTerms, QString);
    CHECK_AND_PRINT_LIST(negatedContentSearchTerms, QString);

#undef CHECK_AND_PRINT_LIST
#undef CHECK_AND_PRINT_ANY_ITEM

    strm << "}; \n";

    return strm;
}

bool NoteSearchQueryPrivate::isMatcheable() const
{
    if (m_hasAnyTag && m_hasNegatedAnyTag) {
        return false;
    }

    if (m_hasAnyTitleName && m_hasNegatedAnyTitleName) {
        return false;
    }

    if (m_hasAnyCreationTimestamp && m_hasNegatedAnyCreationTimestamp) {
        return false;
    }

    if (m_hasAnyModificationTimestamp && m_hasNegatedAnyModificationTimestamp) {
        return false;
    }

    if (m_hasAnyResourceMimeType && m_hasNegatedAnyResourceMimeType) {
        return false;
    }

    if (m_hasAnySubjectDateTimestamp && m_hasNegatedAnySubjectDateTimestamp) {
        return false;
    }

    if (m_hasAnyLatitude && m_hasNegatedAnyLatitude) {
        return false;
    }

    if (m_hasAnyLongitude && m_hasNegatedAnyLongitude) {
        return false;
    }

    if (m_hasAnyAltitude && m_hasNegatedAnyAltitude) {
        return false;
    }

    if (m_hasAnyAuthor && m_hasNegatedAnyAuthor) {
        return false;
    }

    if (m_hasAnySource && m_hasNegatedAnySource) {
        return false;
    }

    if (m_hasAnySourceApplication && m_hasNegatedAnySourceApplication) {
        return false;
    }

    if (m_hasAnyContentClass && m_hasNegatedAnyContentClass) {
        return false;
    }

    if (m_hasAnyPlaceName && m_hasNegatedAnyPlaceName) {
        return false;
    }

    if (m_hasAnyApplicationData && m_hasNegatedAnyApplicationData) {
        return false;
    }

    if (m_hasAnyReminderOrder && m_hasNegatedAnyReminderOrder) {
        return false;
    }

    if (m_hasAnyReminderTime && m_hasNegatedAnyReminderTime) {
        return false;
    }

    if (m_hasAnyReminderDoneTime && m_hasNegatedAnyReminderDoneTime) {
        return false;
    }

    if (m_hasAnyToDo && m_hasNegatedAnyToDo) {
        return false;
    }

    if (m_hasEncryption && m_hasNegatedEncryption) {
        return false;
    }

    return true;
}

bool NoteSearchQueryPrivate::hasAdvancedSearchModifiers() const
{
    return (!m_notebookModifier.isEmpty() || m_hasAnyModifier || !m_tagNames.isEmpty() || !m_negatedTagNames.isEmpty() ||
            m_hasAnyTag || m_hasNegatedAnyTag || !m_titleNames.isEmpty() || !m_negatedTitleNames.isEmpty() ||
            m_hasAnyTitleName || m_hasNegatedAnyTitleName || !m_creationTimestamps.isEmpty() || !m_negatedCreationTimestamps.isEmpty() ||
            m_hasAnyCreationTimestamp || m_hasNegatedAnyCreationTimestamp || !m_modificationTimestamps.isEmpty() ||
            !m_negatedModificationTimestamps.isEmpty() || m_hasAnyModificationTimestamp || m_hasNegatedAnyModificationTimestamp ||
            !m_resourceMimeTypes.isEmpty() || !m_negatedResourceMimeTypes.isEmpty() || m_hasAnyResourceMimeType || m_hasNegatedAnyResourceMimeType ||
            !m_subjectDateTimestamps.isEmpty() || !m_negatedSubjectDateTimestamps.isEmpty() || m_hasAnySubjectDateTimestamp ||
            m_hasNegatedAnySubjectDateTimestamp || !m_latitudes.isEmpty() || !m_negatedLatitudes.isEmpty() ||
            m_hasAnyLatitude || m_hasNegatedAnyLatitude || !m_longitudes.isEmpty() || !m_negatedLongitudes.isEmpty() ||
            m_hasAnyLongitude || m_hasNegatedAnyLongitude || !m_altitudes.isEmpty() || !m_negatedAltitudes.isEmpty() ||
            m_hasAnyAltitude || m_hasNegatedAnyAltitude || !m_authors.isEmpty() || !m_negatedAuthors.isEmpty() ||
            m_hasAnyAuthor || m_hasNegatedAnyAuthor || !m_sources.isEmpty() || !m_negatedSources.isEmpty() ||
            m_hasAnySource || m_hasNegatedAnySource || !m_sourceApplications.isEmpty() || !m_negatedSourceApplications.isEmpty() ||
            m_hasAnySourceApplication || m_hasNegatedAnySourceApplication || !m_contentClasses.isEmpty() || !m_negatedContentClasses.isEmpty() ||
            m_hasAnyContentClass || m_hasNegatedAnyContentClass || !m_placeNames.isEmpty() || !m_negatedPlaceNames.isEmpty() ||
            m_hasAnyPlaceName || m_hasNegatedAnyPlaceName || !m_applicationData.isEmpty() || !m_negatedApplicationData.isEmpty() ||
            m_hasAnyApplicationData || m_hasNegatedAnyApplicationData || !m_reminderOrders.isEmpty() || !m_negatedReminderOrders.isEmpty() ||
            m_hasAnyReminderOrder || m_hasNegatedAnyReminderOrder || !m_reminderTimes.isEmpty() || !m_negatedReminderTimes.isEmpty() ||
            m_hasAnyReminderTime || m_hasNegatedAnyReminderTime || !m_reminderDoneTimes.isEmpty() || !m_negatedReminderDoneTimes.isEmpty() ||
            m_hasAnyReminderDoneTime || m_hasNegatedAnyReminderDoneTime || m_hasUnfinishedToDo || m_hasNegatedUnfinishedToDo ||
            m_hasFinishedToDo || m_hasNegatedFinishedToDo || m_hasAnyToDo || m_hasNegatedAnyToDo || m_hasEncryption || m_hasNegatedEncryption);
}

QStringList NoteSearchQueryPrivate::splitSearchQueryString(const QString & searchQueryString) const
{
    QStringList words;

    // Retrieving single words from the query string considering any text between quotes a single word
    bool insideQuotedText = false;
    bool insideUnquotedWord = false;
    int length = searchQueryString.length();
    const QChar space = ' ';
    const QChar quote = '\"';
    QString currentWord;
    for(int i = 0; i < length; ++i)
    {
        QChar chr = searchQueryString[i];

        if (chr == space)
        {
            if (insideQuotedText) {
                currentWord += chr;
                continue;
            }

            if (insideUnquotedWord && !currentWord.isEmpty()) {
                words << currentWord;
                currentWord.resize(0);
                insideUnquotedWord = false;
                continue;
            }
        }
        else if (chr == quote)
        {
            currentWord += chr;

            if (i == (length - 1))
            {
                // The last word, grab it and go
                words << currentWord;
                currentWord.resize(0);
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
                                    // Yes, backslash is escaped itself, so the quote at i is really the enclosing one
                                    backslashEscaped = true;
                                }
                            }

                            if (!backslashEscaped) {
                                continue;
                            }
                        }
                    }

                    words << currentWord;
                    currentWord.resize(0);
                    insideQuotedText = false;
                    insideUnquotedWord = false;
                    continue;
                }
            }
            else
            {
                if (!insideQuotedText) {
                    insideQuotedText = true;
                }

                continue;
            }
        }
        else
        {
            currentWord += chr;

            if (!insideQuotedText && !insideUnquotedWord) {
                insideUnquotedWord = true;
            }
        }
    }

    if (!currentWord.isEmpty()) {
        words << currentWord;
        currentWord.resize(0);
    }

    // Now we can remove any quotes from the words from the splitted query string
    int numWords = words.size();
    for(int i = 0; i < numWords; ++i)
    {
        QString & word = words[i];
        removeBoundaryQuotesFromWord(word);
    }

    return words;
}

void NoteSearchQueryPrivate::parseStringValue(const QString & key, QStringList & words,
                                              QStringList & container, QStringList & negatedContainer,
                                              bool & hasAnyValue, bool & hasNegatedAnyValue) const
{
    int keyIndex = 0;
    QChar negation('-');
    QStringList processedWords;

    QString asterisk = "*";

    QRegExp regexp(asterisk + key + QString(":*"));
    regexp.setPatternSyntax(QRegExp::Wildcard);

    while(keyIndex >= 0)
    {
        keyIndex = words.indexOf(regexp, (keyIndex >= 0 ? keyIndex : 0));
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

        if (isNegated) {
            word = word.remove(QString("-") + key + QString(":"));
        }
        else {
            word = word.remove(key + QString(":"));
        }
        removeBoundaryQuotesFromWord(word);

        if (word == asterisk)
        {
            if (isNegated) {
                hasNegatedAnyValue = true;
            }
            else {
                hasAnyValue = true;
            }
        }

        if (isNegated) {
            negatedContainer << word;
        }
        else {
            container << word;
        }
    }

    foreach(const QString & word, processedWords) {
        words.removeAll(word);
    }
}

bool NoteSearchQueryPrivate::parseIntValue(const QString & key, QStringList & words,
                                           QVector<qint64> & container, QVector<qint64> & negatedContainer,
                                           bool & hasAnyValue, bool & hasNegatedAnyValue, QString & error) const
{
    int keyIndex = 0;
    QChar negation('-');
    QStringList processedWords;

    QString asterisk = "*";

    QRegExp regexp(asterisk + key + QString(":*"));
    regexp.setPatternSyntax(QRegExp::Wildcard);

    while(keyIndex >= 0)
    {
        keyIndex = words.indexOf(regexp, (keyIndex >= 0 ? keyIndex : 0));
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

        if (isNegated) {
            word = word.remove(QString("-") + key + QString(":"));
        }
        else {
            word = word.remove(key + QString(":"));
        }
        removeBoundaryQuotesFromWord(word);

        if (word == asterisk)
        {
            if (isNegated) {
                hasNegatedAnyValue = true;
            }
            else {
                hasAnyValue = true;
            }
        }
        else
        {
            bool conversionResult = false;
            qint64 value = static_cast<qint64>(word.toLongLong(&conversionResult));
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid search query: cannot parse integer value \"");
                error += word;
                error += QT_TR_NOOP("\", search term of parsed value is \"");
                error += key;
                error += "\"";
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
                                              bool & hasAnyValue, bool & hasNegatedAnyValue, QString & error) const
{
    int keyIndex = 0;
    QChar negation('-');
    QStringList processedWords;

    QString asterisk = "*";

    QRegExp regexp(asterisk + key + QString(":*"));
    regexp.setPatternSyntax(QRegExp::Wildcard);

    while(keyIndex >= 0)
    {
        keyIndex = words.indexOf(regexp, (keyIndex >= 0 ? keyIndex : 0));
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

        if (isNegated) {
            word = word.remove(QString("-") + key + QString(":"));
        }
        else {
            word = word.remove(key + QString(":"));
        }
        removeBoundaryQuotesFromWord(word);

        if (word == asterisk)
        {
            if (isNegated) {
                hasNegatedAnyValue = true;
            }
            else {
                hasAnyValue = true;
            }
        }
        else
        {
            bool conversionResult = false;
            double value = static_cast<double>(word.toDouble(&conversionResult));
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid search query: cannot parse double value \"");
                error += word;
                error += QT_TR_NOOP("\", search term of parsed value is \"");
                error += key;
                error += "\"";
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

bool NoteSearchQueryPrivate::dateTimeStringToTimestamp(QString dateTimeString,
                                                       qint64 & timestamp, QString & error) const
{
    QDateTime todayMidnight = QDateTime::currentDateTime();
    todayMidnight.setTime(QTime(0, 0, 0, 0));

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

            dateTime = dateTime.addDays(daysOffset);
        }
    }
    else if (dateTimeString.startsWith("week"))
    {
        relativeDateArgumentFound = true;

        int dayOfWeek = dateTime.date().dayOfWeek();
        dateTime = dateTime.addDays(-1 * dayOfWeek);   // go to week start

        QString offsetSubstr = dateTimeString.remove("week");
        if (!offsetSubstr.isEmpty())
        {
            bool conversionResult = false;
            int weekOffset = offsetSubstr.toInt(&conversionResult);
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid query string: unable to convert weeks offset to integer");
                return false;
            }

            dateTime = dateTime.addDays(7 * weekOffset);
        }
    }
    else if (dateTimeString.startsWith("month"))
    {
        relativeDateArgumentFound = true;

        int dayOfMonth = dateTime.date().day();
        dateTime = dateTime.addDays(-1 * (dayOfMonth - 1));   // go to month start

        QString offsetSubstr = dateTimeString.remove("month");
        if (!offsetSubstr.isEmpty())
        {
            bool conversionResult = false;
            int monthOffset = offsetSubstr.toInt(&conversionResult);
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid query string: unable to convert months offset to integer");
                return false;
            }

            dateTime = dateTime.addMonths(monthOffset);
        }
    }
    else if (dateTimeString.startsWith("year"))
    {
        relativeDateArgumentFound = true;

        int dayOfMonth = dateTime.date().day();
        dateTime = dateTime.addDays(-1 * (dayOfMonth - 1));    // go to month start

        int currentMonth = dateTime.date().month();
        dateTime = dateTime.addMonths(-1 * (currentMonth - 1));    // go to year start

        QString offsetSubstr = dateTimeString.remove("year");
        if (!offsetSubstr.isEmpty())
        {
            bool conversionResult = false;
            int yearsOffset = offsetSubstr.toInt(&conversionResult);
            if (!conversionResult) {
                error = QT_TR_NOOP("Invalid query string: unable to convert years offset to integer");
                return false;
            }

            dateTime = dateTime.addYears(yearsOffset);
        }
    }

    if (relativeDateArgumentFound)
    {
        if (!dateTime.isValid()) {
            error = QT_TR_NOOP("Internal error: datetime processed from query string is invalid: ");
            error += QT_TR_NOOP("Datetime in ISO 861 format: ") + dateTime.toString(Qt::ISODate);
            error += QT_TR_NOOP(", datetime in simple text format: ") + dateTime.toString(Qt::TextDate);
            QNWARNING(error);
            return false;
        }

        timestamp = dateTime.toMSecsSinceEpoch();
        return true;
    }

    // Getting here means the datetime in the string is actually a datetime in ISO 8601 compact profile
    dateTime = QDateTime::fromString(dateTimeString, Qt::ISODate);
    if (!dateTime.isValid()) {
        error = QT_TR_NOOP("Invalid query string: cannot parse datetime from \"");
        error += dateTimeString;
        error += "\"";
        QNDEBUG(error);
        return false;
    }

    timestamp = dateTime.toMSecsSinceEpoch();
    return true;
}

bool NoteSearchQueryPrivate::convertAbsoluteAndRelativeDateTimesToTimestamps(QStringList & words, QString & error) const
{
    QStringList dateTimePrefixes;
    dateTimePrefixes << "created:" << "-created:" << "updated:" << "-updated:"
                     << "subjectDate:" << "-subjectDate:" << "reminderTime:"
                     << "-reminderTime:" << "reminderDoneTime:" << "-reminderDoneTime:";

    QString asterisk = "*";
    int numWords = words.size();
    QString wordCopy;
    for(int i = 0; i < numWords; ++i)
    {
        QString & word = words[i];

        foreach (const QString & prefix, dateTimePrefixes)
        {
            if (word.startsWith(prefix))
            {
                wordCopy = word;

                QString dateTimeString = wordCopy.remove(prefix);
                if (dateTimeString == asterisk) {
                    continue;
                }

                word = wordCopy;

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

void NoteSearchQueryPrivate::removeBoundaryQuotesFromWord(QString & word) const
{
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

} // namespace qute_note
