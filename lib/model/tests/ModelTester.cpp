/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include <lib/model/SavedSearchModel.h>
#include <lib/model/tag/TagModel.h>

#include <quentier/exception/IQuentierException.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/UidGenerator.h>
#include <quentier/utility/Utility.h>

#include <QtGui/QtGui>
#include <QtTest/QtTest>

#include <QApplication>
#include <QByteArray>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QTreeWidget>
#include <QTreeWidgetItem>

// 10 minutes, the timeout for async stuff to complete
#define MAX_ALLOWED_MILLISECONDS 600000

#define qnPrintable(string) QString::fromUtf8(string).toLocal8Bit().constData()

ModelTester::ModelTester(QObject * parent) : QObject(parent) {}

ModelTester::~ModelTester() {}

void ModelTester::testSavedSearchModel()
{
    using namespace quentier;

    QString error;
    auto status = EventLoopWithExitStatus::ExitStatus::Failure;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;

        Account account(
            QStringLiteral("ModelTester_saved_search_model_test_fake_user"),
            Account::Type::Evernote, 300);

        LocalStorageManager::StartupOptions startupOptions(
            LocalStorageManager::StartupOption::ClearDatabase);

        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(
            account, startupOptions, this);

        m_pLocalStorageManagerAsync->init();

        SavedSearchModelTestHelper savedSearchModelTestHelper(
            m_pLocalStorageManagerAsync);

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

        Q_UNUSED(loop.exec())
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
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;

        Account account(
            QStringLiteral("ModelTester_tag_model_test_fake_user"),
            Account::Type::Evernote, 400);

        LocalStorageManager::StartupOptions startupOptions(
            LocalStorageManager::StartupOption::ClearDatabase);

        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(
            account, startupOptions, this);

        m_pLocalStorageManagerAsync->init();

        TagModelTestHelper tagModelTestHelper(m_pLocalStorageManagerAsync);

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

        Q_UNUSED(loop.exec())
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
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;

        Account account(
            QStringLiteral("ModelTester_notebook_model_test_fake_user"),
            Account::Type::Evernote, 500);

        LocalStorageManager::StartupOptions startupOptions(
            LocalStorageManager::StartupOption::ClearDatabase);

        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(
            account, startupOptions, this);

        m_pLocalStorageManagerAsync->init();

        NotebookModelTestHelper notebookModelTestHelper(
            m_pLocalStorageManagerAsync);

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

        Q_UNUSED(loop.exec())
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
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;

        Account account(
            QStringLiteral("ModelTester_note_model_test_fake_user"),
            Account::Type::Evernote, 700);

        LocalStorageManager::StartupOptions startupOptions(
            LocalStorageManager::StartupOption::ClearDatabase);

        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(
            account, startupOptions, this);

        m_pLocalStorageManagerAsync->init();

        NoteModelTestHelper noteModelTestHelper(m_pLocalStorageManagerAsync);

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

        Q_UNUSED(loop.exec())
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
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageManagerAsync;

        Account account(
            QStringLiteral("ModelTester_favorites_model_test_fake_user"),
            Account::Type::Evernote, 800);

        LocalStorageManager::StartupOptions startupOptions(
            LocalStorageManager::StartupOption::ClearDatabase);

        m_pLocalStorageManagerAsync = new quentier::LocalStorageManagerAsync(
            account, startupOptions, this);

        m_pLocalStorageManagerAsync->init();

        FavoritesModelTestHelper favoritesModelTestHelper(
            m_pLocalStorageManagerAsync);

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

        Q_UNUSED(loop.exec())
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
    item.setLocalUid(UidGenerator::Generate());
    item.setName(QStringLiteral("Test item"));
    item.setGuid(UidGenerator::Generate());
    item.setLinkedNotebookGuid(UidGenerator::Generate());
    item.setDirty(true);
    item.setSynchronizable(false);
    item.setGuid(UidGenerator::Generate());
    item.setParentLocalUid(parentTagItem.localUid());
    item.setParentGuid(parentTagItem.guid());
    item.setParent(&parentTagItem);

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << item;

    QDataStream in(&encodedItem, QIODevice::ReadOnly);
    qint32 type = 0;
    in >> type;

    QVERIFY(type == static_cast<int>(ITagModelItem::Type::Tag));

    TagItem restoredItem;
    in >> restoredItem;

    QVERIFY(restoredItem.localUid() == item.localUid());
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
