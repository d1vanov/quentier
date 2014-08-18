#include "LocalStorageManagerNoteSearchQueryTest.h"
#include <client/local_storage/LocalStorageManager.h>
#include <client/local_storage/NoteSearchQuery.h>
#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

bool CheckQueryString(const QString & queryString, const QVector<Note> & notes,
                      const QVector<bool> expectedContainedNotesIndices,
                      const LocalStorageManager & localStorageManager,
                      QString & errorDescription)
{
    NoteSearchQuery noteSearchQuery;
    bool res = noteSearchQuery.setQueryString(queryString, errorDescription);
    if (!res) {
        return false;
    }

    errorDescription.clear();
    NoteList foundNotes = localStorageManager.FindNotesWithSearchQuery(noteSearchQuery, errorDescription);
    if (foundNotes.isEmpty())
    {
        if (!expectedContainedNotesIndices.contains(true)) {
            return true;
        }

        if (errorDescription.isEmpty())
        {
            errorDescription = "Internal error: no notes containing corresponding to note search "
                               "query were found and the error description is empty as well; "
                               "query string: ";
            errorDescription += queryString;
            errorDescription += "; \nNoteSearchQuery: ";
            errorDescription += noteSearchQuery.ToQString();
        }

        return false;
    }

    int numFoundNotes = foundNotes.size();
    QVector<Note> foundNotesVec;
    foundNotesVec.reserve(numFoundNotes);
    for(int i = 0; i < numFoundNotes; ++i) {
        foundNotesVec.push_back(*(foundNotes[i]));
    }

    res = true;
    int numOriginalNotes = notes.size();
    for(int i = 0; i < numOriginalNotes; ++i) {
        res &= (foundNotesVec.contains(notes[i]) == expectedContainedNotesIndices[i]);
    }

    if (!res)
    {
        errorDescription = "Internal error: unexpected result of note search query processing: \n";

        for(int i = 0; i < numFoundNotes; ++i)
        {
            errorDescription += "foundNotes.contains(notes[";
            errorDescription += QString::number(i);
            errorDescription += "]) = ";
            errorDescription += (foundNotesVec.contains(notes[i]) ? "true" : "false");
            errorDescription += "; expected: ";
            errorDescription += (expectedContainedNotesIndices[i] ? "true" : "false");
            errorDescription += "\n";
        }

        errorDescription += "Query string: ";
        errorDescription += queryString;
        errorDescription += "; \nNoteSearchQuery: ";
        errorDescription += noteSearchQuery.ToQString();

        for(int i = 0; i < numFoundNotes; ++i)
        {
            const Note & note = foundNotesVec[i];
            errorDescription += "foundNotes[";
            errorDescription += QString::number(i);
            errorDescription += "]: ";
            errorDescription += note.ToQString();
            errorDescription += "\n";
        }

        return false;
    }

    return true;
}

