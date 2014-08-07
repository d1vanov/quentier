#include "CoreTester.h"
#include "LocalStorageManagerTests.h"
#include "SavedSearchLocalStorageManagerAsyncTester.h"
#include "LinkedNotebookLocalStorageManagerAsyncTester.h"
#include "TagLocalStorageManagerAsyncTester.h"
#include "UserLocalStorageManagerAsyncTester.h"
#include "NotebookLocalStorageManagerAsyncTester.h"
#include "NoteLocalStorageManagerAsyncTester.h"
#include "ResourceLocalStorageManagerAsyncTester.h"
#include <tools/IQuteNoteException.h>
#include <tools/EventLoopWithExitStatus.h>
#include <client/local_storage/LocalStorageManager.h>
#include <client/local_storage/NoteSearchQuery.h>
#include <client/types/SavedSearch.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/UserWrapper.h>
#include <logging/QuteNoteLogger.h>
#include <QApplication>
#include <QTextStream>
#include <QtTest/QTest>
#include <bitset>

// 10 minutes should be enough
#define MAX_ALLOWED_MILLISECONDS 600000

namespace qute_note {
namespace test {

CoreTester::CoreTester(QObject * parent) :
    QObject(parent)
{}

CoreTester::~CoreTester()
{}

#if QT_VERSION >= 0x050000
void nullMessageHandler(QtMsgType type, const QMessageLogContext &, const QString & message) {
    if (type != QtDebugMsg) {
        QTextStream(stdout) << message << "\n";
    }
}
#else
void nullMessageHandler(QtMsgType type, const char * message) {
    if (type != QtDebugMsg) {
        QTextStream(stdout) << message << "\n";
    }
}
#endif

void CoreTester::initTestCase()
{
#if QT_VERSION >= 0x050000
    qInstallMessageHandler(nullMessageHandler);
#else
    qInstallMsgHandler(nullMessageHandler);
#endif
}

#define CATCH_EXCEPTION() \
    catch(const std::exception & exception) { \
        QFAIL(qPrintable(QString("Caught exception: ") + QString(exception.what()))); \
    }

void CoreTester::resourceRecognitionIndicesTest()
{
    try
    {
        QString recognitionData = "<recoIndex docType=\"unknown\" objType=\"image\" "
                "objID=\"fc83e58282d8059be17debabb69be900\" engineVersion=\"5.5.22.7\" "
                "recoType=\"service\" lang=\"en\" objWidth=\"2398\" objHeight=\"1798\"> "
                "<item x=\"437\" y=\"589\" w=\"1415\" h=\"190\"> "
                "<t w=\"87\">EVER ?</t> "
                "</item> "
                "<item x=\"1850\" y=\"1465\" w=\"14\" h=\"12\"> "
                "<t w=\"11\">et</t> "
                "<t w=\"10\">TQ</t> "
                "</item> "
                "</recoIndex>]]>";

        recognitionData += "<recoIndex docType=\"printed\" objType=\"image\" "
                "objID=\"fc83e58282d8059be17debabb69be900\" engineVersion=\"5.5.22.7\" "
                "recoType=\"service\" lang=\"en\" objWidth=\"2398\" objHeight=\"1798\"> "
                "<item x=\"437\" y=\"589\" w=\"1415\" h=\"190\"> "
                "<t w=\"87\">EVER ?</t> "
                "</item> "
                "<item x=\"1850\" y=\"1465\" w=\"14\" h=\"12\"> "
                "<t w=\"11\">et</t> "
                "<t w=\"10\">TQ</t> "
                "</item> "
                "</recoIndex>]]>";

        recognitionData += "<recoIndex docType=\"handwritten\" objType=\"image\" "
                "objID=\"fc83e58282d8059be17debabb69be900\" engineVersion=\"5.5.22.7\" "
                "recoType=\"service\" lang=\"en\" objWidth=\"2398\" objHeight=\"1798\"> "
                "<item x=\"437\" y=\"589\" w=\"1415\" h=\"190\"> "
                "<t w=\"87\">EVER ?</t> "
                "</item> "
                "<item x=\"1850\" y=\"1465\" w=\"14\" h=\"12\"> "
                "<t w=\"11\">et</t> "
                "<t w=\"10\">TQ</t> "
                "</item> "
                "</recoIndex>]]>";

        ResourceWrapper resource;
        resource.setRecognitionDataBody(recognitionData.toUtf8());

        QStringList recognitionIndices = resource.recognitionIndices();
        QString error = "Recognition index not found";
        QVERIFY2(recognitionIndices.contains("unknown", Qt::CaseInsensitive), qPrintable(error));
        QVERIFY2(recognitionIndices.contains("printed", Qt::CaseInsensitive), qPrintable(error));
        QVERIFY2(recognitionIndices.contains("handwritten", Qt::CaseInsensitive), qPrintable(error));

        QVERIFY2(resource.hasRecognitionIndex("unknown"), qPrintable(error));
        QVERIFY2(resource.hasRecognitionIndex("printed"), qPrintable(error));
        QVERIFY2(resource.hasRecognitionIndex("handwritten"), qPrintable(error));

        error = "Found non-existing recognition index";
        QVERIFY2(!recognitionIndices.contains("picture", Qt::CaseInsensitive), qPrintable(error));
        QVERIFY2(!recognitionIndices.contains("speech", Qt::CaseInsensitive), qPrintable(error));

        QVERIFY2(!resource.hasRecognitionIndex("picture"), qPrintable(error));
        QVERIFY2(!resource.hasRecognitionIndex("speech"), qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::noteContainsToDoTest()
{
    try
    {
        QString noteContent = "<en-note><h1>Hello, world!</h1><en-todo checked = \"true\"/>"
                              "Completed item<en-todo/>Not yet completed item</en-note>";
        Note note;
        note.setContent(noteContent);

        QString error = "Wrong result of Note's containsToDo method";
        QVERIFY2(note.containsCheckedTodo(), qPrintable(error));
        QVERIFY2(note.containsUncheckedTodo(), qPrintable(error));
        QVERIFY2(note.containsTodo(), qPrintable(error));

        noteContent = "<en-note><h1>Hello, world!</h1><en-todo checked = \"true\"/>"
                      "Completed item</en-note>";
        note.setContent(noteContent);

        QVERIFY2(note.containsCheckedTodo(), qPrintable(error));
        QVERIFY2(!note.containsUncheckedTodo(), qPrintable(error));
        QVERIFY2(note.containsTodo(), qPrintable(error));

        noteContent = "<en-note><h1>Hello, world!</h1><en-todo/>Not yet completed item</en-note>";
        note.setContent(noteContent);

        QVERIFY2(!note.containsCheckedTodo(), qPrintable(error));
        QVERIFY2(note.containsUncheckedTodo(), qPrintable(error));
        QVERIFY2(note.containsTodo(), qPrintable(error));

        noteContent = "<en-note><h1>Hello, world!</h1><en-todo checked = \"false\"/>Not yet completed item</en-note>";
        note.setContent(noteContent);

        QVERIFY2(!note.containsCheckedTodo(), qPrintable(error));
        QVERIFY2(note.containsUncheckedTodo(), qPrintable(error));
        QVERIFY2(note.containsTodo(), qPrintable(error));

        noteContent = "<en-note><h1>Hello, world!</h1></en-note>";
        note.setContent(noteContent);

        QVERIFY2(!note.containsCheckedTodo(), qPrintable(error));
        QVERIFY2(!note.containsUncheckedTodo(), qPrintable(error));
        QVERIFY2(!note.containsTodo(), qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::noteContainsEncryptionTest()
{
    try
    {
        QString noteContent = "<en-note><h1>Hello, world!</h1><en-crypt hint = \"the hint\" "
                              "cipher = \"RC2\" length = \"64\">NKLHX5yK1MlpzemJQijAN6C4545s2EODxQ8Bg1r==</en-crypt></en-note>";

        Note note;
        note.setContent(noteContent);

        QString error = "Wrong result of Note's containsEncryption method";
        QVERIFY2(note.containsEncryption(), qPrintable(error));

        QString noteContentWithoutEncryption = "<en-note><h1>Hello, world!</h1></en-note>";
        note.setContent(noteContentWithoutEncryption);

        QVERIFY2(!note.containsEncryption(), qPrintable(error));

        note.clear();
        note.setContent(noteContentWithoutEncryption);

        QVERIFY2(!note.containsEncryption(), qPrintable(error));

        note.setContent(noteContent);
        QVERIFY2(note.containsEncryption(), qPrintable(error));

        note.clear();
        QVERIFY2(!note.containsEncryption(), qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::noteSearchQueryTest()
{
    // Disclaimer: unfortunately, it doesn't seem possible to verify every potential combinations
    // of different keywords and scope modifiers within the search query. Due to that, only a subset
    // of a limited number of combinations of keywords would be tested
    try
    {
        QString notebookName = "fake notebook name";

        QStringList tagNames;
        tagNames << "allowed tag 1" << "allowed tag 2" << "allowed tag 3";

        QStringList negatedTagNames;
        negatedTagNames << "disallowed tag 1" << "disallowed tag 3" << "disallowed tag 3";

        QStringList titleNames;
        titleNames << "allowed title name 1" << "allowed title name 2" << "allowed title name 3";

        QStringList negatedTitleNames;
        negatedTitleNames << "disallowed title name 1" << "disallowed title name 2"
                          << "disallowed title name 3";

        // Adding the relative datetime attributes with their respective timestamps
        // for future comparison with query string parsing results

        QHash<QString,qint64> timestampForDateTimeString;

        QDateTime datetime = QDateTime::currentDateTime();
        datetime.setTime(QTime(0, 0, 0, 1));  // today midnight
        timestampForDateTimeString["day"] = datetime.toMSecsSinceEpoch();

        datetime.addDays(-1);
        timestampForDateTimeString["day-1"] = datetime.toMSecsSinceEpoch();

        datetime.addDays(-1);
        timestampForDateTimeString["day-2"] = datetime.toMSecsSinceEpoch();

        datetime.addDays(-1);
        timestampForDateTimeString["day-3"] = datetime.toMSecsSinceEpoch();

        datetime.addDays(4);
        timestampForDateTimeString["day+1"] = datetime.toMSecsSinceEpoch();

        datetime.addDays(1);
        timestampForDateTimeString["day+2"] = datetime.toMSecsSinceEpoch();

        datetime.addDays(1);
        timestampForDateTimeString["day+3"] = datetime.toMSecsSinceEpoch();

        // TODO: continue from here with week, month, year, add also absolute datetime argument

        qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

        QVector<qint64> creationTimestamps;
        creationTimestamps << (currentTimestamp-9) << (currentTimestamp-8) << (currentTimestamp-7);

        QVector<qint64> negatedCreationTimestamps;
        negatedCreationTimestamps << (currentTimestamp-12) << (currentTimestamp-11) << (currentTimestamp-10);

        QVector<qint64> modificationTimestamps;
        modificationTimestamps << (currentTimestamp-6) << (currentTimestamp-5) << (currentTimestamp-4);

        QVector<qint64> negatedModificationTimestamps;
        negatedModificationTimestamps << (currentTimestamp-3) << (currentTimestamp-2) << (currentTimestamp-1);

        QStringList resourceMimeTypes;
        resourceMimeTypes << "allowed resource mime type 1" << "allowed resource mime type 2" << "allowed resource mime type 3";

        QStringList negatedResourceMimeTypes;
        negatedResourceMimeTypes << "disallowed resource mime type 1" << "disallowed resource mime type 2" << "disallowed resource mime type 3";

        QVector<qint64> subjectDateTimestamps;
        subjectDateTimestamps << (currentTimestamp-15) << (currentTimestamp-14) << (currentTimestamp-13);

        QVector<qint64> negatedSubjectDateTimestamps;
        negatedSubjectDateTimestamps << (currentTimestamp-18) << (currentTimestamp-17) << (currentTimestamp-16);

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
        authors << "author 1" << "author 2" << "author 3";

        QStringList negatedAuthors;
        negatedAuthors << "author 4" << "author 5" << "author 6";

        QStringList sources;
        sources << "web.clip" << "mail.clip" << "mail.smpt";

        QStringList negatedSources;
        negatedSources << "mobile.ios" << "mobile.android" << "mobile.*";

        QStringList sourceApplications;
        sourceApplications << "qutenote" << "nixnote2" << "everpad";

        QStringList negatedSourceApplications;
        negatedSourceApplications << "food.*" << "hello.*" << "skitch.*";

        QStringList contentClasses;
        contentClasses << "content class 1" << "content class 2" << "content class 3";

        QStringList negatedContentClasses;
        negatedContentClasses << "content class 4" << "content class 5" << "content class 6";

        QStringList placeNames;
        placeNames << "home" << "university" << "work";

        QStringList negatedPlaceNames;
        negatedPlaceNames << "bus" << "caffee" << "shower";

        QStringList applicationData;
        applicationData << "application data 1" << "application data 2" << "applicationData 3";

        QStringList negatedApplicationData;
        negatedApplicationData << "negated application data 1" << "negated application data 2"
                               << "negated application data 3";

        QStringList recognitionTypes;
        recognitionTypes << "printed" << "speech" << "handwritten";

        QStringList negatedRecognitionTypes;
        negatedRecognitionTypes << "picture" << "unknown" << "*";

        QVector<qint64> reminderOrders;
        reminderOrders << (currentTimestamp-3) << (currentTimestamp-2) << (currentTimestamp-1);

        QVector<qint64> negatedReminderOrders;
        negatedReminderOrders << (currentTimestamp-6) << (currentTimestamp-5) << (currentTimestamp-4);

        QVector<qint64> reminderTimes;
        reminderTimes << (currentTimestamp+1) << (currentTimestamp+2) << (currentTimestamp+3);

        QVector<qint64> negatedReminderTimes;
        negatedReminderTimes << (currentTimestamp+4) << (currentTimestamp+5) << (currentTimestamp+6);

        QVector<qint64> reminderDoneTimes;
        reminderDoneTimes << (currentTimestamp-3) << (currentTimestamp-2) << (currentTimestamp-1);

        QVector<qint64> negatedReminderDoneTimes;
        negatedReminderDoneTimes << (currentTimestamp-6) << (currentTimestamp-5) << (currentTimestamp-4);

        QStringList contentSearchTerms;
        contentSearchTerms << "think" << "do" << "act";

        QStringList negatedContentSearchTerms;
        negatedContentSearchTerms << "bird" << "is" << "a word";

        NoteSearchQuery noteSearchQuery;

        // Iterating over all combinations of 7 boolean factors with a special meaning
        for(int mask = 0; mask != (1<<7); ++mask)
        {
            std::bitset<9> bits(mask);

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
                queryString += "reminderOrder:* ";
            }

            if (bits[3]) {
                queryString += "todo:false ";
            }
            else {
                queryString += "-todo:false ";
            }

            if (bits[4]) {
                queryString += "todo:true ";
            }
            else {
                queryString += "-todo:true ";
            }

            if (bits[5]) {
                queryString += "todo:* ";
            }

            if (bits[6]) {
                queryString += "encryption: ";
            }

#define ADD_LIST_TO_QUERY_STRING(keyword, list, type, ...) \
    foreach(const type & item, list) { \
        queryString += #keyword ":"; \
        queryString += "\""; \
        queryString += __VA_ARGS__(item); \
        queryString += "\""; \
        queryString += " "; \
    }

            ADD_LIST_TO_QUERY_STRING(tag, tagNames, QString);
            ADD_LIST_TO_QUERY_STRING(-tag, negatedTagNames, QString);
            ADD_LIST_TO_QUERY_STRING(intitle, titleNames, QString);
            ADD_LIST_TO_QUERY_STRING(-intitle, negatedTitleNames, QString);
            // FIXME: timestamps should be translated to proper datetime in ISO 861 profile
            // + I should perhaps also have some kind of test for "day", "week", "month" and "year" wordings
            /*
            ADD_LIST_TO_QUERY_STRING(created, creationTimestamps, qint64, QString::number);
            ADD_LIST_TO_QUERY_STRING(-created, negatedCreationTimestamps, qint64, QString::number);
            ADD_LIST_TO_QUERY_STRING(modified, modificationTimestamps, qint64, QString::number);
            ADD_LIST_TO_QUERY_STRING(-modified, negatedModificationTimestamps, qint64, QString::number);
            */
            ADD_LIST_TO_QUERY_STRING(resource, resourceMimeTypes, QString);
            ADD_LIST_TO_QUERY_STRING(-resource, negatedResourceMimeTypes, QString);
            /*
            ADD_LIST_TO_QUERY_STRING(subjectDate, subjectDateTimestamps, qint64, QString::number);
            ADD_LIST_TO_QUERY_STRING(-subjectDate, negatedSubjectDateTimestamps, qint64, QString::number);
            */
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
            /*
            ADD_LIST_TO_QUERY_STRING(reminderTime, reminderTimes, qint64, QString::number);
            ADD_LIST_TO_QUERY_STRING(-reminderTime, negatedReminderTimes, qint64, QString::number);
            ADD_LIST_TO_QUERY_STRING(reminderDoneTime, reminderDoneTimes, qint64, QString::number);
            ADD_LIST_TO_QUERY_STRING(-reminderDoneTime, negatedReminderDoneTimes, qint64, QString::number);
            */

#undef ADD_LIST_TO_QUERY_STRING

            foreach(const QString & searchTerm, contentSearchTerms) {
                queryString += "\"";
                queryString += searchTerm;
                queryString += "\" ";
            }

            foreach(const QString & negatedSearchTerm, negatedContentSearchTerms) {
                queryString += "\"-";
                queryString += negatedSearchTerm;
                queryString += "\" ";
            }

            QString error;
            bool res = noteSearchQuery.setQueryString(queryString, error);
            QVERIFY2(res == true, qPrintable(error));

            const QString & noteSearchQueryString = noteSearchQuery.queryString();
            if (noteSearchQueryString != queryString)
            {
                error = "NoteSearchQuery has query string not equal to original string: ";
                error += "original string: " + queryString + "; NoteSearchQuery's internal query string: ";
                error += noteSearchQuery.queryString();
                QFAIL(qPrintable(error));
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
                    QFAIL(qPrintable(error));
                }
            }
            else
            {
                QVERIFY2(noteSearchQuery.notebookModifier().isEmpty(), "NoteSearchQuery's notebook modified is not empty while it should be!");
            }

            // TODO: continue from here
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerIndividualSavedSearchTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        SavedSearch search;
        search.setGuid("00000000-0000-0000-c000-000000000046");
        search.setUpdateSequenceNumber(1);
        search.setName("Fake saved search name");
        search.setQuery("Fake saved search query");
        search.setQueryFormat(1);
        search.setIncludeAccount(true);
        search.setIncludeBusinessLinkedNotebooks(false);
        search.setIncludePersonalLinkedNotebooks(true);

        QString error;
        bool res = TestSavedSearchAddFindUpdateExpungeInLocalStorage(search, localStorageManager, error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerIndividualLinkedNotebookTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        LinkedNotebook linkedNotebook;
        linkedNotebook.setGuid("00000000-0000-0000-c000-000000000046");
        linkedNotebook.setUpdateSequenceNumber(1);
        linkedNotebook.setShareName("Fake linked notebook share name");
        linkedNotebook.setUsername("Fake linked notebook username");
        linkedNotebook.setShardId("Fake linked notebook shard id");
        linkedNotebook.setShareKey("Fake linked notebook share key");
        linkedNotebook.setUri("Fake linked notebook uri");
        linkedNotebook.setNoteStoreUrl("Fake linked notebook note store url");
        linkedNotebook.setWebApiUrlPrefix("Fake linked notebook web api url prefix");
        linkedNotebook.setStack("Fake linked notebook stack");
        linkedNotebook.setBusinessId(1);

        QString error;
        bool res = TestLinkedNotebookAddFindUpdateExpungeInLocalStorage(linkedNotebook, localStorageManager, error);
        QVERIFY2(res == true, error.toStdString().c_str());
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerIndividualTagTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        Tag tag;
        tag.setGuid("00000000-0000-0000-c000-000000000046");
        tag.setUpdateSequenceNumber(1);
        tag.setName("Fake tag name");

        QString error;
        bool res = TestTagAddFindUpdateExpungeInLocalStorage(tag, localStorageManager, error);
        QVERIFY2(res == true, error.toStdString().c_str());
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerIndividualResourceTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        QString error;
        bool res = localStorageManager.AddNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        Note note;
        note.setGuid("00000000-0000-0000-c000-000000000046");
        note.setUpdateSequenceNumber(1);
        note.setTitle("Fake note title");
        note.setContent("<en-note><h1>Hello, world</h1></en-note>");
        note.setCreationTimestamp(1);
        note.setModificationTimestamp(1);
        note.setActive(true);
        note.setNotebookGuid(notebook.guid());

        error.clear();
        res = localStorageManager.AddNote(note, notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        ResourceWrapper resource;
        resource.setGuid("00000000-0000-0000-c000-000000000046");
        resource.setUpdateSequenceNumber(1);
        resource.setNoteGuid(note.guid());
        resource.setDataBody("Fake resource data body");
        resource.setDataSize(resource.dataBody().size());
        resource.setDataHash("Fake hash      1");

        resource.setRecognitionDataBody("Fake resource recognition data body");
        resource.setRecognitionDataSize(resource.recognitionDataBody().size());
        resource.setRecognitionDataHash("Fake hash      2");

        resource.setAlternateDataBody("Fake alternate data body");
        resource.setAlternateDataSize(resource.alternateDataBody().size());
        resource.setAlternateDataHash("Fake hash      3");

        resource.setMime("text/plain");
        resource.setWidth(1);
        resource.setHeight(1);

        qevercloud::ResourceAttributes & resourceAttributes = resource.resourceAttributes();

        resourceAttributes.sourceURL = "Fake resource source URL";
        resourceAttributes.timestamp = 1;
        resourceAttributes.latitude = 0.0;
        resourceAttributes.longitude = 0.0;
        resourceAttributes.altitude = 0.0;
        resourceAttributes.cameraMake = "Fake resource camera make";
        resourceAttributes.cameraModel = "Fake resource camera model";

        note.unsetLocalGuid();

        error.clear();
        res = TestResourceAddFindUpdateExpungeInLocalStorage(resource, note, localStorageManager, error);
        QVERIFY2(res == true, error.toStdString().c_str());
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagedIndividualNoteTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        QString error;
        bool res = localStorageManager.AddNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        Note note;
        note.setGuid("00000000-0000-0000-c000-000000000046");
        note.setUpdateSequenceNumber(1);
        note.setTitle("Fake note title");
        note.setContent("<en-note><h1>Hello, world</h1></en-note>");
        note.setCreationTimestamp(1);
        note.setModificationTimestamp(1);
        note.setActive(true);
        note.setNotebookGuid(notebook.guid());

        qevercloud::NoteAttributes & noteAttributes = note.noteAttributes();
        noteAttributes.subjectDate = 1;
        noteAttributes.latitude = 1.0;
        noteAttributes.longitude = 1.0;
        noteAttributes.altitude = 1.0;
        noteAttributes.author = "author";
        noteAttributes.source = "source";
        noteAttributes.sourceURL = "source URL";
        noteAttributes.sourceApplication = "source application";
        noteAttributes.shareDate = 2;

        note.unsetLocalGuid();
        notebook.unsetLocalGuid();

        res = localStorageManager.AddNote(note, notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        Tag tag;
        tag.setGuid("00000000-0000-0000-c000-000000000048");
        tag.setUpdateSequenceNumber(1);
        tag.setName("Fake tag name");

        res = localStorageManager.AddTag(tag, error);
        QVERIFY2(res == true, qPrintable(error));

        res = localStorageManager.LinkTagWithNote(tag, note, error);
        QVERIFY2(res == true, qPrintable(error));

        note.addTagGuid(tag.guid());

        ResourceWrapper resource;
        resource.setGuid("00000000-0000-0000-c000-000000000049");
        resource.setUpdateSequenceNumber(1);
        resource.setFreeAccount(true);
        resource.setNoteGuid(note.guid());
        resource.setDataBody("Fake resource data body");
        resource.setDataSize(resource.dataBody().size());
        resource.setDataHash("Fake hash      1");
        resource.setMime("text/plain");
        resource.setWidth(1);
        resource.setHeight(1);
        resource.setRecognitionDataBody("Fake resource recognition data body");
        resource.setRecognitionDataSize(resource.recognitionDataBody().size());
        resource.setRecognitionDataHash("Fake hash      2");

        qevercloud::ResourceAttributes & resourceAttributes = resource.resourceAttributes();

        resourceAttributes.sourceURL = "Fake resource source URL";
        resourceAttributes.timestamp = 1;
        resourceAttributes.latitude = 0.0;
        resourceAttributes.longitude = 0.0;
        resourceAttributes.altitude = 0.0;
        resourceAttributes.cameraMake = "Fake resource camera make";
        resourceAttributes.cameraModel = "Fake resource camera model";

        note.addResource(resource);

        res = localStorageManager.UpdateNote(note, notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        res = TestNoteFindUpdateDeleteExpungeInLocalStorage(note, notebook, localStorageManager, error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerIndividualNotebookTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);
        notebook.setDefaultNotebook(true);
        notebook.setLastUsed(false);
        notebook.setPublishingUri("Fake publishing uri");
        notebook.setPublishingOrder(1);
        notebook.setPublishingAscending(true);
        notebook.setPublishingPublicDescription("Fake public description");
        notebook.setPublished(true);
        notebook.setStack("Fake notebook stack");
        notebook.setBusinessNotebookDescription("Fake business notebook description");
        notebook.setBusinessNotebookPrivilegeLevel(1);
        notebook.setBusinessNotebookRecommended(true);
        // NotebookRestrictions
        notebook.setCanReadNotes(true);
        notebook.setCanCreateNotes(true);
        notebook.setCanUpdateNotes(true);
        notebook.setCanExpungeNotes(false);
        notebook.setCanShareNotes(true);
        notebook.setCanEmailNotes(true);
        notebook.setCanSendMessageToRecipients(true);
        notebook.setCanUpdateNotebook(true);
        notebook.setCanExpungeNotebook(false);
        notebook.setCanSetDefaultNotebook(true);
        notebook.setCanSetNotebookStack(true);
        notebook.setCanPublishToPublic(true);
        notebook.setCanPublishToBusinessLibrary(false);
        notebook.setCanCreateTags(true);
        notebook.setCanUpdateTags(true);
        notebook.setCanExpungeTags(false);
        notebook.setCanSetParentTag(true);
        notebook.setCanCreateSharedNotebooks(true);
        notebook.setCanCreateSharedNotebooks(true);
        notebook.setCanUpdateNotebook(true);
        notebook.setUpdateWhichSharedNotebookRestrictions(1);
        notebook.setExpungeWhichSharedNotebookRestrictions(1);

        SharedNotebookWrapper sharedNotebook;
        sharedNotebook.setId(1);
        sharedNotebook.setUserId(1);
        sharedNotebook.setNotebookGuid(notebook.guid());
        sharedNotebook.setEmail("Fake shared notebook email");
        sharedNotebook.setCreationTimestamp(1);
        sharedNotebook.setModificationTimestamp(1);
        sharedNotebook.setShareKey("Fake shared notebook share key");
        sharedNotebook.setUsername("Fake shared notebook username");
        sharedNotebook.setPrivilegeLevel(1);
        sharedNotebook.setAllowPreview(true);
        sharedNotebook.setReminderNotifyEmail(true);
        sharedNotebook.setReminderNotifyApp(false);

        notebook.addSharedNotebook(sharedNotebook);

        notebook.unsetLocalGuid();

        QString error;
        bool res = localStorageManager.AddNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        Note note;
        note.setGuid("00000000-0000-0000-c000-000000000049");
        note.setUpdateSequenceNumber(1);
        note.setTitle("Fake note title");
        note.setContent("<en-note><h1>Hello, world</h1></en-note>");
        note.setCreationTimestamp(1);
        note.setModificationTimestamp(1);
        note.setActive(true);
        note.setNotebookGuid(notebook.guid());

        res = localStorageManager.AddNote(note, notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        Tag tag;
        tag.setGuid("00000000-0000-0000-c000-000000000048");
        tag.setUpdateSequenceNumber(1);
        tag.setName("Fake tag name");

        res = localStorageManager.AddTag(tag, error);
        QVERIFY2(res == true, qPrintable(error));

        res = localStorageManager.LinkTagWithNote(tag, note, error);
        QVERIFY2(res == true, qPrintable(error));

        note.addTagGuid(tag.guid());

        res = TestNotebookFindUpdateDeleteExpungeInLocalStorage(notebook, localStorageManager, error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagedIndividualUserTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        UserWrapper user;
        user.setId(1);
        user.setUsername("fake_user_username");
        user.setEmail("fake_user _mail");
        user.setName("fake_user_name");
        user.setTimezone("fake_user_timezone");
        user.setPrivilegeLevel(1);
        user.setCreationTimestamp(2);
        user.setModificationTimestamp(3);
        user.setActive(true);

        qevercloud::UserAttributes userAttributes;
        userAttributes.defaultLocationName = "fake_default_location_name";
        userAttributes.defaultLatitude = 1.0;
        userAttributes.defaultLongitude = 2.0;
        userAttributes.preactivation = false;
        QList<QString> viewedPromotions;
        viewedPromotions.push_back("Viewed promotion 1");
        viewedPromotions.push_back("Viewed promotion 2");
        viewedPromotions.push_back("Viewed promotion 3");
        userAttributes.viewedPromotions = viewedPromotions;
        userAttributes.incomingEmailAddress = "fake_incoming_email_address";
        QList<QString> recentEmailAddresses;
        recentEmailAddresses.push_back("recent_email_address_1");
        recentEmailAddresses.push_back("recent_email_address_2");
        userAttributes.recentMailedAddresses = recentEmailAddresses;
        userAttributes.comments = "Fake comments";
        userAttributes.dateAgreedToTermsOfService = 1;
        userAttributes.maxReferrals = 3;
        userAttributes.refererCode = "fake_referer_code";
        userAttributes.sentEmailDate = 5;
        userAttributes.sentEmailCount = 4;
        userAttributes.dailyEmailLimit = 2;
        userAttributes.emailOptOutDate = 6;
        userAttributes.partnerEmailOptInDate = 7;
        userAttributes.preferredLanguage = "ru";
        userAttributes.preferredCountry = "Russia";
        userAttributes.clipFullPage = true;
        userAttributes.twitterUserName = "fake_twitter_username";
        userAttributes.twitterId = "fake_twitter_id";
        userAttributes.groupName = "fake_group_name";
        userAttributes.recognitionLanguage = "ru";
        userAttributes.referralProof = "I_have_no_idea_what_this_means";
        userAttributes.educationalDiscount = false;
        userAttributes.businessAddress = "fake_business_address";
        userAttributes.hideSponsorBilling = true;
        userAttributes.taxExempt = true;
        userAttributes.useEmailAutoFiling = true;
        userAttributes.reminderEmailConfig = qevercloud::ReminderEmailConfig::DO_NOT_SEND;

        user.setUserAttributes(std::move(userAttributes));

        qevercloud::BusinessUserInfo businessUserInfo;
        businessUserInfo.businessId = 1;
        businessUserInfo.businessName = "Fake business name";
        businessUserInfo.role = qevercloud::BusinessUserRole::NORMAL;
        businessUserInfo.email = "Fake business email";

        user.setBusinessUserInfo(std::move(businessUserInfo));

        qevercloud::Accounting accounting;
        accounting.uploadLimit = 1000;
        accounting.uploadLimitEnd = 9;
        accounting.uploadLimitNextMonth = 1200;
        accounting.premiumServiceStatus = qevercloud::PremiumOrderStatus::PENDING;
        accounting.premiumOrderNumber = "Fake premium order number";
        accounting.premiumCommerceService = "Fake premium commerce service";
        accounting.premiumServiceStart = 8;
        accounting.premiumServiceSKU = "Fake code associated with the purchase";
        accounting.lastSuccessfulCharge = 7;
        accounting.lastFailedCharge = 5;
        accounting.lastFailedChargeReason = "No money, no honey";
        accounting.nextPaymentDue = 12;
        accounting.premiumLockUntil = 11;
        accounting.updated = 10;
        accounting.premiumSubscriptionNumber = "Fake premium subscription number";
        accounting.lastRequestedCharge = 9;
        accounting.currency = "USD";
        accounting.unitPrice = 100;
        accounting.unitDiscount = 2;
        accounting.nextChargeDate = 12;

        user.setAccounting(std::move(accounting));

        qevercloud::PremiumInfo premiumInfo;
        premiumInfo.currentTime = 1;
        premiumInfo.premium = false;
        premiumInfo.premiumRecurring = false;
        premiumInfo.premiumExpirationDate = 11;
        premiumInfo.premiumExtendable = true;
        premiumInfo.premiumPending = true;
        premiumInfo.premiumCancellationPending = false;
        premiumInfo.canPurchaseUploadAllowance = true;
        premiumInfo.sponsoredGroupName = "Fake sponsored group name";
        premiumInfo.premiumUpgradable = true;

        user.setPremiumInfo(std::move(premiumInfo));

        QString error;
        bool res = TestUserAddFindUpdateDeleteExpungeInLocalStorage(user, localStorageManager, error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListAllSavedSearchesTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        QString error;

        int nSearches = 5;
        QList<SavedSearch> searches;
        searches.reserve(nSearches);
        for(int i = 0; i < nSearches; ++i)
        {
            searches << SavedSearch();
            SavedSearch & search = searches.back();

            search.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            search.setUpdateSequenceNumber(i);
            search.setName("SavedSearch #" + QString::number(i));
            search.setQuery("Fake saved search query #" + QString::number(i));
            search.setQueryFormat(1);
            search.setIncludeAccount(true);
            search.setIncludeBusinessLinkedNotebooks(true);
            search.setIncludePersonalLinkedNotebooks(true);

            bool res = localStorageManager.AddSavedSearch(search, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        error.clear();
        QList<SavedSearch> foundSearches = localStorageManager.ListAllSavedSearches(error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        int numFoundSearches = foundSearches.size();
        if (numFoundSearches != nSearches) {
            QFAIL(qPrintable("Error: number of saved searches in the result of LocalStorageManager::ListAllSavedSearches (" +
                             QString::number(numFoundSearches) + ") does not match the original number of added saved searches (" +
                             QString::number(nSearches) + ")"));
        }

        for(size_t i = 0; i < numFoundSearches; ++i)
        {
            const SavedSearch & foundSearch = foundSearches.at(i);
            if (!searches.contains(foundSearch)) {
                QFAIL("One of saved searches from the result of LocalStorageManager::ListAllSavedSearches "
                      "was not found in the list of original searches");
            }
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListAllLinkedNotebooksTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        QString error;

        int nLinkedNotebooks = 5;
        QList<LinkedNotebook> linkedNotebooks;
        linkedNotebooks.reserve(nLinkedNotebooks);
        for(int i = 0; i < nLinkedNotebooks; ++i)
        {
            linkedNotebooks << LinkedNotebook();
            LinkedNotebook & linkedNotebook = linkedNotebooks.back();

            linkedNotebook.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            linkedNotebook.setUpdateSequenceNumber(i);
            linkedNotebook.setShareName("Linked notebook share name #" + QString::number(i));
            linkedNotebook.setUsername("Linked notebook username #" + QString::number(i));
            linkedNotebook.setShardId("Linked notebook shard id #" + QString::number(i));
            linkedNotebook.setShareKey("Linked notebook share key #" + QString::number(i));
            linkedNotebook.setUri("Linked notebook uri #" + QString::number(i));
            linkedNotebook.setNoteStoreUrl("Linked notebook note store url #" + QString::number(i));
            linkedNotebook.setWebApiUrlPrefix("Linked notebook web api url prefix #" + QString::number(i));
            linkedNotebook.setStack("Linked notebook stack #" + QString::number(i));
            linkedNotebook.setBusinessId(1);

            bool res = localStorageManager.AddLinkedNotebook(linkedNotebook, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        error.clear();
        QList<LinkedNotebook> foundLinkedNotebooks = localStorageManager.ListAllLinkedNotebooks(error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        int numFoundLinkedNotebooks = foundLinkedNotebooks.size();
        if (numFoundLinkedNotebooks != nLinkedNotebooks) {
            QFAIL(qPrintable("Error: number of linked notebooks in the result of LocalStorageManager::ListAllLinkedNotebooks (" +
                             QString::number(numFoundLinkedNotebooks) + ") does not match the original number of added linked notebooks (" +
                             QString::number(nLinkedNotebooks) + ")"));
        }

        for(size_t i = 0; i < numFoundLinkedNotebooks; ++i)
        {
            const LinkedNotebook & foundLinkedNotebook = foundLinkedNotebooks.at(i);
            if (!linkedNotebooks.contains(foundLinkedNotebook)) {
                QFAIL("One of linked notebooks from the result of LocalStorageManager::ListAllLinkedNotebooks "
                      "was not found in the list of original linked notebooks");
            }
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListAllTagsTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        QString error;

        size_t nTags = 5;
        std::vector<Tag> tags;
        tags.reserve(nTags);
        for(size_t i = 0; i < nTags; ++i)
        {
            tags.push_back(Tag());
            Tag & tag = tags.back();

            tag.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            tag.setUpdateSequenceNumber(i);
            tag.setName("Tag name #" + QString::number(i));

            if (i != 0) {
                tag.setParentGuid(tags.at(i-1).guid());
            }

            bool res = localStorageManager.AddTag(tag, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        error.clear();
        QList<Tag> foundTags = localStorageManager.ListAllTags(error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        size_t numFoundTags = foundTags.size();
        if (numFoundTags != nTags) {
            QFAIL(qPrintable("Error: number of tags in the result of LocalStorageManager::ListAllTags (" +
                             QString::number(numFoundTags) + ") does not match the original number of added tags (" +
                             QString::number(nTags) + ")"));
        }

        for(size_t i = 0; i < numFoundTags; ++i)
        {
            const Tag & foundTag = foundTags.at(i);
            const auto it = std::find(tags.cbegin(), tags.cend(), foundTag);
            if (it == tags.cend()) {
                QFAIL("One of tags from the result of LocalStorageManager::ListAllTags "
                      "was not found in the list of original tags");
            }
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListAllSharedNotebooksTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000000");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);
        notebook.setDefaultNotebook(true);
        notebook.setPublished(false);
        notebook.setStack("Fake notebook stack");


        int numSharedNotebooks = 5;
        QList<SharedNotebookWrapper> sharedNotebooks;
        sharedNotebooks.reserve(numSharedNotebooks);
        for(int i = 0; i < numSharedNotebooks; ++i)
        {
            sharedNotebooks << SharedNotebookWrapper();
            SharedNotebookWrapper & sharedNotebook = sharedNotebooks.back();

            sharedNotebook.setId(i);
            sharedNotebook.setUserId(i);
            sharedNotebook.setNotebookGuid(notebook.guid());
            sharedNotebook.setEmail("Fake shared notebook email #" + QString::number(i));
            sharedNotebook.setCreationTimestamp(i+1);
            sharedNotebook.setModificationTimestamp(i+1);
            sharedNotebook.setShareKey("Fake shared notebook share key #" + QString::number(i));
            sharedNotebook.setUsername("Fake shared notebook username #" + QString::number(i));
            sharedNotebook.setPrivilegeLevel(1);
            sharedNotebook.setAllowPreview(true);
            sharedNotebook.setReminderNotifyEmail(true);
            sharedNotebook.setReminderNotifyApp(false);

            notebook.addSharedNotebook(sharedNotebook);
        }

        QString error;
        bool res = localStorageManager.AddNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        QList<SharedNotebookWrapper> foundSharedNotebooks = localStorageManager.ListAllSharedNotebooks(error);
        QVERIFY2(!foundSharedNotebooks.isEmpty(), qPrintable(error));

        int numFoundSharedNotebooks = foundSharedNotebooks.size();
        if (numFoundSharedNotebooks != numSharedNotebooks) {
            QFAIL(qPrintable("Error: number of shared notebooks in the result of LocalStorageManager::ListAllSharedNotebooks (" +
                             QString::number(numFoundSharedNotebooks) + ") does not match the original number of added shared notebooks (" +
                             QString::number(numSharedNotebooks) + ")"));
        }

        for(size_t i = 0; i < numFoundSharedNotebooks; ++i)
        {
            const SharedNotebookWrapper & foundSharedNotebook = foundSharedNotebooks.at(i);
            if (!sharedNotebooks.contains(foundSharedNotebook)) {
                QFAIL("One of shared notebooks from the result of LocalStorageManager::ListAllSharedNotebooks "
                      "was not found in the list of original shared notebooks");
            }
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListAllTagsPerNoteTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        QString error;
        bool res = localStorageManager.AddNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        Note note;
        note.setGuid("00000000-0000-0000-c000-000000000046");
        note.setUpdateSequenceNumber(1);
        note.setTitle("Fake note title");
        note.setContent("<en-note><h1>Hello, world</h1></en-note>");
        note.setCreationTimestamp(1);
        note.setModificationTimestamp(1);
        note.setActive(true);
        note.setNotebookGuid(notebook.guid());

        res = localStorageManager.AddNote(note, notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        size_t numTags = 5;
        QList<Tag> tags;
        tags.reserve(numTags);
        for(size_t i = 0; i < numTags; ++i)
        {
            tags << Tag();
            Tag & tag = tags.back();

            tag.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            tag.setUpdateSequenceNumber(i);
            tag.setName("Tag name #" + QString::number(i));

            res = localStorageManager.AddTag(tag, error);
            QVERIFY2(res == true, qPrintable(error));

            res = localStorageManager.LinkTagWithNote(tag, note, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        Tag tagNotLinkedWithNote;
        tagNotLinkedWithNote.setGuid("00000000-0000-0000-c000-000000000045");
        tagNotLinkedWithNote.setUpdateSequenceNumber(9);
        tagNotLinkedWithNote.setName("Tag not linked with note");

        res = localStorageManager.AddTag(tagNotLinkedWithNote, error);
        QVERIFY2(res == true, qPrintable(error));

        error.clear();
        QList<Tag> foundTags = localStorageManager.ListAllTagsPerNote(note, error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        size_t numFoundTags = foundTags.size();
        if (numFoundTags != numTags) {
            QFAIL(qPrintable("Error: number of tags in the result of LocalStorageManager::ListAllTagsPerNote (" +
                             QString::number(numFoundTags) + ") does not match the original number of added tags (" +
                             QString::number(numTags) + ")"));
        }

        for(size_t i = 0; i < numFoundTags; ++i)
        {
            const Tag & foundTag = foundTags.at(i);
            if (!tags.contains(foundTag)) {
                QFAIL("One of tags from the result of LocalStorageManager::ListAllTagsPerNote "
                      "was not found in the list of original tags");
            }
        }

        if (foundTags.contains(tagNotLinkedWithNote)) {
            QFAIL("Found tag not linked with testing note in the result of LocalStorageManager::ListAllTagsPerNote");
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListAllNotesPerNotebookTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        QString error;
        bool res = localStorageManager.AddNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        Notebook notebookNotLinkedWithNotes;
        notebookNotLinkedWithNotes.setGuid("00000000-0000-0000-c000-000000000048");
        notebookNotLinkedWithNotes.setUpdateSequenceNumber(1);
        notebookNotLinkedWithNotes.setName("Fake notebook not linked with notes name name");
        notebookNotLinkedWithNotes.setCreationTimestamp(1);
        notebookNotLinkedWithNotes.setModificationTimestamp(1);

        res = localStorageManager.AddNotebook(notebookNotLinkedWithNotes, error);
        QVERIFY2(res == true, qPrintable(error));

        int numNotes = 5;
        QList<Note> notes;
        notes.reserve(numNotes);
        for(int i = 0; i < numNotes; ++i)
        {
            notes << Note();
            Note & note = notes.back();

            note.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            note.setUpdateSequenceNumber(i+1);
            note.setTitle("Fake note title #" + QString::number(i));
            note.setContent("<en-note><h1>Hello, world #" + QString::number(i) + "</h1></en-note>");
            note.setCreationTimestamp(i+1);
            note.setModificationTimestamp(i+1);
            note.setActive(true);
            note.setNotebookGuid(notebook.guid());

            res = localStorageManager.AddNote(note, notebook, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        error.clear();
        QList<Note> foundNotes = localStorageManager.ListAllNotesPerNotebook(notebook, error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        size_t numFoundNotes = foundNotes.size();
        if (numFoundNotes != numNotes) {
            QFAIL(qPrintable("Error: number of notes in the result of LocalStorageManager::ListAllNotesPerNotebook (" +
                             QString::number(numFoundNotes) + ") does not match the original number of added notes (" +
                             QString::number(numNotes) + ")"));
        }

        for(size_t i = 0; i < numFoundNotes; ++i)
        {
            const Note & foundNote = foundNotes.at(i);
            if (!notes.contains(foundNote)) {
                QFAIL("One of notes from the result of LocalStorageManager::ListAllNotesPerNotebook "
                      "was not found in the list of original notes");
            }
        }

        error.clear();
        foundNotes = localStorageManager.ListAllNotesPerNotebook(notebookNotLinkedWithNotes, error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        if (foundNotes.size() != 0) {
            QFAIL(qPrintable("Found non-zero number of notes in the result of LocalStorageManager::ListAllNotesPerNotebook "
                             "called with guid of notebook not containing any notes (found " +
                             QString::number(foundNotes.size()) + " notes)"));
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListAllNotebooksTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        QString error;

        int numNotebooks = 5;
        QList<Notebook> notebooks;
        for(int i = 0; i < numNotebooks; ++i)
        {
            notebooks << Notebook();
            Notebook & notebook = notebooks.back();

            notebook.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            notebook.setUpdateSequenceNumber(i+1);
            notebook.setName("Fake notebook name #" + QString::number(i+1));
            notebook.setCreationTimestamp(i+1);
            notebook.setModificationTimestamp(i+1);

            notebook.setDefaultNotebook(false);
            notebook.setLastUsed(false);
            notebook.setPublishingUri("Fake publishing uri #" + QString::number(i+1));
            notebook.setPublishingOrder(1);
            notebook.setPublishingAscending(true);
            notebook.setPublishingPublicDescription("Fake public description");
            notebook.setPublished(true);
            notebook.setStack("Fake notebook stack");
            notebook.setBusinessNotebookDescription("Fake business notebook description");
            notebook.setBusinessNotebookPrivilegeLevel(1);
            notebook.setBusinessNotebookRecommended(true);
            // NotebookRestrictions
            notebook.setCanReadNotes(true);
            notebook.setCanCreateNotes(true);
            notebook.setCanUpdateNotes(true);
            notebook.setCanExpungeNotes(false);
            notebook.setCanShareNotes(true);
            notebook.setCanEmailNotes(true);
            notebook.setCanSendMessageToRecipients(true);
            notebook.setCanUpdateNotebook(true);
            notebook.setCanExpungeNotebook(false);
            notebook.setCanSetDefaultNotebook(true);
            notebook.setCanSetNotebookStack(true);
            notebook.setCanPublishToPublic(true);
            notebook.setCanPublishToBusinessLibrary(false);
            notebook.setCanCreateTags(true);
            notebook.setCanUpdateTags(true);
            notebook.setCanExpungeTags(false);
            notebook.setCanSetParentTag(true);
            notebook.setCanCreateSharedNotebooks(true);
            notebook.setUpdateWhichSharedNotebookRestrictions(1);
            notebook.setExpungeWhichSharedNotebookRestrictions(1);

            SharedNotebookWrapper sharedNotebook;
            sharedNotebook.setId(i+1);
            sharedNotebook.setUserId(i+1);
            sharedNotebook.setNotebookGuid(notebook.guid());
            sharedNotebook.setEmail("Fake shared notebook email #" + QString::number(i+1));
            sharedNotebook.setCreationTimestamp(i+1);
            sharedNotebook.setModificationTimestamp(i+1);
            sharedNotebook.setShareKey("Fake shared notebook share key #" + QString::number(i+1));
            sharedNotebook.setUsername("Fake shared notebook username #" + QString::number(i+1));
            sharedNotebook.setPrivilegeLevel(1);
            sharedNotebook.setAllowPreview(true);
            sharedNotebook.setReminderNotifyEmail(true);
            sharedNotebook.setReminderNotifyApp(false);

            notebook.addSharedNotebook(sharedNotebook);

            bool res = localStorageManager.AddNotebook(notebook, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        QList<Notebook> foundNotebooks = localStorageManager.ListAllNotebooks(error);
        QVERIFY2(!foundNotebooks.isEmpty(), qPrintable(error));

        int numFoundNotebooks = foundNotebooks.size();
        if (numFoundNotebooks != numNotebooks) {
            QFAIL(qPrintable("Error: number of notebooks in the result of LocalStorageManager::ListAllNotebooks (" +
                             QString::number(numFoundNotebooks) + ") does not match the original number of added notebooks (" +
                             QString::number(numNotebooks) + ")"));
        }

        for(size_t i = 0; i < numFoundNotebooks; ++i)
        {
            const Notebook & foundNotebook = foundNotebooks.at(i);
            if (!notebooks.contains(foundNotebook)) {
                QFAIL("One of notebooks from the result of LocalStorageManager::ListAllNotebooks "
                      "was not found in the list of original notebooks");
            }
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerAsyncSavedSearchesTest()
{
    int savedSeachAsyncTestsResult = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        SavedSearchLocalStorageManagerAsyncTester savedSearchAsyncTester;

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&savedSearchAsyncTester, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&savedSearchAsyncTester, SIGNAL(failure(QString)), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &savedSearchAsyncTester, SLOT(onInitTestCase()));
        savedSeachAsyncTestsResult = loop.exec();
    }

    if (savedSeachAsyncTestsResult == -1) {
        QFAIL("Internal error: incorrect return status from SavedSearch async tester");
    }
    else if (savedSeachAsyncTestsResult == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in SavedSearch async tester");
    }
    else if (savedSeachAsyncTestsResult == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("SavedSearch async tester failed to finish in time");
    }
}

void CoreTester::localStorageManagerAsyncLinkedNotebooksTest()
{
    int linkedNotebookAsyncTestResult = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        LinkedNotebookLocalStorageManagerAsyncTester linkedNotebookAsyncTester;

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&linkedNotebookAsyncTester, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&linkedNotebookAsyncTester, SIGNAL(failure(QString)), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &linkedNotebookAsyncTester, SLOT(onInitTestCase()));
        linkedNotebookAsyncTestResult = loop.exec();
    }

    if (linkedNotebookAsyncTestResult == -1) {
        QFAIL("Internal error: incorrect return status from LinkedNotebook async tester");
    }
    else if (linkedNotebookAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in LinkedNotebook async tester");
    }
    else if (linkedNotebookAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("LinkedNotebook async tester failed to finish in time");
    }
}

void CoreTester::localStorageManagerAsyncTagsTest()
{
    int tagAsyncTestResult = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        TagLocalStorageManagerAsyncTester tagAsyncTester;

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&tagAsyncTester, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&tagAsyncTester, SIGNAL(failure(QString)), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &tagAsyncTester, SLOT(onInitTestCase()));
        tagAsyncTestResult = loop.exec();
    }

    if (tagAsyncTestResult == -1) {
        QFAIL("Internal error: incorrect return status from Tag async tester");
    }
    else if (tagAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in Tag async tester");
    }
    else if (tagAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Tag async tester failed to finish in time");
    }
}

void CoreTester::localStorageManagerAsyncUsersTest()
{
    int userAsyncTestResult = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        UserLocalStorageManagerAsyncTester userAsyncTester;

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&userAsyncTester, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&userAsyncTester, SIGNAL(failure(QString)), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &userAsyncTester, SLOT(onInitTestCase()));
        userAsyncTestResult = loop.exec();
    }

    if (userAsyncTestResult == -1) {
        QFAIL("Internal error: incorrect return status from UserWrapper async tester");
    }
    else if (userAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in UserWrapper async tester");
    }
    else if (userAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("UserWrapper async tester failed to finish in time");
    }
}

void CoreTester::localStorageManagerAsyncNotebooksTest()
{
    int notebookAsyncTestResult = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        NotebookLocalStorageManagerAsyncTester notebookAsyncTester;

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&notebookAsyncTester, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&notebookAsyncTester, SIGNAL(failure(QString)), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &notebookAsyncTester, SLOT(onInitTestCase()));
        notebookAsyncTestResult = loop.exec();
    }

    if (notebookAsyncTestResult == -1) {
        QFAIL("Internal error: incorrect return status from Notebook async tester");
    }
    else if (notebookAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in Notebook async tester");
    }
    else if (notebookAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Notebook async tester failed to finish in time");
    }
}

void CoreTester::localStorageManagerAsyncNotesTest()
{
    int noteAsyncTestResult = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        NoteLocalStorageManagerAsyncTester noteAsyncTester;

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&noteAsyncTester, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&noteAsyncTester, SIGNAL(failure(QString)), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &noteAsyncTester, SLOT(onInitTestCase()));
        noteAsyncTestResult = loop.exec();
    }

    if (noteAsyncTestResult == -1) {
        QFAIL("Internal error: incorrect return status from Note async tester");
    }
    else if (noteAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in Note async tester");
    }
    else if (noteAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Note async tester failed to finish in time");
    }
}

void CoreTester::localStorageManagerAsyncResourceTest()
{
    int resourceAsyncTestResult = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        ResourceLocalStorageManagerAsyncTester resourceAsyncTester;

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&resourceAsyncTester, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&resourceAsyncTester, SIGNAL(failure(QString)), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &resourceAsyncTester, SLOT(onInitTestCase()));
        resourceAsyncTestResult = loop.exec();
    }

    if (resourceAsyncTestResult == -1) {
        QFAIL("Internal error: incorrect return status from Resource async tester");
    }
    else if (resourceAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in Resource async tester");
    }
    else if (resourceAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Resource async tester failed to finish in time");
    }
}

#undef CATCH_EXCEPTION

}
}
