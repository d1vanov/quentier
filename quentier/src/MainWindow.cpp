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

// workarouding Qt4 Designer's inability to work with namespaces

#include "widgets/FindAndReplaceWidget.h"
using quentier::FindAndReplaceWidget;

#include <quentier/note_editor/NoteEditor.h>
using quentier::NoteEditor;
using quentier::NoteEditorWidget;
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

#define NOTIFY_ERROR(error) \
    QNWARNING(error); \
    onSetStatusBarText(error)

using namespace quentier;

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow),
    m_currentStatusBarChildWidget(Q_NULLPTR),
    m_pNoteEditor(Q_NULLPTR),
    m_lastNoteEditorHtml(),
    m_testNotebook(),
    m_testNote(),
    m_pAccountManager(new AccountManager(this)),
    m_pAccount(),
    m_lastFontSizeComboBoxIndex(-1),
    m_lastFontComboBoxFontFamily(),
    m_pUndoStack(new QUndoStack(this)),
    m_shortcutManager(this)
{
    QNTRACE(QStringLiteral("MainWindow constructor"));

    m_pUI->setupUi(this);

    m_pAccount.reset(new Account(m_pAccountManager->currentAccount()));

    setupDefaultShortcuts();
    setupUserShortcuts();

    addMenuActionsToMainWindow();

    checkThemeIconsAndSetFallbacks();

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

    m_pNoteEditor->setNoteAndNotebook(m_testNote, m_testNotebook);
    m_pNoteEditor->setFocus();
}

MainWindow::~MainWindow()
{
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
    QObject::connect(m_pUI->ActionSelectAll, QNSIGNAL(QAction,triggered), m_pNoteEditor, QNSLOT(NoteEditor,selectAll));
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

    QActionGroup * accountsActionGroup = new QActionGroup(this);
    accountsActionGroup->setExclusive(true);

    const QVector<AvailableAccount> & availableAccounts = m_pAccountManager->availableAccounts();

    int numAvailableAccounts = availableAccounts.size();
    for(int i = 0; i < numAvailableAccounts; ++i)
    {
        const AvailableAccount & availableAccount = availableAccounts[i];
        QString availableAccountRepresentationName = availableAccount.username();
        if (availableAccount.isLocal()) {
            availableAccountRepresentationName += QStringLiteral(" (");
            availableAccountRepresentationName += tr("local");
            availableAccountRepresentationName += QStringLiteral(")");
        }

        QAction * accountAction = new QAction(availableAccountRepresentationName, Q_NULLPTR);
        switchAccountSubMenu->addAction(accountAction);

        accountAction->setCheckable(true);
        addAction(accountAction);
        QObject::connect(accountAction, QNSIGNAL(QAction,toggled,bool),
                         this, QNSLOT(MainWindow,onSwitchAccountActionToggled,bool));

        accountsActionGroup->addAction(accountAction);
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
    m_pNoteEditor->setNoteAndNotebook(m_testNote, m_testNotebook);
    m_pNoteEditor->setFocus();
}

void MainWindow::onSetTestNoteWithResources()
{
    QNDEBUG(QStringLiteral("MainWindow::onSetTestNoteWithResources"));

    prepareTestNoteWithResources();

    m_testNote.setLocalUid(QStringLiteral("{ce8e5ea1-28fc-4842-a726-0d4a78dfcbe6}"));
    m_testNotebook.setCanUpdateNotes(true);

    m_pNoteEditor->setNoteAndNotebook(m_testNote, m_testNotebook);
    m_pNoteEditor->setFocus();
}

void MainWindow::onSetTestReadOnlyNote()
{
    prepareTestNoteWithResources();

    m_testNote.setLocalUid(QStringLiteral("{ce8e5ea1-28fc-4842-a726-0d4a78dfcbe5}"));
    m_testNotebook.setCanUpdateNotes(false);

    m_pNoteEditor->setNoteAndNotebook(m_testNote, m_testNotebook);
    m_pNoteEditor->setFocus();
}

void MainWindow::onSetInkNote()
{
    prepareTestInkNote();

    m_testNote.setLocalUid(QStringLiteral("{96c747e2-7bdc-4805-a704-105cbfcc7fbe}"));
    m_testNotebook.setCanUpdateNotes(true);

    m_pNoteEditor->setNoteAndNotebook(m_testNote, m_testNotebook);
    m_pNoteEditor->setFocus();
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
        NOTIFY_ERROR(QT_TR_NOOP("Internal error: can't identify account to switch to"));
        return;
    }

    QString accountName = action->text();
    Q_UNUSED(accountName);
    // TODO: implement further
}

