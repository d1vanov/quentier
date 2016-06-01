#include "CoreTester.h"
#include "LocalStorageManagerTests.h"
#include "SavedSearchLocalStorageManagerAsyncTester.h"
#include "LinkedNotebookLocalStorageManagerAsyncTester.h"
#include "TagLocalStorageManagerAsyncTester.h"
#include "UserLocalStorageManagerAsyncTester.h"
#include "NotebookLocalStorageManagerAsyncTester.h"
#include "NoteLocalStorageManagerAsyncTester.h"
#include "NoteSearchQueryTest.h"
#include "ResourceLocalStorageManagerAsyncTester.h"
#include "LocalStorageManagerNoteSearchQueryTest.h"
#include "LocalStorageCacheAsyncTester.h"
#include "EncryptionManagerTests.h"
#include "ENMLConverterTests.h"
#include "ResourceRecognitionIndicesParsingTest.h"
#include <qute_note/exception/IQuteNoteException.h>
#include <qute_note/utility/EventLoopWithExitStatus.h>
#include <qute_note/local_storage/LocalStorageManager.h>
#include <qute_note/types/SavedSearch.h>
#include <qute_note/types/LinkedNotebook.h>
#include <qute_note/types/Tag.h>
#include <qute_note/types/ResourceWrapper.h>
#include <qute_note/types/Notebook.h>
#include <qute_note/types/SharedNotebookWrapper.h>
#include <qute_note/types/UserWrapper.h>
#include <qute_note/types/RegisterMetatypes.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/SysInfo.h>
#include <QApplication>
#include <QTextStream>
#include <QtTest/QTest>

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
    registerMetatypes();

#if QT_VERSION >= 0x050000
    qInstallMessageHandler(nullMessageHandler);
#else
    qInstallMsgHandler(nullMessageHandler);
#endif
}

