#include "NoteSearchQueryTest.h"
#include <qute_note/local_storage/NoteSearchQuery.h>
#include <qute_note/utility/StringUtils.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QStringList>
#include <QDateTime>
#include <bitset>

namespace qute_note {
namespace test {

bool NoteSearchQueryTest(QString & error)
{
    // Disclaimer: unfortunately, it doesn't seem possible to verify every potential combinations
    // of different keywords and scope modifiers within the search query. Due to that, only a subset
    // of a limited number of combinations of keywords would be tested

    QString notebookName = "fake notebook name";

    QStringList tagNames;
    tagNames << "allowed tag 1" << "allowed_tag_2" << "allowed tag 3" << "*";

    QStringList negatedTagNames;
    negatedTagNames << "disallowed tag 1" << "disallowed_tag_2" << "disallowed tag 3" << "*";

    QStringList titleNames;
    titleNames << "allowed title name 1" << "allowed_title_name_2" << "allowed title name 3" << "*";

    QStringList negatedTitleNames;
    negatedTitleNames << "disallowed title name 1" << "disallowed_title_name_2"
                      << "disallowed title name 3" << "*";

    // Adding the relative datetime attributes with their respective timestamps
    // for future comparison with query string parsing results

    QHash<QString,qint64> timestampForDateTimeString;

    QDateTime datetime = QDateTime::currentDateTime();
    datetime.setTime(QTime(0, 0, 0, 0));  // today midnight
    timestampForDateTimeString["day"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-1);
    timestampForDateTimeString["day-1"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-1);
    timestampForDateTimeString["day-2"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-1);
    timestampForDateTimeString["day-3"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(4);
    timestampForDateTimeString["day+1"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(1);
    timestampForDateTimeString["day+2"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(1);
    timestampForDateTimeString["day+3"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-3);   // return back to today midnight

    int dayOfWeek = datetime.date().dayOfWeek();
    datetime = datetime.addDays(-1 * dayOfWeek);   // go to the closest previous Sunday

    timestampForDateTimeString["week"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-7);
    timestampForDateTimeString["week-1"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-7);
    timestampForDateTimeString["week-2"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-7);
    timestampForDateTimeString["week-3"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(28);
    timestampForDateTimeString["week+1"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(7);
    timestampForDateTimeString["week+2"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(7);
    timestampForDateTimeString["week+3"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-21 + dayOfWeek);  // return to today midnight

    int dayOfMonth = datetime.date().day();
    datetime = datetime.addDays(-(dayOfMonth-1));
    timestampForDateTimeString["month"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(-1);
    timestampForDateTimeString["month-1"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(-1);
    timestampForDateTimeString["month-2"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(-1);
    timestampForDateTimeString["month-3"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(4);
    timestampForDateTimeString["month+1"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(1);
    timestampForDateTimeString["month+2"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(1);
    timestampForDateTimeString["month+3"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(-3);
    datetime = datetime.addDays(dayOfMonth-1);  // return back to today midnight

    int monthOfYear = datetime.date().month();
    datetime = datetime.addMonths(-(monthOfYear-1));
    datetime = datetime.addDays(-(dayOfMonth-1));
    timestampForDateTimeString["year"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(-1);
    timestampForDateTimeString["year-1"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(-1);
    timestampForDateTimeString["year-2"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(-1);
    timestampForDateTimeString["year-3"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(4);
    timestampForDateTimeString["year+1"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(1);
    timestampForDateTimeString["year+2"] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(1);
    timestampForDateTimeString["year+3"] = datetime.toMSecsSinceEpoch();

    QStringList created;
    created << "day-3" << "day-2" << "day-1";

    QStringList negatedCreated;
    negatedCreated << "day" << "day+1" << "day+2";

    QStringList updated;
    updated << "day+3" << "week-3" << "week-2";

    QStringList negatedUpdated;
    negatedUpdated << "week-1" << "week" << "week+1";

    QStringList resourceMimeTypes;
    resourceMimeTypes << "allowed resource mime type 1" << "allowed_resource_mime_type_2"
                      << "allowed resource mime type 3" << "*";

    QStringList negatedResourceMimeTypes;
    negatedResourceMimeTypes << "disallowed resource mime type 1" << "disallowed_resource_mime_type_2"
                             << "disallowed resource mime type 3" << "*";

    QStringList subjectDate;
    subjectDate << "week+2" << "week+3" << "month-3";

    QStringList negatedSubjectDate;
    negatedSubjectDate << "month-2" << "month-1" << "month";

    QVector<double> latitudes;
    latitudes << 32.15 << 17.41 << 12.02;

    QVector<double> negatedLatitudes;
    negatedLatitudes << -33.13 << -21.02 << -10.55;

    QVector<double> longitudes;
    longitudes << 33.14 << 18.92 << 11.04;

    QVector<double> negatedLongitudes;
    negatedLongitudes << -35.18 << -21.93 << -13.24;

    QVector<double> altitudes;
    altitudes << 34.10 << 16.17 << 10.93;

    QVector<double> negatedAltitudes;
    negatedAltitudes << -32.96 << -19.15 << -10.25;

    QStringList authors;
    authors << "author 1" << "author_2" << "author 3" << "*";

    QStringList negatedAuthors;
    negatedAuthors << "author 4" << "author_5" << "author 6" << "*";

    QStringList sources;
    sources << "web.clip" << "mail.clip" << "mail.smpt" << "*";

    QStringList negatedSources;
    negatedSources << "mobile.ios" << "mobile.android" << "mobile.*" << "*";

    QStringList sourceApplications;
    sourceApplications << "qutenote" << "nixnote2" << "everpad" << "*";

    QStringList negatedSourceApplications;
    negatedSourceApplications << "food.*" << "hello.*" << "skitch.*" << "*";

    QStringList contentClasses;
    contentClasses << "content class 1" << "content_class_2" << "content class 3" << "*";

    QStringList negatedContentClasses;
    negatedContentClasses << "content class 4" << "content_class_5" << "content class 6" << "*";

    QStringList placeNames;
    placeNames << "home" << "university" << "work" << "*";

    QStringList negatedPlaceNames;
    negatedPlaceNames << "bus" << "caffee" << "shower" << "*";

    QStringList applicationData;
    applicationData << "application data 1" << "application_data_2" << "applicationData 3" << "*";

    QStringList negatedApplicationData;
    negatedApplicationData << "negated application data 1" << "negated_application_data_2"
                           << "negated application data 3" << "*";

    QStringList recognitionTypes;
    recognitionTypes << "printed" << "speech" << "handwritten" << "*";

    QStringList negatedRecognitionTypes;
    negatedRecognitionTypes << "picture" << "unknown" << "*";

    QVector<qint64> reminderOrders;
    reminderOrders << 1 << 2 << 3;

    QVector<qint64> negatedReminderOrders;
    negatedReminderOrders << 4 << 5 << 6;

    QStringList reminderTimes;
    reminderTimes << "month+1" << "month+2" << "month+3";

    QStringList negatedReminderTimes;
    negatedReminderTimes << "year" << "year-1" << "year-2";

    QStringList reminderDoneTimes;
    reminderDoneTimes << "year-3" << "year+1" << "year+2";

    QStringList negatedReminderDoneTimes;
    negatedReminderDoneTimes << "year+3" << datetime.addYears(-1).toString(Qt::ISODate)
                             << datetime.toString(Qt::ISODate);

    timestampForDateTimeString[datetime.toString(Qt::ISODate)] = datetime.toMSecsSinceEpoch();
    timestampForDateTimeString[datetime.addYears(-1).toString(Qt::ISODate)] = datetime.addYears(-1).toMSecsSinceEpoch();

    QStringList contentSearchTerms;
    contentSearchTerms << "THINK[] !" << "[do! " << " act]!" << "*";

    QStringList negatedContentSearchTerms;
    negatedContentSearchTerms << "BIRD[" << "*is" << "a" << "word*" << "***";

    NoteSearchQuery noteSearchQuery;

    // Iterating over all combinations of 8 boolean factors with a special meaning
    for(int mask = 0; mask != (1<<8); ++mask)
    {
        // NOTE: workarounding VC++ 2010 bug
#if defined(_MSC_VER) && _MSC_VER <= 1600
        std::bitset<static_cast<unsigned long long>(8)> bits(static_cast<unsigned long long>(mask));
#else
        std::bitset<8> bits(static_cast<quint32>(mask));
#endif

        QString queryString;

        if (bits[0]) {
            queryString += "notebook:";
            queryString += "\"";
            queryString += notebookName;
            queryString += "\" ";
        }

        if (bits[1]) {
            queryString += "any: ";
        }

        if (bits[2]) {
            queryString += "todo:false ";
        }
        else {
            queryString += "-todo:false ";
        }

        if (bits[3]) {
            queryString += "todo:true ";
        }
        else {
            queryString += "-todo:true ";
        }

        if (bits[4]) {
            queryString += "todo:* ";
        }

        if (bits[5]) {
            queryString += "-todo:* ";
        }

        if (bits[6]) {
            queryString += "encryption: ";
        }

        if (bits[7]) {
            queryString += "-encryption: ";
        }

#define ADD_LIST_TO_QUERY_STRING(keyword, list, type, ...) \
    foreach(const type & item, list) { \
        queryString += #keyword ":"; \
        QString itemStr = __VA_ARGS__(item); \
        bool itemContainsSpace = itemStr.contains(" "); \
        if (itemContainsSpace) { \
            queryString += "\""; \
        } \
        queryString += itemStr; \
        if (itemContainsSpace) { \
            queryString += "\""; \
        } \
        queryString += " "; \
    } \

        ADD_LIST_TO_QUERY_STRING(tag, tagNames, QString);
        ADD_LIST_TO_QUERY_STRING(-tag, negatedTagNames, QString);
        ADD_LIST_TO_QUERY_STRING(intitle, titleNames, QString);
        ADD_LIST_TO_QUERY_STRING(-intitle, negatedTitleNames, QString);
        ADD_LIST_TO_QUERY_STRING(created, created, QString);
        ADD_LIST_TO_QUERY_STRING(-created, negatedCreated, QString);
        ADD_LIST_TO_QUERY_STRING(updated, updated, QString);
        ADD_LIST_TO_QUERY_STRING(-updated, negatedUpdated, QString);
        ADD_LIST_TO_QUERY_STRING(resource, resourceMimeTypes, QString);
        ADD_LIST_TO_QUERY_STRING(-resource, negatedResourceMimeTypes, QString);
        ADD_LIST_TO_QUERY_STRING(subjectDate, subjectDate, QString);
        ADD_LIST_TO_QUERY_STRING(-subjectDate, negatedSubjectDate, QString);
        ADD_LIST_TO_QUERY_STRING(latitude, latitudes, double, QString::number);
        ADD_LIST_TO_QUERY_STRING(-latitude, negatedLatitudes, double, QString::number);
        ADD_LIST_TO_QUERY_STRING(longitude, longitudes, double, QString::number);
        ADD_LIST_TO_QUERY_STRING(-longitude, negatedLongitudes, double, QString::number);
        ADD_LIST_TO_QUERY_STRING(altitude, altitudes, double, QString::number);
        ADD_LIST_TO_QUERY_STRING(-altitude, negatedAltitudes, double, QString::number);
        ADD_LIST_TO_QUERY_STRING(author, authors, QString);
        ADD_LIST_TO_QUERY_STRING(-author, negatedAuthors, QString);
        ADD_LIST_TO_QUERY_STRING(source, sources, QString);
        ADD_LIST_TO_QUERY_STRING(-source, negatedSources, QString);
        ADD_LIST_TO_QUERY_STRING(sourceApplication, sourceApplications, QString);
        ADD_LIST_TO_QUERY_STRING(-sourceApplication, negatedSourceApplications, QString);
        ADD_LIST_TO_QUERY_STRING(contentClass, contentClasses, QString);
        ADD_LIST_TO_QUERY_STRING(-contentClass, negatedContentClasses, QString);
        ADD_LIST_TO_QUERY_STRING(placeName, placeNames, QString);
        ADD_LIST_TO_QUERY_STRING(-placeName, negatedPlaceNames, QString);
        ADD_LIST_TO_QUERY_STRING(applicationData, applicationData, QString);
        ADD_LIST_TO_QUERY_STRING(-applicationData, negatedApplicationData, QString);
        ADD_LIST_TO_QUERY_STRING(recoType, recognitionTypes, QString);
        ADD_LIST_TO_QUERY_STRING(-recoType, negatedRecognitionTypes, QString);
        ADD_LIST_TO_QUERY_STRING(reminderOrder, reminderOrders, qint64, QString::number);
        ADD_LIST_TO_QUERY_STRING(-reminderOrder, negatedReminderOrders, qint64, QString::number);
        ADD_LIST_TO_QUERY_STRING(reminderTime, reminderTimes, QString);
        ADD_LIST_TO_QUERY_STRING(-reminderTime, negatedReminderTimes, QString);
        ADD_LIST_TO_QUERY_STRING(reminderDoneTime, reminderDoneTimes, QString);
        ADD_LIST_TO_QUERY_STRING(-reminderDoneTime, negatedReminderDoneTimes, QString);

#undef ADD_LIST_TO_QUERY_STRING

        queryString += "created:* -created:* ";
        queryString += "updated:* -updated:* ";
        queryString += "subjectDate:* -subjectDate:* ";
        queryString += "latitude:* -latitude:* ";
        queryString += "longitude:* -longitude:* ";
        queryString += "altitude:* -altitude:* ";
        queryString += "reminderOrder:* -reminderOrder:* ";
        queryString += "reminderTime:* -reminderTime:* ";
        queryString += "reminderDoneTime:* -reminderDoneTime:* ";

        foreach(const QString & searchTerm, contentSearchTerms) {
            bool searchTermContainsSpace = searchTerm.contains(" ");
            if (searchTermContainsSpace) {
                queryString += "\"";
            }
            queryString += searchTerm;
            if (searchTermContainsSpace) {
                queryString += "\" ";
            }
        }

        foreach(const QString & negatedSearchTerm, negatedContentSearchTerms) {
            bool negatedSearchTermContainsSpace = negatedSearchTerm.contains(" ");
            if (negatedSearchTermContainsSpace) {
                queryString += "\"";
            }
            queryString += "-";
            queryString += negatedSearchTerm;
            if (negatedSearchTermContainsSpace) {
                queryString += "\" ";
            }
        }

        bool res = noteSearchQuery.setQueryString(queryString, error);
        if (!res) {
            return false;
        }

        const QString & noteSearchQueryString = noteSearchQuery.queryString();
        if (noteSearchQueryString != queryString)
        {
            error = "NoteSearchQuery has query string not equal to original string: ";
            error += "original string: " + queryString + "; NoteSearchQuery's internal query string: ";
            error += noteSearchQuery.queryString();
            return false;
        }

        if (!noteSearchQuery.hasAnyTag()) {
            error = "NoteSearchQuery doesn't have any tag while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyTag()) {
            error = "NoteSearchQuery doesn't have negated any tag while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyTitleName()) {
            error = "NoteSearchQuery doesn't have any title name while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyTitleName()) {
            error = "NoteSearchQuery doesn't have negated any title name while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyCreationTimestamp()) {
            error = "NoteSearchQuery doesn't have any creation timestamp while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyCreationTimestamp()) {
            error = "NoteSearchQuery doesn't have negated any creation timestamp while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyModificationTimestamp()) {
            error = "NoteSearchQuery doesn't have any modification timestamp while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyModificationTimestamp()) {
            error = "NoteSearchQuery doesn't have negated any modification timestamp while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyResourceMimeType()) {
            error = "NoteSearchQuery doesn't have any resource mime type while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyResourceMimeType()) {
            error = "NoteSearchQuery doesn't have negated any resource mime type while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnySubjectDateTimestamp()) {
            error = "NoteSearchQuery doesn't have any subject date timestamp while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnySubjectDateTimestamp()) {
            error = "NoteSearchQuery doesn't have negated any subject date timestamp while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyLatitude()) {
            error = "NoteSearchQuery doesn't have any latitude while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyLatitude()) {
            error = "NoteSearchQuery doesn't have negated any latitude while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyLongitude()) {
            error = "NoteSearchQuery doesn't have any longitude while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyLongitude()) {
            error = "NoteSearchQuery doesn't have negated any longitude while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyAltitude()) {
            error = "NoteSearchQuery doesn't have any altitude while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyAltitude()) {
            error = "NoteSearchQuery doesn't have negated any altitude while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyAuthor()) {
            error = "NoteSearchQuery doesn't have any author while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyAuthor()) {
            error = "NoteSearchQuery doesn't have negated any author while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnySource()) {
            error = "NoteSearchQuery doesn't have any source while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnySource()) {
            error = "NoteSearchQuery doesn't have negated any source while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnySourceApplication()) {
            error = "NoteSearchQuery doesn't have any source application while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnySourceApplication()) {
            error = "NoteSearchQuery doesn't have negated any source application while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyContentClass()) {
            error = "NoteSearchQuery doesn't have any content class while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyContentClass()) {
            error = "NoteSearchQuery doesn't have negated any content class while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyPlaceName()) {
            error = "NoteSearchQuery doesn't have any place name while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyPlaceName()) {
            error = "NoteSearchQuery doesn't have negated any place name while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyApplicationData()) {
            error = "NoteSearchQuery doesn't have any application data while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyApplicationData()) {
            error = "NoteSearchQuery doesn't have negated any application data while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyRecognitionType()) {
            error = "NoteSearchQuery doesn't have any recognition type while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyRecognitionType()) {
            error = "NoteSearchQuery doesn't have negated any recognition type while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyReminderOrder()) {
            error = "NoteSearchQuery doesn't have any reminder order while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyReminderOrder()) {
            error = "NoteSearchQuery doesn't have negated any reminder order while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyReminderTime()) {
            error = "NoteSearchQuery doesn't have any reminder time while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyReminderTime()) {
            error = "NoteSearchQuery doesn't have negated any reminder time while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyReminderDoneTime()) {
            error = "NoteSearchQuery doesn't have any reminder done time while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyReminderDoneTime()) {
            error = "NoteSearchQuery doesn't have negated any reminder done time while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAdvancedSearchModifiers()) {
            error = "NoteSearchQuery doesn't have advanced search modifiers while it should have some";
            return false;
        }

        if (bits[0])
        {
            const QString & noteSearchQueryNotebookModifier = noteSearchQuery.notebookModifier();
            if (noteSearchQueryNotebookModifier != notebookName)
            {
                error = "NoteSearchQuery's notebook modifier is not equal to original notebook name: ";
                error += "original notebook name: " + notebookName;
                error += ", NoteSearchQuery's internal notebook modifier: ";
                error += noteSearchQuery.notebookModifier();
                return false;
            }
        }
        else if (!noteSearchQuery.notebookModifier().isEmpty())
        {
            error = "NoteSearchQuery's notebook modified is not empty while it should be!";
            return false;
        }

        if (bits[2])
        {
            if (!noteSearchQuery.hasUnfinishedToDo()) {
                error = "NoteSearchQuery doesn't have unfinished todo while it should have one";
                return false;
            }

            if (noteSearchQuery.hasNegatedUnfinishedToDo()) {
                error = "NoteSearchQUery has negated unfinished todo while it should have non-negated one";
                return false;
            }
        }
        else
        {
            if (!noteSearchQuery.hasNegatedUnfinishedToDo()) {
                error = "NoteSearchQUery doesn't have negated unfinished todo while it should have one";
                return false;
            }

            if (noteSearchQuery.hasUnfinishedToDo()) {
                error = "NoteSearchQuery has unfinished todo while it should have negated unfinished todo";
                return false;
            }
        }

        if (bits[3])
        {
            if (!noteSearchQuery.hasFinishedToDo()) {
                error = "NoteSearchQuery hasn't finished todo while it should have one";
                return false;
            }

            if (noteSearchQuery.hasNegatedFinishedToDo()) {
                error = "NoteSearchQuery has negated finished todo while it should have non-negated one";
                return false;
            }
        }
        else
        {
            if (!noteSearchQuery.hasNegatedFinishedToDo()) {
                error = "NoteSearchQuery doesn't have negated finished todo while it should have one";
                return false;
            }

            if (noteSearchQuery.hasFinishedToDo()) {
                error = "NoteSearchQuery has finished todo while it should have negated finished todo";
                return false;
            }
        }

        if (bits[4]) {
            if (!noteSearchQuery.hasAnyToDo()) {
                error = "NoteSearchQuery doesn't have \"any\" todo modifier while it should have one";
                return false;
            }
        }
        else {
            if (noteSearchQuery.hasAnyToDo()) {
                error = "NoteSearchQuery has \"any\" todo modifier while it should not have one";
                return false;
            }
        }

        if (bits[5]) {
            if (!noteSearchQuery.hasNegatedAnyToDo()) {
                error = "NoteSearchQuery doesn't have negated \"any\" todo modifier while it should have one";
                return false;
            }
        }
        else {
            if (noteSearchQuery.hasNegatedAnyToDo()) {
                error = "NoteSearchQuery has negated \"any\" todo modifier while it should not have one";
                return false;
            }
        }

        if (bits[6]) {
            if (!noteSearchQuery.hasEncryption()) {
                error = "NoteSearchQuery doesn't have encryption while it should have one";
                return false;
            }
        }
        else {
            if (noteSearchQuery.hasEncryption()) {
                error = "NoteSearchQuery has encryption while it should not have one";
                return false;
            }
        }

        if (bits[7]) {
            if (!noteSearchQuery.hasNegatedEncryption()) {
                error = "NoteSearchQuery doesn't have negated encryption while it should have one";
                return false;
            }
        }
        else {
            if (noteSearchQuery.hasNegatedEncryption()) {
                error = "NoteSearchQuery has negated encryption while it should not have one";
                return false;
            }
        }

#define CHECK_LIST(list, accessor, ...) \
    auto noteSearchQueryList##list = noteSearchQuery.accessor(); \
    if (noteSearchQueryList##list != list) { \
        error = "NoteSearchQuery: " #list " doesn't match the one from the original list; "; \
        error += "original " #list ": "; \
        foreach(const auto & item, list) { \
            error += __VA_ARGS__(item); \
            error += "; "; \
        } \
        error += "; \nNoteSearchQuery's list: "; \
        foreach(const auto & item, noteSearchQueryList##list) { \
            error += __VA_ARGS__(item); \
            error += "; "; \
        } \
        return false; \
    }

        CHECK_LIST(tagNames, tagNames);
        CHECK_LIST(negatedTagNames, negatedTagNames);
        CHECK_LIST(titleNames, titleNames);
        CHECK_LIST(negatedTitleNames, negatedTitleNames);
        CHECK_LIST(resourceMimeTypes, resourceMimeTypes);
        CHECK_LIST(negatedResourceMimeTypes, negatedResourceMimeTypes);
        CHECK_LIST(latitudes, latitudes, QString::number);
        CHECK_LIST(negatedLatitudes, negatedLatitudes, QString::number);
        CHECK_LIST(longitudes, longitudes, QString::number);
        CHECK_LIST(negatedLongitudes, negatedLongitudes, QString::number);
        CHECK_LIST(altitudes, altitudes, QString::number);
        CHECK_LIST(negatedAltitudes, negatedAltitudes, QString::number);
        CHECK_LIST(authors, authors);
        CHECK_LIST(negatedAuthors, negatedAuthors);
        CHECK_LIST(sources, sources);
        CHECK_LIST(negatedSources, negatedSources);
        CHECK_LIST(sourceApplications, sourceApplications);
        CHECK_LIST(negatedSourceApplications, negatedSourceApplications);
        CHECK_LIST(contentClasses, contentClasses);
        CHECK_LIST(negatedContentClasses, negatedContentClasses);
        CHECK_LIST(placeNames, placeNames);
        CHECK_LIST(negatedPlaceNames, negatedPlaceNames);
        CHECK_LIST(applicationData, applicationData);
        CHECK_LIST(negatedApplicationData, negatedApplicationData);
        CHECK_LIST(recognitionTypes, recognitionTypes);
        CHECK_LIST(negatedRecognitionTypes, negatedRecognitionTypes);
        CHECK_LIST(reminderOrders, reminderOrders, QString::number);
        CHECK_LIST(negatedReminderOrders, negatedReminderOrders, QString::number);

#undef CHECK_LIST

#define CHECK_DATETIME_LIST(list, accessor) \
    auto noteSearchQuery##list = noteSearchQuery.accessor(); \
    int size##list = noteSearchQuery##list.size() ; \
    for(int i = 0; i < size##list; ++i) \
    { \
        const auto & item = noteSearchQuery##list[i]; \
        const auto & strItem = list.at(i); \
        \
        if (!timestampForDateTimeString.contains(strItem)) { \
            error = "Internal error in test: unknown datetime argument "; \
            error += strItem; \
            return false; \
        } \
        \
        if (item != timestampForDateTimeString[strItem]) { \
            error = "Timestamp from NoteSearchQuery is different from precalculated one "; \
            error += "for string item \""; \
            error += strItem; \
            error += "\": precalculated timestamp = "; \
            error += QString::number(timestampForDateTimeString[strItem]); \
            error += ", timestamp from NoteSearchQuery: "; \
            error += QString::number(item); \
            return false; \
        } \
    }

        CHECK_DATETIME_LIST(created, creationTimestamps);
        CHECK_DATETIME_LIST(negatedCreated, negatedCreationTimestamps);
        CHECK_DATETIME_LIST(updated, modificationTimestamps);
        CHECK_DATETIME_LIST(negatedUpdated, negatedModificationTimestamps);
        CHECK_DATETIME_LIST(subjectDate, subjectDateTimestamps);
        CHECK_DATETIME_LIST(negatedSubjectDate, negatedSubjectDateTimestamps);
        CHECK_DATETIME_LIST(reminderTimes, reminderTimes);
        CHECK_DATETIME_LIST(negatedReminderTimes, negatedReminderTimes);
        CHECK_DATETIME_LIST(reminderDoneTimes, reminderDoneTimes);
        CHECK_DATETIME_LIST(negatedReminderDoneTimes, negatedReminderDoneTimes);

#undef CHECK_DATETIME_LIST
    }

    // Check that search terms and negated search terms are processed correctly
    QString contentSearchTermsStr = contentSearchTerms.join(" ");
    QString negatedContentSearchTermsStr;
    for(int i = 0, numNegatedContentSearchTerms = negatedContentSearchTerms.size(); i < numNegatedContentSearchTerms; ++i) {
        negatedContentSearchTermsStr += "-" + negatedContentSearchTerms[i] + " ";
    }

    bool res = noteSearchQuery.setQueryString(contentSearchTermsStr + " " + negatedContentSearchTermsStr, error);
    if (!res) {
        error = "Internal error: can't set simple search query string without any search modifiers: " + error;
        return false;
    }

    if (noteSearchQuery.hasAdvancedSearchModifiers()) {
        error = "NoteSearchQuery has advanced search modifiers while it shouldn't have them";
        return false;
    }

    const QStringList & contentSearchTermsFromQuery = noteSearchQuery.contentSearchTerms();
    const int numContentSearchTermsFromQuery = contentSearchTermsFromQuery.size();

    QRegExp asteriskFilter("[*]");

    QStringList filteredContentSearchTerms;
    filteredContentSearchTerms.reserve(numContentSearchTermsFromQuery);
    for(int i = 0, numOriginalContentSearchTerms = contentSearchTerms.size(); i < numOriginalContentSearchTerms; ++i)
    {
        QString filteredSearchTerm = contentSearchTerms[i];
        removePunctuation(filteredSearchTerm);
        if (filteredSearchTerm.isEmpty()) {
            continue;
        }

        // Don't accept search terms consisting only of asterisks
        QString filteredSearchTermWithoutAsterisks = filteredSearchTerm;
        filteredSearchTermWithoutAsterisks.remove(asteriskFilter);
        if (filteredSearchTermWithoutAsterisks.isEmpty()) {
            continue;
        }

        // Only accept asterisk if there's a single one in the end of the search term
        int indexOfAsterisk = filteredSearchTerm.indexOf("*");
        int lastIndexOfAsterisk = filteredSearchTerm.lastIndexOf("*");
        if (indexOfAsterisk != lastIndexOfAsterisk) {
            continue;
        }

        if ((indexOfAsterisk >= 0) && (indexOfAsterisk != (filteredSearchTerm.size() - 1))) {
            continue;
        }

        filteredContentSearchTerms << filteredSearchTerm.simplified().toLower();
    }

    if (numContentSearchTermsFromQuery != filteredContentSearchTerms.size()) {
        error = "Internal error: the number of content search terms doesn't match the expected one after parsing the note search query";
        QNWARNING(error << "; original content search terms: " << contentSearchTerms.join(" ") << "; filtered content search terms: "
                  << filteredContentSearchTerms.join(" ") << "; processed search query terms: " << contentSearchTermsFromQuery.join(" "));
        return false;
    }

    for(int i = 0; i < numContentSearchTermsFromQuery; ++i)
    {
        const QString & contentSearchTermFromQuery = contentSearchTermsFromQuery[i];
        int index = filteredContentSearchTerms.indexOf(contentSearchTermFromQuery);
        if (index < 0) {
            error = "Internal error: can't find one of original content search terms after parsing the note search query";
            QNWARNING(error << "; filtered original content search terms: " << filteredContentSearchTerms.join(" ")
                      << "; parsed content search terms: " << contentSearchTermsFromQuery.join(" "));
            return false;
        }
    }

    const QStringList & negatedContentSearchTermsFromQuery = noteSearchQuery.negatedContentSearchTerms();
    const int numNegatedContentSearchTermsFromQuery = negatedContentSearchTermsFromQuery.size();

    QStringList filteredNegatedContentSearchTerms;
    filteredNegatedContentSearchTerms.reserve(numNegatedContentSearchTermsFromQuery);
    for(int i = 0, numOriginalNegatedContentSearchTerms = negatedContentSearchTerms.size(); i < numOriginalNegatedContentSearchTerms; ++i)
    {
        QString filteredNegatedSearchTerm = negatedContentSearchTerms[i];
        removePunctuation(filteredNegatedSearchTerm);
        if (filteredNegatedSearchTerm.isEmpty()) {
            continue;
        }

        // Don't accept search terms consisting only of asterisks
        QString filteredNegatedSearchTermWithoutAsterisks = filteredNegatedSearchTerm;
        filteredNegatedSearchTermWithoutAsterisks.remove(asteriskFilter);
        if (filteredNegatedSearchTermWithoutAsterisks.isEmpty()) {
            continue;
        }

        // Only accept asterisk if there's a single one in the end of the search term
        int indexOfAsterisk = filteredNegatedSearchTerm.indexOf("*");
        int lastIndexOfAsterisk = filteredNegatedSearchTerm.lastIndexOf("*");
        if (indexOfAsterisk != lastIndexOfAsterisk) {
            continue;
        }

        if ((indexOfAsterisk >= 0) && (indexOfAsterisk != (filteredNegatedSearchTerm.size() - 1))) {
            continue;
        }

        filteredNegatedContentSearchTerms << filteredNegatedSearchTerm.simplified().toLower();
    }

    if (numNegatedContentSearchTermsFromQuery != filteredNegatedContentSearchTerms.size()) {
        error = "Internal error: the number of negated content search terms doesn't match the expected one after parsing the note search query";
        QNWARNING(error << "; original negated content search terms: " << negatedContentSearchTerms.join(" ") << "; filtered negated content search terms: "
                  << filteredNegatedContentSearchTerms.join(" ") << "; processed negated search query terms: " << negatedContentSearchTermsFromQuery.join(" "));
        return false;
    }

    for(int i = 0; i < numNegatedContentSearchTermsFromQuery; ++i)
    {
        const QString & negatedContentSearchTermFromQuery = negatedContentSearchTermsFromQuery[i];
        int index = filteredNegatedContentSearchTerms.indexOf(negatedContentSearchTermFromQuery);
        if (index < 0) {
            error = "Internal error: can't find one of original negated content search terms after parsing the note search query";
            QNWARNING(error << "; filtered original negated content search terms: " << filteredNegatedContentSearchTerms.join(" ")
                      << "; parsed negated content search terms: " << negatedContentSearchTermsFromQuery.join(" "));
            return false;
        }
    }

    return true;
}

} // namespace test
} // namespace qute_note
