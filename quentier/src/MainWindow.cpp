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
#include "NoteEditorTabWidgetManager.h"
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
#include "delegates/NoteItemDelegate.h"
#include "delegates/TagItemDelegate.h"
#include "delegates/DeletedNoteTitleColumnDelegate.h"
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
#include <quentier/utility/ApplicationSettings.h>
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
#include <QCheckBox>
#include <QPalette>

#define NOTIFY_ERROR(error) \
    QNWARNING(error); \
    onSetStatusBarText(error)

using namespace quentier;

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow),
    m_currentStatusBarChildWidget(Q_NULLPTR),
    m_lastNoteEditorHtml(),
    m_nativeIconThemeName(),
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
    m_pNoteEditorTabWidgetManager(Q_NULLPTR),
    m_testNotebook(),
    m_testNote(),
    m_pUndoStack(new QUndoStack(this)),
    m_styleSheetInfo(),
    m_currentPanelStyle(),
    m_shortcutManager(this)
{
    QNTRACE(QStringLiteral("MainWindow constructor"));

    setupThemeIcons();

    m_pUI->setupUi(this);

    if (m_nativeIconThemeName.isEmpty()) {
        m_pUI->ActionIconsNative->setVisible(false);
        m_pUI->ActionIconsNative->setDisabled(true);
    }

    collectBaseStyleSheets();
    setupPanelOverlayStyleSheets();

    m_availableAccountsActionGroup->setExclusive(true);

    setupAccountManager();
    m_pAccount.reset(new Account(m_pAccountManager->currentAccount()));

    setupLocalStorageManager();
    setupModels();
    setupViews();

    setupNoteEditorTabWidgetManager();

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
    QObject::connect(m_pUI->ActionSelectAll, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextSelectAllToggled));
    // Font actions
    QObject::connect(m_pUI->ActionFontBold, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextBoldToggled));
    QObject::connect(m_pUI->ActionFontItalic, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextItalicToggled));
    QObject::connect(m_pUI->ActionFontUnderlined, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextUnderlineToggled));
    QObject::connect(m_pUI->ActionFontStrikethrough, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextStrikethroughToggled));
    QObject::connect(m_pUI->ActionIncreaseFontSize, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextIncreaseFontSizeAction));
    QObject::connect(m_pUI->ActionDecreaseFontSize, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextDecreaseFontSizeAction));
    QObject::connect(m_pUI->ActionFontHighlight, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextHighlightAction));
    // Spell checking
    QObject::connect(m_pUI->ActionSpellCheck, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextSpellCheckToggled));
    // Text format actions
    QObject::connect(m_pUI->ActionAlignLeft, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAlignLeftAction));
    QObject::connect(m_pUI->ActionAlignCenter, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAlignCenterAction));
    QObject::connect(m_pUI->ActionAlignRight, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAlignRightAction));
    QObject::connect(m_pUI->ActionInsertHorizontalLine, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAddHorizontalLineAction));
    QObject::connect(m_pUI->ActionIncreaseIndentation, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextIncreaseIndentationAction));
    QObject::connect(m_pUI->ActionDecreaseIndentation, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextDecreaseIndentationAction));
    QObject::connect(m_pUI->ActionInsertBulletedList, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextInsertUnorderedListAction));
    QObject::connect(m_pUI->ActionInsertNumberedList, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextInsertOrderedListAction));
    QObject::connect(m_pUI->ActionInsertTable, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextInsertTableDialogAction));
    QObject::connect(m_pUI->ActionEditHyperlink, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextEditHyperlinkAction));
    QObject::connect(m_pUI->ActionCopyHyperlink, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextCopyHyperlinkAction));
    QObject::connect(m_pUI->ActionRemoveHyperlink, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextRemoveHyperlinkAction));
    // Look and feel actions
    QObject::connect(m_pUI->ActionIconsNative, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconsToNativeAction));
    QObject::connect(m_pUI->ActionIconsOxygen, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconsToOxygenAction));
    QObject::connect(m_pUI->ActionIconsTango, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconsToTangoAction));
    QObject::connect(m_pUI->ActionPanelStyleBuiltIn, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchPanelStyleToBuiltIn));
    QObject::connect(m_pUI->ActionPanelStyleLighter, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchPanelStyleToLighter));
    QObject::connect(m_pUI->ActionPanelStyleDarker, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchPanelStyleToDarker));
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

    if (Q_UNLIKELY(m_pUI->noteEditorsTabWidget->count() == 0)) {
        QNTRACE(QStringLiteral("No open note editors"));
        return Q_NULLPTR;
    }

    int currentIndex = m_pUI->noteEditorsTabWidget->currentIndex();
    if (Q_UNLIKELY(currentIndex < 0)) {
        QNTRACE(QStringLiteral("No current note editor"));
        return Q_NULLPTR;
    }

    QWidget * currentWidget = m_pUI->noteEditorsTabWidget->widget(currentIndex);
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

