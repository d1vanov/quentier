/*
 * Copyright 2016 Dmitry Ivanov
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

#include "MainWindow.h"
#include "color-picker-tool-button/ColorPickerToolButton.h"
#include "insert-table-tool-button/InsertTableToolButton.h"
#include "insert-table-tool-button/TableSettingsDialog.h"
#include "tests/ManualTestingHelper.h"
#include "widgets/FindAndReplaceWidget.h"
#include "delegates/NotebookItemDelegate.h"
#include "delegates/SynchronizableColumnDelegate.h"
#include "delegates/DirtyColumnDelegate.h"
#include "delegates/FavoriteItemDelegate.h"
#include "delegates/FromLinkedNotebookColumnDelegate.h"
#include "models/ColumnChangeRerouter.h"
#include "views/TableView.h"
#include "views/TreeView.h"
#include <quentier/note_editor/NoteEditor.h>
#include "ui_MainWindow.h"
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Resource.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/logging/QuentierLogger.h>
#include <cmath>
#include <QPushButton>
#include <QIcon>
#include <QLabel>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextList>
#include <QColorDialog>
#include <QFile>
#include <QScopedPointer>
#include <QMessageBox>
#include <QtDebug>
#include <QFontDatabase>
#include <QKeySequence>
#include <QUndoStack>
#include <QCryptographicHash>
#include <QXmlStreamWriter>
#include <QDateTime>

#define NOTIFY_ERROR(error) \
    QNWARNING(error); \
    onSetStatusBarText(error)

using namespace quentier;

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow),
    m_currentStatusBarChildWidget(Q_NULLPTR),
    m_lastNoteEditorHtml(),
    m_availableAccountsActionGroup(new QActionGroup(this)),
    m_pAccountManager(new AccountManager(this)),
    m_pAccount(),
    m_pLocalStorageManagerThread(Q_NULLPTR),
    m_pLocalStorageManager(Q_NULLPTR),
    m_lastLocalStorageSwitchUserRequest(),
    m_pSynchronizationManagerThread(Q_NULLPTR),
    m_pSynchronizationManager(Q_NULLPTR),
    m_synchronizationManagerHost(),
    m_notebookCache(),
    m_tagCache(),
    m_savedSearchCache(),
    m_noteCache(),
    m_pNotebookModel(Q_NULLPTR),
    m_pTagModel(Q_NULLPTR),
    m_pSavedSearchModel(Q_NULLPTR),
    m_pNoteModel(Q_NULLPTR),
    m_pNotebookModelColumnChangeRerouter(new ColumnChangeRerouter(NotebookModel::Columns::NumNotesPerNotebook,
                                                                  NotebookModel::Columns::Name, this)),
    m_pTagModelColumnChangeRerouter(new ColumnChangeRerouter(TagModel::Columns::NumNotesPerTag,
                                                             TagModel::Columns::Name, this)),
    m_pDeletedNotesModel(Q_NULLPTR),
    m_pFavoritesModel(Q_NULLPTR),
    m_blankModel(),
    m_testNotebook(),
    m_testNote(),
    m_pUndoStack(new QUndoStack(this)),
    m_shortcutManager(this)
{
    QNTRACE(QStringLiteral("MainWindow constructor"));

    checkThemeIconsAndSetFallbacks();

    m_pUI->setupUi(this);

    m_availableAccountsActionGroup->setExclusive(true);

    setupAccountManager();
    m_pAccount.reset(new Account(m_pAccountManager->currentAccount()));

    setupLocalStorageManager();
    setupModels();
    setupViews();

    setupDefaultShortcuts();
    setupUserShortcuts();

    addMenuActionsToMainWindow();

    connectActionsToSlots();

    // Stuff primarily for manual testing
    QObject::connect(m_pUI->ActionShowNoteSource, QNSIGNAL(QAction, triggered),
                     this, QNSLOT(MainWindow, onShowNoteSource));
    QObject::connect(m_pUI->ActionSetTestNoteWithEncryptedData, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSetTestNoteWithEncryptedData));
    QObject::connect(m_pUI->ActionSetTestNoteWithResources, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSetTestNoteWithResources));
    QObject::connect(m_pUI->ActionSetTestReadOnlyNote, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSetTestReadOnlyNote));
    QObject::connect(m_pUI->ActionSetTestInkNote, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSetInkNote));

    QString consumerKey, consumerSecret;
    setupConsumerKeyAndSecret(consumerKey, consumerSecret);
}

MainWindow::~MainWindow()
{
    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
    }

    delete m_pUI;
}

void MainWindow::connectActionsToSlots()
{
    QObject::connect(m_pUI->ActionFindInsideNote, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onFindInsideNoteAction));
    QObject::connect(m_pUI->ActionFindNext, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onFindInsideNoteAction));
    QObject::connect(m_pUI->ActionFindPrevious, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onFindPreviousInsideNoteAction));
    QObject::connect(m_pUI->ActionReplaceInNote, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onReplaceInsideNoteAction));

    // Undo/redo actions
    QObject::connect(m_pUI->ActionUndo, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onUndoAction));
    QObject::connect(m_pUI->ActionRedo, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onRedoAction));
    // Copy/cut/paste actions
    QObject::connect(m_pUI->ActionCopy, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onCopyAction));
    QObject::connect(m_pUI->ActionCut, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onCutAction));
    QObject::connect(m_pUI->ActionPaste, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onPasteAction));
    // Select all action
    QObject::connect(m_pUI->ActionSelectAll, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextSelectAllToggled));
    // Font actions
    QObject::connect(m_pUI->ActionFontBold, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextBoldToggled));
    QObject::connect(m_pUI->ActionFontItalic, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextItalicToggled));
    QObject::connect(m_pUI->ActionFontUnderlined, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextUnderlineToggled));
    QObject::connect(m_pUI->ActionFontStrikethrough, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextStrikethroughToggled));
    QObject::connect(m_pUI->ActionIncreaseFontSize, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextIncreaseFontSizeAction));
    QObject::connect(m_pUI->ActionDecreaseFontSize, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextDecreaseFontSizeAction));
    QObject::connect(m_pUI->ActionFontHighlight, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextHighlightAction));
    // Spell checking
    QObject::connect(m_pUI->ActionSpellCheck, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextSpellCheckToggled));
    // Format actions
    QObject::connect(m_pUI->ActionAlignLeft, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextAlignLeftAction));
    QObject::connect(m_pUI->ActionAlignCenter, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextAlignCenterAction));
    QObject::connect(m_pUI->ActionAlignRight, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextAlignRightAction));
    QObject::connect(m_pUI->ActionInsertHorizontalLine, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextAddHorizontalLineAction));
    QObject::connect(m_pUI->ActionIncreaseIndentation, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextIncreaseIndentationAction));
    QObject::connect(m_pUI->ActionDecreaseIndentation, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextDecreaseIndentationAction));
    QObject::connect(m_pUI->ActionInsertBulletedList, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextInsertUnorderedListAction));
    QObject::connect(m_pUI->ActionInsertNumberedList, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextInsertOrderedListAction));
    QObject::connect(m_pUI->ActionInsertTable, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextInsertTableDialogAction));
    QObject::connect(m_pUI->ActionEditHyperlink, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextEditHyperlinkAction));
    QObject::connect(m_pUI->ActionCopyHyperlink, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextCopyHyperlinkAction));
    QObject::connect(m_pUI->ActionRemoveHyperlink, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextRemoveHyperlinkAction));
}

void MainWindow::addMenuActionsToMainWindow()
{
    QNDEBUG(QStringLiteral("MainWindow::addMenuActionsToMainWindow"));

    // NOTE: adding the actions from the menu bar's menus is required for getting the shortcuts
    // of these actions to work properly; action shortcuts only fire when the menu is shown
    // which is not really the purpose behind those shortcuts
    QList<QMenu*> menus = m_pUI->menuBar->findChildren<QMenu*>();
    const int numMenus = menus.size();
    for(int i = 0; i < numMenus; ++i)
    {
        QMenu * menu = menus[i];

        QList<QAction*> actions = menu->actions();
        const int numActions = actions.size();
        for(int j = 0; j < numActions; ++j) {
            QAction * action = actions[j];
            addAction(action);
        }
    }

    QMenu * switchAccountSubMenu = new QMenu(tr("Switch account"));
    QAction * separatorAction = m_pUI->menuFile->insertSeparator(m_pUI->ActionQuit);
    QAction * switchAccountSubMenuAction = m_pUI->menuFile->insertMenu(separatorAction, switchAccountSubMenu);
    Q_UNUSED(m_pUI->menuFile->insertSeparator(switchAccountSubMenuAction));

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();

    int numAvailableAccounts = availableAccounts.size();
    for(int i = 0; i < numAvailableAccounts; ++i)
    {
        const Account & availableAccount = availableAccounts[i];
        QString availableAccountRepresentationName = availableAccount.name();
        if (availableAccount.type() == Account::Type::Local) {
            availableAccountRepresentationName += QStringLiteral(" (");
            availableAccountRepresentationName += tr("local");
            availableAccountRepresentationName += QStringLiteral(")");
        }

        QAction * accountAction = new QAction(availableAccountRepresentationName, Q_NULLPTR);
        switchAccountSubMenu->addAction(accountAction);

        accountAction->setData(i);

        accountAction->setCheckable(true);
        addAction(accountAction);
        QObject::connect(accountAction, QNSIGNAL(QAction,toggled,bool),
                         this, QNSLOT(MainWindow,onSwitchAccountActionToggled,bool));

        m_availableAccountsActionGroup->addAction(accountAction);
    }

    if (Q_LIKELY(numAvailableAccounts != 0)) {
        Q_UNUSED(switchAccountSubMenu->addSeparator())
    }

    QAction * addAccountAction = switchAccountSubMenu->addAction(tr("Add account"));
    addAction(addAccountAction);
    QObject::connect(addAccountAction, QNSIGNAL(QAction,triggered,bool),
                     this, QNSLOT(MainWindow,onAddAccountActionTriggered,bool));

    QAction * manageAccountsAction = switchAccountSubMenu->addAction(tr("Manage accounts"));
    addAction(manageAccountsAction);
    QObject::connect(manageAccountsAction, QNSIGNAL(QAction,triggered,bool),
                     this, QNSLOT(MainWindow,onManageAccountsActionTriggered,bool));
}

NoteEditorWidget * MainWindow::currentNoteEditor()
{
    QNDEBUG(QStringLiteral("MainWindow::currentNoteEditor"));

    if (Q_UNLIKELY(m_pUI->NoteTabWidget->count() == 0)) {
        QNTRACE(QStringLiteral("No open note editors"));
        return Q_NULLPTR;
    }

    int currentIndex = m_pUI->NoteTabWidget->currentIndex();
    if (Q_UNLIKELY(currentIndex < 0)) {
        QNTRACE(QStringLiteral("No current note editor"));
        return Q_NULLPTR;
    }

    QWidget * currentWidget = m_pUI->NoteTabWidget->widget(currentIndex);
    if (Q_UNLIKELY(!currentWidget)) {
        QNTRACE(QStringLiteral("No current widget"));
        return Q_NULLPTR;
    }

    NoteEditorWidget * noteEditorWidget = qobject_cast<NoteEditorWidget*>(currentWidget);
    if (Q_UNLIKELY(!noteEditorWidget)) {
        QNWARNING(QStringLiteral("Can't cast current note tag widget's widget to note editor widget"));
        return Q_NULLPTR;
    }

    return noteEditorWidget;
}

void MainWindow::connectSynchronizationManager()
{
    QNDEBUG(QStringLiteral("MainWindow::connectSynchronizationManager"));

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNDEBUG(QStringLiteral("No synchronization manager"));
        return;
    }

    // Connect local signals to SynchronizationManager slots
    QObject::connect(this, QNSIGNAL(MainWindow,authenticate),
                     m_pSynchronizationManager, QNSLOT(SynchronizationManager,authenticate));

    // Connect SynchronizationManager signals to local slots
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,failed,QNLocalizedString),
                     this, QNSLOT(MainWindow,onSynchronizationManagerFailure,QNLocalizedString));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,finished,Account),
                     this, QNSLOT(MainWindow,onSynchronizationFinished,Account));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,authenticationFinished,bool,QNLocalizedString,Account),
                     this, QNSLOT(MainWindow,onAuthenticationFinished,bool,QNLocalizedString,Account));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,authenticationRevoked,bool,QNLocalizedString,qevercloud::UserID),
                     this, QNSLOT(MainWindow,onAuthenticationRevoked,bool,QNLocalizedString,qevercloud::UserID));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,remoteToLocalSyncStopped),
                     this, QNSLOT(MainWindow,onRemoteToLocalSyncStopped));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,sendLocalChangesStopped),
                     this, QNSLOT(MainWindow,onSendLocalChangesStopped));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,rateLimitExceeded,qint32),
                     this, QNSLOT(MainWindow,onRateLimitExceeded,qint32));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,remoteToLocalSyncDone),
                     this, QNSLOT(MainWindow,onRemoteToLocalSyncDone));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,progress,QNLocalizedString,double),
                     this, QNSLOT(MainWindow,onSynchronizationProgressUpdate,QNLocalizedString,double));
}

void MainWindow::disconnectSynchronizationManager()
{
    QNDEBUG(QStringLiteral("MainWindow::disconnectSynchronizationManager"));

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNDEBUG(QStringLiteral("No synchronization manager"));
        return;
    }

    // Disconnect local signals from SynchronizationManager slots
    QObject::disconnect(this, QNSIGNAL(MainWindow,authenticate),
                        m_pSynchronizationManager, QNSLOT(SynchronizationManager,authenticate));

    // Disonnect SynchronizationManager signals from local slots
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,failed,QNLocalizedString),
                        this, QNSLOT(MainWindow,onSynchronizationManagerFailure,QNLocalizedString));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,finished,Account),
                        this, QNSLOT(MainWindow,onSynchronizationFinished,Account));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,authenticationFinished,bool,QNLocalizedString,Account),
                        this, QNSLOT(MainWindow,onAuthenticationFinished,bool,QNLocalizedString,Account));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,authenticationRevoked,bool,QNLocalizedString,qevercloud::UserID),
                        this, QNSLOT(MainWindow,onAuthenticationRevoked,bool,QNLocalizedString,qevercloud::UserID));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,remoteToLocalSyncStopped),
                        this, QNSLOT(MainWindow,onRemoteToLocalSyncStopped));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,sendLocalChangesStopped),
                        this, QNSLOT(MainWindow,onSendLocalChangesStopped));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,rateLimitExceeded,qint32),
                        this, QNSLOT(MainWindow,onRateLimitExceeded,qint32));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,remoteToLocalSyncDone),
                        this, QNSLOT(MainWindow,onRemoteToLocalSyncDone));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,progress,QNLocalizedString,double),
                        this, QNSLOT(MainWindow,onSynchronizationProgressUpdate,QNLocalizedString,double));
}

void MainWindow::onSyncStopped()
{
    onSetStatusBarText(tr("Synchronization was stopped"));
}

void MainWindow::prepareTestNoteWithResources()
{
    QNDEBUG(QStringLiteral("MainWindow::prepareTestNoteWithResources"));

    m_testNote = Note();

    QString noteContent = test::ManualTestingHelper::noteContentWithResources();
    m_testNote.setContent(noteContent);

    // Read the first result from qrc
    QFile resourceFile(QStringLiteral(":/test_notes/Architecture_whats_the_architecture.png"));
    Q_UNUSED(resourceFile.open(QIODevice::ReadOnly))
    QByteArray resourceData = resourceFile.readAll();
    resourceFile.close();

    // Assemble the first resource data
    quentier::Resource resource;
    resource.setLocalUid(QStringLiteral("{e2f201df-8718-499b-ac92-4c9970170cba}"));
    resource.setNoteLocalUid(m_testNote.localUid());
    resource.setDataBody(resourceData);
    resource.setDataSize(resourceData.size());
    resource.setDataHash(QCryptographicHash::hash(resource.dataBody(), QCryptographicHash::Md5));
    resource.setNoteLocalUid(m_testNote.localUid());
    resource.setMime(QStringLiteral("image/png"));

    qevercloud::ResourceAttributes resourceAttributes;
    resourceAttributes.fileName = QStringLiteral("Architecture_whats_the_architecture.png");

    resource.setResourceAttributes(resourceAttributes);

    // Gather the recognition data as well
    resourceFile.setFileName(QStringLiteral(":/test_notes/Architecture_whats_the_architecture_recognition_data.xml"));
    resourceFile.open(QIODevice::ReadOnly);
    QByteArray resourceRecognitionData = resourceFile.readAll();
    resourceFile.close();

    resource.setRecognitionDataBody(resourceRecognitionData);
    resource.setRecognitionDataSize(resourceRecognitionData.size());
    resource.setRecognitionDataHash(QCryptographicHash::hash(resourceRecognitionData, QCryptographicHash::Md5).toHex());

    // Add the first resource to the note
    m_testNote.addResource(resource);

    // Read the second resource from qrc
    resourceFile.setFileName(QStringLiteral(":/test_notes/cDock_v8.2.zip"));
    resourceFile.open(QIODevice::ReadOnly);
    resourceData = resourceFile.readAll();
    resourceFile.close();

    // Assemble the second resource data
    resource = quentier::Resource();
    resource.setLocalUid(QStringLiteral("{c3acdcba-d6a4-407d-a85f-5fc3c15126df}"));
    resource.setNoteLocalUid(m_testNote.localUid());
    resource.setDataBody(resourceData);
    resource.setDataSize(resourceData.size());
    resource.setDataHash(QCryptographicHash::hash(resource.dataBody(), QCryptographicHash::Md5));
    resource.setMime(QStringLiteral("application/zip"));

    resourceAttributes = qevercloud::ResourceAttributes();
    resourceAttributes.fileName = QStringLiteral("cDock_v8.2.zip");

    resource.setResourceAttributes(resourceAttributes);

    // Add the second resource to the note
    m_testNote.addResource(resource);

    // Read the third resource from qrc
    resourceFile.setFileName(QStringLiteral(":/test_notes/GrowlNotify.pkg"));
    resourceFile.open(QIODevice::ReadOnly);
    resourceData = resourceFile.readAll();
    resourceFile.close();

    // Assemble the third resource data
    resource = quentier::Resource();
    resource.setLocalUid(QStringLiteral("{d44d85f4-d4e2-4788-a172-4d477741b233}"));
    resource.setNoteLocalUid(m_testNote.localUid());
    resource.setDataBody(resourceData);
    resource.setDataSize(resourceData.size());
    resource.setDataHash(QCryptographicHash::hash(resource.dataBody(), QCryptographicHash::Md5));
    resource.setMime(QStringLiteral("application/octet-stream"));

    resourceAttributes = qevercloud::ResourceAttributes();
    resourceAttributes.fileName = QStringLiteral("GrowlNotify.pkg");

    resource.setResourceAttributes(resourceAttributes);

    // Add the third resource to the note
    m_testNote.addResource(resource);
}

void MainWindow::prepareTestInkNote()
{
    QNDEBUG(QStringLiteral("MainWindow::prepareTestInkNote"));

    m_testNote = Note();
    m_testNote.setGuid(QStringLiteral("a458d0ac-5c0b-446d-bc39-5b069148f66a"));

    // Read the ink note image data from qrc
    QFile inkNoteImageQrc(QStringLiteral(":/test_notes/inknoteimage.png"));
    Q_UNUSED(inkNoteImageQrc.open(QIODevice::ReadOnly))
    QByteArray inkNoteImageData = inkNoteImageQrc.readAll();
    inkNoteImageQrc.close();

    quentier::Resource resource;
    resource.setGuid(QStringLiteral("6bdf808c-7bd9-4a39-bef8-20b84779956e"));
    resource.setDataBody(QByteArray("aaa"));
    resource.setDataHash(QByteArray("2e0f79af4ca47b473e5105156a18c7cb"));
    resource.setMime(QStringLiteral("application/vnd.evernote.ink"));
    resource.setHeight(308);
    resource.setWidth(602);
    resource.setNoteGuid(m_testNote.guid());

    m_testNote.addResource(resource);

    QString inkNoteImageFilePath = quentier::applicationPersistentStoragePath() + QStringLiteral("/NoteEditorPage/inkNoteImages");
    QDir inkNoteImageFileDir(inkNoteImageFilePath);
    if (!inkNoteImageFileDir.exists())
    {
        if (Q_UNLIKELY(!inkNoteImageFileDir.mkpath(inkNoteImageFilePath))) {
            onSetStatusBarText(QStringLiteral("Can't set test ink note to the editor: can't create folder to hold the ink note resource images"));
            return;
        }
    }

    inkNoteImageFilePath += QStringLiteral("/") + resource.guid() + QStringLiteral(".png");
    QFile inkNoteImageFile(inkNoteImageFilePath);
    if (Q_UNLIKELY(!inkNoteImageFile.open(QIODevice::WriteOnly))) {
        onSetStatusBarText(QStringLiteral("Can't set test ink note to the editor: can't open file meant to hold the ink note resource image for writing"));
        return;
    }

    inkNoteImageFile.write(inkNoteImageData);
    inkNoteImageFile.close();
}

void MainWindow::onSetStatusBarText(QString message, const int duration)
{
    QStatusBar * pStatusBar = m_pUI->statusBar;

    if (m_currentStatusBarChildWidget != Q_NULLPTR) {
        pStatusBar->removeWidget(m_currentStatusBarChildWidget);
        m_currentStatusBarChildWidget = Q_NULLPTR;
    }

    if (duration == 0) {
        m_currentStatusBarChildWidget = new QLabel(message);
        pStatusBar->addWidget(m_currentStatusBarChildWidget);
    }
    else {
        pStatusBar->showMessage(message, duration);
    }
}

#define DISPATCH_TO_NOTE_EDITOR(MainWindowMethod, NoteEditorMethod) \
    void MainWindow::MainWindowMethod() \
    { \
        QNDEBUG(QStringLiteral("MainWindow::" #MainWindowMethod)); \
        \
        NoteEditorWidget * noteEditorWidget = currentNoteEditor(); \
        if (!noteEditorWidget) { \
            return; \
        } \
        \
        noteEditorWidget->NoteEditorMethod(); \
    }

DISPATCH_TO_NOTE_EDITOR(onUndoAction, onUndoAction)
DISPATCH_TO_NOTE_EDITOR(onRedoAction, onRedoAction)
DISPATCH_TO_NOTE_EDITOR(onCopyAction, onCopyAction)
DISPATCH_TO_NOTE_EDITOR(onCutAction, onCutAction)
DISPATCH_TO_NOTE_EDITOR(onPasteAction, onPasteAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextSelectAllToggled, onSelectAllAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextBoldToggled, onEditorTextBoldToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextItalicToggled, onEditorTextItalicToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextUnderlineToggled, onEditorTextUnderlineToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextStrikethroughToggled, onEditorTextStrikethroughToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignLeftAction, onEditorTextAlignLeftAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignCenterAction, onEditorTextAlignCenterAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignRightAction, onEditorTextAlignRightAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAddHorizontalLineAction, onEditorTextAddHorizontalLineAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextIncreaseFontSizeAction, onEditorTextIncreaseFontSizeAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextDecreaseFontSizeAction, onEditorTextDecreaseFontSizeAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextHighlightAction, onEditorTextHighlightAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextIncreaseIndentationAction, onEditorTextIncreaseIndentationAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextDecreaseIndentationAction, onEditorTextDecreaseIndentationAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertUnorderedListAction, onEditorTextInsertUnorderedListAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertOrderedListAction, onEditorTextInsertOrderedListAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertTableDialogAction, onEditorTextInsertTableDialogRequested)
DISPATCH_TO_NOTE_EDITOR(onNoteTextEditHyperlinkAction, onEditorTextEditHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextCopyHyperlinkAction, onEditorTextCopyHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextRemoveHyperlinkAction, onEditorTextRemoveHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onFindInsideNoteAction, onFindInsideNoteAction)
DISPATCH_TO_NOTE_EDITOR(onFindPreviousInsideNoteAction, onFindPreviousInsideNoteAction)
DISPATCH_TO_NOTE_EDITOR(onReplaceInsideNoteAction, onReplaceInsideNoteAction)

#undef DISPATCH_TO_NOTE_EDITOR

void MainWindow::onSynchronizationManagerFailure(QNLocalizedString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationManagerFailure: ") << errorDescription);
    onSetStatusBarText(errorDescription.localizedString());
}

void MainWindow::onSynchronizationFinished(Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationFinished: ") << account);

    onSetStatusBarText(tr("Synchronization finished!"));

    QNINFO(QStringLiteral("Synchronization finished for user ") << account.name()
           << QStringLiteral(", id ") << account.id());

    // TODO: figure out what to do with the account object now: should anything be done with it?
}

void MainWindow::onAuthenticationFinished(bool success, QNLocalizedString errorDescription,
                                          Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onAuthenticationFinished: success = ")
            << (success ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription
            << QStringLiteral(", account = ") << account);

    if (!success) {
        onSetStatusBarText(tr("Couldn't authenticate the Evernote user") + QStringLiteral(": ") +
                           errorDescription.localizedString());
        return;
    }

    m_pAccountManager->switchAccount(account);
}

void MainWindow::onAuthenticationRevoked(bool success, QNLocalizedString errorDescription,
                                         qevercloud::UserID userId)
{
    QNDEBUG(QStringLiteral("MainWindow::onAuthenticationRevoked: success = ")
            << (success ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription
            << QStringLiteral(", user id = ") << userId);

    if (!success) {
        onSetStatusBarText(tr("Couldn't revoke the authentication") + QStringLiteral(": ") +
                           errorDescription.localizedString());
        return;
    }

    QNINFO(QStringLiteral("Revoked authentication for user with id ") << userId);
}

void MainWindow::onRateLimitExceeded(qint32 secondsToWait)
{
    QNDEBUG(QStringLiteral("MainWindow::onRateLimitExceeded: seconds to wait = ")
            << secondsToWait);

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    qint64 futureTimestamp = currentTimestamp + secondsToWait * 1000;
    QDateTime futureDateTime;
    futureDateTime.setMSecsSinceEpoch(futureTimestamp);

    QDateTime today = QDateTime::currentDateTime();
    bool includeDate = (today.date() != futureDateTime.date());

    QString dateTimeToShow = (includeDate
                              ? futureDateTime.toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"))
                              : futureDateTime.toString(QStringLiteral("hh:mm:ss")));

    onSetStatusBarText(tr("The synchronization has reached the Evernote API rate limit, "
                          "it will continue automatically at approximately") + QStringLiteral(" ") +
                       dateTimeToShow);

    QNINFO(QStringLiteral("Evernote API rate limit exceeded, need to wait for ")
           << secondsToWait << QStringLiteral(" seconds, the synchronization will continue at ")
           << dateTimeToShow);
}

void MainWindow::onRemoteToLocalSyncDone()
{
    QNDEBUG(QStringLiteral("MainWindow::onRemoteToLocalSyncDone"));

    onSetStatusBarText(tr("Received all updates from Evernote servers, sending local changes"));
    QNINFO(QStringLiteral("Remote to local sync done"));
}

void MainWindow::onSynchronizationProgressUpdate(QNLocalizedString message, double workDonePercentage)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationProgressUpdate: message = ")
            << message << QStringLiteral(", work done percentage = ") << workDonePercentage);

    if (Q_UNLIKELY(message.isEmpty())) {
        return;
    }

    bool showProgress = true;
    if (Q_UNLIKELY((workDonePercentage < 0.0) || (workDonePercentage >= 1.0))) {
        showProgress = false;
    }

    workDonePercentage *= 1.0e4;
    workDonePercentage = std::floor(workDonePercentage + 0.5);
    workDonePercentage *= 1.0e-2;

    QString messageToShow = message.localizedString();
    if (showProgress) {
        messageToShow += QStringLiteral(", ");
        messageToShow += tr("progress");
        messageToShow += QStringLiteral(": ");
        messageToShow += QString::number(workDonePercentage);
        messageToShow += QStringLiteral("%");
    }

    onSetStatusBarText(messageToShow);
}

void MainWindow::onRemoteToLocalSyncStopped()
{
    QNDEBUG(QStringLiteral("MainWindow::onRemoteToLocalSyncStopped"));
    onSyncStopped();
}

void MainWindow::onSendLocalChangesStopped()
{
    QNDEBUG(QStringLiteral("MainWindow::onSendLocalChangesStopped"));
    onSyncStopped();
}

void MainWindow::onEvernoteAccountAuthenticationRequested(QString host)
{
    QNDEBUG(QStringLiteral("MainWindow::onEvernoteAccountAuthenticationRequested: host = ") << host);

    if ((host != m_synchronizationManagerHost) || !m_pSynchronizationManager) {
        m_synchronizationManagerHost = host;
        setupSynchronizationManager();
    }

    emit authenticate();
}

void MainWindow::onNoteTextSpellCheckToggled()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteTextSpellCheckToggled"));

    NoteEditorWidget * noteEditorWidget = currentNoteEditor();
    if (!noteEditorWidget) {
        return;
    }

    if (noteEditorWidget->isSpellCheckEnabled()) {
        noteEditorWidget->onEditorSpellCheckStateChanged(Qt::Unchecked);
    }
    else {
        noteEditorWidget->onEditorSpellCheckStateChanged(Qt::Checked);
    }
}

void MainWindow::onShowNoteSource()
{
    QNDEBUG(QStringLiteral("MainWindow::onShowNoteSource"));

    NoteEditorWidget * noteEditorWidget = currentNoteEditor();
    if (!noteEditorWidget) {
        return;
    }

    if (!noteEditorWidget->isNoteSourceShown()) {
        noteEditorWidget->showNoteSource();
    }
    else {
        noteEditorWidget->hideNoteSource();
    }
}

void MainWindow::onSetTestNoteWithEncryptedData()
{
    QNDEBUG(QStringLiteral("MainWindow::onSetTestNoteWithEncryptedData"));

    m_testNote = Note();
    m_testNote.setLocalUid(QStringLiteral("{7ae26137-9b62-4c30-85a9-261b435f6db3}"));

    QString noteContent = test::ManualTestingHelper::noteContentWithEncryption();
    m_testNote.setContent(noteContent);

    // TODO: set this note to editor, in new tab or replace existing tab (if any)
}

void MainWindow::onSetTestNoteWithResources()
{
    QNDEBUG(QStringLiteral("MainWindow::onSetTestNoteWithResources"));

    prepareTestNoteWithResources();

    m_testNote.setLocalUid(QStringLiteral("{ce8e5ea1-28fc-4842-a726-0d4a78dfcbe6}"));
    m_testNotebook.setCanUpdateNotes(true);

    // TODO: set this note to editor, in new tab or replace existing tab (if any)
}

void MainWindow::onSetTestReadOnlyNote()
{
    prepareTestNoteWithResources();

    m_testNote.setLocalUid(QStringLiteral("{ce8e5ea1-28fc-4842-a726-0d4a78dfcbe5}"));
    m_testNotebook.setCanUpdateNotes(false);

    // TODO: set this note to editor, in new tab or replace existing tab (if any)
}

void MainWindow::onSetInkNote()
{
    prepareTestInkNote();

    m_testNote.setLocalUid(QStringLiteral("{96c747e2-7bdc-4805-a704-105cbfcc7fbe}"));
    m_testNotebook.setCanUpdateNotes(true);

    // TODO: set this note to editor, in new tab or replace existing tab (if any)
}

void MainWindow::onNoteEditorError(QNLocalizedString error)
{
    QNINFO(QStringLiteral("MainWindow::onNoteEditorError: ") << error);

    NoteEditorWidget * noteEditor = qobject_cast<NoteEditorWidget*>(sender());
    if (!noteEditor) {
        QNTRACE(QStringLiteral("Can't cast caller to note editor widget, skipping"));
        return;
    }

    NoteEditorWidget * currentEditor = currentNoteEditor();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(QStringLiteral("Not an update from current note editor, skipping"));
        return;
    }

    onSetStatusBarText(error.localizedString(), 20000);
}

void MainWindow::onNoteEditorSpellCheckerNotReady()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorSpellCheckerNotReady"));

    NoteEditorWidget * noteEditor = qobject_cast<NoteEditorWidget*>(sender());
    if (!noteEditor) {
        QNTRACE(QStringLiteral("Can't cast caller to note editor widget, skipping"));
        return;
    }

    NoteEditorWidget * currentEditor = currentNoteEditor();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(QStringLiteral("Not an update from current note editor, skipping"));
        return;
    }

    onSetStatusBarText(tr("Spell checker is loading dictionaries, please wait"));
}

void MainWindow::onNoteEditorSpellCheckerReady()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorSpellCheckerReady"));

    NoteEditorWidget * noteEditor = qobject_cast<NoteEditorWidget*>(sender());
    if (!noteEditor) {
        QNTRACE(QStringLiteral("Can't cast caller to note editor widget, skipping"));
        return;
    }

    NoteEditorWidget * currentEditor = currentNoteEditor();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(QStringLiteral("Not an update from current note editor, skipping"));
        return;
    }

    onSetStatusBarText(QString());
}

void MainWindow::onAddAccountActionTriggered(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onAddAccountActionTriggered"));

    Q_UNUSED(checked)

    m_pAccountManager->raiseAddAccountDialog();
}

void MainWindow::onManageAccountsActionTriggered(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onManageAccountsActionTriggered"));

    Q_UNUSED(checked)

    m_pAccountManager->raiseManageAccountsDialog();
}

void MainWindow::onSwitchAccountActionToggled(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchAccountActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!checked) {
        QNTRACE(QStringLiteral("Ignoring the unchecking of account"));
        return;
    }

    QAction * action = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!action)) {
        NOTIFY_ERROR(QT_TR_NOOP("Internal error: account switching action is unexpectedly null"));
        return;
    }

    QVariant indexData = action->data();
    bool conversionResult = false;
    int index = indexData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        NOTIFY_ERROR(QT_TR_NOOP("Internal error: can't get identification data "
                                "from the account switching action"));
        return;
    }

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    if ((index < 0) || (index >= numAvailableAccounts)) {
        NOTIFY_ERROR(QT_TR_NOOP("Internal error: wrong index into available accounts "
                                "in account switching action"));
        return;
    }

    const Account & availableAccount = availableAccounts[index];
    m_pAccountManager->switchAccount(availableAccount);
    // Will continue in slot connected to AccountManager's switchedAccount signal
}

void MainWindow::onAccountSwitched(Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onAccountSwitched: ") << account);

    // Now need to ask the local stortage manager to switch the account
    m_lastLocalStorageSwitchUserRequest = QUuid::createUuid();
    emit localStorageSwitchUserRequest(account, /* start from scratch = */ false,
                                       m_lastLocalStorageSwitchUserRequest);
}

