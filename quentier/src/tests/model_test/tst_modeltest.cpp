/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Modified by Dmitry Ivanov to support both Qt 4.8 and Qt 5.5+ versions + add tests for Quentier's own models
// in addition to some tests for Qt's built-in models

#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
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

#include "modeltest.h"
#include "dynamictreemodel.h"

#include "../../models/SavedSearchModel.h"
#include "../../models/TagModel.h"
#include "SavedSearchModelTestHelper.h"
#include "TagModelTestHelper.h"
#include "NotebookModelTestHelper.h"
#include "NoteModelTestHelper.h"
#include "FavoritesModelTestHelper.h"

// 10 minutes should be enough
#define MAX_ALLOWED_MILLISECONDS 600000

class tst_ModelTest : public QObject
{
    Q_OBJECT
public:
    tst_ModelTest();
    virtual ~tst_ModelTest() {}

public Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private Q_SLOTS:
    void stringListModel();
    void treeWidgetModel();
    void standardItemModel();
    void testInsertThroughProxy();
    void moveSourceItems();
    void testResetThroughProxy();

    void testSavedSearchModel();
    void testTagModel();
    void testNotebookModel();
    void testNoteModel();
    void testFavoritesModel();
    void testTagModelItemSerialization();

private:
    quentier::LocalStorageManagerThreadWorker  * m_pLocalStorageWorker;
};

tst_ModelTest::tst_ModelTest() :
    m_pLocalStorageWorker(Q_NULLPTR)
{}

void tst_ModelTest::initTestCase()
{}

void tst_ModelTest::cleanupTestCase()
{}

void tst_ModelTest::init()
{
}

void tst_ModelTest::cleanup()
{
}
/*
  tests
*/

void tst_ModelTest::stringListModel()
{
    QStringListModel model;
    QSortFilterProxyModel proxy;

    ModelTest t1(&model);
    ModelTest t2(&proxy);

    proxy.setSourceModel(&model);

    model.setStringList(QStringList() << "2" << "3" << "1");
    model.setStringList(QStringList() << "a" << "e" << "plop" << "b" << "c" );

    proxy.setDynamicSortFilter(true);
    proxy.setFilterRegExp(QRegExp("[^b]"));
}

void tst_ModelTest::treeWidgetModel()
{
    QTreeWidget widget;

    ModelTest t1(widget.model());

    QTreeWidgetItem *root = new QTreeWidgetItem(&widget, QStringList("root"));
    for (int i = 0; i < 20; ++i) {
        new QTreeWidgetItem(root, QStringList(QString::number(i)));
    }
    QTreeWidgetItem *remove = root->child(2);
    root->removeChild(remove);
    QTreeWidgetItem *parent = new QTreeWidgetItem(&widget, QStringList("parent"));
    new QTreeWidgetItem(parent, QStringList("child"));
    widget.setItemHidden(parent, true);

    widget.sortByColumn(0);
}

void tst_ModelTest::standardItemModel()
{
    QStandardItemModel model(10,10);
    QSortFilterProxyModel proxy;


    ModelTest t1(&model);
    ModelTest t2(&proxy);

    proxy.setSourceModel(&model);

    model.insertRows(2, 5);
    model.removeRows(4, 5);

    model.insertColumns(2, 5);
    model.removeColumns(4, 5);

    model.insertRows(0,5, model.index(1,1));
    model.insertColumns(0,5, model.index(1,3));
}

void tst_ModelTest::testInsertThroughProxy()
{
    DynamicTreeModel *model = new DynamicTreeModel(this);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);

    new ModelTest(proxy, this);

    ModelInsertCommand *insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setNumCols(4);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    // Parent is QModelIndex()
    insertCommand->doCommand();

    insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setNumCols(4);
    insertCommand->setAncestorRowNumbers(QList<int>() << 5);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(9);
    insertCommand->doCommand();

    ModelMoveCommand *moveCommand = new ModelMoveCommand(model, this);
    moveCommand->setNumCols(4);
    moveCommand->setStartRow(0);
    moveCommand->setEndRow(0);
    moveCommand->setDestRow(9);
    moveCommand->setDestAncestors(QList<int>() << 5);
    moveCommand->doCommand();
}

/**
  Makes the persistent index list publicly accessible
*/
class AccessibleProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    AccessibleProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent) {}

    QModelIndexList persistent()
    {
        return persistentIndexList();
    }
};