void MainWindow::persistChosenIconTheme(const QString & iconThemeName)
{
    QNDEBUG(QStringLiteral("MainWindow::persistChosenIconTheme: ") << iconThemeName);

    ApplicationSettings appSettings;
    appSettings.beginGroup(QStringLiteral("LookAndFeel"));
    appSettings.setValue(QStringLiteral("iconTheme"), iconThemeName);
    appSettings.endGroup();
}

void MainWindow::refreshChildWidgetsThemeIcons()
{
    QNDEBUG(QStringLiteral("MainWindow::refreshChildWidgetsThemeIcons"));

    refreshThemeIcons<QAction>();
    refreshThemeIcons<QPushButton>();
    refreshThemeIcons<QCheckBox>();
    refreshThemeIcons<ColorPickerToolButton>();
    refreshThemeIcons<InsertTableToolButton>();
}

void MainWindow::collectBaseStyleSheets()
{
    QNDEBUG(QStringLiteral("MainWindow::collectBaseStyleSheets"));

    QList<QWidget*> childWidgets = findChildren<QWidget*>();
    for(auto it = childWidgets.constBegin(), end = childWidgets.constEnd(); it != end; ++it)
    {
        QWidget * widget = *it;
        if (Q_UNLIKELY(!widget)) {
            continue;
        }

        QString styleSheet = widget->styleSheet();
        if (styleSheet.isEmpty()) {
            continue;
        }

        StyleSheetInfo & info = m_styleSheetInfo[widget];
        info.m_baseStyleSheet = styleSheet;
        info.m_targetWidget = QPointer<QWidget>(widget);
    }
}

void MainWindow::setupPanelOverlayStyleSheets()
{
    QNDEBUG(QStringLiteral("MainWindow::setupPanelOverlayStyleSheets"));

    ApplicationSettings appSettings;
    appSettings.beginGroup(QStringLiteral("LookAndFeel"));
    QString panelStyle = appSettings.value(QStringLiteral("panelStyle")).toString();
    appSettings.endGroup();

    if (panelStyle.isEmpty()) {
        QNDEBUG(QStringLiteral("No last chosen panel style"));
        return;
    }

    StyleSheetProperties properties;
    getPanelStyleSheetProperties(panelStyle, properties);
    if (Q_UNLIKELY(properties.isEmpty())) {
        return;
    }

    m_currentPanelStyle = panelStyle;

    setPanelsOverlayStyleSheet(properties);
}