void MainWindow::onAccountManagerError(QNLocalizedString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onAccountManagerError: ") << errorDescription);
    onSetStatusBarText(errorDescription.localizedString());
}

void MainWindow::onLocalStorageSwitchUserRequestComplete(Account account, QUuid requestId)
{
    QNDEBUG(QStringLiteral("MainWindow::onLocalStorageSwitchUserRequestComplete: account = ")
            << account << QStringLiteral(", request id = ") << requestId);

    bool expected = (m_lastLocalStorageSwitchUserRequest == requestId);
    m_lastLocalStorageSwitchUserRequest = QUuid();

    if (!expected) {
        NOTIFY_ERROR(QT_TR_NOOP("Local storage user was switched without explicit user action"));
        // Trying to undo it
        m_pAccountManager->switchAccount(*m_pAccount); // This should trigger the switch in local storage as well
        return;
    }

    *m_pAccount = account;

    if (m_pAccount->type() == Account::Type::Local)
    {
        clearSynchronizationManager();
    }
    else
    {
        if (m_synchronizationManagerHost == m_pAccount->evernoteHost())
        {
            if (!m_pSynchronizationManager) {
                setupSynchronizationManager();
            }
        }
        else
        {
            m_synchronizationManagerHost = m_pAccount->evernoteHost();
            setupSynchronizationManager();
        }

        m_pSynchronizationManager->setAccount(*m_pAccount);
    }

    setupModels();
}

