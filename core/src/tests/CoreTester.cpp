#include "CoreTester.h"
#include "SerializationTests.h"
#include "LocalStorageManagerTests.h"
#include <tools/IQuteNoteException.h>
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/SavedSearch.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/Serialization.h>
#include <QtTest/QTest>

namespace qute_note {
namespace test {

CoreTester::CoreTester(QObject * parent) :
    QObject(parent)
{}

CoreTester::~CoreTester()
{}

#define TEST(component) \
    void CoreTester::serializationTest##component() \
    { \
        QString error; \
        bool res = Test##component##Serialization(error); \
        QVERIFY2(res == true, error.toStdString().c_str()); \
    }

TEST(BusinessUserInfo)
TEST(PremiumInfo)
TEST(Accounting)
TEST(UserAttributes)
TEST(NoteAttributes)
TEST(ResourceAttributes)

#undef TEST

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
        QVERIFY2(res == true, error.toStdString().c_str());
    }
    catch(IQuteNoteException & exception) {
        QFAIL(QString("Caught exception: " + exception.errorMessage()).toStdString().c_str());
    }
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
    catch(IQuteNoteException & exception) {
        QFAIL(QString("Caught exception: " + exception.errorMessage()).toStdString().c_str());
    }
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
    catch(IQuteNoteException & exception) {
        QFAIL(QString("Caught exception: " + exception.errorMessage()).toStdString().c_str());
    }
}

void CoreTester::localStorageManagerIndividualResourceTest()
{
    try
    {
        const bool startFromScratch = true;
        LocalStorageManager localStorageManager("CoreTesterFakeUser", 0, startFromScratch);

        ResourceWrapper resource;
        resource.setGuid("00000000-0000-0000-c000-000000000046");
        resource.setUpdateSequenceNumber(1);
        resource.setNoteGuid("00000000-0000-0000-c000-000000000047");
        resource.setDataBody("Fake resource data body");
        resource.setDataSize(resource.dataBody().size());
        resource.setDataHash("Fake hash      1");

        resource.setRecognitionDataBody("Fake resource recognition data body");
        resource.setRecognitionDataSize(resource.recognitionDataBody().size());
        resource.setRecognitionDataHash("Fake hash      2");

        resource.setMime("text/plain");
        resource.setWidth(1);
        resource.setHeight(1);

        evernote::edam::ResourceAttributes resourceAttributes;

        resourceAttributes.sourceURL = "Fake resource source URL";
        resourceAttributes.__isset.sourceURL = true;

        resourceAttributes.timestamp = 1;
        resourceAttributes.__isset.timestamp = true;

        resourceAttributes.latitude = 0.0;
        resourceAttributes.__isset.latitude = true;

        resourceAttributes.longitude = 0.0;
        resourceAttributes.__isset.longitude = true;

        resourceAttributes.altitude = 0.0;
        resourceAttributes.__isset.altitude = true;

        resourceAttributes.cameraMake = "Fake resource camera make";
        resourceAttributes.__isset.cameraMake = true;

        resourceAttributes.cameraModel = "Fake resource camera model";
        resourceAttributes.__isset.cameraModel = true;

        QByteArray serializedResourceAttributes = GetSerializedResourceAttributes(resourceAttributes);

        resource.setResourceAttributes(serializedResourceAttributes);

        QString error;
        bool res = TestResourceAddFindUpdateExpungeInLocalStorage(resource, localStorageManager, error);
        QVERIFY2(res == true, error.toStdString().c_str());
    }
    catch(IQuteNoteException & exception) {
        QFAIL(QString("Caught exception: " + exception.errorMessage()).toStdString().c_str());
    }
}

}
}