void MainWindow::getPanelStyleSheetProperties(const QString & panelStyleOption,
                                              StyleSheetProperties & properties) const
{
    properties.clear();
    properties.reserve(4);

    QString backgroundColorName, colorName, backgroundColorButtonHoverName,
            backgroundColorButtonPressedName;

    if (panelStyleOption == QStringLiteral("Lighter"))
    {
        QColor backgroundColor = palette().color(QPalette::Light);
        backgroundColorName = backgroundColor.name();

        QColor color = palette().color(QPalette::WindowText).name();
        colorName = color.name();

        QColor backgroundColorButtonHover = backgroundColor.darker(200);
        backgroundColorButtonHoverName = backgroundColorButtonHover.name();

        QColor backgroundColorButtonPressed = backgroundColor.lighter(150);
        backgroundColorButtonPressedName = backgroundColorButtonPressed.name();
    }
    else if (panelStyleOption == QStringLiteral("Darker"))
    {
        QColor backgroundColor = palette().color(QPalette::Dark);
        backgroundColorName = backgroundColor.name();

        QColor color = palette().color(QPalette::BrightText);
        colorName = color.name();

        QColor backgroundColorButtonHover = backgroundColor.lighter(150);
        backgroundColorButtonHoverName = backgroundColorButtonHover.name();

        QColor backgroundColorButtonPressed = backgroundColor.darker(200);
        backgroundColorButtonPressedName = backgroundColorButtonPressed.name();
    }
    else
    {
        QNDEBUG(QStringLiteral("Unidentified panel style name: ") << panelStyleOption);
        return;
    }

    properties.push_back(StyleSheetProperty(StyleSheetProperty::Target::None,
                                            "background-color", backgroundColorName));
    properties.push_back(StyleSheetProperty(StyleSheetProperty::Target::None,
                                            "color", colorName));
    properties.push_back(StyleSheetProperty(StyleSheetProperty::Target::ButtonHover,
                                            "background-color", backgroundColorButtonHoverName));
    properties.push_back(StyleSheetProperty(StyleSheetProperty::Target::ButtonPressed,
                                            "background-color", backgroundColorButtonPressedName));

    if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
    {
        QNTRACE("Computed stylesheet properties: ");
        for(auto it = properties.constBegin(), end = properties.constEnd(); it != end; ++it)
        {
            const StyleSheetProperty & property = *it;
            QNTRACE("Property: target = " << property.m_targetType << ", property name = " << property.m_name
                    << ", property value = " << property.m_value);
        }
    }

    return;
}

void MainWindow::setPanelsOverlayStyleSheet(const StyleSheetProperties & properties)
{
    for(auto it = m_styleSheetInfo.begin(); it != m_styleSheetInfo.end(); )
    {
        StyleSheetInfo & info = it.value();

        const QPointer<QWidget> & widget = info.m_targetWidget;
        if (Q_UNLIKELY(widget.isNull())) {
            it = m_styleSheetInfo.erase(it);
            continue;
        }

        ++it;

        QString widgetName = widget->objectName();
        if (!widgetName.contains(QStringLiteral("Panel"))) {
            continue;
        }

        if (properties.isEmpty()) {
            widget->setStyleSheet(info.m_baseStyleSheet);
            continue;
        }

        QString overlayStyleSheet = alterStyleSheet(info.m_baseStyleSheet, properties);
        if (Q_UNLIKELY(overlayStyleSheet.isEmpty())) {
            widget->setStyleSheet(info.m_baseStyleSheet);
            continue;
        }

        widget->setStyleSheet(overlayStyleSheet);
    }
}