void MainWindow::onLocalStorageSwitchUserRequestFailed(Account account, QNLocalizedString errorDescription, QUuid requestId)
{
    bool expected = (m_lastLocalStorageSwitchUserRequest == requestId);
    if (!expected) {
        return;
    }

    QNDEBUG(QStringLiteral("MainWindow::onLocalStorageSwitchUserRequestFailed: ") << account << QStringLiteral("\nError description: ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_lastLocalStorageSwitchUserRequest = QUuid();

    onSetStatusBarText(tr("Could not switch account") + QStringLiteral(": ") + errorDescription.localizedString());

    if (!m_pAccount) {
        // If there was no any account set previously, nothing to do
        return;
    }

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    // Trying to restore the previously selected account as the current one in the UI
    QList<QAction*> availableAccountActions = m_availableAccountsActionGroup->actions();
    for(auto it = availableAccountActions.constBegin(), end = availableAccountActions.constEnd(); it != end; ++it)
    {
        QAction * action = *it;
        if (Q_UNLIKELY(!action)) {
            QNDEBUG(QStringLiteral("Found null pointer to action within the available accounts action group"));
            continue;
        }

        QVariant actionData = action->data();
        bool conversionResult = false;
        int index = actionData.toInt(&conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG(QStringLiteral("Can't convert available account's user data to int: ") << actionData);
            continue;
        }

        if (Q_UNLIKELY((index < 0) || (index >= numAvailableAccounts))) {
            QNDEBUG(QStringLiteral("Available account's index is beyond the range of available accounts: index = ")
                    << index << QStringLiteral(", num available accounts = ") << numAvailableAccounts);
            continue;
        }

        const Account & actionAccount = availableAccounts.at(index);
        if (actionAccount == *m_pAccount) {
            QNDEBUG(QStringLiteral("Restoring the current account in UI: index = ") << index << QStringLiteral(", account = ")
                    << actionAccount);
            action->setChecked(true);
            return;
        }
    }

    // If we got here, it means we haven't found the proper previous account
    QNDEBUG(QStringLiteral("Couldn't find the action corresponding to the previous available account: ") << *m_pAccount);
}

void MainWindow::checkThemeIconsAndSetFallbacks()
{
    QNTRACE(QStringLiteral("MainWindow::checkThemeIconsAndSetFallbacks"));

    if (!QIcon::hasThemeIcon(QStringLiteral("document-open"))) {
        QIcon::setThemeName(QStringLiteral("oxygen"));
    }
}

void MainWindow::setupAccountManager()
{
    QNDEBUG(QStringLiteral("MainWindow::setupAccountManager"));

    QObject::connect(m_pAccountManager, QNSIGNAL(AccountManager,evernoteAccountAuthenticationRequested,QString),
                     this, QNSLOT(MainWindow,onEvernoteAccountAuthenticationRequested,QString));
    QObject::connect(m_pAccountManager, QNSIGNAL(AccountManager,switchedAccount,Account),
                     this, QNSLOT(MainWindow,onAccountSwitched,Account));
    QObject::connect(m_pAccountManager, QNSIGNAL(AccountManager,notifyError,QNLocalizedString),
                     this, QNSLOT(MainWindow,onAccountManagerError,QNLocalizedString));
}

void MainWindow::setupLocalStorageManager()
{
    QNDEBUG(QStringLiteral("MainWindow::setupLocalStorageManager"));

    m_pLocalStorageManagerThread = new QThread;
    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));
    m_pLocalStorageManagerThread->start();

    m_pLocalStorageManager = new LocalStorageManagerThreadWorker(*m_pAccount, /* start from scratch = */ false,
                                                                 /* override lock = */ false);
    m_pLocalStorageManager->init();
    m_pLocalStorageManager->moveToThread(m_pLocalStorageManagerThread);

    QObject::connect(this, QNSIGNAL(MainWindow,localStorageSwitchUserRequest,Account,bool,QUuid),
                     m_pLocalStorageManager, QNSLOT(LocalStorageManagerThreadWorker,onSwitchUserRequest,Account,bool,QUuid));
    QObject::connect(m_pLocalStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,switchUserComplete,Account,QUuid),
                     this, QNSLOT(MainWindow,onLocalStorageSwitchUserRequestComplete,Account,QUuid));
    QObject::connect(m_pLocalStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,switchUserFailed,Account,QNLocalizedString,QUuid),
                     this, QNSLOT(MainWindow,onLocalStorageSwitchUserRequestFailed,Account,QNLocalizedString,QUuid));
}

