#include "CoreTester.h"
#include "LocalStorageManagerTests.h"
#include "SavedSearchLocalStorageManagerAsyncTester.h"
#include "LinkedNotebookLocalStorageManagerAsyncTester.h"
#include "TagLocalStorageManagerAsyncTester.h"
#include "UserLocalStorageManagerAsyncTester.h"
#include "NotebookLocalStorageManagerAsyncTester.h"
#include "NoteLocalStorageManagerAsyncTester.h"
#include <tools/IQuteNoteException.h>
#include <tools/EventLoopWithExitStatus.h>
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/SavedSearch.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/UserWrapper.h>
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
        note.setContent("Fake note content");
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
        note.setContent("Fake note content");
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
        note.setContent("Fake note content");
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
        note.setContent("Fake note content");
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
            note.setContent("Fake note content #" + QString::number(i));
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

#undef CATCH_EXCEPTION

}
}