QString MainWindow::alterStyleSheet(const QString & originalStyleSheet,
                                    const StyleSheetProperties & properties)
{
    QNDEBUG(QStringLiteral("MainWindow::alterStyleSheet: original stylesheet = ")
            << originalStyleSheet);

    QString result = originalStyleSheet;

#define REPORT_ERROR() \
    QNLocalizedString errorDescription = QT_TR_NOOP("Can't alter the stylesheet: stylesheet parsing failed"); \
    QNINFO(errorDescription << QStringLiteral(", original stylesheet: ") \
           << originalStyleSheet << QStringLiteral(", stylesheet modified so far: ") \
           << result << QStringLiteral(", property index = ") \
           << propertyIndex); \
    onSetStatusBarText(errorDescription.localizedString())

    for(auto it = properties.constBegin(), end = properties.constEnd(); it != end; ++it)
    {
        const StyleSheetProperty & property = *it;

        QString propertyName = QString::fromUtf8(property.m_name);

        int propertyIndex = -1;
        while(true)
        {
            if (propertyIndex >= result.size()) {
                break;
            }

            propertyIndex = result.indexOf(propertyName, propertyIndex + 1);
            if (propertyIndex < 0) {
                break;
            }

            if (propertyIndex > 0)
            {
                // Check that what we've found is the start of a word
                QChar previousChar = result[propertyIndex - 1];
                QString previousCharString(previousChar);
                if (!previousCharString.trimmed().isEmpty()) {
                    continue;
                }
            }

            bool error = false;
            bool insideHover = isInsideStyleBlock(result, QStringLiteral(":hover:!pressed"), propertyIndex, error);
            if (Q_UNLIKELY(error)) {
                REPORT_ERROR();
                return QString();
            }

            error = false;
            bool insidePressed = isInsideStyleBlock(result, QStringLiteral(":pressed"), propertyIndex, error);
            if (Q_UNLIKELY(error)) {
                REPORT_ERROR();
                return QString();
            }

            error = false;
            bool insidePseudoButtonHover = isInsideStyleBlock(result, QStringLiteral("pseudoPushButton:hover:!pressed"),
                                                              propertyIndex, error);
            if (Q_UNLIKELY(error)) {
                REPORT_ERROR();
                return QString();
            }

            error = false;
            bool insidePseudoButtonPressed = isInsideStyleBlock(result, QStringLiteral("pseudoPushButton:hover:pressed"),
                                                                propertyIndex, error);
            if (Q_UNLIKELY(error)) {
                REPORT_ERROR();
                return QString();
            }

            int propertyEndIndex = result.indexOf(QStringLiteral(";"), propertyIndex + 1);
            if (Q_UNLIKELY(propertyEndIndex < 0)) {
                QNLocalizedString error = QT_TR_NOOP("Can't alter the stylesheet: stylesheet parsing failed");
                QNINFO(error << QStringLiteral(", original stylesheet: ")
                       << originalStyleSheet << QStringLiteral(", stylesheet modified so far: ")
                       << result << QStringLiteral(", property index = ")
                       << propertyIndex);
                onSetStatusBarText(error.localizedString());
                return QString();
            }

            QString replacement = propertyName + QStringLiteral(": ") + property.m_value;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#define REPLACE() \
            QNDEBUG(QStringLiteral("Replacing substring ") \
                    << result.midRef(propertyIndex, propertyEndIndex - propertyIndex) \
                    << QStringLiteral(" with ") << replacement); \
            result.replace(propertyIndex, propertyEndIndex - propertyIndex, replacement); \
            QNDEBUG(QStringLiteral("Stylesheet after the replacement: ") << result)
#else
#define REPLACE() \
            QNDEBUG(QStringLiteral("Replacing substring ") \
                    << result.mid(propertyIndex, propertyEndIndex - propertyIndex) \
                    << QStringLiteral(" with ") << replacement); \
            result.replace(propertyIndex, propertyEndIndex - propertyIndex, replacement); \
            QNDEBUG(QStringLiteral("Stylesheet after the replacement: ") << result)
#endif

            if (property.m_targetType == StyleSheetProperty::Target::None)
            {
                if (insideHover && !insidePseudoButtonHover) {
                    continue;
                }

                if (insidePressed && !insidePseudoButtonPressed) {
                    continue;
                }

                REPLACE();
            }
            else if (property.m_targetType == StyleSheetProperty::Target::ButtonHover)
            {
                if (insidePressed) {
                    continue;
                }

                if (!insideHover || insidePseudoButtonHover) {
                    continue;
                }

                REPLACE();
            }
            else if (property.m_targetType == StyleSheetProperty::Target::ButtonPressed)
            {
                if (!insidePressed || insidePseudoButtonPressed) {
                    continue;
                }

                REPLACE();
            }
        }
    }

    QNDEBUG(QStringLiteral("Altered stylesheet: ") << result);
    return result;
}