void MainWindow::setupModels()
{
    QNDEBUG(QStringLiteral("MainWindow::setupModels"));

    clearModels();

    m_pFavoritesModel = new FavoritesModel(*m_pLocalStorageManager, m_noteCache, m_notebookCache, m_tagCache, m_savedSearchCache, this);
    m_pNotebookModel = new NotebookModel(*m_pAccount, *m_pLocalStorageManager, m_notebookCache, this);
    m_pTagModel = new TagModel(*m_pAccount, *m_pLocalStorageManager, m_tagCache, this);
    m_pSavedSearchModel = new SavedSearchModel(*m_pAccount, *m_pLocalStorageManager, m_savedSearchCache, this);
    m_pNoteModel = new NoteModel(*m_pAccount, *m_pLocalStorageManager, m_noteCache, m_notebookCache, this, NoteModel::IncludedNotes::NonDeleted);
    m_pDeletedNotesModel = new NoteModel(*m_pAccount, *m_pLocalStorageManager, m_noteCache, m_notebookCache, this, NoteModel::IncludedNotes::Deleted);

    m_pUI->favoritesTableView->setModel(m_pFavoritesModel);
    m_pUI->notebooksTableView->setModel(m_pNotebookModel);
    m_pUI->tagsTreeView->setModel(m_pTagModel);
    m_pUI->savedSearchesTableView->setModel(m_pSavedSearchModel);
    m_pUI->noteListView->setModel(m_pNoteModel);
    m_pUI->deletedNotesTableView->setModel(m_pDeletedNotesModel);

    m_pNotebookModelColumnChangeRerouter->setModel(m_pNotebookModel);
    m_pTagModelColumnChangeRerouter->setModel(m_pTagModel);
}

