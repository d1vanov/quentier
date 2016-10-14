/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteSearchQueryTest.h"
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/logging/QuentierLogger.h>
#include <QStringList>
#include <QDateTime>
#include <QString>
#include <QVector>
#include <bitset>

namespace quentier {
namespace test {

bool NoteSearchQueryTest(QString & error)
{
    // Disclaimer: unfortunately, it doesn't seem possible to verify every potential combinations
    // of different keywords and scope modifiers within the search query. Due to that, only a subset
    // of a limited number of combinations of keywords would be tested

    QString notebookName = QStringLiteral("fake notebook name");

    QStringList tagNames;
    tagNames << QStringLiteral("allowed tag 1")
             << QStringLiteral("allowed_tag_2")
             << QStringLiteral("allowed tag 3")
             << QStringLiteral("*");

    QStringList negatedTagNames;
    negatedTagNames << QStringLiteral("disallowed tag 1")
                    << QStringLiteral("disallowed_tag_2")
                    << QStringLiteral("disallowed tag 3")
                    << QStringLiteral("*");

    QStringList titleNames;
    titleNames << QStringLiteral("allowed title name 1")
               << QStringLiteral("allowed_title_name_2")
               << QStringLiteral("allowed title name 3")
               << QStringLiteral("*");

    QStringList negatedTitleNames;
    negatedTitleNames << QStringLiteral("disallowed title name 1")
                      << QStringLiteral("disallowed_title_name_2")
                      << QStringLiteral("disallowed title name 3")
                      << QStringLiteral("*");

    // Adding the relative datetime attributes with their respective timestamps
    // for future comparison with query string parsing results

    QHash<QString,qint64> timestampForDateTimeString;

    QDateTime datetime = QDateTime::currentDateTime();
    datetime.setTime(QTime(0, 0, 0, 0));  // today midnight
    timestampForDateTimeString[QStringLiteral("day")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-1);
    timestampForDateTimeString[QStringLiteral("day-1")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-1);
    timestampForDateTimeString[QStringLiteral("day-2")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-1);
    timestampForDateTimeString[QStringLiteral("day-3")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(4);
    timestampForDateTimeString[QStringLiteral("day+1")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(1);
    timestampForDateTimeString[QStringLiteral("day+2")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(1);
    timestampForDateTimeString[QStringLiteral("day+3")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-3);   // return back to today midnight

    int dayOfWeek = datetime.date().dayOfWeek();
    datetime = datetime.addDays(-1 * dayOfWeek);   // go to the closest previous Sunday

    timestampForDateTimeString[QStringLiteral("week")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-7);
    timestampForDateTimeString[QStringLiteral("week-1")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-7);
    timestampForDateTimeString[QStringLiteral("week-2")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-7);
    timestampForDateTimeString[QStringLiteral("week-3")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(28);
    timestampForDateTimeString[QStringLiteral("week+1")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(7);
    timestampForDateTimeString[QStringLiteral("week+2")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(7);
    timestampForDateTimeString[QStringLiteral("week+3")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addDays(-21 + dayOfWeek);  // return to today midnight

    int dayOfMonth = datetime.date().day();
    datetime = datetime.addDays(-(dayOfMonth-1));
    timestampForDateTimeString[QStringLiteral("month")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(-1);
    timestampForDateTimeString[QStringLiteral("month-1")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(-1);
    timestampForDateTimeString[QStringLiteral("month-2")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(-1);
    timestampForDateTimeString[QStringLiteral("month-3")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(4);
    timestampForDateTimeString[QStringLiteral("month+1")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(1);
    timestampForDateTimeString[QStringLiteral("month+2")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(1);
    timestampForDateTimeString[QStringLiteral("month+3")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addMonths(-3);
    datetime = datetime.addDays(dayOfMonth-1);  // return back to today midnight

    int monthOfYear = datetime.date().month();
    datetime = datetime.addMonths(-(monthOfYear-1));
    datetime = datetime.addDays(-(dayOfMonth-1));
    timestampForDateTimeString[QStringLiteral("year")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(-1);
    timestampForDateTimeString[QStringLiteral("year-1")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(-1);
    timestampForDateTimeString[QStringLiteral("year-2")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(-1);
    timestampForDateTimeString[QStringLiteral("year-3")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(4);
    timestampForDateTimeString[QStringLiteral("year+1")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(1);
    timestampForDateTimeString[QStringLiteral("year+2")] = datetime.toMSecsSinceEpoch();

    datetime = datetime.addYears(1);
    timestampForDateTimeString[QStringLiteral("year+3")] = datetime.toMSecsSinceEpoch();

    QStringList created;
    created << QStringLiteral("day-3")
            << QStringLiteral("day-2")
            << QStringLiteral("day-1");

    QStringList negatedCreated;
    negatedCreated << QStringLiteral("day")
                   << QStringLiteral("day+1")
                   << QStringLiteral("day+2");

    QStringList updated;
    updated << QStringLiteral("day+3")
            << QStringLiteral("week-3")
            << QStringLiteral("week-2");

    QStringList negatedUpdated;
    negatedUpdated << QStringLiteral("week-1")
                   << QStringLiteral("week")
                   << QStringLiteral("week+1");

    QStringList resourceMimeTypes;
    resourceMimeTypes << QStringLiteral("allowed resource mime type 1")
                      << QStringLiteral("allowed_resource_mime_type_2")
                      << QStringLiteral("allowed resource mime type 3")
                      << QStringLiteral("*");

    QStringList negatedResourceMimeTypes;
    negatedResourceMimeTypes << QStringLiteral("disallowed resource mime type 1")
                             << QStringLiteral("disallowed_resource_mime_type_2")
                             << QStringLiteral("disallowed resource mime type 3")
                             << QStringLiteral("*");

    QStringList subjectDate;
    subjectDate << QStringLiteral("week+2")
                << QStringLiteral("week+3")
                << QStringLiteral("month-3");

    QStringList negatedSubjectDate;
    negatedSubjectDate << QStringLiteral("month-2")
                       << QStringLiteral("month-1")
                       << QStringLiteral("month");

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
    authors << QStringLiteral("author 1")
            << QStringLiteral("author_2")
            << QStringLiteral("author 3")
            << QStringLiteral("*");

    QStringList negatedAuthors;
    negatedAuthors << QStringLiteral("author 4")
                   << QStringLiteral("author_5")
                   << QStringLiteral("author 6")
                   << QStringLiteral("*");

    QStringList sources;
    sources << QStringLiteral("web.clip")
            << QStringLiteral("mail.clip")
            << QStringLiteral("mail.smpt")
            << QStringLiteral("*");

    QStringList negatedSources;
    negatedSources << QStringLiteral("mobile.ios")
                   << QStringLiteral("mobile.android")
                   << QStringLiteral("mobile.*")
                   << QStringLiteral("*");

    QStringList sourceApplications;
    sourceApplications << QStringLiteral("quentier")
                       << QStringLiteral("nixnote2")
                       << QStringLiteral("everpad")
                       << QStringLiteral("*");

    QStringList negatedSourceApplications;
    negatedSourceApplications << QStringLiteral("food.*")
                              << QStringLiteral("hello.*")
                              << QStringLiteral("skitch.*")
                              << QStringLiteral("*");

    QStringList contentClasses;
    contentClasses << QStringLiteral("content class 1")
                   << QStringLiteral("content_class_2")
                   << QStringLiteral("content class 3")
                   << QStringLiteral("*");

    QStringList negatedContentClasses;
    negatedContentClasses << QStringLiteral("content class 4")
                          << QStringLiteral("content_class_5")
                          << QStringLiteral("content class 6")
                          << QStringLiteral("*");

    QStringList placeNames;
    placeNames << QStringLiteral("home")
               << QStringLiteral("university")
               << QStringLiteral("work")
               << QStringLiteral("*");

    QStringList negatedPlaceNames;
    negatedPlaceNames << QStringLiteral("bus")
                      << QStringLiteral("caffee")
                      << QStringLiteral("shower")
                      << QStringLiteral("*");

    QStringList applicationData;
    applicationData << QStringLiteral("application data 1")
                    << QStringLiteral("application_data_2")
                    << QStringLiteral("applicationData 3")
                    << QStringLiteral("*");

    QStringList negatedApplicationData;
    negatedApplicationData << QStringLiteral("negated application data 1")
                           << QStringLiteral("negated_application_data_2")
                           << QStringLiteral("negated application data 3")
                           << QStringLiteral("*");

    QVector<qint64> reminderOrders;
    reminderOrders << 1 << 2 << 3;

    QVector<qint64> negatedReminderOrders;
    negatedReminderOrders << 4 << 5 << 6;

    QStringList reminderTimes;
    reminderTimes << QStringLiteral("month+1")
                  << QStringLiteral("month+2")
                  << QStringLiteral("month+3");

    QStringList negatedReminderTimes;
    negatedReminderTimes << QStringLiteral("year")
                         << QStringLiteral("year-1")
                         << QStringLiteral("year-2");

    QStringList reminderDoneTimes;
    reminderDoneTimes << QStringLiteral("year-3")
                      << QStringLiteral("year+1")
                      << QStringLiteral("year+2");

    QStringList negatedReminderDoneTimes;
    negatedReminderDoneTimes << QStringLiteral("year+3") << datetime.addYears(-1).toString(Qt::ISODate)
                             << datetime.toString(Qt::ISODate);

    timestampForDateTimeString[datetime.toString(Qt::ISODate)] = datetime.toMSecsSinceEpoch();
    timestampForDateTimeString[datetime.addYears(-1).toString(Qt::ISODate)] = datetime.addYears(-1).toMSecsSinceEpoch();

    QStringList contentSearchTerms;
    contentSearchTerms << QStringLiteral("THINK[]!")
                       << QStringLiteral("[do! ")
                       << QStringLiteral(" act]!")
                       << QStringLiteral("*")
                       << QStringLiteral("a*t")
                       << QStringLiteral("*ct");

    QStringList negatedContentSearchTerms;
    negatedContentSearchTerms << QStringLiteral("BIRD[")
                              << QStringLiteral("*is")
                              << QStringLiteral("a")
                              << QStringLiteral("word*")
                              << QStringLiteral("***")
                              << QStringLiteral("w*rd")
                              << QStringLiteral("*ord");

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
            queryString += QStringLiteral("notebook:");
            queryString += QStringLiteral("\"");
            queryString += notebookName;
            queryString += QStringLiteral("\" ");
        }

        if (bits[1]) {
            queryString += QStringLiteral("any: ");
        }

        if (bits[2]) {
            queryString += QStringLiteral("todo:false ");
        }
        else {
            queryString += QStringLiteral("-todo:false ");
        }

        if (bits[3]) {
            queryString += QStringLiteral("todo:true ");
        }
        else {
            queryString += QStringLiteral("-todo:true ");
        }

        if (bits[4]) {
            queryString += QStringLiteral("todo:* ");
        }

        if (bits[5]) {
            queryString += QStringLiteral("-todo:* ");
        }

        if (bits[6]) {
            queryString += QStringLiteral("encryption: ");
        }

        if (bits[7]) {
            queryString += "-encryption: ";
        }

#define ADD_LIST_TO_QUERY_STRING(keyword, list, type, ...) \
    for(int i = 0, size = list.size(); i < size; ++i) { \
        const type & item = list[i]; \
        queryString += QStringLiteral(#keyword ":"); \
        QString itemStr = __VA_ARGS__(item); \
        bool itemContainsSpace = itemStr.contains(QStringLiteral(" ")); \
        if (itemContainsSpace) { \
            queryString += QStringLiteral("\""); \
        } \
        queryString += itemStr; \
        if (itemContainsSpace) { \
            queryString += QStringLiteral("\""); \
        } \
        queryString += QStringLiteral(" "); \
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
        ADD_LIST_TO_QUERY_STRING(reminderOrder, reminderOrders, qint64, QString::number);
        ADD_LIST_TO_QUERY_STRING(-reminderOrder, negatedReminderOrders, qint64, QString::number);
        ADD_LIST_TO_QUERY_STRING(reminderTime, reminderTimes, QString);
        ADD_LIST_TO_QUERY_STRING(-reminderTime, negatedReminderTimes, QString);
        ADD_LIST_TO_QUERY_STRING(reminderDoneTime, reminderDoneTimes, QString);
        ADD_LIST_TO_QUERY_STRING(-reminderDoneTime, negatedReminderDoneTimes, QString);

#undef ADD_LIST_TO_QUERY_STRING

        queryString += QStringLiteral("created:* -created:* ");
        queryString += QStringLiteral("updated:* -updated:* ");
        queryString += QStringLiteral("subjectDate:* -subjectDate:* ");
        queryString += QStringLiteral("latitude:* -latitude:* ");
        queryString += QStringLiteral("longitude:* -longitude:* ");
        queryString += QStringLiteral("altitude:* -altitude:* ");
        queryString += QStringLiteral("reminderOrder:* -reminderOrder:* ");
        queryString += QStringLiteral("reminderTime:* -reminderTime:* ");
        queryString += QStringLiteral("reminderDoneTime:* -reminderDoneTime:* ");

        for(int i = 0, size = contentSearchTerms.size(); i < size; ++i)
        {
            const QString & searchTerm = contentSearchTerms[i];
            bool searchTermContainsSpace = searchTerm.contains(QStringLiteral(" "));
            if (searchTermContainsSpace) {
                queryString += QStringLiteral("\"");
            }
            queryString += searchTerm;
            if (searchTermContainsSpace) {
                queryString += QStringLiteral("\" ");
            }
        }

        for(int i = 0, size = negatedContentSearchTerms.size(); i < size; ++i)
        {
            const QString & negatedSearchTerm = negatedContentSearchTerms[i];
            bool negatedSearchTermContainsSpace = negatedSearchTerm.contains(QStringLiteral(" "));
            if (negatedSearchTermContainsSpace) {
                queryString += QStringLiteral("\"");
            }
            queryString += QStringLiteral("-");
            queryString += negatedSearchTerm;
            if (negatedSearchTermContainsSpace) {
                queryString += QStringLiteral("\" ");
            }
        }

        QNLocalizedString errorMessage;
        bool res = noteSearchQuery.setQueryString(queryString, errorMessage);
        if (!res) {
            error = errorMessage.nonLocalizedString();
            return false;
        }

        const QString & noteSearchQueryString = noteSearchQuery.queryString();
        if (noteSearchQueryString != queryString)
        {
            error = QStringLiteral("NoteSearchQuery has query string not equal to original string: original string: ");
            error += queryString + QStringLiteral("; NoteSearchQuery's internal query string: ");
            error += noteSearchQuery.queryString();
            return false;
        }

        if (!noteSearchQuery.hasAnyTag()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any tag while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyTag()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any tag while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyTitleName()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any title name while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyTitleName()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any title name while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyCreationTimestamp()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any creation timestamp while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyCreationTimestamp()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any creation timestamp while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyModificationTimestamp()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any modification timestamp while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyModificationTimestamp()) {
            error = "NoteSearchQuery doesn't have negated any modification timestamp while it should have one";
            return false;
        }

        if (!noteSearchQuery.hasAnyResourceMimeType()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any resource mime type while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyResourceMimeType()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any resource mime type while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnySubjectDateTimestamp()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any subject date timestamp while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnySubjectDateTimestamp()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any subject date timestamp while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyLatitude()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any latitude while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyLatitude()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any latitude while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyLongitude()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any longitude while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyLongitude()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any longitude while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyAltitude()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any altitude while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyAltitude()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any altitude while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyAuthor()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any author while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyAuthor()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any author while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnySource()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any source while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnySource()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any source while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnySourceApplication()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any source application while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnySourceApplication()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any source application while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyContentClass()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any content class while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyContentClass()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any content class while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyPlaceName()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any place name while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyPlaceName()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any place name while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyApplicationData()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any application data while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyApplicationData()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any application data while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyReminderTime()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any reminder time while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyReminderTime()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any reminder time while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasAnyReminderDoneTime()) {
            error = QStringLiteral("NoteSearchQuery doesn't have any reminder done time while it should have one");
            return false;
        }

        if (!noteSearchQuery.hasNegatedAnyReminderDoneTime()) {
            error = QStringLiteral("NoteSearchQuery doesn't have negated any reminder done time while it should have one");
            return false;
        }

        if (bits[0])
        {
            const QString & noteSearchQueryNotebookModifier = noteSearchQuery.notebookModifier();
            if (noteSearchQueryNotebookModifier != notebookName)
            {
                error = QStringLiteral("NoteSearchQuery's notebook modifier is not equal to original notebook name: original notebook name: ");
                error += notebookName;
                error += QStringLiteral(", NoteSearchQuery's internal notebook modifier: ");
                error += noteSearchQuery.notebookModifier();
                return false;
            }
        }
        else if (!noteSearchQuery.notebookModifier().isEmpty())
        {
            error = QStringLiteral("NoteSearchQuery's notebook modified is not empty while it should be!");
            return false;
        }

        if (bits[2])
        {
            if (!noteSearchQuery.hasUnfinishedToDo()) {
                error = QStringLiteral("NoteSearchQuery doesn't have unfinished todo while it should have one");
                return false;
            }

            if (noteSearchQuery.hasNegatedUnfinishedToDo()) {
                error = QStringLiteral("NoteSearchQUery has negated unfinished todo while it should have non-negated one");
                return false;
            }
        }
        else
        {
            if (!noteSearchQuery.hasNegatedUnfinishedToDo()) {
                error = QStringLiteral("NoteSearchQUery doesn't have negated unfinished todo while it should have one");
                return false;
            }

            if (noteSearchQuery.hasUnfinishedToDo()) {
                error = QStringLiteral("NoteSearchQuery has unfinished todo while it should have negated unfinished todo");
                return false;
            }
        }

        if (bits[3])
        {
            if (!noteSearchQuery.hasFinishedToDo()) {
                error = QStringLiteral("NoteSearchQuery hasn't finished todo while it should have one");
                return false;
            }

            if (noteSearchQuery.hasNegatedFinishedToDo()) {
                error = QStringLiteral("NoteSearchQuery has negated finished todo while it should have non-negated one");
                return false;
            }
        }
        else
        {
            if (!noteSearchQuery.hasNegatedFinishedToDo()) {
                error = QStringLiteral("NoteSearchQuery doesn't have negated finished todo while it should have one");
                return false;
            }

            if (noteSearchQuery.hasFinishedToDo()) {
                error = QStringLiteral("NoteSearchQuery has finished todo while it should have negated finished todo");
                return false;
            }
        }

        if (bits[4]) {
            if (!noteSearchQuery.hasAnyToDo()) {
                error = QStringLiteral("NoteSearchQuery doesn't have \"any\" todo modifier while it should have one");
                return false;
            }
        }
        else {
            if (noteSearchQuery.hasAnyToDo()) {
                error = QStringLiteral("NoteSearchQuery has \"any\" todo modifier while it should not have one");
                return false;
            }
        }

        if (bits[5]) {
            if (!noteSearchQuery.hasNegatedAnyToDo()) {
                error = QStringLiteral("NoteSearchQuery doesn't have negated \"any\" todo modifier while it should have one");
                return false;
            }
        }
        else {
            if (noteSearchQuery.hasNegatedAnyToDo()) {
                error = QStringLiteral("NoteSearchQuery has negated \"any\" todo modifier while it should not have one");
                return false;
            }
        }

        if (bits[6]) {
            if (!noteSearchQuery.hasEncryption()) {
                error = QStringLiteral("NoteSearchQuery doesn't have encryption while it should have one");
                return false;
            }
        }
        else {
            if (noteSearchQuery.hasEncryption()) {
                error = QStringLiteral("NoteSearchQuery has encryption while it should not have one");
                return false;
            }
        }

        if (bits[7]) {
            if (!noteSearchQuery.hasNegatedEncryption()) {
                error = QStringLiteral("NoteSearchQuery doesn't have negated encryption while it should have one");
                return false;
            }
        }
        else {
            if (noteSearchQuery.hasNegatedEncryption()) {
                error = QStringLiteral("NoteSearchQuery has negated encryption while it should not have one");
                return false;
            }
        }

#define CHECK_LIST(list, accessor, ...) \
    auto noteSearchQueryList##list = noteSearchQuery.accessor(); \
    if (noteSearchQueryList##list != list) { \
        error = QStringLiteral("NoteSearchQuery: " #list " doesn't match the one from the original list; original " #list ": "); \
        for(int i = 0, size = list.size(); i < size; ++i) { \
            const auto & item = list[i]; \
            error += __VA_ARGS__(item); \
            error += QStringLiteral("; "); \
        } \
        error += QStringLiteral("; \nNoteSearchQuery's list: "); \
        for(int i = 0, size = noteSearchQueryList##list.size(); i < size; ++i) { \
            const auto & item = noteSearchQueryList##list[i]; \
            error += __VA_ARGS__(item); \
            error += QStringLiteral("; "); \
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
            error = QStringLiteral("Internal error in test: unknown datetime argument "); \
            error += strItem; \
            return false; \
        } \
        \
        if (item != timestampForDateTimeString[strItem]) { \
            error = QStringLiteral("Timestamp from NoteSearchQuery is different from precalculated one for string item \""); \
            error += strItem; \
            error += QStringLiteral("\": precalculated timestamp = "); \
            error += QString::number(timestampForDateTimeString[strItem]); \
            error += QStringLiteral(", timestamp from NoteSearchQuery: "); \
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
    QString contentSearchTermsStr = contentSearchTerms.join(QStringLiteral(" "));
    QString negatedContentSearchTermsStr;
    for(int i = 0, numNegatedContentSearchTerms = negatedContentSearchTerms.size(); i < numNegatedContentSearchTerms; ++i) {
        negatedContentSearchTermsStr += QStringLiteral("-") + negatedContentSearchTerms[i] + QStringLiteral(" ");
    }

    QNLocalizedString errorMessage;
    bool res = noteSearchQuery.setQueryString(contentSearchTermsStr + QStringLiteral(" ") + negatedContentSearchTermsStr, errorMessage);
    if (!res) {
        error = QStringLiteral("Internal error: can't set simple search query string without any search modifiers: ") + errorMessage.nonLocalizedString();
        return false;
    }

    const QStringList & contentSearchTermsFromQuery = noteSearchQuery.contentSearchTerms();
    const int numContentSearchTermsFromQuery = contentSearchTermsFromQuery.size();

    QRegExp asteriskFilter(QStringLiteral("[*]"));

    QStringList filteredContentSearchTerms;
    filteredContentSearchTerms.reserve(numContentSearchTermsFromQuery);
    for(int i = 0, numOriginalContentSearchTerms = contentSearchTerms.size(); i < numOriginalContentSearchTerms; ++i)
    {
        QString filteredSearchTerm = contentSearchTerms[i];

        // Don't accept search terms consisting only of asterisks
        QString filteredSearchTermWithoutAsterisks = filteredSearchTerm;
        filteredSearchTermWithoutAsterisks.remove(asteriskFilter);
        if (filteredSearchTermWithoutAsterisks.isEmpty()) {
            continue;
        }

        filteredContentSearchTerms << filteredSearchTerm.simplified().toLower();
    }

    if (numContentSearchTermsFromQuery != filteredContentSearchTerms.size()) {
        error = QStringLiteral("Internal error: the number of content search terms doesn't match the expected one after parsing the note search query");
        QNWARNING(error << QStringLiteral("; original content search terms: ") << contentSearchTerms.join(QStringLiteral("; "))
                  << QStringLiteral("; filtered content search terms: ") << filteredContentSearchTerms.join(QStringLiteral("; "))
                  << QStringLiteral("; processed search query terms: ") << contentSearchTermsFromQuery.join(QStringLiteral("; ")));
        return false;
    }

    for(int i = 0; i < numContentSearchTermsFromQuery; ++i)
    {
        const QString & contentSearchTermFromQuery = contentSearchTermsFromQuery[i];
        int index = filteredContentSearchTerms.indexOf(contentSearchTermFromQuery);
        if (index < 0) {
            error = QStringLiteral("Internal error: can't find one of original content search terms after parsing the note search query");
            QNWARNING(error << QStringLiteral("; filtered original content search terms: ") << filteredContentSearchTerms.join(QStringLiteral("; "))
                      << QStringLiteral("; parsed content search terms: ") << contentSearchTermsFromQuery.join(QStringLiteral("; ")));
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

        // Don't accept search terms consisting only of asterisks
        QString filteredNegatedSearchTermWithoutAsterisks = filteredNegatedSearchTerm;
        filteredNegatedSearchTermWithoutAsterisks.remove(asteriskFilter);
        if (filteredNegatedSearchTermWithoutAsterisks.isEmpty()) {
            continue;
        }

        filteredNegatedContentSearchTerms << filteredNegatedSearchTerm.simplified().toLower();
    }

    if (numNegatedContentSearchTermsFromQuery != filteredNegatedContentSearchTerms.size()) {
        error = QStringLiteral("Internal error: the number of negated content search terms doesn't match the expected one after parsing the note search query");
        QNWARNING(error << QStringLiteral("; original negated content search terms: ") << negatedContentSearchTerms.join(QStringLiteral(" "))
                  << QStringLiteral("; filtered negated content search terms: ") << filteredNegatedContentSearchTerms.join(QStringLiteral(" "))
                  << QStringLiteral("; processed negated search query terms: ") << negatedContentSearchTermsFromQuery.join(QStringLiteral(" ")));
        return false;
    }

    for(int i = 0; i < numNegatedContentSearchTermsFromQuery; ++i)
    {
        const QString & negatedContentSearchTermFromQuery = negatedContentSearchTermsFromQuery[i];
        int index = filteredNegatedContentSearchTerms.indexOf(negatedContentSearchTermFromQuery);
        if (index < 0) {
            error = QStringLiteral("Internal error: can't find one of original negated content search terms after parsing the note search query");
            QNWARNING(error << QStringLiteral("; filtered original negated content search terms: ") << filteredNegatedContentSearchTerms.join(QStringLiteral(" "))
                      << QStringLiteral("; parsed negated content search terms: ") << negatedContentSearchTermsFromQuery.join(QStringLiteral(" ")));
            return false;
        }
    }

    return true;
}

} // namespace test
} // namespace quentier