bool MainWindow::isInsideStyleBlock(const QString & styleSheet, const QString & styleBlockStartSearchString,
                                    const int currentIndex, bool & error) const
{
    error = false;

    int blockStartIndex = styleSheet.lastIndexOf(styleBlockStartSearchString, currentIndex);
    if (blockStartIndex < 0) {
        // No block start before the current index => not inside the style block of this kind
        return false;
    }

    int blockEndIndex = styleSheet.lastIndexOf(QStringLiteral("}"), currentIndex);
    if (blockEndIndex > blockStartIndex) {
        // Block end was found after the last block start before the current index => the current index is not within the style block
        return false;
    }

    // There's no block end between block start and the current index, need to look for the block end after the current index
    blockEndIndex = styleSheet.indexOf(QStringLiteral("}"), currentIndex + 1);
    if (Q_UNLIKELY(blockEndIndex < 0)) {
        QNDEBUG(QStringLiteral("Can't parse stylesheet: can't find block end for style block search string ")
                << styleBlockStartSearchString);
        error = true;
        return false;
    }

    // Found first style block end after the current index while the block start is before the current index =>
    // the current index is inside the style block
    return true;
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

void MainWindow::onSwitchIconsToNativeAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchIconsToNativeAction"));

    if (m_nativeIconThemeName.isEmpty()) {
        QNLocalizedString error = QNLocalizedString("No native icon theme is available", this);
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString());
        return;
    }

    if (QIcon::themeName() == m_nativeIconThemeName) {
        QNLocalizedString error = QNLocalizedString("Already using native icon theme", this);
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString());
        return;
    }

    QIcon::setThemeName(m_nativeIconThemeName);
    persistChosenIconTheme(m_nativeIconThemeName);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconsToTangoAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchIconsToTangoAction"));

    QString tango = QStringLiteral("tango");

    if (QIcon::themeName() == tango) {
        QNLocalizedString error = QNLocalizedString("Already using tango icon theme", this);
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString());
        return;
    }

    QIcon::setThemeName(tango);
    persistChosenIconTheme(tango);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconsToOxygenAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchIconsToOxygenAction"));

    QString oxygen = QStringLiteral("oxygen");

    if (QIcon::themeName() == oxygen) {
        QNLocalizedString error = QNLocalizedString("Already using oxygen icon theme", this);
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString());
        return;
    }

    QIcon::setThemeName(oxygen);
    persistChosenIconTheme(oxygen);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchPanelStyleToBuiltIn()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchPanelStyleToBuiltIn"));

    if (m_currentPanelStyle.isEmpty()) {
        QNLocalizedString error = QNLocalizedString("Already using the built-in panel style", this);
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString());
        return;
    }

    m_currentPanelStyle.clear();

    ApplicationSettings appSettings;
    appSettings.beginGroup("LookAndFeel");
    appSettings.remove(QStringLiteral("panelStyle"));
    appSettings.endGroup();

    setPanelsOverlayStyleSheet(StyleSheetProperties());
}

void MainWindow::onSwitchPanelStyleToLighter()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchPanelStyleToLighter"));

    if (m_currentPanelStyle == QStringLiteral("Lighter")) {
        QNLocalizedString error = QNLocalizedString("Already using the lighter panel style", this);
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString());
        return;
    }

    m_currentPanelStyle = QStringLiteral("Lighter");

    ApplicationSettings appSettings;
    appSettings.beginGroup("LookAndFeel");
    appSettings.setValue(QStringLiteral("panelStyle"), m_currentPanelStyle);
    appSettings.endGroup();

    StyleSheetProperties properties;
    getPanelStyleSheetProperties(m_currentPanelStyle, properties);
    setPanelsOverlayStyleSheet(properties);
}

void MainWindow::onSwitchPanelStyleToDarker()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchPanelStyleToDarker"));

    if (m_currentPanelStyle == QStringLiteral("Darker")) {
        QNLocalizedString error = QNLocalizedString("Already using the darker panel style", this);
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString());
        return;
    }

    m_currentPanelStyle = QStringLiteral("Darker");

    ApplicationSettings appSettings;
    appSettings.beginGroup("LookAndFeel");
    appSettings.setValue(QStringLiteral("panelStyle"), m_currentPanelStyle);
    appSettings.endGroup();

    StyleSheetProperties properties;
    getPanelStyleSheetProperties(m_currentPanelStyle, properties);
    setPanelsOverlayStyleSheet(properties);
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