#define CATCH_EXCEPTION() \
    catch(const std::exception & exception) { \
        QFAIL(qPrintable(QString("Caught exception: ") + QString(exception.what()) + \
                         QString(", backtrace: ") + SysInfo::GetSingleton().GetStackTrace())); \
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

void CoreTester::encryptDecryptNoteTest()
{
    try
    {
        QString error;
        bool res = encryptDecryptTest(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::decryptNoteAesTest()
{
    try
    {
        QString error;
        bool res = decryptAesTest(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::decryptNoteRc2Test()
{
    try
    {
        QString error;
        bool res = decryptRc2Test(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterSimpleTest()
{
    try
    {
        QString error;
        bool res = convertSimpleNoteToHtmlAndBack(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterToDoTest()
{
    try
    {
        QString error;
        bool res = convertNoteWithToDoTagsToHtmlAndBack(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterEnCryptTest()
{
    try
    {
        QString error;
        bool res = convertNoteWithEncryptionToHtmlAndBack(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterEnCryptWithModifiedDecryptedTextTest()
{
    try
    {
        QString error;
        bool res = convertHtmlWithModifiedDecryptedTextToEnml(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterEnMediaTest()
{
    try
    {
        QString error;
        bool res = convertNoteWithResourcesToHtmlAndBack(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterComplexTest()
{
    try
    {
        QString error;
        bool res = convertComplexNoteToHtmlAndBack(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterComplexTest2()
{
    try
    {
        QString error;
        bool res = convertComplexNote2ToHtmlAndBack(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterComplexTest3()
{
    try
    {
        QString error;
        bool res = convertComplexNote3ToHtmlAndBack(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterComplexTest4()
{
    try
    {
        QString error;
        bool res = convertComplexNote4ToHtmlAndBack(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterHtmlWithTableHelperTags()
{
    try
    {
        QString error;
        bool res = convertHtmlWithTableHelperTagsToEnml(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::enmlConverterHtmlWithTableAndHilitorHelperTags()
{
    try
    {
        QString error;
        bool res = convertHtmlWithTableAndHilitorHelperTagsToEnml(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::resourceRecognitionIndicesParsingTest()
{
    try
    {
        QString error;
        bool res = parseResourceRecognitionIndicesAndItemsTest(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::noteSearchQueryTest()
{
    try
    {
        QString error;
        bool res = NoteSearchQueryTest(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerNoteSearchQueryTest()
{
    try
    {
        QString error;
        bool res = LocalStorageManagerNoteSearchQueryTest(error);
        if (!res) {
            QFAIL(qPrintable(error));
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerIndividualSavedSearchTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

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
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

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
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerIndividualTagTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        LinkedNotebook linkedNotebook;
        linkedNotebook.setGuid("00000000-0000-0000-c000-000000000001");
        linkedNotebook.setUpdateSequenceNumber(1);
        linkedNotebook.setShareName("Linked notebook share name");
        linkedNotebook.setUsername("Linked notebook username");
        linkedNotebook.setShardId("Linked notebook shard id");
        linkedNotebook.setShareKey("Linked notebook share key");
        linkedNotebook.setUri("Linked notebook uri");
        linkedNotebook.setNoteStoreUrl("Linked notebook note store url");
        linkedNotebook.setWebApiUrlPrefix("Linked notebook web api url prefix");
        linkedNotebook.setStack("Linked notebook stack");
        linkedNotebook.setBusinessId(1);

        Tag tag;
        tag.setGuid("00000000-0000-0000-c000-000000000046");
        tag.setLinkedNotebookGuid(linkedNotebook.guid());
        tag.setUpdateSequenceNumber(1);
        tag.setName("Fake tag name");

        QString error;
        bool res = localStorageManager.addLinkedNotebook(linkedNotebook, error);
        QVERIFY2(res == true, qPrintable(error));

        error.clear();
        res = TestTagAddFindUpdateExpungeInLocalStorage(tag, localStorageManager, error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerIndividualResourceTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        QString error;
        bool res = localStorageManager.addNotebook(notebook, error);
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
        note.setNotebookLocalUid(notebook.localUid());

        error.clear();
        res = localStorageManager.addNote(note, error);
        QVERIFY2(res == true, qPrintable(error));

        ResourceWrapper resource;
        resource.setGuid("00000000-0000-0000-c000-000000000046");
        resource.setUpdateSequenceNumber(1);
        resource.setNoteGuid(note.guid());
        resource.setDataBody("Fake resource data body");
        resource.setDataSize(resource.dataBody().size());
        resource.setDataHash("Fake hash      1");

        resource.setRecognitionDataBody("<recoIndex docType=\"handwritten\" objType=\"image\" objID=\"fc83e58282d8059be17debabb69be900\" "
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
                                        "</recoIndex>");
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

        note.unsetLocalUid();

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
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        QString error;
        bool res = localStorageManager.addNotebook(notebook, error);
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
        note.setNotebookLocalUid(notebook.localUid());

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

        note.unsetLocalUid();

        res = localStorageManager.addNote(note, error);
        QVERIFY2(res == true, qPrintable(error));

        Tag tag;
        tag.setGuid("00000000-0000-0000-c000-000000000048");
        tag.setUpdateSequenceNumber(1);
        tag.setName("Fake tag name");

        res = localStorageManager.addTag(tag, error);
        QVERIFY2(res == true, qPrintable(error));

        res = localStorageManager.linkTagWithNote(tag, note, error);
        QVERIFY2(res == true, qPrintable(error));

        note.addTagGuid(tag.guid());
        note.addTagLocalUid(tag.localUid());

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
        resource.setRecognitionDataBody("<recoIndex docType=\"handwritten\" objType=\"image\" objID=\"fc83e58282d8059be17debabb69be900\" "
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
                                        "</recoIndex>");
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

        res = localStorageManager.updateNote(note, /* update resources = */ true,
                                             /* update tags = */ true, error);
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
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        LinkedNotebook linkedNotebook;
        linkedNotebook.setGuid("00000000-0000-0000-c000-000000000001");
        linkedNotebook.setUpdateSequenceNumber(1);
        linkedNotebook.setShareName("Linked notebook share name");
        linkedNotebook.setUsername("Linked notebook username");
        linkedNotebook.setShardId("Linked notebook shard id");
        linkedNotebook.setShareKey("Linked notebook share key");
        linkedNotebook.setUri("Linked notebook uri");
        linkedNotebook.setNoteStoreUrl("Linked notebook note store url");
        linkedNotebook.setWebApiUrlPrefix("Linked notebook web api url prefix");
        linkedNotebook.setStack("Linked notebook stack");
        linkedNotebook.setBusinessId(1);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setLinkedNotebookGuid(linkedNotebook.guid());
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

        QString error;
        bool res = localStorageManager.addLinkedNotebook(linkedNotebook, error);
        QVERIFY2(res == true, qPrintable(error));

        error.clear();
        res = localStorageManager.addNotebook(notebook, error);
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
        note.setNotebookLocalUid(notebook.localUid());

        res = localStorageManager.addNote(note, error);
        QVERIFY2(res == true, qPrintable(error));

        Tag tag;
        tag.setGuid("00000000-0000-0000-c000-000000000048");
        tag.setUpdateSequenceNumber(1);
        tag.setName("Fake tag name");

        res = localStorageManager.addTag(tag, error);
        QVERIFY2(res == true, qPrintable(error));

        res = localStorageManager.linkTagWithNote(tag, note, error);
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
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

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

void CoreTester::localStorageManagerSequentialUpdatesTest()
{
    try
    {
        QString error;
        bool res = TestSequentialUpdatesInLocalStorage(error);
        QVERIFY2(res == true, qPrintable(error));
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListSavedSearchesTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        QString error;

        int nSearches = 5;
        QList<SavedSearch> searches;
        searches.reserve(nSearches);
        for(int i = 0; i < nSearches; ++i)
        {
            searches << SavedSearch();
            SavedSearch & search = searches.back();

            if (i > 1) {
                search.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            }

            search.setUpdateSequenceNumber(i);
            search.setName("SavedSearch #" + QString::number(i));
            search.setQuery("Fake saved search query #" + QString::number(i));
            search.setQueryFormat(1);
            search.setIncludeAccount(true);
            search.setIncludeBusinessLinkedNotebooks(true);
            search.setIncludePersonalLinkedNotebooks(true);

            if (i > 2) {
                search.setDirty(true);
            }
            else {
                search.setDirty(false);
            }

            if (i < 3) {
                search.setLocal(true);
            }
            else {
                search.setLocal(false);
            }

            if ((i == 0) || (i == 4)) {
                search.setShortcut(true);
            }
            else {
                search.setShortcut(false);
            }

            bool res = localStorageManager.addSavedSearch(search, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        // 1) Test method listing all saved searches

        error.clear();
        QList<SavedSearch> foundSearches = localStorageManager.listAllSavedSearches(error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        int numFoundSearches = foundSearches.size();
        if (numFoundSearches != nSearches) {
            QFAIL(qPrintable("Error: number of saved searches in the result of LocalStorageManager::ListAllSavedSearches (" +
                             QString::number(numFoundSearches) + ") does not match the original number of added saved searches (" +
                             QString::number(nSearches) + ")"));
        }

        for(int i = 0; i < numFoundSearches; ++i)
        {
            const SavedSearch & foundSearch = foundSearches.at(i);
            if (!searches.contains(foundSearch)) {
                QFAIL("One of saved searches from the result of LocalStorageManager::ListAllSavedSearches "
                      "was not found in the list of original searches");
            }
        }

#define CHECK_LIST_SAVED_SEARCHES_BY_FLAG(flag, flag_name, true_condition, false_condition) \
        error.clear(); \
        foundSearches = localStorageManager.listSavedSearches(flag, error); \
        QVERIFY2(error.isEmpty(), qPrintable(error)); \
        \
        for(int i = 0; i < nSearches; ++i) \
        { \
            const SavedSearch & search = searches.at(i); \
            bool res = foundSearches.contains(search); \
            if ((true_condition) && !res) { \
                QNWARNING("Not found saved search: " << search); \
                QFAIL("One of " flag_name " SavedSearches was not found by LocalStorageManager::ListSavedSearches"); \
            } \
            else if ((false_condition) && res) { \
                QNWARNING("Found irrelevant saved search: " << search); \
                QFAIL("LocalStorageManager::ListSavedSearches with flag " flag_name " returned incorrect saved search"); \
            } \
        }

        // 2) Test method listing only dirty saved searches
        CHECK_LIST_SAVED_SEARCHES_BY_FLAG(LocalStorageManager::ListDirty, "dirty", i > 2, i <= 2);

        // 3) Test method listing only local saved searches
        CHECK_LIST_SAVED_SEARCHES_BY_FLAG(LocalStorageManager::ListLocal, "local", i < 3, i >= 3);

        // 4) Test method listing only saved searches without guid
        CHECK_LIST_SAVED_SEARCHES_BY_FLAG(LocalStorageManager::ListElementsWithoutGuid, "guidless", i <= 1, i > 1);

        // 5) Test method listing only saved searches with shortcut
        CHECK_LIST_SAVED_SEARCHES_BY_FLAG(LocalStorageManager::ListElementsWithShortcuts, "having shortcut", (i == 0) || (i == 4), (i != 0) && (i != 4));

        // 6) Test method listing dirty saved searches with guid and with shortcut
        CHECK_LIST_SAVED_SEARCHES_BY_FLAG(LocalStorageManager::ListDirty | LocalStorageManager::ListElementsWithGuid | LocalStorageManager::ListElementsWithShortcuts,
                                          "dirty, having guid, having shortcut", i == 4, i != 4);

        // 7) Test method listing local saved searches having shortcut
        CHECK_LIST_SAVED_SEARCHES_BY_FLAG(LocalStorageManager::ListLocal | LocalStorageManager::ListElementsWithShortcuts,
                                          "local, having shortcut", i == 0, i != 0);

        // 8) Test method listing saved searches with guid set also specifying limit, offset and order
        size_t limit = 2;
        size_t offset = 1;
        LocalStorageManager::ListSavedSearchesOrder::type order = LocalStorageManager::ListSavedSearchesOrder::ByUpdateSequenceNumber;

        error.clear();
        foundSearches = localStorageManager.listSavedSearches(LocalStorageManager::ListElementsWithGuid, error, limit, offset, order);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        if (foundSearches.size() != static_cast<int>(limit)) {
            QFAIL(qPrintable("Unexpected number of found saved searches not corresponding to the specified limit: limit = " +
                             QString::number(limit) + ", number of searches found is " + QString::number(foundSearches.size())));
        }

        const SavedSearch & firstSearch = foundSearches[0];
        const SavedSearch & secondSearch = foundSearches[1];

        if (!firstSearch.hasUpdateSequenceNumber() || !secondSearch.hasUpdateSequenceNumber()) {
            QFAIL(qPrintable("One of found saved searches doesn't have the update sequence number which is unexpected: first search: " +
                             firstSearch.toString() + "\nSecond search: " + secondSearch.toString()));
        }

        if (firstSearch.updateSequenceNumber() != 3) {
            QFAIL(qPrintable("First saved search was expected to have update sequence number of 3, instead it is " +
                             QString::number(firstSearch.updateSequenceNumber())));
        }

        if (secondSearch.updateSequenceNumber() != 4) {
            QFAIL(qPrintable("Second saved search was expected to have update sequence number of 4, instead it is " +
                             QString::number(secondSearch.updateSequenceNumber())));
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListLinkedNotebooksTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

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

            if (i > 2) {
                linkedNotebook.setDirty(true);
            }
            else {
                linkedNotebook.setDirty(false);
            }

            bool res = localStorageManager.addLinkedNotebook(linkedNotebook, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        // 1) Test method listing all linked notebooks

        error.clear();
        QList<LinkedNotebook> foundLinkedNotebooks = localStorageManager.listAllLinkedNotebooks(error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        int numFoundLinkedNotebooks = foundLinkedNotebooks.size();
        if (numFoundLinkedNotebooks != nLinkedNotebooks) {
            QFAIL(qPrintable("Error: number of linked notebooks in the result of LocalStorageManager::ListAllLinkedNotebooks (" +
                             QString::number(numFoundLinkedNotebooks) + ") does not match the original number of added linked notebooks (" +
                             QString::number(nLinkedNotebooks) + ")"));
        }

        for(int i = 0; i < numFoundLinkedNotebooks; ++i)
        {
            const LinkedNotebook & foundLinkedNotebook = foundLinkedNotebooks.at(i);
            if (!linkedNotebooks.contains(foundLinkedNotebook)) {
                QFAIL("One of linked notebooks from the result of LocalStorageManager::ListAllLinkedNotebooks "
                      "was not found in the list of original linked notebooks");
            }
        }

        // 2) Test method listing only dirty linked notebooks
        error.clear();
        foundLinkedNotebooks = localStorageManager.listLinkedNotebooks(LocalStorageManager::ListDirty, error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        for(int i = 0; i < nLinkedNotebooks; ++i)
        {
            const LinkedNotebook & linkedNotebook = linkedNotebooks.at(i);
            bool res = foundLinkedNotebooks.contains(linkedNotebook);
            if ((i > 2) && !res) {
                QNWARNING("Not found linked notebook: " << linkedNotebook);
                QFAIL("One of dirty linked notebooks was not found by LocalStorageManager::ListLinkedNotebooks");
            }
            else if ((i <= 2) && res) {
                QNWARNING("Found irrelevant linked notebook: " << linkedNotebook);
                QFAIL("LocalStorageManager::ListLinkedNotebooks with flag ListDirty returned incorrect linked notebook");
            }
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListTagsTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        QString error;

        int nTags = 5;
        QList<Tag> tags;
        tags.reserve(nTags);
        for(int i = 0; i < nTags; ++i)
        {
            tags.push_back(Tag());
            Tag & tag = tags.back();

            if (i > 1) {
                tag.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            }

            tag.setUpdateSequenceNumber(i);
            tag.setName("Tag name #" + QString::number(i));

            if (i > 2) {
                tag.setParentGuid(tags.at(i-1).guid());
            }

            if (i > 2) {
                tag.setDirty(true);
            }
            else {
                tag.setDirty(false);
            }

            if (i < 3) {
                tag.setLocal(true);
            }
            else {
                tag.setLocal(false);
            }

            if ((i == 0) || (i == 4)) {
                tag.setShortcut(true);
            }
            else {
                tag.setShortcut(false);
            }

            bool res = localStorageManager.addTag(tag, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        // 1) Test method listing all tags

        error.clear();
        QList<Tag> foundTags = localStorageManager.listAllTags(error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        int numFoundTags = foundTags.size();
        if (numFoundTags != nTags) {
            QFAIL(qPrintable("Error: number of tags in the result of LocalStorageManager::ListAllTags (" +
                             QString::number(numFoundTags) + ") does not match the original number of added tags (" +
                             QString::number(nTags) + ")"));
        }

        for(int i = 0; i < numFoundTags; ++i)
        {
            const Tag & foundTag = foundTags.at(i);
            if (!tags.contains(foundTag)) {
                QFAIL("One of tags from the result of LocalStorageManager::ListAllTags "
                      "was not found in the list of original tags");
            }
        }

#define CHECK_LIST_TAGS_BY_FLAG(flag, flag_name, true_condition, false_condition) \
        error.clear(); \
        foundTags = localStorageManager.listTags(flag, error); \
        QVERIFY2(error.isEmpty(), qPrintable(error)); \
        \
        for(int i = 0; i < nTags; ++i) \
        { \
            const Tag & tag = tags.at(i); \
            bool res = foundTags.contains(tag); \
            if ((true_condition) && !res) { \
                QNWARNING("Not found tag: " << tag); \
                QFAIL("One of " flag_name " Tags was not found by LocalStorageManager::ListTags"); \
            } \
            else if ((false_condition) && res) { \
                QNWARNING("Found irrelevant tag: " << tag); \
                QFAIL("LocalStorageManager::ListTags with flag " flag_name " returned incorrect tag"); \
            } \
        }

        // 2) Test method listing only dirty tags
        CHECK_LIST_TAGS_BY_FLAG(LocalStorageManager::ListDirty, "dirty", i > 2, i <= 2);

        // 3) Test method listing only local tags
        CHECK_LIST_TAGS_BY_FLAG(LocalStorageManager::ListLocal, "local", i < 3, i >= 3);

        // 4) Test method listing only tags without guid
        CHECK_LIST_TAGS_BY_FLAG(LocalStorageManager::ListElementsWithoutGuid, "guidless", i <= 1, i > 1);

        // 5) Test method listing only tags with shortcut
        CHECK_LIST_TAGS_BY_FLAG(LocalStorageManager::ListElementsWithShortcuts, "having shortcut", (i == 0) || (i == 4), (i != 0) && (i != 4));

        // 6) Test method listing dirty tags with guid and with shortcut
        CHECK_LIST_TAGS_BY_FLAG(LocalStorageManager::ListDirty | LocalStorageManager::ListElementsWithGuid | LocalStorageManager::ListElementsWithShortcuts,
                                "dirty, having guid, having shortcut", i == 4, i != 4);

        // 7) Test method listing local tags having shortcut
        CHECK_LIST_TAGS_BY_FLAG(LocalStorageManager::ListLocal | LocalStorageManager::ListElementsWithShortcuts,
                                "local, having shortcut", i == 0, i != 0);
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListAllSharedNotebooksTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

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
        bool res = localStorageManager.addNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        QList<SharedNotebookWrapper> foundSharedNotebooks = localStorageManager.listAllSharedNotebooks(error);
        QVERIFY2(!foundSharedNotebooks.isEmpty(), qPrintable(error));

        int numFoundSharedNotebooks = foundSharedNotebooks.size();
        if (numFoundSharedNotebooks != numSharedNotebooks) {
            QFAIL(qPrintable("Error: number of shared notebooks in the result of LocalStorageManager::ListAllSharedNotebooks (" +
                             QString::number(numFoundSharedNotebooks) + ") does not match the original number of added shared notebooks (" +
                             QString::number(numSharedNotebooks) + ")"));
        }

        for(int i = 0; i < numFoundSharedNotebooks; ++i)
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
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        QString error;
        bool res = localStorageManager.addNotebook(notebook, error);
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
        note.setNotebookLocalUid(notebook.localUid());

        res = localStorageManager.addNote(note, error);
        QVERIFY2(res == true, qPrintable(error));

        int numTags = 5;
        QList<Tag> tags;
        tags.reserve(numTags);
        for(int i = 0; i < numTags; ++i)
        {
            tags << Tag();
            Tag & tag = tags.back();

            tag.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            tag.setUpdateSequenceNumber(i);
            tag.setName("Tag name #" + QString::number(i));

            if (i > 1) {
                tag.setDirty(true);
            }
            else {
                tag.setDirty(false);
            }

            res = localStorageManager.addTag(tag, error);
            QVERIFY2(res == true, qPrintable(error));

            res = localStorageManager.linkTagWithNote(tag, note, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        Tag tagNotLinkedWithNote;
        tagNotLinkedWithNote.setGuid("00000000-0000-0000-c000-000000000045");
        tagNotLinkedWithNote.setUpdateSequenceNumber(9);
        tagNotLinkedWithNote.setName("Tag not linked with note");

        res = localStorageManager.addTag(tagNotLinkedWithNote, error);
        QVERIFY2(res == true, qPrintable(error));

        // 1) Test method listing all tags per given note without any additional conditions

        error.clear();
        QList<Tag> foundTags = localStorageManager.listAllTagsPerNote(note, error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        int numFoundTags = foundTags.size();
        if (numFoundTags != numTags) {
            QFAIL(qPrintable("Error: number of tags in the result of LocalStorageManager::ListAllTagsPerNote (" +
                             QString::number(numFoundTags) + ") does not match the original number of added tags (" +
                             QString::number(numTags) + ")"));
        }

        for(int i = 0; i < numFoundTags; ++i)
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

        // 2) Test method listing all tags per note consideting only dirty ones + with limit, offset,
        // specific order and order direction

        error.clear();
        const size_t limit = 2;
        const size_t offset = 1;
        const LocalStorageManager::ListObjectsOptions flag = LocalStorageManager::ListDirty;
        const LocalStorageManager::ListTagsOrder::type order = LocalStorageManager::ListTagsOrder::ByUpdateSequenceNumber;
        const LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Descending;
        foundTags = localStorageManager.listAllTagsPerNote(note, error, flag, limit, offset,
                                                           order, orderDirection);
        if (foundTags.size() != static_cast<int>(limit)) {
            QFAIL(qPrintable("Found unexpected amount of tags: expected to find " + QString::number(limit) +
                             " tags, found " + QString::number(foundTags.size()) + " tags"));
        }

        const Tag & firstTag = foundTags[0];
        const Tag & secondTag = foundTags[1];

        if (!firstTag.hasUpdateSequenceNumber()) {
            QFAIL("First of found tags doesn't have the update sequence number set");
        }

        if (!secondTag.hasUpdateSequenceNumber()) {
            QFAIL("Second of found tags doesn't have the update sequence number set");
        }

        if ((firstTag.updateSequenceNumber() != 3) || (secondTag.updateSequenceNumber() != 2)) {
            QFAIL(qPrintable("Unexpected order of found tags by update sequence number: first tag: " +
                             firstTag.toString() + "\nSecond tag: " + secondTag.toString()));
        }
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListNotesTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        QString error;
        bool res = localStorageManager.addNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        Notebook notebookNotLinkedWithNotes;
        notebookNotLinkedWithNotes.setGuid("00000000-0000-0000-c000-000000000048");
        notebookNotLinkedWithNotes.setUpdateSequenceNumber(1);
        notebookNotLinkedWithNotes.setName("Fake notebook not linked with notes name name");
        notebookNotLinkedWithNotes.setCreationTimestamp(1);
        notebookNotLinkedWithNotes.setModificationTimestamp(1);

        res = localStorageManager.addNotebook(notebookNotLinkedWithNotes, error);
        QVERIFY2(res == true, qPrintable(error));

        Tag testTag;
        testTag.setName("My test tag");
        res = localStorageManager.addTag(testTag, error);
        QVERIFY2(res == true, qPrintable(error));

        int numNotes = 5;
        QList<Note> notes;
        notes.reserve(numNotes);
        for(int i = 0; i < numNotes; ++i)
        {
            notes << Note();
            Note & note = notes.back();

            if (i > 1) {
                note.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            }

            if (i > 2) {
                note.setDirty(true);
            }
            else {
                note.setDirty(false);
            }

            if (i < 3) {
                note.setLocal(true);
            }
            else {
                note.setLocal(false);
            }

            if ((i == 0) || (i == 4)) {
                note.setShortcut(true);
            }
            else {
                note.setShortcut(false);
            }

            if ((i == 1) || (i == 2) || (i == 4)) {
                note.addTagLocalUid(testTag.localUid());
            }

            note.setUpdateSequenceNumber(i+1);
            note.setTitle("Fake note title #" + QString::number(i));
            note.setContent("<en-note><h1>Hello, world #" + QString::number(i) + "</h1></en-note>");
            note.setCreationTimestamp(i+1);
            note.setModificationTimestamp(i+1);
            note.setActive(true);
            note.setNotebookGuid(notebook.guid());
            note.setNotebookLocalUid(notebook.localUid());

            res = localStorageManager.addNote(note, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        // 1) Test method listing all notes per notebook

        error.clear();
        QList<Note> foundNotes = localStorageManager.listAllNotesPerNotebook(notebook, error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        int numFoundNotes = foundNotes.size();
        if (numFoundNotes != numNotes) {
            QFAIL(qPrintable("Error: number of notes in the result of LocalStorageManager::ListAllNotesPerNotebook (" +
                             QString::number(numFoundNotes) + ") does not match the original number of added notes (" +
                             QString::number(numNotes) + ")"));
        }

        for(int i = 0; i < numFoundNotes; ++i)
        {
            const Note & foundNote = foundNotes.at(i);
            if (!notes.contains(foundNote)) {
                QFAIL("One of notes from the result of LocalStorageManager::ListAllNotesPerNotebook "
                      "was not found in the list of original notes");
            }
        }

        // 2) Ensure the method listing notes per notebook actually checks the notebook

        error.clear();
        foundNotes = localStorageManager.listAllNotesPerNotebook(notebookNotLinkedWithNotes, error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        if (foundNotes.size() != 0) {
            QFAIL(qPrintable("Found non-zero number of notes in the result of LocalStorageManager::ListAllNotesPerNotebook "
                             "called with guid of notebook not containing any notes (found " +
                             QString::number(foundNotes.size()) + " notes)"));
        }

        // 3) Test method listing notes per notebook considering only the notes with guid + with limit, offset,
        // specific order and order direction

        error.clear();
        size_t limit = 2;
        size_t offset = 1;
        LocalStorageManager::ListNotesOrder::type order = LocalStorageManager::ListNotesOrder::ByUpdateSequenceNumber;
        LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Descending;
        foundNotes = localStorageManager.listAllNotesPerNotebook(notebook, error, /* with resource binary data = */ true,
                                                                 LocalStorageManager::ListElementsWithGuid, limit, offset,
                                                                 order, orderDirection);
        if (foundNotes.size() != static_cast<int>(limit)) {
            QFAIL(qPrintable("Found unexpected amount of notes: expected to find " + QString::number(limit) +
                             " notes, found " + QString::number(foundNotes.size()) + " notes"));
        }

        const Note & firstNote = foundNotes[0];
        const Note & secondNote = foundNotes[1];

        if (!firstNote.hasUpdateSequenceNumber()) {
            QFAIL("First of found notes doesn't have the update sequence number set");
        }

        if (!secondNote.hasUpdateSequenceNumber()) {
            QFAIL("Second of found notes doesn't have the update sequence number set");
        }

        if ((firstNote.updateSequenceNumber() != 4) || (secondNote.updateSequenceNumber() != 3)) {
            QFAIL(qPrintable("Unexpected order of found notes by update sequence number: first note: " +
                             firstNote.toString() + "\nSecond note: " + secondNote.toString()));
        }

        // 4) Test method listing notes per tag considering only the notes with guid + with limit, offset,
        // specific order and order direction

        error.clear();
        limit = 2;
        offset = 0;

        foundNotes = localStorageManager.listNotesPerTag(testTag, error, /* with resource binary data = */ true,
                                                         LocalStorageManager::ListElementsWithGuid, limit, offset,
                                                         order, orderDirection);
        if (foundNotes.size() != static_cast<int>(limit)) {
            QFAIL(qPrintable("Found unexpected amount of notes: expected to find " + QString::number(limit) +
                             " notes, found " + QString::number(foundNotes.size()) + " notes"));
        }

        const Note & firstNotePerTag = foundNotes[0];
        const Note & secondNotePerTag = foundNotes[1];

        if (!firstNotePerTag.hasUpdateSequenceNumber()) {
            QFAIL("First of found notes doesn't have the update sequence number set");
        }

        if (!secondNotePerTag.hasUpdateSequenceNumber()) {
            QFAIL("Second of found notes doesn't have the update sequence number set");
        }

        if (firstNotePerTag.updateSequenceNumber() < secondNotePerTag.updateSequenceNumber()) {
            QFAIL("Incorrect sorting of found notes, expected descending sorting by update sequence number");
        }

        if ((firstNotePerTag != notes[4]) && (secondNotePerTag != notes[2])) {
            QFAIL("Found unexpected notes per tag");
        }

        // 5) Test method listing all notes
        error.clear();
        foundNotes = localStorageManager.listNotes(LocalStorageManager::ListAll, error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        numFoundNotes = foundNotes.size();
        if (numFoundNotes != numNotes) {
            QFAIL(qPrintable("Error number of notes in the result of LocalStorageManager::ListNotes with flag ListAll (" +
                             QString::number(numFoundNotes) + ") does not match the original number of added notes (" +
                             QString::number(numNotes) + ")"));
        }

        for(int i = 0; i < numFoundNotes; ++i)
        {
            const Note & foundNote = foundNotes[i];
            if (!notes.contains(foundNote)) {
                QFAIL("One of notes from the result of LocalStorageManager::ListNotes with flag ListAll "
                      "was not found in the list of original notes");
            }
        }

#define CHECK_LIST_NOTES_BY_FLAG(flag, flag_name, true_condition, false_condition) \
        error.clear(); \
        foundNotes = localStorageManager.listNotes(flag, error); \
        QVERIFY2(error.isEmpty(), qPrintable(error)); \
        \
        for(int i = 0; i < numNotes; ++i) \
        { \
            const Note & note = notes[i]; \
            bool res = foundNotes.contains(note); \
            if ((true_condition) && !res) { \
                QNWARNING("Not found note: " << note); \
                QFAIL("One of " flag_name " notes was not found by LocalStorageManager::ListNotes"); \
            } \
            else if ((false_condition) && res) { \
                QNWARNING("Found irrelevant note: " << note); \
                QFAIL("LocalStorageManager::ListNotes with flag " flag_name " returned incorrect note"); \
            } \
        }

        // 6) Test method listing only dirty notes
        CHECK_LIST_NOTES_BY_FLAG(LocalStorageManager::ListDirty, "dirty", i > 2, i <= 2);

        // 7) Test method listing only local notes
        CHECK_LIST_NOTES_BY_FLAG(LocalStorageManager::ListLocal, "local", i < 3, i >= 3);

        // 8) Test method listing only notes without guid
        CHECK_LIST_NOTES_BY_FLAG(LocalStorageManager::ListElementsWithoutGuid, "guidless", i <= 1, i > 1);

        // 9) Test method listing only notes with shortcut
        CHECK_LIST_NOTES_BY_FLAG(LocalStorageManager::ListElementsWithShortcuts, "having shortcut", (i == 0) || (i == 4), (i != 0) && (i != 4));

        // 10) Test method listing dirty notes with guid and with shortcut
        CHECK_LIST_NOTES_BY_FLAG(LocalStorageManager::ListDirty | LocalStorageManager::ListElementsWithGuid | LocalStorageManager::ListElementsWithShortcuts,
                                 "dirty, having guid, having shortcut", i == 4, i != 4);

        // 11) Test method listing local notes having shortcut
        CHECK_LIST_NOTES_BY_FLAG(LocalStorageManager::ListLocal | LocalStorageManager::ListElementsWithShortcuts,
                                 "local, having shortcut", i == 0, i != 0);
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerListNotebooksTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        QString error;

        int numNotebooks = 5;
        QList<Notebook> notebooks;
        notebooks.reserve(numNotebooks);
        for(int i = 0; i < numNotebooks; ++i)
        {
            notebooks << Notebook();
            Notebook & notebook = notebooks.back();

            if (i > 1) {
                notebook.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            }

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

            if (i > 2) {
                notebook.setDirty(true);
            }
            else {
                notebook.setDirty(false);
            }

            if (i < 3) {
                notebook.setLocal(true);
            }
            else {
                notebook.setLocal(false);
            }

            if ((i == 0) || (i == 4)) {
                notebook.setShortcut(true);
            }
            else {
                notebook.setShortcut(false);
            }

            if (i > 1) {
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
            }

            bool res = localStorageManager.addNotebook(notebook, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        // 1) Test method listing all notebooks

        QList<Notebook> foundNotebooks = localStorageManager.listAllNotebooks(error);
        QVERIFY2(!foundNotebooks.isEmpty(), qPrintable(error));

        int numFoundNotebooks = foundNotebooks.size();
        if (numFoundNotebooks != numNotebooks) {
            QFAIL(qPrintable("Error: number of notebooks in the result of LocalStorageManager::ListAllNotebooks (" +
                             QString::number(numFoundNotebooks) + ") does not match the original number of added notebooks (" +
                             QString::number(numNotebooks) + ")"));
        }

        for(int i = 0; i < numFoundNotebooks; ++i)
        {
            const Notebook & foundNotebook = foundNotebooks.at(i);
            if (!notebooks.contains(foundNotebook)) {
                QFAIL("One of notebooks from the result of LocalStorageManager::ListAllNotebooks "
                      "was not found in the list of original notebooks");
            }
        }

#define CHECK_LIST_NOTEBOOKS_BY_FLAG(flag, flag_name, true_condition, false_condition) \
        error.clear(); \
        foundNotebooks = localStorageManager.listNotebooks(flag, error); \
        QVERIFY2(error.isEmpty(), qPrintable(error)); \
        \
        for(int i = 0; i < numNotebooks; ++i) \
        { \
            const Notebook & notebook = notebooks.at(i); \
            bool res = foundNotebooks.contains(notebook); \
            if ((true_condition) && !res) { \
                QNWARNING("Not found notebook: " << notebook); \
                QFAIL("One of " flag_name " notebooks was not found by LocalStorageManager::ListNotebooks"); \
            } \
            else if ((false_condition) && res) { \
                QNWARNING("Found irrelevant notebook: " << notebook); \
                QFAIL("LocalStorageManager::ListNotebooks with flag " flag_name " returned incorrect notebook"); \
            } \
        }

        // 2) Test method listing only dirty notebooks
        CHECK_LIST_NOTEBOOKS_BY_FLAG(LocalStorageManager::ListDirty, "dirty", i > 2, i <= 2);

        // 3) Test method listing only local notebooks
        CHECK_LIST_NOTEBOOKS_BY_FLAG(LocalStorageManager::ListLocal, "local", i < 3, i >= 3);

        // 4) Test method listing only notebooks without guid
        CHECK_LIST_NOTEBOOKS_BY_FLAG(LocalStorageManager::ListElementsWithoutGuid, "guidless", i <= 1, i > 1);

        // 5) Test method listing only notebooks with shortcut
        CHECK_LIST_NOTEBOOKS_BY_FLAG(LocalStorageManager::ListElementsWithShortcuts, "having shortcut", (i == 0) || (i == 4), (i != 0) && (i != 4));

        // 6) Test method listing dirty notebooks with guid and with shortcut
        CHECK_LIST_NOTEBOOKS_BY_FLAG(LocalStorageManager::ListDirty | LocalStorageManager::ListElementsWithGuid |
                                     LocalStorageManager::ListElementsWithShortcuts,
                                     "dirty, having guid, having shortcut", i == 4, i != 4);

        // 7) Test method listing local notebooks having shortcut
        CHECK_LIST_NOTEBOOKS_BY_FLAG(LocalStorageManager::ListLocal | LocalStorageManager::ListElementsWithShortcuts,
                                     "local, having shortcut", i == 0, i != 0);
    }
    CATCH_EXCEPTION();
}

void CoreTester::localStorageManagerExpungeNotelessTagsFromLinkedNotebooksTest()
{
    try
    {
        const bool startFromScratch = true;
        const bool overrideLock = false;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch, overrideLock);

        LinkedNotebook linkedNotebook;
        linkedNotebook.setGuid("00000000-0000-0000-c000-000000000001");
        linkedNotebook.setUpdateSequenceNumber(1);
        linkedNotebook.setShareName("Linked notebook share name");
        linkedNotebook.setUsername("Linked notebook username");
        linkedNotebook.setShardId("Linked notebook shard id");
        linkedNotebook.setShareKey("Linked notebook share key");
        linkedNotebook.setUri("Linked notebook uri");
        linkedNotebook.setNoteStoreUrl("Linked notebook note store url");
        linkedNotebook.setWebApiUrlPrefix("Linked notebook web api url prefix");
        linkedNotebook.setStack("Linked notebook stack");
        linkedNotebook.setBusinessId(1);

        Notebook notebook;
        notebook.setGuid("00000000-0000-0000-c000-000000000047");
        notebook.setLinkedNotebookGuid(linkedNotebook.guid());
        notebook.setUpdateSequenceNumber(1);
        notebook.setName("Fake notebook name");
        notebook.setCreationTimestamp(1);
        notebook.setModificationTimestamp(1);

        Note note;
        note.setGuid("00000000-0000-0000-c000-000000000046");
        note.setUpdateSequenceNumber(1);
        note.setTitle("Fake note title");
        note.setContent("<en-note><h1>Hello, world</h1></en-note>");
        note.setCreationTimestamp(1);
        note.setModificationTimestamp(1);
        note.setActive(true);
        note.setNotebookGuid(notebook.guid());
        note.setNotebookLocalUid(notebook.localUid());

        QString error;
        bool res = localStorageManager.addLinkedNotebook(linkedNotebook, error);
        QVERIFY2(res == true, qPrintable(error));

        error.clear();
        res = localStorageManager.addNotebook(notebook, error);
        QVERIFY2(res == true, qPrintable(error));

        error.clear();
        res = localStorageManager.addNote(note, error);
        QVERIFY2(res == true, qPrintable(error));

        int nTags = 5;
        QList<Tag> tags;
        tags.reserve(nTags);
        for(int i = 0; i < nTags; ++i)
        {
            tags.push_back(Tag());
            Tag & tag = tags.back();

            tag.setGuid("00000000-0000-0000-c000-00000000000" + QString::number(i+1));
            tag.setUpdateSequenceNumber(i);
            tag.setName("Tag name #" + QString::number(i));

            if (i > 2) {
                tag.setLinkedNotebookGuid(linkedNotebook.guid());
            }

            error.clear();
            res = localStorageManager.addTag(tag, error);
            QVERIFY2(res == true, qPrintable(error));

            error.clear();
            res = localStorageManager.linkTagWithNote(tag, note, error);
            QVERIFY2(res == true, qPrintable(error));
        }

        error.clear();
        res = localStorageManager.expungeNote(note, error);
        QVERIFY2(res == true, qPrintable(error));

        error.clear();
        res = localStorageManager.expungeNotelessTagsFromLinkedNotebooks(error);
        QVERIFY2(res == true, qPrintable(error));

        QList<Tag> foundTags;
        foundTags.reserve(3);
        error.clear();
        foundTags = localStorageManager.listAllTags(error);
        if (foundTags.isEmpty() && !error.isEmpty()) {
            QFAIL(qPrintable(error));
        }

        for(int i = 0; i < nTags; ++i)
        {
            const Tag & tag = tags[i];

            if ((i > 2) && foundTags.contains(tag)) {
                error = "Found tag from linked notebook which should have been expunged";
                QNWARNING(error);
                QFAIL(qPrintable(error));
            }
            else if ((i <= 2) && !foundTags.contains(tag)) {
                error = "Could not find tag which should have remained in the local storage";
                QNWARNING(error);
                QFAIL(qPrintable(error));
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
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&savedSearchAsyncTester, QNSIGNAL(SavedSearchLocalStorageManagerAsyncTester,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&savedSearchAsyncTester, QNSIGNAL(SavedSearchLocalStorageManagerAsyncTester,failure,QString), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailureWithError,QString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &savedSearchAsyncTester, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onInitTestCase));
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
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&linkedNotebookAsyncTester, QNSIGNAL(LinkedNotebookLocalStorageManagerAsyncTester,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&linkedNotebookAsyncTester, QNSIGNAL(LinkedNotebookLocalStorageManagerAsyncTester,failure,QString), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailureWithError,QString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &linkedNotebookAsyncTester, QNSLOT(LinkedNotebookLocalStorageManagerAsyncTester,onInitTestCase));
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
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&tagAsyncTester, QNSIGNAL(TagLocalStorageManagerAsyncTester,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&tagAsyncTester, QNSIGNAL(TagLocalStorageManagerAsyncTester,failure,QString), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailureWithError,QString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &tagAsyncTester, QNSLOT(TagLocalStorageManagerAsyncTester,onInitTestCase));
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
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&userAsyncTester, QNSIGNAL(UserLocalStorageManagerAsyncTester,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&userAsyncTester, QNSIGNAL(UserLocalStorageManagerAsyncTester,failure,QString), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailureWithError,QString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &userAsyncTester, QNSLOT(UserLocalStorageManagerAsyncTester,onInitTestCase));
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
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&notebookAsyncTester, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&notebookAsyncTester, QNSIGNAL(NotebookLocalStorageManagerAsyncTester,failure,QString), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailureWithError,QString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &notebookAsyncTester, QNSLOT(NotebookLocalStorageManagerAsyncTester,onInitTestCase));
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
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&noteAsyncTester, QNSIGNAL(NoteLocalStorageManagerAsyncTester,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&noteAsyncTester, QNSIGNAL(NoteLocalStorageManagerAsyncTester,failure,QString), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailureWithError,QString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &noteAsyncTester, QNSLOT(NoteLocalStorageManagerAsyncTester,onInitTestCase));
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
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&resourceAsyncTester, QNSIGNAL(ResourceLocalStorageManagerAsyncTester,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&resourceAsyncTester, QNSIGNAL(ResourceLocalStorageManagerAsyncTester,failure,QString), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailureWithError,QString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &resourceAsyncTester, QNSLOT(ResourceLocalStorageManagerAsyncTester,onInitTestCase));
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

void CoreTester::localStorageCacheManagerTest()
{
    int localStorageCacheAsyncTestResult = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        LocalStorageCacheAsyncTester localStorageCacheAsyncTester;

        EventLoopWithExitStatus loop;
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&localStorageCacheAsyncTester, QNSIGNAL(LocalStorageCacheAsyncTester,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&localStorageCacheAsyncTester, QNSIGNAL(LocalStorageCacheAsyncTester,failure,QString), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailureWithError,QString));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &localStorageCacheAsyncTester, QNSLOT(LocalStorageCacheAsyncTester,onInitTestCase));
        localStorageCacheAsyncTestResult = loop.exec();
    }

    if (localStorageCacheAsyncTestResult == -1) {
        QFAIL("Internal error: incorrect return status from local storage cache async tester");
    }
    else if (localStorageCacheAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in local storage cache async tester");
    }
    else if (localStorageCacheAsyncTestResult == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Local storage cache async tester failed to finish in time");
    }
}

#undef CATCH_EXCEPTION

}
}