void MainWindow::clearModels()
{
    QNDEBUG(QStringLiteral("MainWindow::clearModels"));

    clearViews();

    if (m_pNotebookModel) {
        delete m_pNotebookModel;
        m_pNotebookModel = Q_NULLPTR;
    }

    if (m_pTagModel) {
        delete m_pTagModel;
        m_pTagModel = Q_NULLPTR;
    }

    if (m_pSavedSearchModel) {
        delete m_pSavedSearchModel;
        m_pSavedSearchModel = Q_NULLPTR;
    }

    if (m_pNoteModel) {
        delete m_pNoteModel;
        m_pNoteModel = Q_NULLPTR;
    }

    if (m_pDeletedNotesModel) {
        delete m_pDeletedNotesModel;
        m_pDeletedNotesModel = Q_NULLPTR;
    }

    if (m_pFavoritesModel) {
        delete m_pFavoritesModel;
        m_pFavoritesModel = Q_NULLPTR;
    }
}

void MainWindow::setupViews()
{
    QNDEBUG(QStringLiteral("MainWindow::setupViews"));

    QTableView * favoritesTableView = m_pUI->favoritesTableView;
    favoritesTableView->horizontalHeader()->hide();
    favoritesTableView->setColumnHidden(FavoritesModel::Columns::NumNotesTargeted, true);
    FavoriteItemDelegate * favoriteItemDelegate = new FavoriteItemDelegate(favoritesTableView);
    favoritesTableView->setItemDelegate(favoriteItemDelegate);
    favoritesTableView->setColumnWidth(FavoritesModel::Columns::Type, favoriteItemDelegate->sideSize());

    TableView * notebooksTableView = m_pUI->notebooksTableView;
    NotebookItemDelegate * notebookItemDelegate = new NotebookItemDelegate(notebooksTableView);
    notebooksTableView->setItemDelegate(notebookItemDelegate);
    SynchronizableColumnDelegate * notebookTableViewSynchronizableColumnDelegate =
            new SynchronizableColumnDelegate(notebooksTableView);
    notebooksTableView->setItemDelegateForColumn(NotebookModel::Columns::Synchronizable,
                                                 notebookTableViewSynchronizableColumnDelegate);
    notebooksTableView->setColumnWidth(NotebookModel::Columns::Synchronizable,
                                       notebookTableViewSynchronizableColumnDelegate->sideSize());
    DirtyColumnDelegate * notebookTableViewDirtyColumnDelegate =
            new DirtyColumnDelegate(notebooksTableView);
    notebooksTableView->setItemDelegateForColumn(NotebookModel::Columns::Dirty,
                                                 notebookTableViewDirtyColumnDelegate);
    notebooksTableView->setColumnWidth(NotebookModel::Columns::Dirty,
                                       notebookTableViewDirtyColumnDelegate->sideSize());
    FromLinkedNotebookColumnDelegate * notebookTableViewFromLinkedNotebookColumnDelegate =
            new FromLinkedNotebookColumnDelegate(notebooksTableView);
    notebooksTableView->setItemDelegateForColumn(NotebookModel::Columns::FromLinkedNotebook,
                                                 notebookTableViewFromLinkedNotebookColumnDelegate);
    notebooksTableView->setColumnWidth(NotebookModel::Columns::FromLinkedNotebook,
                                       notebookTableViewFromLinkedNotebookColumnDelegate->sideSize());
    notebooksTableView->horizontalHeader()->hide();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(m_pNotebookModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&),
                     notebooksTableView, QNSLOT(TableView,dataChanged,const QModelIndex&,const QModelIndex&));