void MainWindow::setupThemeIcons()
{
    QNTRACE(QStringLiteral("MainWindow::setupThemeIcons"));

    m_nativeIconThemeName = QIcon::themeName();
    QNDEBUG(QStringLiteral("Native icon theme name: ") << m_nativeIconThemeName);

    if (!QIcon::hasThemeIcon(QStringLiteral("document-open"))) {
        QNDEBUG(QStringLiteral("There seems to be no native icon theme available: "
                               "document-open icon is not present within the theme"));
        m_nativeIconThemeName.clear();
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(QStringLiteral("LookAndFeel"));
    QString iconThemeName = appSettings.value(QStringLiteral("iconTheme")).toString();
    appSettings.endGroup();

    if (!iconThemeName.isEmpty()) {
        QNDEBUG(QStringLiteral("Last chosen icon theme: ") << iconThemeName);
    }
    else {
        QNDEBUG(QStringLiteral("No last chosen icon theme, fallback to oxygen"));
        iconThemeName = QStringLiteral("oxygen");
    }

    QIcon::setThemeName(iconThemeName);
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
    m_pUI->notebooksTreeView->setModel(m_pNotebookModel);
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

    TreeView * notebooksTreeView = m_pUI->notebooksTreeView;
    NotebookItemDelegate * notebookItemDelegate = new NotebookItemDelegate(notebooksTreeView);
    notebooksTreeView->setItemDelegate(notebookItemDelegate);
    SynchronizableColumnDelegate * notebookTreeViewSynchronizableColumnDelegate =
            new SynchronizableColumnDelegate(notebooksTreeView);
    notebooksTreeView->setItemDelegateForColumn(NotebookModel::Columns::Synchronizable,
                                                notebookTreeViewSynchronizableColumnDelegate);
    notebooksTreeView->setColumnWidth(NotebookModel::Columns::Synchronizable,
                                      notebookTreeViewSynchronizableColumnDelegate->sideSize());
    DirtyColumnDelegate * notebookTreeViewDirtyColumnDelegate =
            new DirtyColumnDelegate(notebooksTreeView);
    notebooksTreeView->setItemDelegateForColumn(NotebookModel::Columns::Dirty,
                                                notebookTreeViewDirtyColumnDelegate);
    notebooksTreeView->setColumnWidth(NotebookModel::Columns::Dirty,
                                      notebookTreeViewDirtyColumnDelegate->sideSize());
    FromLinkedNotebookColumnDelegate * notebookTreeViewFromLinkedNotebookColumnDelegate =
            new FromLinkedNotebookColumnDelegate(notebooksTreeView);
    notebooksTreeView->setItemDelegateForColumn(NotebookModel::Columns::FromLinkedNotebook,
                                                notebookTreeViewFromLinkedNotebookColumnDelegate);
    notebooksTreeView->setColumnWidth(NotebookModel::Columns::FromLinkedNotebook,
                                      notebookTreeViewFromLinkedNotebookColumnDelegate->sideSize());
    notebooksTreeView->header()->hide();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(m_pNotebookModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&),
                     notebooksTreeView, QNSLOT(TreeView,dataChanged,const QModelIndex&,const QModelIndex&));
#else
    QObject::connect(m_pNotebookModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&),
                     notebooksTreeView, QNSLOT(TreeView,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&));
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
    TagItemDelegate * tagsTreeViewNameColumnDelegate = new TagItemDelegate(tagsTreeView);
    tagsTreeView->setItemDelegateForColumn(TagModel::Columns::Name, tagsTreeViewNameColumnDelegate);
    tagsTreeView->setColumnHidden(TagModel::Columns::NumNotesPerTag, true);
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

    QListView * noteListView = m_pUI->noteListView;
    noteListView->setModelColumn(NoteModel::Columns::Title);
    noteListView->setItemDelegate(new NoteItemDelegate(noteListView));

    QTableView * deletedNotesTableView = m_pUI->deletedNotesTableView;
    deletedNotesTableView->setColumnHidden(NoteModel::Columns::ModificationTimestamp, true);
    deletedNotesTableView->setColumnHidden(NoteModel::Columns::PreviewText, true);
    deletedNotesTableView->setColumnHidden(NoteModel::Columns::ThumbnailImageFilePath, true);
    deletedNotesTableView->setColumnHidden(NoteModel::Columns::TagNameList, true);
    deletedNotesTableView->setColumnHidden(NoteModel::Columns::Size, true);
    SynchronizableColumnDelegate * deletedNotesTableViewSynchronizableColumnDelegate =
            new SynchronizableColumnDelegate(deletedNotesTableView);
    deletedNotesTableView->setItemDelegateForColumn(NoteModel::Columns::Synchronizable,
                                                    deletedNotesTableViewSynchronizableColumnDelegate);
    deletedNotesTableView->setColumnWidth(NoteModel::Columns::Synchronizable,
                                          deletedNotesTableViewSynchronizableColumnDelegate->sideSize());
    DirtyColumnDelegate * deletedNotesTableViewDirtyColumnDelegate =
            new DirtyColumnDelegate(deletedNotesTableView);
    deletedNotesTableView->setItemDelegateForColumn(NoteModel::Columns::Dirty,
                                                    deletedNotesTableViewDirtyColumnDelegate);
    deletedNotesTableView->setColumnWidth(NoteModel::Columns::Dirty,
                                          deletedNotesTableViewDirtyColumnDelegate->sideSize());
    DeletedNoteTitleColumnDelegate * deletedNoteTitleColumnDelegate =
            new DeletedNoteTitleColumnDelegate(deletedNotesTableView);
    deletedNotesTableView->setItemDelegateForColumn(NoteModel::Columns::Title, deletedNoteTitleColumnDelegate);
    m_pUI->deletedNotesTableView->horizontalHeader()->hide();
}

