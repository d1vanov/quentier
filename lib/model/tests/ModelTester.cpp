/*
 * Copyright 2016-2024 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ModelTester.h"

#include "FavoritesModelTestHelper.h"
#include "NoteModelTestHelper.h"
#include "NotebookModelTestHelper.h"
#include "SavedSearchModelTestHelper.h"
#include "TagModelTestHelper.h"

#include <lib/model/saved_search/SavedSearchModel.h>
#include <lib/model/tag/TagModel.h>

#include <quentier/exception/IQuentierException.h>
#include <quentier/local_storage/Factory.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Factory.h>
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <quentier/utility/Initialize.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/UidGenerator.h>

#include <QtGui>
#include <QtTest>

#include <QApplication>
#include <QByteArray>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#define qnPrintable(string) QString::fromUtf8(string).toLocal8Bit().constData()

namespace {

// 10 minutes, the timeout for async stuff to complete
constexpr int gMaxAllowedMilliseconds = 600000;

} // namespace

ModelTester::ModelTester(QObject * parent) : QObject{parent} {}

ModelTester::~ModelTester() = default;

void ModelTester::testSavedSearchModel()
{
    using namespace quentier;

    QString error;
    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(gMaxAllowedMilliseconds);
        timer.setSingleShot(true);

        const Account account{
            QStringLiteral("ModelTester_saved_search_model_test_fake_user"),
            Account::Type::Evernote, 300};

        const QDir localStorageDir{m_tempDir.path()};

        m_localStorage = quentier::local_storage::createSqliteLocalStorage(
            account, localStorageDir, quentier::threading::globalThreadPool());

        SavedSearchModelTestHelper savedSearchModelTestHelper{m_localStorage};

        EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &savedSearchModelTestHelper, &SavedSearchModelTestHelper::success,
            &loop, &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &savedSearchModelTestHelper, &SavedSearchModelTestHelper::failure,
            &loop, &EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();

        slotInvokingTimer.singleShot(
            0, &savedSearchModelTestHelper, &SavedSearchModelTestHelper::test);

        loop.exec();
        status = loop.exitStatus();
        error = loop.errorDescription().nonLocalizedString();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
        error.prepend(
            QStringLiteral("Detected failure during the asynchronous loop "
                           "processing in saved search model async tester: "));

        QFAIL(qPrintable(error));
    }
    else if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Saved search model async tester failed to finish in time");
    }
}

void ModelTester::testTagModel()
{
    using namespace quentier;

    QString error;
    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(gMaxAllowedMilliseconds);
        timer.setSingleShot(true);

        const Account account{
            QStringLiteral("ModelTester_tag_model_test_fake_user"),
            Account::Type::Evernote, 400};

        const QDir localStorageDir{m_tempDir.path()};

        m_localStorage = quentier::local_storage::createSqliteLocalStorage(
            account, localStorageDir, quentier::threading::globalThreadPool());

        TagModelTestHelper tagModelTestHelper{m_localStorage};

        EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &tagModelTestHelper, &TagModelTestHelper::success, &loop,
            &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &tagModelTestHelper, &TagModelTestHelper::failure, &loop,
            &EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();

        slotInvokingTimer.singleShot(
            0, &tagModelTestHelper, &TagModelTestHelper::test);

        loop.exec();
        status = loop.exitStatus();
        error = loop.errorDescription().nonLocalizedString();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
        error.prepend(
            QStringLiteral("Detected failure during the asynchronous "
                           "loop processing in tag model async tester: "));

        QFAIL(qPrintable(error));
    }
    else if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Tag model async tester failed to finish in time");
    }
}

void ModelTester::testNotebookModel()
{
    using namespace quentier;

    QString error;
    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(gMaxAllowedMilliseconds);
        timer.setSingleShot(true);

        const Account account{
            QStringLiteral("ModelTester_notebook_model_test_fake_user"),
            Account::Type::Evernote, 500};

        const QDir localStorageDir{m_tempDir.path()};

        m_localStorage = quentier::local_storage::createSqliteLocalStorage(
            account, localStorageDir, quentier::threading::globalThreadPool());

        NotebookModelTestHelper notebookModelTestHelper{m_localStorage};

        EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &notebookModelTestHelper, &NotebookModelTestHelper::success, &loop,
            &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &notebookModelTestHelper, &NotebookModelTestHelper::failure, &loop,
            &EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();

        slotInvokingTimer.singleShot(
            0, &notebookModelTestHelper, &NotebookModelTestHelper::test);

        loop.exec();
        status = loop.exitStatus();
        error = loop.errorDescription().nonLocalizedString();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
        error.prepend(
            QStringLiteral("Detected failure during the asynchronous loop "
                           "processing in notebook model async tester: "));

        QFAIL(qPrintable(error));
    }
    else if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Notebook model async tester failed to finish in time");
    }
}

void ModelTester::testNoteModel()
{
    using namespace quentier;

    QString error;
    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(gMaxAllowedMilliseconds);
        timer.setSingleShot(true);

        const Account account{
            QStringLiteral("ModelTester_note_model_test_fake_user"),
            Account::Type::Evernote, 700};

        const QDir localStorageDir{m_tempDir.path()};

        m_localStorage = quentier::local_storage::createSqliteLocalStorage(
            account, localStorageDir, quentier::threading::globalThreadPool());

        NoteModelTestHelper noteModelTestHelper{m_localStorage};

        EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &noteModelTestHelper, &NoteModelTestHelper::success, &loop,
            &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &noteModelTestHelper, &NoteModelTestHelper::failure, &loop,
            &EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();

        slotInvokingTimer.singleShot(
            0, &noteModelTestHelper, &NoteModelTestHelper::test);

        loop.exec();
        status = loop.exitStatus();
        error = loop.errorDescription().nonLocalizedString();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
        error.prepend(
            QStringLiteral("Detected failure during the asynchronous "
                           "loop processing in note model async tester: "));

        QFAIL(qPrintable(error));
    }
    else if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Note model async tester failed to finish in time");
    }
}

void ModelTester::testFavoritesModel()
{
    using namespace quentier;

    QString error;
    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(gMaxAllowedMilliseconds);
        timer.setSingleShot(true);

        const Account account{
            QStringLiteral("ModelTester_favorites_model_test_fake_user"),
            Account::Type::Evernote, 800};

        const QDir localStorageDir{m_tempDir.path()};

        m_localStorage = quentier::local_storage::createSqliteLocalStorage(
            account, localStorageDir, quentier::threading::globalThreadPool());

        FavoritesModelTestHelper favoritesModelTestHelper{m_localStorage};

        EventLoopWithExitStatus loop;

        QObject::connect(
            &timer, &QTimer::timeout, &loop,
            &EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            &favoritesModelTestHelper, &FavoritesModelTestHelper::success,
            &loop, &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            &favoritesModelTestHelper, &FavoritesModelTestHelper::failure,
            &loop, &EventLoopWithExitStatus::exitAsFailureWithErrorString);

        QTimer slotInvokingTimer;
        slotInvokingTimer.setInterval(500);
        slotInvokingTimer.setSingleShot(true);

        timer.start();

        slotInvokingTimer.singleShot(
            0, &favoritesModelTestHelper, &FavoritesModelTestHelper::test);

        loop.exec();
        status = loop.exitStatus();
        error = loop.errorDescription().nonLocalizedString();
    }

    if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
        error.prepend(
            QStringLiteral("Detected failure during the asynchronous loop "
                           "processing in favorites model async tester: "));

        QFAIL(qPrintable(error));
    }
    else if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QFAIL("Favorites model async tester failed to finish in time");
    }
}

void ModelTester::testTagModelItemSerialization()
{
    using namespace quentier;

    TagItem parentTagItem(UidGenerator::Generate(), UidGenerator::Generate());

    TagItem item;
    item.setLocalId(UidGenerator::Generate());
    item.setName(QStringLiteral("Test item"));
    item.setGuid(UidGenerator::Generate());
    item.setLinkedNotebookGuid(UidGenerator::Generate());
    item.setDirty(true);
    item.setSynchronizable(false);
    item.setGuid(UidGenerator::Generate());
    item.setParentLocalId(parentTagItem.localId());
    item.setParentGuid(parentTagItem.guid());
    item.setParent(&parentTagItem);

    QByteArray encodedItem;
    QDataStream out{&encodedItem, QIODevice::WriteOnly};
    out << item;

    QDataStream in{&encodedItem, QIODevice::ReadOnly};
    qint32 type = 0;
    in >> type;

    QVERIFY(type == static_cast<int>(ITagModelItem::Type::Tag));

    TagItem restoredItem;
    in >> restoredItem;

    QVERIFY(restoredItem.localId() == item.localId());
    QVERIFY(restoredItem.guid() == item.guid());
    QVERIFY(restoredItem.parent() == item.parent());
}

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);
    quentier::initializeLibquentier();
    ModelTester tester;
    return QTest::qExec(&tester, argc, argv);
}