#else
    QObject::connect(m_pNotebookModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&),
                     notebooksTableView, QNSLOT(TableView,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&));
#endif

    TreeView * tagsTreeView = m_pUI->tagsTreeView;
    SynchronizableColumnDelegate * tagsTreeViewSynchronizableColumnDelegate =
            new SynchronizableColumnDelegate(tagsTreeView);
    tagsTreeView->setItemDelegateForColumn(TagModel::Columns::Synchronizable,
                                           tagsTreeViewSynchronizableColumnDelegate);
    tagsTreeView->setColumnWidth(TagModel::Columns::Synchronizable,
                                 tagsTreeViewSynchronizableColumnDelegate->sideSize());
    DirtyColumnDelegate * tagsTreeViewDirtyColumnDelegate =
            new DirtyColumnDelegate(tagsTreeView);
    tagsTreeView->setItemDelegateForColumn(TagModel::Columns::Dirty,
                                           tagsTreeViewDirtyColumnDelegate);
    tagsTreeView->setColumnWidth(TagModel::Columns::Dirty,
                                 tagsTreeViewDirtyColumnDelegate->sideSize());
    FromLinkedNotebookColumnDelegate * tagsTreeViewFromLinkedNotebookColumnDelegate =
            new FromLinkedNotebookColumnDelegate(tagsTreeView);
    tagsTreeView->setItemDelegateForColumn(TagModel::Columns::FromLinkedNotebook,
                                           tagsTreeViewFromLinkedNotebookColumnDelegate);
    tagsTreeView->setColumnWidth(TagModel::Columns::FromLinkedNotebook,
                                 tagsTreeViewFromLinkedNotebookColumnDelegate->sideSize());
    tagsTreeView->header()->hide();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(m_pTagModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&),
                     tagsTreeView, QNSLOT(TreeView,dataChanged,const QModelIndex&,const QModelIndex&));