void MainWindow::clearViews()
{
    QNDEBUG(QStringLiteral("MainWindow::clearViews"));

    m_pUI->favoritesTableView->setModel(&m_blankModel);
    m_pUI->notebooksTreeView->setModel(&m_blankModel);
    m_pUI->tagsTreeView->setModel(&m_blankModel);
    m_pUI->savedSearchesTableView->setModel(&m_blankModel);

    m_pUI->noteListView->setModel(&m_blankModel);
    m_pUI->deletedNotesTableView->setModel(&m_blankModel);
}

void MainWindow::setupNoteEditorTabWidgetManager()
{
    QNDEBUG(QStringLiteral("MainWindow::setupNoteEditorTabWidgetManager"));

    delete m_pNoteEditorTabWidgetManager;
    m_pNoteEditorTabWidgetManager = new NoteEditorTabWidgetManager(*m_pAccount, *m_pLocalStorageManager,
                                                                   m_noteCache, m_notebookCache,
                                                                   m_tagCache, *m_pTagModel,
                                                                   m_pUI->noteEditorsTabWidget);

    // TODO: connect relevant signals-slots
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

template <class T>
void MainWindow::refreshThemeIcons()
{
    QList<T*> objects = findChildren<T*>();
    QNDEBUG(QStringLiteral("Found ") << objects.size() << QStringLiteral(" child objects"));

    for(auto it = objects.constBegin(), end = objects.constEnd(); it != end; ++it)
    {
        T * object = *it;
        if (Q_UNLIKELY(!object)) {
            continue;
        }

        QIcon icon = object->icon();
        if (icon.isNull()) {
            continue;
        }

        QString iconName = icon.name();
        if (iconName.isEmpty()) {
            continue;
        }

        QIcon newIcon;
        if (QIcon::hasThemeIcon(iconName)) {
            newIcon = QIcon::fromTheme(iconName);
        }
        else {
            newIcon.addFile(QStringLiteral("."), QSize(), QIcon::Normal, QIcon::Off);
        }
        object->setIcon(newIcon);
    }
}