bool LocalStorageManagerNoteSearchQueryTest(QString & errorDescription)
{
    // 1) =========== Create some notebooks ================

    int numNotebooks = 3;
    QVector<Notebook> notebooks;
    notebooks.reserve(numNotebooks);

    for(int i = 0; i < numNotebooks; ++i)
    {
        notebooks << Notebook();
        Notebook & notebook = notebooks.back();

        notebook.setName(QString("Test notebook #") + QString::number(i));
        notebook.setUpdateSequenceNumber(i);
        notebook.setDefaultNotebook(i == 0 ? true : false);
        notebook.setLastUsed(i == 1 ? true : false);
        notebook.setCreationTimestamp(i);
        notebook.setModificationTimestamp(i+1);
    }

    // 2) =============== Create some tags =================

    int numTags = 9;
    QVector<Tag> tags;
    tags.reserve(numTags);

    for(int i = 0; i < numTags; ++i)
    {
        tags << Tag();
        Tag & tag = tags.back();

        tag.setName(QString("Test tag #") + QString::number(i));
        tag.setUpdateSequenceNumber(i);
    }

    tags[0].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813a");
    tags[1].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813b");
    tags[2].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813c");
    tags[3].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813d");
    tags[4].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813e");
    tags[5].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813f");
    tags[6].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813g");
    tags[7].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813h");
    tags[8].setGuid("8743428c-ef91-4d05-9e7c-4a2e856e813i");

    // 3) ================= Create some resources ==================

    int numResources = 3;
    QVector<ResourceWrapper> resources;
    resources.reserve(numResources);

    for(int i = 0; i < numResources; ++i)
    {
        resources << ResourceWrapper();
        ResourceWrapper & resource = resources.back();

        resource.setUpdateSequenceNumber(i);
    }

    ResourceWrapper & res0 = resources[0];
    res0.setMime("image/gif");
    res0.setDataBody(QByteArray("fake image/gif byte array"));
    res0.setDataSize(res0.dataBody().size());
    res0.setDataHash("Fake_hash______1");

    ResourceWrapper & res1 = resources[1];
    res1.setMime("audio/*");
    res1.setDataBody(QByteArray("fake audio/* byte array"));
    res1.setDataSize(res1.dataBody().size());
    res1.setDataHash("Fake_hash______2");

    ResourceWrapper & res2 = resources[2];
    res2.setMime("application/vnd.evernote.ink");
    res2.setDataBody(QByteArray("fake application/vnd.evernote.ink byte array"));
    res2.setDataSize(res2.dataBody().size());
    res2.setDataHash("Fake_hash______3");
    res2.setRecognitionDataBody(QByteArray("<recoIndex docType=\"handwritten\" objType=\"image\" objID=\"fc83e58282d8059be17debabb69be900\" "
                                           "engineVersion=\"5.5.22.7\" recoType=\"service\" lang=\"en\" objWidth=\"2398\" objHeight=\"1798\"> "
                                           "<item x=\"437\" y=\"589\" w=\"1415\" h=\"190\">"
                                           "<t w=\"87\">EVER ?</t>"
                                           "<t w=\"83\">EVER NOTE</t>"
                                           "<t w=\"82\">EVERNOTE</t>"
                                           "<t w=\"71\">EVER NaTE</t>"
                                           "<t w=\"67\">EVER nine</t>"
                                           "<t w=\"67\">EVER none</t>"
                                           "<t w=\"66\">EVER not</t>"
                                           "<t w=\"62\">over NOTE</t>"
                                           "<t w=\"62\">even NOTE</t>"
                                           "<t w=\"61\">EVER nose</t>"
                                           "<t w=\"50\">EV£RNoTE</t>"
                                           "</item>"
                                           "<item x=\"1850\" y=\"1465\" w=\"14\" h=\"12\">"
                                           "<t w=\"11\">et</t>"
                                           "<t w=\"10\">TQ</t>"
                                           "</item>"
                                           "</recoIndex>]]>"));
    res2.setRecognitionDataSize(res2.recognitionDataBody().size());
    res2.setRecognitionDataHash("Fake_hash______4");

    // 4) ============= Create some ranges for note's properties ==============
    int numTitles = 3;
    QVector<QString> titles;
    titles.reserve(numTitles);
    titles << "Potato" << "Ham" << "Eggs";

    int numContents = 9;
    QVector<QString> contents;
    contents.reserve(numContents);
    contents << "<en-note><h1>The unique identifier of this note. Will be set by the server</h1></en-note>"
             << "<en-note><h1>The XHTML block that makes up the note. This is the canonical form of the note's contents</h1><en-todo checked = \"true\"/></en-note>"
             << "<en-note><h1>The binary MD5 checksum of the UTF-8 encoded content body.</h1></en-note>"
             << "<en-note><h1>The number of Unicode characters in the content of the note.</h1><en-todo/></en-note>"
             << "<en-note><en-todo checked = \"true\"/><h1>The date and time when the note was created in one of the clients.</h1><en-todo checked = \"false\"/></en-note>"
             << "<en-note><h1>If present, the note is considered \"deleted\", and this stores the date and time when the note was deleted</h1></en-note>"
             << "<en-note><h1>If the note is available for normal actions and viewing, this flag will be set to true.</h1><en-crypt hint=\"My Cat\'s Name\">NKLHX5yK1MlpzemJQijAN6C4545s2EODxQ8Bg1r==</en-crypt></en-note>"
             << "<en-note><h1>A number identifying the last transaction to modify the state of this note</h1></en-note>"
             << "<en-note><h1>The list of resources that are embedded within this note.</h1><en-todo checked = \"true\"/><en-crypt hint=\"My Cat\'s Name\">NKLHX5yK1MlpzemJQijAN6C4545s2EODxQ8Bg1r==</en-crypt></en-note>";

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

    int numCreationTimestamps = 9;
    QVector<qint64> creationTimestamps;
    creationTimestamps.reserve(numCreationTimestamps);
    creationTimestamps << timestampForDateTimeString["day-3"]
                       << timestampForDateTimeString["day-2"]
                       << timestampForDateTimeString["day-1"]
                       << timestampForDateTimeString["day"]
                       << timestampForDateTimeString["day+1"]
                       << timestampForDateTimeString["day+2"]
                       << timestampForDateTimeString["day+3"]
                       << timestampForDateTimeString["week-3"]
                       << timestampForDateTimeString["week-2"];

    int numModificationTimestamps = 9;
    QVector<qint64> modificationTimestamps;
    modificationTimestamps.reserve(numModificationTimestamps);
    modificationTimestamps << timestampForDateTimeString["month-3"]
                           << timestampForDateTimeString["month-2"]
                           << timestampForDateTimeString["month-1"]
                           << timestampForDateTimeString["month"]
                           << timestampForDateTimeString["month+1"]
                           << timestampForDateTimeString["month+2"]
                           << timestampForDateTimeString["month+3"]
                           << timestampForDateTimeString["week-1"]
                           << timestampForDateTimeString["week"];

    int numSubjectDateTimestamps = 3;
    QVector<qint64> subjectDateTimestamps;
    subjectDateTimestamps.reserve(numSubjectDateTimestamps);
    subjectDateTimestamps << timestampForDateTimeString["week+1"]
                          << timestampForDateTimeString["week+2"]
                          << timestampForDateTimeString["week+3"];

    int numLatitudes = 9;
    QVector<double> latitudes;
    latitudes.reserve(numLatitudes);
    latitudes << -72.5 << -51.3 << -32.1 << -11.03 << 10.24
              << 32.33 << 54.78 << 72.34 << 91.18;

    int numLongitudes = 9;
    QVector<double> longitudes;
    longitudes.reserve(numLongitudes);
    longitudes << -71.15 << -52.42 << -31.91 << -12.25 << 9.78
               << 34.62 << 56.17 << 73.27 << 92.46;

    int numAltitudes = 9;
    QVector<double> altitudes;
    altitudes.reserve(numAltitudes);
    altitudes << -70.23 << -51.81 << -32.62 << -11.14 << 10.45
              << 31.73 << 52.73 << 71.82 << 91.92;

    int numAuthors = 3;
    QVector<QString> authors;
    authors.reserve(numAuthors);
    authors << "Shakespeare" << "Homer" << "Socrates";

    int numSources = 3;
    QVector<QString> sources;
    sources.reserve(numSources);
    sources << "web.clip" << "mail.clip" << "mobile.android";

    int numSourceApplications = 3;
    QVector<QString> sourceApplications;
    sourceApplications.reserve(numSourceApplications);
    sourceApplications << "food.*" << "hello.*" << "skitch.*";

    int numContentClasses = 3;
    QVector<QString> contentClasses;
    contentClasses.reserve(numContentClasses);
    contentClasses << "evernote.food.meal" << "evernote.hello.*" << "whatever";

    int numPlaceNames = 3;
    QVector<QString> placeNames;
    placeNames.reserve(numPlaceNames);
    placeNames << "home" << "school" << "bus";

    int numApplicationData = 3;
    QVector<QString> applicationData;
    applicationData.reserve(numApplicationData);
    applicationData << "myapp" << "Evernote" << "QuteNote";

    int numReminderOrders = 3;
    QVector<qint64> reminderOrders;
    reminderOrders.reserve(numReminderOrders);
    reminderOrders << 1 << 2 << 3;

    int numReminderTimes = 3;
    QVector<qint64> reminderTimes;
    reminderTimes.reserve(numReminderTimes);
    reminderTimes << timestampForDateTimeString["year-3"]
                  << timestampForDateTimeString["year-2"]
                  << timestampForDateTimeString["year-1"];

    int numReminderDoneTimes = 3;
    QVector<qint64> reminderDoneTimes;
    reminderDoneTimes.reserve(numReminderDoneTimes);
    reminderDoneTimes << timestampForDateTimeString["year"]
                      << timestampForDateTimeString["year+1"]
                      << timestampForDateTimeString["year+2"];

    // 5) ============= Create some notes ================

    int numNotes = 9;
    QVector<Note> notes;
    notes.reserve(numNotes);

    for(int i = 0; i < numNotes; ++i)
    {
        notes << Note();
        Note & note = notes.back();

        note.setTitle(titles[i/numTitles] + QString(" #") + QString::number(i));
        note.setContent(contents[i]);
        note.setCreationTimestamp(creationTimestamps[i]);
        note.setModificationTimestamp(modificationTimestamps[i]);

        qevercloud::NoteAttributes & attributes = note.noteAttributes();

        attributes.subjectDate = subjectDateTimestamps[i/numSubjectDateTimestamps];
        attributes.latitude = latitudes[i];
        attributes.longitude = longitudes[i];
        attributes.altitude = altitudes[i];
        attributes.author = authors[i/numAuthors];
        attributes.source = sources[i/numSources];
        attributes.sourceApplication = sourceApplications[i/numSourceApplications];
        attributes.contentClass = contentClasses[i/numContentClasses];
        attributes.placeName = placeNames[i/numPlaceNames];

        attributes.applicationData = qevercloud::LazyMap();
        attributes.applicationData->keysOnly = QSet<QString>();
        attributes.applicationData->fullMap = QMap<QString,QString>();

        attributes.applicationData->keysOnly->insert(applicationData[i/numApplicationData]);
        attributes.applicationData->fullMap->insert(applicationData[i/numApplicationData],
                QString("Application data value at key ") + applicationData[i/numApplicationData]);

        attributes.reminderOrder = reminderOrders[i/numReminderOrders];
        attributes.reminderTime = reminderTimes[i/numReminderTimes];
        attributes.reminderDoneTime = reminderDoneTimes[i/numReminderDoneTimes];

        note.addTagGuid(tags[i/numTags].guid());
        note.addTagGuid(tags[numTags-1 - i/numTags].guid());

        ResourceWrapper resource = resources[i/numResources];
        resource.setLocalGuid(QUuid::createUuid().toString());
        note.addResource(resource);
    }

    QMap<int,int> notebookIndexForNoteIndex;
    for(int i = 0; i < numNotes; ++i) {
        notebookIndexForNoteIndex[i] = i/numNotebooks;
    }

    // 6) =========== Create local storage, add created notebooks, tags and notes there ===========

    const bool startFromScratch = true;
    LocalStorageManager localStorageManager("LocalStorageManagerNoteSearchQueryTestFakeUser",
                                            0, startFromScratch);

    for(int i = 0; i < numNotebooks; ++i)
    {
        bool res = localStorageManager.AddNotebook(notebooks[i], errorDescription);
        if (!res) {
            return false;
        }
    }

    for(int i = 0; i < numTags; ++i)
    {
        bool res = localStorageManager.AddTag(tags[i], errorDescription);
        if (!res) {
            return false;
        }
    }

    for(int i = 0; i < numNotes; ++i)
    {
        bool res = localStorageManager.AddNote(notes[i], notebooks[notebookIndexForNoteIndex[i]], errorDescription);
        if (!res) {
            return false;
        }
    }

    // 7) =========== Create and execute some note search queries, verify they are consistent

    // 7.1) ToDo queries

    // 7.1.1) Finished todo query
    QString queryString = "todo:true";
    QVector<bool> expectedContainedNotesIndices(numNotes, false);
    expectedContainedNotesIndices[1] = true;
    expectedContainedNotesIndices[4] = true;
    expectedContainedNotesIndices[8] = true;

    bool res = CheckQueryString(queryString, notes, expectedContainedNotesIndices,
                                localStorageManager, errorDescription);
    if (!res) {
        return false;
    }

    return true;
}

} // namespace test
} // namespace qute_note