#else
    QObject::connect(m_pTagModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&),
                     tagsTreeView, QNSLOT(TreeView,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&));
#endif

    QTableView * savedSearchesTableView = m_pUI->savedSearchesTableView;
    savedSearchesTableView->setColumnHidden(SavedSearchModel::Columns::Query, true);
    SynchronizableColumnDelegate * savedSearchesTableViewSynchronizableColumnDelegate =
            new SynchronizableColumnDelegate(savedSearchesTableView);
    savedSearchesTableView->setItemDelegateForColumn(SavedSearchModel::Columns::Synchronizable,
                                                     savedSearchesTableViewSynchronizableColumnDelegate);
    savedSearchesTableView->setColumnWidth(SavedSearchModel::Columns::Synchronizable,
                                           savedSearchesTableViewSynchronizableColumnDelegate->sideSize());
    DirtyColumnDelegate * savedSearchesTableViewDirtyColumnDelegate =
            new DirtyColumnDelegate(savedSearchesTableView);
    savedSearchesTableView->setItemDelegateForColumn(SavedSearchModel::Columns::Dirty,
                                                     savedSearchesTableViewDirtyColumnDelegate);
    savedSearchesTableView->setColumnWidth(SavedSearchModel::Columns::Dirty,
                                           savedSearchesTableViewDirtyColumnDelegate->sideSize());
    savedSearchesTableView->horizontalHeader()->hide();

    // TODO: setup m_pUI->noteListView

    m_pUI->deletedNotesTableView->horizontalHeader()->hide();
    // TODO: setup this view further
}