class ObservingObject : public QObject
{
    Q_OBJECT
public:
    ObservingObject(AccessibleProxyModel  *proxy, QObject *parent = 0)
    : QObject(parent)
    , m_proxy(proxy)
    , storePersistentFailureCount(0)
    , checkPersistentFailureCount(0)
    {
        connect(m_proxy, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)), SLOT(storePersistent()));
        connect(m_proxy, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), SLOT(checkPersistent()));
    }

public slots:

    void storePersistent(const QModelIndex &parent)
    {
        for (int row = 0; row < m_proxy->rowCount(parent); ++row) {
            QModelIndex proxyIndex = m_proxy->index(row, 0, parent);
            QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
            if (!proxyIndex.isValid()) {
                qWarning("%s: Invalid proxy index", Q_FUNC_INFO);
                ++storePersistentFailureCount;
            }
            if (!sourceIndex.isValid()) {
                qWarning("%s: invalid source index", Q_FUNC_INFO);
                ++storePersistentFailureCount;
            }
            m_persistentSourceIndexes.append(sourceIndex);
            m_persistentProxyIndexes.append(proxyIndex);
            if (m_proxy->hasChildren(proxyIndex))
                storePersistent(proxyIndex);
        }
    }

    void storePersistent()
    {
        // This method is called from rowsAboutToBeMoved. Persistent indexes should be valid
        foreach(const QModelIndex &idx, m_persistentProxyIndexes)
            if (!idx.isValid()) {
                qWarning("%s: persistentProxyIndexes contains invalid index", Q_FUNC_INFO);
                ++storePersistentFailureCount;
            }

        if (!m_proxy->persistent().isEmpty()) {
            qWarning("%s: proxy should have no persistent indexes when storePersistent called",
                     Q_FUNC_INFO);
            ++storePersistentFailureCount;
        }
        storePersistent(QModelIndex());
        if (m_proxy->persistent().isEmpty()) {
            qWarning("%s: proxy should have persistent index after storePersistent called",
                     Q_FUNC_INFO);
            ++storePersistentFailureCount;
        }
    }

    void checkPersistent()
    {
        for (int row = 0; row < m_persistentProxyIndexes.size(); ++row) {
            QModelIndex updatedProxy = m_persistentProxyIndexes.at(row);
            QModelIndex updatedSource = m_persistentSourceIndexes.at(row);
            Q_UNUSED(updatedProxy)
            Q_UNUSED(updatedSource)
        }
        for (int row = 0; row < m_persistentProxyIndexes.size(); ++row) {
            QModelIndex updatedProxy = m_persistentProxyIndexes.at(row);
            QModelIndex updatedSource = m_persistentSourceIndexes.at(row);
            if (m_proxy->mapToSource(updatedProxy) != updatedSource) {
                qWarning("%s: check failed at row %d", Q_FUNC_INFO, row);
                ++checkPersistentFailureCount;
            }
        }
        m_persistentSourceIndexes.clear();
        m_persistentProxyIndexes.clear();
    }

private:
    AccessibleProxyModel  *m_proxy;
    QList<QPersistentModelIndex> m_persistentSourceIndexes;
    QList<QPersistentModelIndex> m_persistentProxyIndexes;
public:
    int storePersistentFailureCount;
    int checkPersistentFailureCount;
};

void tst_ModelTest::moveSourceItems()
{
    DynamicTreeModel *model = new DynamicTreeModel(this);
    AccessibleProxyModel *proxy = new AccessibleProxyModel(this);
    proxy->setSourceModel(model);

    ModelInsertCommand *insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(2);
    insertCommand->doCommand();

    insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setAncestorRowNumbers(QList<int>() << 1);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(2);
    insertCommand->doCommand();

    ObservingObject observer(proxy);

    ModelMoveCommand *moveCommand = new ModelMoveCommand(model, this);
    moveCommand->setStartRow(0);
    moveCommand->setEndRow(0);
    moveCommand->setDestAncestors(QList<int>() << 1);
    moveCommand->setDestRow(0);
    moveCommand->doCommand();

    QCOMPARE(observer.storePersistentFailureCount, 0);
    QCOMPARE(observer.checkPersistentFailureCount, 0);
}

void tst_ModelTest::testResetThroughProxy()
{
    DynamicTreeModel *model = new DynamicTreeModel(this);

    ModelInsertCommand *insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(2);
    insertCommand->doCommand();

    QPersistentModelIndex persistent = model->index(0, 0);

    AccessibleProxyModel *proxy = new AccessibleProxyModel(this);
    proxy->setSourceModel(model);

    ObservingObject observer(proxy);
    observer.storePersistent();

    ModelResetCommand *resetCommand = new ModelResetCommand(model, this);
    resetCommand->setNumCols(0);
    resetCommand->doCommand();

    QCOMPARE(observer.storePersistentFailureCount, 0);
    QCOMPARE(observer.checkPersistentFailureCount, 0);
}

