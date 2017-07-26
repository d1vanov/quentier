#include "ModelTester.h"
#include "../../models/SavedSearchModel.h"
#include "../../models/TagModel.h"
#include "SavedSearchModelTestHelper.h"
#include "TagModelTestHelper.h"
#include "NotebookModelTestHelper.h"
#include "NoteModelTestHelper.h"
#include "FavoritesModelTestHelper.h"
#include <quentier/exception/IQuentierException.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <quentier/utility/UidGenerator.h>
#include <quentier/logging/QuentierLogger.h>
#include <QtTest/QtTest>
#include <QtGui/QtGui>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QApplication>
#include <QByteArray>

// 10 minutes, the timeout for async stuff to complete
#define MAX_ALLOWED_MILLISECONDS 600000

#define qnPrintable(string) QString::fromUtf8(string).toLocal8Bit().constData()

ModelTester::ModelTester(QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerAsync(Q_NULLPTR)
{}

ModelTester::~ModelTester()
{}

void ModelTester::testSavedSearchModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;
        Account account(QStringLiteral("ModelTester_saved_search_model_test_fake_user"), Account::Type::Evernote, 300);
        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(account, /* start from scratch = */ true,
                                                                             /* override lock = */ false, this);
        m_pLocalStorageManagerAsync->init();

        SavedSearchModelTestHelper savedSearchModelTestHelper(m_pLocalStorageManagerAsync);

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&savedSearchModelTestHelper, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&savedSearchModelTestHelper, SIGNAL(failure()), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &savedSearchModelTestHelper, SLOT(test()));
        res = loop.exec();
    }

    if (res == -1) {
        QFAIL("Internal error: incorrect return status from saved search model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in saved search model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Saved search model async tester failed to finish in time");
    }
}

void ModelTester::testTagModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;
        Account account(QStringLiteral("ModelTester_tag_model_test_fake_user"), Account::Type::Evernote, 400);
        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(account, /* start from scratch = */ true,
                                                                             /* override lock = */ false, this);
        m_pLocalStorageManagerAsync->init();

        TagModelTestHelper tagModelTestHelper(m_pLocalStorageManagerAsync);

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&tagModelTestHelper, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&tagModelTestHelper, SIGNAL(failure()), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &tagModelTestHelper, SLOT(test()));
        res = loop.exec();
    }

    if (res == -1) {
        QFAIL("Internal error: incorrect return status from tag model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in tag model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Tag model async tester failed to finish in time");
    }
}

void ModelTester::testNotebookModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;
        Account account(QStringLiteral("ModelTester_notebook_model_test_fake_user"), Account::Type::Evernote, 500);
        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(account, /* start from scratch = */ true,
                                                                             /* override lock = */ false, this);
        m_pLocalStorageManagerAsync->init();

        NotebookModelTestHelper notebookModelTestHelper(m_pLocalStorageManagerAsync);

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&notebookModelTestHelper, SIGNAL(success()), SLOT(exitAsSuccess()));
        loop.connect(&notebookModelTestHelper, SIGNAL(failure()), SLOT(exitAsFailure()));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &notebookModelTestHelper, SLOT(test()));
        res = loop.exec();
    }

    if (res == -1) {
        QFAIL("Internal error: incorrect return status from notebook model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in notebook model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Notebook model async tester failed to finish in time");
    }
}

void ModelTester::testNoteModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;
        Account account(QStringLiteral("ModelTester_note_model_test_fake_user"), Account::Type::Evernote, 700);
        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(account, /* start from scratch = */ true,
                                                                             /* override lock = */ false, this);
        m_pLocalStorageManagerAsync->init();

        NoteModelTestHelper noteModelTestHelper(m_pLocalStorageManagerAsync);

        EventLoopWithExitStatus loop;
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&noteModelTestHelper, QNSIGNAL(NoteModelTestHelper,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&noteModelTestHelper, QNSIGNAL(NoteModelTestHelper,failure), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailure));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &noteModelTestHelper, SLOT(launchTest()));
        res = loop.exec();
    }

    if (res == -1) {
        QFAIL("Internal error: incorrect return status from note model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in note model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Note model async tester failed to finish in time");
    }
}

void ModelTester::testFavoritesModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;
        Account account(QStringLiteral("ModelTester_favorites_model_test_fake_user"), Account::Type::Evernote, 800);
        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(account, /* start from scratch = */ true,
                                                                             /* override lock = */ false, this);
        m_pLocalStorageManagerAsync->init();

        FavoritesModelTestHelper favoritesModelTestHelper(m_pLocalStorageManagerAsync);

        EventLoopWithExitStatus loop;
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(&favoritesModelTestHelper, QNSIGNAL(FavoritesModelTestHelper,success), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(&favoritesModelTestHelper, QNSIGNAL(FavoritesModelTestHelper,failure), &loop, QNSLOT(EventLoopWithExitStatus,exitAsFailure));

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();
        slotInvokingTimer.singleShot(0, &favoritesModelTestHelper, SLOT(launchTest()));
        res = loop.exec();
    }

    if (res == -1) {
        QFAIL("Internal error: incorrect return status from note model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Failure) {
        QFAIL("Detected failure during the asynchronous loop processing in note model async tester");
    }
    else if (res == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Note model async tester failed to finish in time");
    }
}

void ModelTester::testTagModelItemSerialization()
{
    using namespace quentier;

    TagItem parentTagItem(UidGenerator::Generate(), UidGenerator::Generate());
    TagModelItem parentItem(TagModelItem::Type::Tag, &parentTagItem);

    TagItem item;
    item.setLocalUid(UidGenerator::Generate());
    item.setName(QStringLiteral("Test item"));
    item.setGuid(UidGenerator::Generate());
    item.setLinkedNotebookGuid(UidGenerator::Generate());
    item.setDirty(true);
    item.setSynchronizable(false);
    item.setGuid(UidGenerator::Generate());
    item.setParentLocalUid(parentTagItem.localUid());
    item.setParentGuid(parentTagItem.guid());

    TagModelItem modelItem(TagModelItem::Type::Tag, &item);
    modelItem.setParent(&parentItem);

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << modelItem;

    QDataStream in(&encodedItem, QIODevice::ReadOnly);
    TagModelItem restoredItem;
    in >> restoredItem;

    QVERIFY2(restoredItem.tagItem() != Q_NULLPTR, qnPrintable("Null pointer to tag item"));
    QVERIFY2(restoredItem.tagItem() == &item, qnPrintable("Wrong pointer to the tag item"));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ModelTester tester;
    return QTest::qExec(&tester, argc, argv);
}