void MainWindow::checkThemeIconsAndSetFallbacks()
{
    QNTRACE(QStringLiteral("MainWindow::checkThemeIconsAndSetFallbacks"));

    if (!QIcon::hasThemeIcon(QStringLiteral("dialog-information"))) {
        m_pUI->ActionShowNoteAttributesButton->setIcon(QIcon(QStringLiteral(":/fallback_icons/png/dialog-information-4.png")));
        QNTRACE(QStringLiteral("set fallback dialog-information icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("document-new"))) {
        m_pUI->ActionNoteAdd->setIcon(QIcon(QStringLiteral(":/fallback_icons/png/document-new-6.png")));
        QNTRACE(QStringLiteral("set fallback document-new icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("printer"))) {
        m_pUI->ActionPrint->setIcon(QIcon(QStringLiteral(":/fallback_icons/png/document-print-5.png")));
        QNTRACE(QStringLiteral("set fallback printer icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("document-save"))) {
        QIcon documentSaveIcon(QStringLiteral(":/fallback_icons/png/document-save-5.png"));
        m_pUI->saveSearchPushButton->setIcon(documentSaveIcon);
        m_pUI->ActionSaveImage->setIcon(documentSaveIcon);
        QNTRACE(QStringLiteral("set fallback document-save icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-copy"))) {
        QIcon editCopyIcon(QStringLiteral(":/fallback_icons/png/edit-copy-6.png"));
        m_pUI->ActionCopy->setIcon(editCopyIcon);
        QNTRACE(QStringLiteral("set fallback edit-copy icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-cut"))) {
        QIcon editCutIcon(QStringLiteral(":/fallback_icons/png/edit-cut-6.png"));
        m_pUI->ActionCut->setIcon(editCutIcon);
        QNTRACE(QStringLiteral("set fallback edit-cut icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-delete"))) {
        QIcon editDeleteIcon(QStringLiteral(":/fallback_icons/png/edit-delete-6.png"));
        m_pUI->ActionNoteDelete->setIcon(editDeleteIcon);
        QNTRACE(QStringLiteral("set fallback edit-delete icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-find"))) {
        m_pUI->ActionFindInsideNote->setIcon(QIcon(QStringLiteral(":/fallback_icons/png/edit-find-7.png")));
        QNTRACE(QStringLiteral("set fallback edit-find icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-paste"))) {
        QIcon editPasteIcon(QStringLiteral(":/fallback_icons/png/edit-paste-6.png"));
        m_pUI->ActionPaste->setIcon(editPasteIcon);
        QNTRACE(QStringLiteral("set fallback edit-paste icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-redo"))) {
        QIcon editRedoIcon(QStringLiteral(":/fallback_icons/png/edit-redo-7.png"));
        m_pUI->ActionRedo->setIcon(editRedoIcon);
        QNTRACE(QStringLiteral("set fallback edit-redo icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-undo"))) {
        QIcon editUndoIcon(QStringLiteral(":/fallback_icons/png/edit-undo-7.png"));
        m_pUI->ActionUndo->setIcon(editUndoIcon);
        QNTRACE(QStringLiteral("set fallback edit-undo icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-indent-less"))) {
        QIcon formatIndentLessIcon(QStringLiteral(":/fallback_icons/png/format-indent-less-5.png"));
        m_pUI->ActionDecreaseIndentation->setIcon(formatIndentLessIcon);
        QNTRACE(QStringLiteral("set fallback format-indent-less icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-indent-more"))) {
        QIcon formatIndentMoreIcon(QStringLiteral(":/fallback_icons/png/format-indent-more-5.png"));
        m_pUI->ActionIncreaseIndentation->setIcon(formatIndentMoreIcon);
        QNTRACE(QStringLiteral("set fallback format-indent-more icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-justify-center"))) {
        QIcon formatJustifyCenterIcon(QStringLiteral(":/fallback_icons/png/format-justify-center-5.png"));
        m_pUI->ActionAlignCenter->setIcon(formatJustifyCenterIcon);
        QNTRACE(QStringLiteral("set fallback format-justify-center icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-justify-left"))) {
        QIcon formatJustifyLeftIcon(QStringLiteral(":/fallback_icons/png/format-justify-left-5.png"));
        m_pUI->ActionAlignLeft->setIcon(formatJustifyLeftIcon);
        QNTRACE(QStringLiteral("set fallback format-justify-left icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-justify-right"))) {
        QIcon formatJustifyRightIcon(QStringLiteral(":/fallback_icons/png/format-justify-right-5.png"));
        m_pUI->ActionAlignRight->setIcon(formatJustifyRightIcon);
        QNTRACE(QStringLiteral("set fallback format-justify-right icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-list-ordered"))) {
        QIcon formatListOrderedIcon(QStringLiteral(":/fallback_icons/png/format-list-ordered.png"));
        m_pUI->ActionInsertNumberedList->setIcon(formatListOrderedIcon);
        QNTRACE(QStringLiteral("set fallback format-list-ordered icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-list-unordered"))) {
        QIcon formatListUnorderedIcon(QStringLiteral(":/fallback_icons/png/format-list-unordered.png"));
        m_pUI->ActionInsertBulletedList->setIcon(formatListUnorderedIcon);
        QNTRACE(QStringLiteral("set fallback format-list-unordered icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-bold"))) {
        QIcon formatTextBoldIcon(QStringLiteral(":/fallback_icons/png/format-text-bold-4.png"));
        m_pUI->ActionFontBold->setIcon(formatTextBoldIcon);
        QNTRACE(QStringLiteral("set fallback format-text-bold icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-italic"))) {
        QIcon formatTextItalicIcon(QStringLiteral(":/fallback_icons/png/format-text-italic-4.png"));
        m_pUI->ActionFontItalic->setIcon(formatTextItalicIcon);
        QNTRACE(QStringLiteral("set fallback format-text-italic icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-strikethrough"))) {
        QIcon formatTextStrikethroughIcon(QStringLiteral(":/fallback_icons/png/format-text-strikethrough-3.png"));
        m_pUI->ActionFontStrikethrough->setIcon(formatTextStrikethroughIcon);
        QNTRACE(QStringLiteral("set fallback format-text-strikethrough icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-underline"))) {
        QIcon formatTextUnderlineIcon(QStringLiteral(":/fallback_icons/png/format-text-underline-4.png"));
        m_pUI->ActionFontUnderlined->setIcon(formatTextUnderlineIcon);
        QNTRACE(QStringLiteral("set fallback format-text-underline icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("go-down"))) {
        QIcon goDownIcon(QStringLiteral(":/fallback_icons/png/go-down-7.png"));
        m_pUI->ActionGoDown->setIcon(goDownIcon);
        QNTRACE(QStringLiteral("set fallback go-down icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("go-up"))) {
        QIcon goUpIcon(QStringLiteral(":/fallback_icons/png/go-up-7.png"));
        m_pUI->ActionGoUp->setIcon(goUpIcon);
        QNTRACE(QStringLiteral("set fallback go-up icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("go-previous"))) {
        QIcon goPreviousIcon(QStringLiteral(":/fallback_icons/png/go-previous-7.png"));
        m_pUI->ActionGoPrevious->setIcon(goPreviousIcon);
        QNTRACE(QStringLiteral("set fallback go-previous icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("go-next"))) {
        QIcon goNextIcon(QStringLiteral(":/fallback_icons/png/go-next-7.png"));
        m_pUI->ActionGoNext->setIcon(goNextIcon);
        QNTRACE(QStringLiteral("set fallback go-next icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("insert-horizontal-rule"))) {
        QIcon insertHorizontalRuleIcon(QStringLiteral(":/fallback_icons/png/insert-horizontal-rule.png"));
        m_pUI->ActionInsertHorizontalLine->setIcon(insertHorizontalRuleIcon);
        QNTRACE(QStringLiteral("set fallback insert-horizontal-rule icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("insert-table"))) {
        QIcon insertTableIcon(QStringLiteral(":/fallback_icons/png/insert-table.png"));
        m_pUI->ActionInsertTable->setIcon(insertTableIcon);
        m_pUI->menuTable->setIcon(insertTableIcon);
        QNTRACE(QStringLiteral("set fallback insert-table icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("mail-send"))) {
        QIcon mailSendIcon(QStringLiteral(":/fallback_icons/png/mail-forward-5.png"));
        m_pUI->ActionSendMail->setIcon(mailSendIcon);
        QNTRACE(QStringLiteral("set fallback mail-send icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("preferences-other"))) {
        QIcon preferencesOtherIcon(QStringLiteral(":/fallback_icons/png/preferences-other-3.png"));
        m_pUI->ActionPreferences->setIcon(preferencesOtherIcon);
        QNTRACE(QStringLiteral("set fallback preferences-other icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("object-rotate-left"))) {
        QIcon objectRotateLeftIcon(QStringLiteral(":/fallback_icons/png/object-rotate-left.png"));
        m_pUI->ActionRotateCounterClockwise->setIcon(objectRotateLeftIcon);
        QNTRACE(QStringLiteral("set fallback object-rotate-left icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("object-rotate-right"))) {
        QIcon objectRotateRightIcon(QStringLiteral(":/fallback_icons/png/object-rotate-right.png"));
        m_pUI->ActionRotateClockwise->setIcon(objectRotateRightIcon);
        QNTRACE(QStringLiteral("set fallback object-rotate-right icon"));
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