void MainWindow::clearViews()
{
    QNDEBUG(QStringLiteral("MainWindow::clearViews"));

    m_pUI->favoritesTableView->setModel(&m_blankModel);
    m_pUI->notebooksTableView->setModel(&m_blankModel);
    m_pUI->tagsTreeView->setModel(&m_blankModel);
    m_pUI->savedSearchesTableView->setModel(&m_blankModel);

    m_pUI->noteListView->setModel(&m_blankModel);
    m_pUI->deletedNotesTableView->setModel(&m_blankModel);
}

void MainWindow::setupSynchronizationManager()
{
    QNDEBUG(QStringLiteral("MainWindow::setupSynchronizationManager"));

    clearSynchronizationManager();

    if (m_synchronizationManagerHost.isEmpty()) {
        QNDEBUG(QStringLiteral("Host is empty"));
        return;
    }

    QString consumerKey, consumerSecret;
    setupConsumerKeyAndSecret(consumerKey, consumerSecret);

    if (Q_UNLIKELY(consumerKey.isEmpty())) {
        QNDEBUG(QStringLiteral("Consumer key is empty"));
        return;
    }

    if (Q_UNLIKELY(consumerSecret.isEmpty())) {
        QNDEBUG(QStringLiteral("Consumer secret is empty"));
        return;
    }

    m_pSynchronizationManagerThread = new QThread;
    QObject::connect(m_pSynchronizationManagerThread, QNSIGNAL(QThread,finished),
                     m_pSynchronizationManagerThread, QNSLOT(QThread,deleteLater));
    m_pSynchronizationManagerThread->start();

    m_pSynchronizationManager = new SynchronizationManager(consumerKey, consumerSecret,
                                                           m_synchronizationManagerHost,
                                                           *m_pLocalStorageManager);
    m_pSynchronizationManager->moveToThread(m_pSynchronizationManagerThread);
    connectSynchronizationManager();
}

void MainWindow::clearSynchronizationManager()
{
    QNDEBUG(QStringLiteral("MainWindow::clearSynchronizationManager"));

    disconnectSynchronizationManager();

    if (m_pSynchronizationManager) {
        m_pSynchronizationManager->deleteLater();
        m_pSynchronizationManager = Q_NULLPTR;
    }

    if (m_pSynchronizationManagerThread) {
        m_pSynchronizationManagerThread->quit();    // The thread would delete itself after it's finished
        m_pSynchronizationManagerThread = Q_NULLPTR;
    }
}

void MainWindow::setupDefaultShortcuts()
{
    QNDEBUG(QStringLiteral("MainWindow::setupDefaultShortcuts"));

    using quentier::ShortcutManager;

#define PROCESS_ACTION_SHORTCUT(action, key, ...) \
    { \
        QKeySequence shortcut = m_pUI->Action##action->shortcut(); \
        if (shortcut.isEmpty()) { \
            QNTRACE(QStringLiteral("No shortcut was found for action " #action)); \
        } \
        else { \
            m_shortcutManager.setDefaultShortcut(key, shortcut, QStringLiteral(#__VA_ARGS__)); \
        } \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, ...) \
    { \
        QKeySequence shortcut = m_pUI->Action##action->shortcut(); \
        if (shortcut.isEmpty()) { \
            QNTRACE(QStringLiteral("No shortcut was found for action " #action)); \
        } \
        else { \
            m_shortcutManager.setNonStandardDefaultShortcut(#action, shortcut, QStringLiteral(#__VA_ARGS__)); \
        } \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::setupUserShortcuts()
{
    QNDEBUG(QStringLiteral("MainWindow::setupUserShortcuts"));

#define PROCESS_ACTION_SHORTCUT(action, key, ...) \
    { \
        QKeySequence shortcut = m_shortcutManager.shortcut(key, QStringLiteral(#__VA_ARGS__)); \
        if (shortcut.isEmpty()) { \
            QNTRACE(QStringLiteral("No shortcut was found for action " #action)); \
        } \
        else { \
            m_pUI->Action##action->setShortcut(shortcut); \
            m_pUI->Action##action->setShortcutContext(Qt::WidgetWithChildrenShortcut); \
        } \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, ...) \
    { \
        QKeySequence shortcut = m_shortcutManager.shortcut(#action, QStringLiteral(#__VA_ARGS__)); \
        if (shortcut.isEmpty()) { \
            QNTRACE(QStringLiteral("No shortcut was found for action " #action)); \
        } \
        else { \
            m_pUI->Action##action->setShortcut(shortcut); \
            m_pUI->Action##action->setShortcutContext(Qt::WidgetWithChildrenShortcut); \
        } \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::setupConsumerKeyAndSecret(QString & consumerKey, QString & consumerSecret)
{
    const char key[10] = "e3zA914Ol";

    QByteArray consumerKeyObf = QByteArray::fromBase64(QStringLiteral("ZVYsYCHKtSuDnK0g0swrUYAHzYy1m1UeVw==").toUtf8());
    char lastChar = 0;
    int size = consumerKeyObf.size();
    for(int i = 0; i < size; ++i) {
        char currentChar = consumerKeyObf[i];
        consumerKeyObf[i] = consumerKeyObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerKeyObf = qUncompress(consumerKeyObf);
    consumerKey = QString::fromUtf8(consumerKeyObf.constData(), consumerKeyObf.size());

    QByteArray consumerSecretObf = QByteArray::fromBase64(QStringLiteral("ZVYsfTzX0KqA+jbDsjC0T2ZnKiRT0+Os7AN9uQ==").toUtf8());
    lastChar = 0;
    size = consumerSecretObf.size();
    for(int i = 0; i < size; ++i) {
        char currentChar = consumerSecretObf[i];
        consumerSecretObf[i] = consumerSecretObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerSecretObf = qUncompress(consumerSecretObf);
    consumerSecret = QString::fromUtf8(consumerSecretObf.constData(), consumerSecretObf.size());
}