void tst_ModelTest::testSavedSearchModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageWorker;
        m_pLocalStorageWorker = new quentier::LocalStorageManagerThreadWorker("tst_ModelTest_saved_search_model_test_fake_user", 300,
                                                                               /* start from scratch = */ true, /* override lock = */ false,
                                                                               this);
        m_pLocalStorageWorker->init();

        SavedSearchModelTestHelper savedSearchModelTestHelper(m_pLocalStorageWorker);

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

void tst_ModelTest::testTagModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageWorker;
        m_pLocalStorageWorker = new quentier::LocalStorageManagerThreadWorker("tst_ModelTest_tag_model_test_fake_user", 400,
                                                                               /* start from scratch = */ true,
                                                                               /* override lock = */ false, this);
        m_pLocalStorageWorker->init();

        TagModelTestHelper tagModelTestHelper(m_pLocalStorageWorker);

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

void tst_ModelTest::testNotebookModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageWorker;
        m_pLocalStorageWorker = new quentier::LocalStorageManagerThreadWorker("tst_ModelTest_notebook_model_test_fake_user", 500,
                                                                               /* start from scratch = */ true,
                                                                               /* override lock = */ false, this);
        m_pLocalStorageWorker->init();

        NotebookModelTestHelper notebookModelTestHelper(m_pLocalStorageWorker);

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

void tst_ModelTest::testNoteModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageWorker;
        m_pLocalStorageWorker = new quentier::LocalStorageManagerThreadWorker("tst_ModelTest_note_model_test_fake_user", 700,
                                                                               /* start from scratch = */ true,
                                                                               /* override lock = */ false, this);
        m_pLocalStorageWorker->init();

        NoteModelTestHelper noteModelTestHelper(m_pLocalStorageWorker);

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

void tst_ModelTest::testFavoritesModel()
{
    using namespace quentier;

    int res = -1;
    {
        QTimer timer;
        timer.setInterval(MAX_ALLOWED_MILLISECONDS);
        timer.setSingleShot(true);

        delete m_pLocalStorageWorker;
        m_pLocalStorageWorker = new quentier::LocalStorageManagerThreadWorker("tst_ModelTest_favorites_model_test_fake_user", 800,
                                                                               /* start from scratch = */ true,
                                                                               /* override lock = */ false, this);
        m_pLocalStorageWorker->init();

        FavoritesModelTestHelper favoritesModelTestHelper(m_pLocalStorageWorker);

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

void tst_ModelTest::testTagModelItemSerialization()
{
    using namespace quentier;

    TagModelItem parentItem(UidGenerator::Generate(), UidGenerator::Generate());

    TagModelItem item;
    item.setLocalUid(UidGenerator::Generate());
    item.setName("Test item");
    item.setGuid(UidGenerator::Generate());
    item.setLinkedNotebookGuid(UidGenerator::Generate());
    item.setDirty(true);
    item.setSynchronizable(false);
    item.setGuid(UidGenerator::Generate());
    item.setParentLocalUid(parentItem.localUid());
    item.setParentGuid(parentItem.guid());
    item.setParent(&parentItem);

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << item;

    QDataStream in(&encodedItem, QIODevice::ReadOnly);
    TagModelItem restoredItem;
    in >> restoredItem;

    QVERIFY2(restoredItem.localUid() == item.localUid(), qPrintable("Local uids of original and deserialized items don't match"));
    QVERIFY2(restoredItem.guid() == item.guid(), qPrintable("Guids of original and deserialized items don't match"));
    QVERIFY2(restoredItem.linkedNotebookGuid() == item.linkedNotebookGuid(), qPrintable("Linked notebook guids of original and deserialized items don't match"));
    QVERIFY2(restoredItem.name() == item.name(), qPrintable("Names of original and deserialized items don't match"));
    QVERIFY2(restoredItem.parentGuid() == item.parentGuid(), qPrintable("Parent guids of original and deserialized items don't match"));
    QVERIFY2(restoredItem.parentLocalUid() == item.parentLocalUid(), qPrintable("Parent local uids of original and deserialized items don't match"));
    QVERIFY2(restoredItem.isSynchronizable() == item.isSynchronizable(), qPrintable("Synchronizable flags of original and deserialized items don't match"));
    QVERIFY2(restoredItem.isDirty() == item.isDirty(), qPrintable("Dirty flags of original and deserialized items don't match"));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    tst_ModelTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_modeltest.moc"
