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
#include "dialogs/ManageAccountsDialog.h"
#include "insert-table-tool-button/InsertTableToolButton.h"
#include "insert-table-tool-button/TableSettingsDialog.h"
#include "widgets/NoteEditorWidget.h"
#include "tests/ManualTestingHelper.h"
#include "BasicXMLSyntaxHighlighter.h"

// workarouding Qt4 Designer's inability to work with namespaces

#include "widgets/FindAndReplaceWidget.h"
using quentier::FindAndReplaceWidget;

#include <quentier/note_editor/NoteEditor.h>
using quentier::NoteEditor;
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
    m_pAccount(),
    m_availableAccounts(),
    m_lastFontSizeComboBoxIndex(-1),
    m_lastFontComboBoxFontFamily(),
    m_pUndoStack(new QUndoStack(this)),
    m_shortcutManager(this)
{
    QNTRACE(QStringLiteral("MainWindow constructor"));

    m_pUI->setupUi(this);
    m_pUI->findAndReplaceWidget->setHidden(true);

    // TODO: try to find within the app settings the last used account and pick it
    m_pAccount.reset(new Account("Default user", Account::Type::Local));
    m_pUI->noteEditorWidget->setAccount(*m_pAccount);

    m_pUI->noteEditorWidget->setUndoStack(m_pUndoStack);

    setupDefaultShortcuts();
    setupUserShortcuts();

    addMenuActionsToMainWindow();

    m_pUI->noteSourceView->setHidden(true);

    m_pUI->fontSizeComboBox->clear();
    QNTRACE(QStringLiteral("fontSizeComboBox num items: ") << m_pUI->fontSizeComboBox->count());
    for(int i = 0; i < m_pUI->fontSizeComboBox->count(); ++i) {
        QVariant value = m_pUI->fontSizeComboBox->itemData(i, Qt::UserRole);
        QNTRACE(QStringLiteral("Font size value for index[") << i << QStringLiteral("] = ") << value);
    }

    BasicXMLSyntaxHighlighter * highlighter = new BasicXMLSyntaxHighlighter(m_pUI->noteSourceView->document());
    Q_UNUSED(highlighter);

    m_pNoteEditor = m_pUI->noteEditorWidget;

    checkThemeIconsAndSetFallbacks();

    connectActionsToSlots();
    connectActionsToEditorSlots();
    connectEditorSignalsToSlots();

    // Stuff primarily for manual testing
    QObject::connect(m_pUI->ActionShowNoteSource, QNSIGNAL(QAction, triggered),
                     this, QNSLOT(MainWindow, onShowNoteSource));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,noteEditorHtmlUpdated,QString),
                     this, QNSLOT(MainWindow,onNoteEditorHtmlUpdate,QString));

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

void MainWindow::connectActionsToEditorSlots()
{
    // Undo/redo actions
    QObject::connect(m_pUI->ActionUndo, QNSIGNAL(QAction,triggered), m_pNoteEditor, QNSLOT(NoteEditor,undo));
    QObject::connect(m_pUI->ActionRedo, QNSIGNAL(QAction,triggered), m_pNoteEditor, QNSLOT(NoteEditor,redo));
    // Undo/redo buttons
    QObject::connect(m_pUI->undoPushButton, QNSIGNAL(QPushButton,clicked), m_pNoteEditor, QNSLOT(NoteEditor,undo));
    QObject::connect(m_pUI->redoPushButton, QNSIGNAL(QPushButton,clicked), m_pNoteEditor, QNSLOT(NoteEditor,redo));
    // Copy/cut/paste actions
    QObject::connect(m_pUI->ActionCopy, QNSIGNAL(QAction,triggered), m_pNoteEditor, QNSLOT(NoteEditor,copy));
    QObject::connect(m_pUI->ActionCut, QNSIGNAL(QAction,triggered), m_pNoteEditor, QNSLOT(NoteEditor,cut));
    QObject::connect(m_pUI->ActionPaste, QNSIGNAL(QAction,triggered), m_pNoteEditor, QNSLOT(NoteEditor,paste));
    // Copy/cut/paste buttons
    QObject::connect(m_pUI->copyPushButton, QNSIGNAL(QPushButton,clicked), m_pNoteEditor, QNSLOT(NoteEditor,copy));
    QObject::connect(m_pUI->cutPushButton, QNSIGNAL(QPushButton,clicked), m_pNoteEditor, QNSLOT(NoteEditor,cut));
    QObject::connect(m_pUI->pastePushButton, QNSIGNAL(QPushButton,clicked), m_pNoteEditor, QNSLOT(NoteEditor,paste));
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
    // Font buttons
    QObject::connect(m_pUI->fontBoldPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextBoldToggled));
    QObject::connect(m_pUI->fontItalicPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextItalicToggled));
    QObject::connect(m_pUI->fontUnderlinePushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextUnderlineToggled));
    QObject::connect(m_pUI->fontStrikethroughPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextStrikethroughToggled));
    // Font combo box
    QObject::connect(m_pUI->fontComboBox, QNSIGNAL(QFontComboBox,currentFontChanged,QFont), this, QNSLOT(MainWindow,onFontComboBoxFontChanged,QFont));
    // Font size combo box
    // NOTE: something seems fishy with these two signal-slot signatures, they don't work with Qt5 connection syntax
    QObject::connect(m_pUI->fontSizeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onFontSizeComboBoxIndexChanged(int)));
    // Spell checking
    QObject::connect(m_pUI->ActionSpellCheck, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onNoteTextSpellCheckToggled));
    QObject::connect(m_pUI->spellCheckBox, QNSIGNAL(QCheckBox,clicked,bool), m_pNoteEditor, QNSLOT(NoteEditor,setSpellcheck,bool));
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
    // Format buttons
    QObject::connect(m_pUI->formatJustifyLeftPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextAlignLeftAction));
    QObject::connect(m_pUI->formatJustifyCenterPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextAlignCenterAction));
    QObject::connect(m_pUI->formatJustifyRightPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextAlignRightAction));
    QObject::connect(m_pUI->insertHorizontalLinePushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextAddHorizontalLineAction));
    QObject::connect(m_pUI->formatIndentMorePushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextIncreaseIndentationAction));
    QObject::connect(m_pUI->formatIndentLessPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextDecreaseIndentationAction));
    QObject::connect(m_pUI->formatListUnorderedPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextInsertUnorderedListAction));
    QObject::connect(m_pUI->formatListOrderedPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextInsertOrderedListAction));
    QObject::connect(m_pUI->insertToDoCheckboxPushButton, QNSIGNAL(QPushButton,clicked), this, QNSLOT(MainWindow,onNoteTextInsertToDoCheckBoxAction));
    QObject::connect(m_pUI->chooseTextColorToolButton, QNSIGNAL(ColorPickerToolButton,colorSelected,QColor),
                     this, QNSLOT(MainWindow,onNoteChooseTextColor,QColor));
    QObject::connect(m_pUI->chooseBackgroundColorToolButton, QNSIGNAL(ColorPickerToolButton,colorSelected,QColor),
                     this, QNSLOT(MainWindow,onNoteChooseBackgroundColor,QColor));
    QObject::connect(m_pUI->insertTableToolButton, QNSIGNAL(InsertTableToolButton,createdTable,int,int,double,bool),
                     this, QNSLOT(MainWindow,onNoteTextInsertTable,int,int,double,bool));
}

void MainWindow::connectActionsToSlots()
{
    QObject::connect(m_pUI->ActionFindInsideNote, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onFindInsideNoteAction));
    QObject::connect(m_pUI->ActionFindNext, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onFindInsideNoteAction));
    QObject::connect(m_pUI->ActionFindPrevious, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onFindPreviousInsideNoteAction));
    QObject::connect(m_pUI->ActionReplaceInNote, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindow,onReplaceInsideNoteAction));

    QObject::connect(m_pUI->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,closed), this, QNSLOT(MainWindow,onFindAndReplaceWidgetClosed));
    QObject::connect(m_pUI->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,textToFindEdited,const QString&),
                     this, QNSLOT(MainWindow,onTextToFindInsideNoteEdited,const QString&));
    QObject::connect(m_pUI->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,findNext,const QString&,const bool),
                     this, QNSLOT(MainWindow,onFindNextInsideNote,const QString&,const bool));
    QObject::connect(m_pUI->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,findPrevious,const QString&,const bool),
                     this, QNSLOT(MainWindow,onFindPreviousInsideNote,const QString&,const bool));
    QObject::connect(m_pUI->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,searchCaseSensitivityChanged,const bool),
                     this, QNSLOT(MainWindow,onFindInsideNoteCaseSensitivityChanged,const bool));
    QObject::connect(m_pUI->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,replace,const QString&,const QString&,const bool),
                     this, QNSLOT(MainWindow,onReplaceInsideNote,const QString&,const QString&,const bool));
    QObject::connect(m_pUI->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,replaceAll,const QString&,const QString&,const bool),
                     this, QNSLOT(MainWindow,onReplaceAllInsideNote,const QString&,const QString&,const bool));
}

void MainWindow::connectEditorSignalsToSlots()
{
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textBoldState,bool), this, QNSLOT(MainWindow,onNoteEditorBoldStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textItalicState,bool), this, QNSLOT(MainWindow,onNoteEditorItalicStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textStrikethroughState,bool), this, QNSLOT(MainWindow,onNoteEditorStrikethroughStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textUnderlineState,bool), this, QNSLOT(MainWindow,onNoteEditorUnderlineStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textAlignLeftState,bool), this, QNSLOT(MainWindow,onNoteEditorAlignLeftStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textAlignCenterState,bool), this, QNSLOT(MainWindow,onNoteEditorAlignCenterStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textAlignRightState,bool), this, QNSLOT(MainWindow,onNoteEditorAlignRightStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textInsideOrderedListState,bool), this, QNSLOT(MainWindow,onNoteEditorInsideOrderedListStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textInsideUnorderedListState,bool), this, QNSLOT(MainWindow,onNoteEditorInsideUnorderedListStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textInsideTableState,bool), this, QNSLOT(MainWindow,onNoteEditorInsideTableStateChanged,bool));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textFontFamilyChanged,QString), this, QNSLOT(MainWindow,onNoteEditorFontFamilyChanged,QString));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,textFontSizeChanged,int), this, QNSLOT(MainWindow,onNoteEditorFontSizeChanged,int));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,insertTableDialogRequested), this, QNSLOT(MainWindow,onNoteTextInsertTableDialogAction));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,spellCheckerNotReady), this, QNSLOT(MainWindow,onNoteEditorSpellCheckerNotReady));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,spellCheckerReady), this, QNSLOT(MainWindow,onNoteEditorSpellCheckerReady));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,notifyError,QNLocalizedString), this, QNSLOT(MainWindow,onNoteEditorError,QNLocalizedString));
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

    QStringList availableAccounts = detectAvailableAccounts();
    if (availableAccounts.isEmpty())
    {
        QNDEBUG(QStringLiteral("It appears the default user account doesn't exist yet, will add it"));
        QString defaultAccount = createDefaultAccount();
        if (Q_LIKELY(!defaultAccount.isEmpty())) {
            // TRANSLATOR this "local" is meant for the name of the default account
            availableAccounts << (defaultAccount + QStringLiteral(" (") + tr("local") + QStringLiteral(")"));
        }
    }

    int numAvailableAccounts = availableAccounts.size();
    for(int i = 0; i < numAvailableAccounts; ++i)
    {
        QAction * accountAction = new QAction(availableAccounts[i], Q_NULLPTR);
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

QString MainWindow::createDefaultAccount()
{
    QNDEBUG(QStringLiteral("MainWindow::createDefaultAccount"));

    QString username = getCurrentUserName();

    QString appPersistenceStoragePath = applicationPersistentStoragePath();
    QString userPersistenceStoragePath = appPersistenceStoragePath + QStringLiteral("/local_") + username;
    QDir userPersistenceStorageDir(userPersistenceStoragePath);
    if (!userPersistenceStorageDir.exists())
    {
        bool created = userPersistenceStorageDir.mkpath(userPersistenceStoragePath);
        if (Q_UNLIKELY(!created)) {
            NOTIFY_ERROR(tr("Can't create directory for the default account storage"));
            return QString();
        }
    }

    QFile accountInfo(userPersistenceStoragePath + QStringLiteral("/accountInfo.txt"));

    bool open = accountInfo.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!open)) {
        NOTIFY_ERROR(tr("Can't open the account info file for the default account for writing") +
                     QStringLiteral(": ") + accountInfo.errorString());
        return QString();
    }

    QXmlStreamWriter writer(&accountInfo);

    writer.writeStartDocument();

    writer.writeStartElement(QStringLiteral("accountName"));
    writer.writeCharacters(username);
    writer.writeEndElement();

    writer.writeStartElement(QStringLiteral("accountType"));
    writer.writeCharacters(QStringLiteral("local"));
    writer.writeEndElement();

    writer.writeEndDocument();

    accountInfo.close();

    AvailableAccount availableAccount(username, userPersistenceStoragePath, /* local = */ true);
    m_availableAccounts << availableAccount;

    return username;
}

QStringList MainWindow::detectAvailableAccounts()
{
    QNDEBUG(QStringLiteral("MainWindow::detectAvailableAccounts"));

    QStringList result;

    QString appPersistenceStoragePath = applicationPersistentStoragePath();
    QDir appPersistenceStorageDir(appPersistenceStoragePath);

    QFileInfoList accountDirInfos = appPersistenceStorageDir.entryInfoList(QDir::AllDirs);
    int numPotentialAccountDirs = accountDirInfos.size();
    result.reserve(numPotentialAccountDirs);

    for(int i = 0; i < numPotentialAccountDirs; ++i)
    {
        const QFileInfo & accountDirInfo = accountDirInfos[i];
        QDir accountDir = accountDirInfo.absoluteDir();
        QFileInfo accountBasicInfoFileInfo(accountDir.absolutePath() + QStringLiteral("/accountInfo.txt"));
        if (!accountBasicInfoFileInfo.exists()) {
            continue;
        }

        QString accountName = accountDir.dirName();
        bool isLocal = accountName.startsWith(QStringLiteral("local_"));
        if (isLocal) {
            accountName.remove(0, 6);
        }

        AvailableAccount availableAccount(accountName, accountDir.absolutePath(), isLocal);
        m_availableAccounts << availableAccount;

        int lastUnderlineIndex = accountName.lastIndexOf(QStringLiteral("_"));
        if (lastUnderlineIndex >= 0) {
            int numCharsToRemove = accountName.length() - lastUnderlineIndex;
            accountName.chop(numCharsToRemove);
        }

        result << accountName;
    }

    return result;
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

void MainWindow::onNoteTextBoldToggled()
{
    m_pNoteEditor->textBold();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextItalicToggled()
{
    m_pNoteEditor->textItalic();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextUnderlineToggled()
{
    m_pNoteEditor->textUnderline();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextStrikethroughToggled()
{
    m_pNoteEditor->textStrikethrough();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextAlignLeftAction()
{
    if (m_pUI->formatJustifyLeftPushButton->isChecked()) {
        m_pUI->formatJustifyCenterPushButton->setChecked(false);
        m_pUI->formatJustifyRightPushButton->setChecked(false);
    }

    m_pNoteEditor->alignLeft();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextAlignCenterAction()
{
    if (m_pUI->formatJustifyCenterPushButton->isChecked()) {
        m_pUI->formatJustifyLeftPushButton->setChecked(false);
        m_pUI->formatJustifyRightPushButton->setChecked(false);
    }

    m_pNoteEditor->alignCenter();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextAlignRightAction()
{
    if (m_pUI->formatJustifyRightPushButton->isChecked()) {
        m_pUI->formatJustifyLeftPushButton->setChecked(false);
        m_pUI->formatJustifyCenterPushButton->setChecked(false);
    }

    m_pNoteEditor->alignRight();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextAddHorizontalLineAction()
{
    m_pNoteEditor->insertHorizontalLine();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextIncreaseFontSizeAction()
{
    m_pNoteEditor->increaseFontSize();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextDecreaseFontSizeAction()
{
    m_pNoteEditor->decreaseFontSize();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextHighlightAction()
{
    m_pNoteEditor->textHighlight();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextIncreaseIndentationAction()
{
    m_pNoteEditor->increaseIndentation();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextDecreaseIndentationAction()
{
    m_pNoteEditor->decreaseIndentation();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextInsertUnorderedListAction()
{
    m_pNoteEditor->insertBulletedList();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextInsertOrderedListAction()
{
    m_pNoteEditor->insertNumberedList();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextEditHyperlinkAction()
{
    m_pNoteEditor->editHyperlinkDialog();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextCopyHyperlinkAction()
{
    m_pNoteEditor->copyHyperlink();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextRemoveHyperlinkAction()
{
    m_pNoteEditor->removeHyperlink();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteChooseTextColor(QColor color)
{
    m_pNoteEditor->setFontColor(color);
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteChooseBackgroundColor(QColor color)
{
    m_pNoteEditor->setBackgroundColor(color);
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextSpellCheckToggled()
{
    m_pNoteEditor->setSpellcheck(m_pUI->spellCheckBox->isEnabled());
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextInsertToDoCheckBoxAction()
{
    m_pNoteEditor->insertToDoCheckbox();
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteTextInsertTableDialogAction()
{
    QScopedPointer<TableSettingsDialog> tableSettingsDialogHolder(new TableSettingsDialog(this));
    TableSettingsDialog * tableSettingsDialog = tableSettingsDialogHolder.data();
    if (tableSettingsDialog->exec() == QDialog::Accepted)
    {
        QNTRACE(QStringLiteral("Returned from TableSettingsDialog::exec: accepted"));
        int numRows = tableSettingsDialog->numRows();
        int numColumns = tableSettingsDialog->numColumns();
        double tableWidth = tableSettingsDialog->tableWidth();
        bool relativeWidth = tableSettingsDialog->relativeWidth();

        if (relativeWidth) {
            m_pNoteEditor->insertRelativeWidthTable(numRows, numColumns, tableWidth);
        }
        else {
            m_pNoteEditor->insertFixedWidthTable(numRows, numColumns, static_cast<int>(tableWidth));
        }

        m_pNoteEditor->setFocus();
        return;
    }

    QNTRACE(QStringLiteral("Returned from TableSettingsDialog::exec: rejected"));
}

void MainWindow::onNoteTextInsertTable(int rows, int columns, double width, bool relativeWidth)
{
    rows = std::max(rows, 1);
    columns = std::max(columns, 1);
    width = std::max(width, 1.0);

    if (relativeWidth) {
        m_pNoteEditor->insertRelativeWidthTable(rows, columns, width);
    }
    else {
        m_pNoteEditor->insertFixedWidthTable(rows, columns, static_cast<int>(width));
    }

    QNTRACE(QStringLiteral("Inserted table: rows = ") << rows << QStringLiteral(", columns = ") << columns
            << QStringLiteral(", width = ") << width << QStringLiteral(", relative width = ")
            << (relativeWidth ? QStringLiteral("true") : QStringLiteral("false")));
    m_pNoteEditor->setFocus();
}

void MainWindow::onShowNoteSource()
{
    QNDEBUG(QStringLiteral("MainWindow::onShowNoteSource"));
    updateNoteHtmlView(m_lastNoteEditorHtml);
    m_pUI->noteSourceView->setHidden(m_pUI->noteSourceView->isVisible());
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

void MainWindow::onFindInsideNoteAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onFindInsideNoteAction"));

    if (m_pUI->findAndReplaceWidget->isHidden())
    {
        QString textToFind = m_pNoteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_pUI->findAndReplaceWidget->textToFind();
        }
        else {
            m_pUI->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_pUI->findAndReplaceWidget->setHidden(false);
        m_pUI->findAndReplaceWidget->show();
    }

    onFindNextInsideNote(m_pUI->findAndReplaceWidget->textToFind(), m_pUI->findAndReplaceWidget->matchCase());
}

void MainWindow::onFindPreviousInsideNoteAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onFindPreviousInsideNoteAction"));

    if (m_pUI->findAndReplaceWidget->isHidden())
    {
        QString textToFind = m_pNoteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_pUI->findAndReplaceWidget->textToFind();
        }
        else {
            m_pUI->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_pUI->findAndReplaceWidget->setHidden(false);
        m_pUI->findAndReplaceWidget->show();
    }

    onFindPreviousInsideNote(m_pUI->findAndReplaceWidget->textToFind(), m_pUI->findAndReplaceWidget->matchCase());
}

void MainWindow::onReplaceInsideNoteAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onReplaceInsideNoteAction"));

    if (m_pUI->findAndReplaceWidget->isHidden() || !m_pUI->findAndReplaceWidget->replaceEnabled())
    {
        QNTRACE(QStringLiteral("At least the replacement part of find and replace widget is hidden, will only show it and do nothing else"));

        QString textToFind = m_pNoteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_pUI->findAndReplaceWidget->textToFind();
        }
        else {
            m_pUI->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_pUI->findAndReplaceWidget->setHidden(false);
        m_pUI->findAndReplaceWidget->setReplaceEnabled(true);
        m_pUI->findAndReplaceWidget->show();
        return;
    }

    onReplaceInsideNote(m_pUI->findAndReplaceWidget->textToFind(), m_pUI->findAndReplaceWidget->replacementText(), m_pUI->findAndReplaceWidget->matchCase());
}

void MainWindow::onFindAndReplaceWidgetClosed()
{
    QNDEBUG(QStringLiteral("MainWindow::onFindAndReplaceWidgetClosed"));
    onFindNextInsideNote(QString(), false);
}

void MainWindow::onTextToFindInsideNoteEdited(const QString & textToFind)
{
    QNDEBUG(QStringLiteral("MainWindow::onTextToFindInsideNoteEdited: ") << textToFind);

    bool matchCase = m_pUI->findAndReplaceWidget->matchCase();
    onFindNextInsideNote(textToFind, matchCase);
}

#define CHECK_FIND_AND_REPLACE_WIDGET_STATE() \
    if (Q_UNLIKELY(m_pUI->findAndReplaceWidget->isHidden())) { \
        QNTRACE(QStringLiteral("Find and replace widget is not shown, nothing to do")); \
        return; \
    }

void MainWindow::onFindNextInsideNote(const QString & textToFind, const bool matchCase)
{
    QNDEBUG(QStringLiteral("MainWindow::onFindNextInsideNote: text to find = ") << textToFind << QStringLiteral(", match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pNoteEditor->findNext(textToFind, matchCase);
}

void MainWindow::onFindPreviousInsideNote(const QString & textToFind, const bool matchCase)
{
    QNDEBUG(QStringLiteral("MainWindow::onFindPreviousInsideNote: text to find = ") << textToFind << QStringLiteral(", match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pNoteEditor->findPrevious(textToFind, matchCase);
}

void MainWindow::onFindInsideNoteCaseSensitivityChanged(const bool matchCase)
{
    QNDEBUG(QStringLiteral("MainWindow::onFindInsideNoteCaseSensitivityChanged: match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()

    QString textToFind = m_pUI->findAndReplaceWidget->textToFind();
    m_pNoteEditor->findNext(textToFind, matchCase);
}

void MainWindow::onReplaceInsideNote(const QString & textToReplace, const QString & replacementText, const bool matchCase)
{
    QNDEBUG(QStringLiteral("MainWindow::onReplaceInsideNote: text to replace = ") << textToReplace << QStringLiteral(", replacement text = ")
            << replacementText << QStringLiteral(", match case = ") << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUI->findAndReplaceWidget->setReplaceEnabled(true);

    m_pNoteEditor->replace(textToReplace, replacementText, matchCase);
}

void MainWindow::onReplaceAllInsideNote(const QString & textToReplace, const QString & replacementText, const bool matchCase)
{
    QNDEBUG(QStringLiteral("MainWindow::onReplaceAllInsideNote: text to replace = ") << textToReplace << QStringLiteral(", replacement text = ")
            << replacementText << QStringLiteral(", match case = ") << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUI->findAndReplaceWidget->setReplaceEnabled(true);

    m_pNoteEditor->replaceAll(textToReplace, replacementText, matchCase);
}

#undef CHECK_FIND_AND_REPLACE_WIDGET_STATE

void MainWindow::onNoteEditorHtmlUpdate(QString html)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorHtmlUpdate"));
    QNTRACE(QStringLiteral("Html: ") << html);

    m_lastNoteEditorHtml = html;

    if (!m_pUI->noteSourceView->isVisible()) {
        return;
    }

    updateNoteHtmlView(html);
}

void MainWindow::onNoteEditorError(QNLocalizedString error)
{
    QNINFO(QStringLiteral("MainWindow::onNoteEditorError: ") << error);
    onSetStatusBarText(error.localizedString(), 20000);
}

void MainWindow::onNoteEditorSpellCheckerNotReady()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorSpellCheckerNotReady"));
    onSetStatusBarText(tr("Spell checker is loading dictionaries, please wait"));
}

void MainWindow::onNoteEditorSpellCheckerReady()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorSpellCheckerReady"));
    onSetStatusBarText(QString());
}

void MainWindow::onNoteEditorBoldStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorBoldStateChanged: ") << (state ? QStringLiteral("bold") : QStringLiteral("not bold")));
    m_pUI->fontBoldPushButton->setChecked(state);
}

void MainWindow::onNoteEditorItalicStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorItalicStateChanged: ") << (state ? QStringLiteral("italic") : QStringLiteral("not italic")));
    m_pUI->fontItalicPushButton->setChecked(state);
}

void MainWindow::onNoteEditorUnderlineStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorUnderlineStateChanged: ") << (state ? QStringLiteral("underline") : QStringLiteral("not underline")));
    m_pUI->fontUnderlinePushButton->setChecked(state);
}

void MainWindow::onNoteEditorStrikethroughStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorStrikethroughStateChanged: ") << (state ? QStringLiteral("strikethrough") : QStringLiteral("not strikethrough")));
    m_pUI->fontStrikethroughPushButton->setChecked(state);
}

void MainWindow::onNoteEditorAlignLeftStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorAlignLeftStateChanged: ") << (state ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUI->formatJustifyLeftPushButton->setChecked(state);

    if (state) {
        m_pUI->formatJustifyCenterPushButton->setChecked(false);
        m_pUI->formatJustifyRightPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorAlignCenterStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorAlignCenterStateChanged: ") << (state ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUI->formatJustifyCenterPushButton->setChecked(state);

    if (state) {
        m_pUI->formatJustifyLeftPushButton->setChecked(false);
        m_pUI->formatJustifyRightPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorAlignRightStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorAlignRightStateChanged: ") << (state ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUI->formatJustifyRightPushButton->setChecked(state);

    if (state) {
        m_pUI->formatJustifyLeftPushButton->setChecked(false);
        m_pUI->formatJustifyCenterPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorInsideOrderedListStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorInsideOrderedListStateChanged: ") << (state ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUI->formatListOrderedPushButton->setChecked(state);

    if (state) {
        m_pUI->formatListUnorderedPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorInsideUnorderedListStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorInsideUnorderedListStateChanged: ") << (state ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUI->formatListUnorderedPushButton->setChecked(state);

    if (state) {
        m_pUI->formatListOrderedPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorInsideTableStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorInsideTableStateChanged: ") << (state ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUI->insertTableToolButton->setEnabled(!state);
}

void MainWindow::onNoteEditorFontFamilyChanged(QString fontFamily)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorFontFamilyChanged: font family = ") << fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE(QStringLiteral("Font family didn't change"));
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    QFont currentFont(fontFamily);
    m_pUI->fontComboBox->setCurrentFont(currentFont);
    QNTRACE(QStringLiteral("Font family from combo box: ") << m_pUI->fontComboBox->currentFont().family()
            << QStringLiteral(", font family set by QFont's constructor from it: ") << currentFont.family());

    QFontDatabase fontDatabase;
    QList<int> fontSizes = fontDatabase.pointSizes(currentFont.family(), currentFont.styleName());
    // NOTE: it is important to use currentFont.family() in the call above instead of fontFamily variable
    // because the two can be different by presence/absence of apostrophes around the font family name
    if (fontSizes.isEmpty()) {
        QNTRACE(QStringLiteral("Coulnd't find point sizes for font family ") << currentFont.family()
                << QStringLiteral(", will use standard sizes instead"));
        fontSizes = fontDatabase.standardSizes();
    }

    m_lastFontSizeComboBoxIndex = 0;    // NOTE: clearing out font sizes combo box causes unwanted update of its index to 0, workarounding it
    m_pUI->fontSizeComboBox->clear();
    int numFontSizes = fontSizes.size();
    QNTRACE(QStringLiteral("Found ") << numFontSizes << QStringLiteral(" font sizes for font family ") << currentFont.family());

    for(int i = 0; i < numFontSizes; ++i) {
        m_pUI->fontSizeComboBox->addItem(QString::number(fontSizes[i]), QVariant(fontSizes[i]));
        QNTRACE(QStringLiteral("Added item ") << fontSizes[i] << QStringLiteral("pt for index ") << i);
    }
    m_lastFontSizeComboBoxIndex = -1;
}

void MainWindow::onNoteEditorFontSizeChanged(int fontSize)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorFontSizeChanged: font size = ") << fontSize);
    int fontSizeIndex = m_pUI->fontSizeComboBox->findData(QVariant(fontSize), Qt::UserRole);
    if (fontSizeIndex >= 0)
    {
        m_lastFontSizeComboBoxIndex = fontSizeIndex;
        if (m_pUI->fontSizeComboBox->currentIndex() != fontSizeIndex) {
            m_pUI->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
            QNTRACE(QStringLiteral("fontSizeComboBox: set current index to ") << fontSizeIndex
                    << QStringLiteral(", found font size = ") << QVariant(fontSize));
        }
    }
    else
    {
        QNDEBUG(QStringLiteral("Can't find font size ") << fontSize
                << QStringLiteral(" within those listed in font size combobox, will try to choose the closest one instead"));
        const int numFontSizes = m_pUI->fontSizeComboBox->count();
        int currentSmallestDiscrepancy = 1e5;
        int currentClosestIndex = -1;
        for(int i = 0; i < numFontSizes; ++i)
        {
            QVariant value = m_pUI->fontSizeComboBox->itemData(i, Qt::UserRole);
            bool conversionResult = false;
            int valueInt = value.toInt(&conversionResult);
            if (!conversionResult) {
                QNWARNING(QStringLiteral("Can't convert value from font size combo box to int: ") << value);
                continue;
            }

            int discrepancy = abs(valueInt - fontSize);
            if (currentSmallestDiscrepancy > discrepancy) {
                currentSmallestDiscrepancy = discrepancy;
                currentClosestIndex = i;
                QNTRACE(QStringLiteral("Updated current closest index to ") << i << QStringLiteral(": font size = ") << valueInt);
            }
        }

        if (currentClosestIndex >= 0) {
            QNTRACE(QStringLiteral("Setting current font size index to ") << currentClosestIndex);
            m_lastFontSizeComboBoxIndex = currentClosestIndex;
            if (m_pUI->fontSizeComboBox->currentIndex() != currentClosestIndex) {
                m_pUI->fontSizeComboBox->setCurrentIndex(currentClosestIndex);
            }
        }
        else {
            QNDEBUG(QStringLiteral("Couldn't find closest font size to ") << fontSize);
        }
    }
}

void MainWindow::onFontComboBoxFontChanged(QFont font)
{
    QNDEBUG(QStringLiteral("MainWindow::onFontComboBoxFontChanged: font family = ") << font.family());

    m_pNoteEditor->setFont(font);
    m_pNoteEditor->setFocus();
}

void MainWindow::onFontSizeComboBoxIndexChanged(int currentIndex)
{
    QNDEBUG(QStringLiteral("MainWindow::onFontSizeComboBoxIndexChanged: current index = ") << currentIndex);

    if (currentIndex == m_lastFontSizeComboBoxIndex) {
        QNTRACE(QStringLiteral("Already cached that index"));
        return;
    }

    if (Q_UNLIKELY(!m_pNoteEditor)) {
        QNDEBUG(QStringLiteral("Note editor is not set"));
        return;
    }

    if (Q_UNLIKELY(currentIndex < 0)) {
        QNDEBUG(QStringLiteral("Invalid font size combo box index = ") << currentIndex);
        return;
    }

    if (m_pUI->fontSizeComboBox->count() == 0) {
        QNDEBUG(QStringLiteral("Font size combo box is empty"));
        return;
    }

    QVariant value = m_pUI->fontSizeComboBox->itemData(currentIndex, Qt::UserRole);
    bool conversionResult = false;
    int valueInt = value.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(QStringLiteral("Can't convert font size combo box value to int: ") << value);
        return;
    }

    m_lastFontSizeComboBoxIndex = currentIndex;

    QNTRACE(QStringLiteral("Parsed font size ") << valueInt << QStringLiteral(" from value ") << value);
    m_pNoteEditor->setFontHeight(valueInt);
}

void MainWindow::onAddAccountActionTriggered(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onAddAccountActionTriggered"));

    Q_UNUSED(checked)

    // TODO: implement
}

void MainWindow::onManageAccountsActionTriggered(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onManageAccountsActionTriggered"));

    Q_UNUSED(checked)

    QScopedPointer<ManageAccountsDialog> manageAccountsDialog(new ManageAccountsDialog(m_availableAccounts, this));
    manageAccountsDialog->setWindowModality(Qt::WindowModal);
    // TODO: setup some signal-slot connections for doing the actual work via the dialog?
    Q_UNUSED(manageAccountsDialog->exec())
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

    if (!QIcon::hasThemeIcon(QStringLiteral("checkbox"))) {
        m_pUI->insertToDoCheckboxPushButton->setIcon(QIcon(QStringLiteral(":/fallback_icons/png/checkbox-2.png")));
        QNTRACE(QStringLiteral("set fallback checkbox icon"));
    }

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
        m_pUI->copyPushButton->setIcon(editCopyIcon);
        m_pUI->ActionCopy->setIcon(editCopyIcon);
        QNTRACE(QStringLiteral("set fallback edit-copy icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-cut"))) {
        QIcon editCutIcon(QStringLiteral(":/fallback_icons/png/edit-cut-6.png"));
        m_pUI->cutPushButton->setIcon(editCutIcon);
        m_pUI->ActionCut->setIcon(editCutIcon);
        QNTRACE(QStringLiteral("set fallback edit-cut icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-delete"))) {
        QIcon editDeleteIcon(QStringLiteral(":/fallback_icons/png/edit-delete-6.png"));
        m_pUI->ActionNoteDelete->setIcon(editDeleteIcon);
        m_pUI->deleteTagButton->setIcon(editDeleteIcon);
        QNTRACE(QStringLiteral("set fallback edit-delete icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-find"))) {
        m_pUI->ActionFindInsideNote->setIcon(QIcon(QStringLiteral(":/fallback_icons/png/edit-find-7.png")));
        QNTRACE(QStringLiteral("set fallback edit-find icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-paste"))) {
        QIcon editPasteIcon(QStringLiteral(":/fallback_icons/png/edit-paste-6.png"));
        m_pUI->pastePushButton->setIcon(editPasteIcon);
        m_pUI->ActionPaste->setIcon(editPasteIcon);
        QNTRACE(QStringLiteral("set fallback edit-paste icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-redo"))) {
        QIcon editRedoIcon(QStringLiteral(":/fallback_icons/png/edit-redo-7.png"));
        m_pUI->redoPushButton->setIcon(editRedoIcon);
        m_pUI->ActionRedo->setIcon(editRedoIcon);
        QNTRACE(QStringLiteral("set fallback edit-redo icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-undo"))) {
        QIcon editUndoIcon(QStringLiteral(":/fallback_icons/png/edit-undo-7.png"));
        m_pUI->undoPushButton->setIcon(editUndoIcon);
        m_pUI->ActionUndo->setIcon(editUndoIcon);
        QNTRACE(QStringLiteral("set fallback edit-undo icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-indent-less"))) {
        QIcon formatIndentLessIcon(QStringLiteral(":/fallback_icons/png/format-indent-less-5.png"));
        m_pUI->formatIndentLessPushButton->setIcon(formatIndentLessIcon);
        m_pUI->ActionDecreaseIndentation->setIcon(formatIndentLessIcon);
        QNTRACE(QStringLiteral("set fallback format-indent-less icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-indent-more"))) {
        QIcon formatIndentMoreIcon(QStringLiteral(":/fallback_icons/png/format-indent-more-5.png"));
        m_pUI->formatIndentMorePushButton->setIcon(formatIndentMoreIcon);
        m_pUI->ActionIncreaseIndentation->setIcon(formatIndentMoreIcon);
        QNTRACE(QStringLiteral("set fallback format-indent-more icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-justify-center"))) {
        QIcon formatJustifyCenterIcon(QStringLiteral(":/fallback_icons/png/format-justify-center-5.png"));
        m_pUI->formatJustifyCenterPushButton->setIcon(formatJustifyCenterIcon);
        m_pUI->ActionAlignCenter->setIcon(formatJustifyCenterIcon);
        QNTRACE(QStringLiteral("set fallback format-justify-center icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-justify-left"))) {
        QIcon formatJustifyLeftIcon(QStringLiteral(":/fallback_icons/png/format-justify-left-5.png"));
        m_pUI->formatJustifyLeftPushButton->setIcon(formatJustifyLeftIcon);
        m_pUI->ActionAlignLeft->setIcon(formatJustifyLeftIcon);
        QNTRACE(QStringLiteral("set fallback format-justify-left icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-justify-right"))) {
        QIcon formatJustifyRightIcon(QStringLiteral(":/fallback_icons/png/format-justify-right-5.png"));
        m_pUI->formatJustifyRightPushButton->setIcon(formatJustifyRightIcon);
        m_pUI->ActionAlignRight->setIcon(formatJustifyRightIcon);
        QNTRACE(QStringLiteral("set fallback format-justify-right icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-list-ordered"))) {
        QIcon formatListOrderedIcon(QStringLiteral(":/fallback_icons/png/format-list-ordered.png"));
        m_pUI->formatListOrderedPushButton->setIcon(formatListOrderedIcon);
        m_pUI->ActionInsertNumberedList->setIcon(formatListOrderedIcon);
        QNTRACE(QStringLiteral("set fallback format-list-ordered icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-list-unordered"))) {
        QIcon formatListUnorderedIcon(QStringLiteral(":/fallback_icons/png/format-list-unordered.png"));
        m_pUI->formatListUnorderedPushButton->setIcon(formatListUnorderedIcon);
        m_pUI->ActionInsertBulletedList->setIcon(formatListUnorderedIcon);
        QNTRACE(QStringLiteral("set fallback format-list-unordered icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-bold"))) {
        QIcon formatTextBoldIcon(QStringLiteral(":/fallback_icons/png/format-text-bold-4.png"));
        m_pUI->fontBoldPushButton->setIcon(formatTextBoldIcon);
        m_pUI->ActionFontBold->setIcon(formatTextBoldIcon);
        QNTRACE(QStringLiteral("set fallback format-text-bold icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-color"))) {
        QIcon formatTextColorIcon(QStringLiteral(":/fallback_icons/png/format-text-color.png"));
        m_pUI->chooseTextColorToolButton->setIcon(formatTextColorIcon);
        QNTRACE(QStringLiteral("set fallback format-text-color icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("color-fill"))) {
        QIcon colorFillIcon(QStringLiteral(":/fallback_icons/png/color-fill.png"));
        m_pUI->chooseBackgroundColorToolButton->setIcon(colorFillIcon);
        QNTRACE(QStringLiteral("set fallback color-fill icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-italic"))) {
        QIcon formatTextItalicIcon(QStringLiteral(":/fallback_icons/png/format-text-italic-4.png"));
        m_pUI->fontItalicPushButton->setIcon(formatTextItalicIcon);
        m_pUI->ActionFontItalic->setIcon(formatTextItalicIcon);
        QNTRACE(QStringLiteral("set fallback format-text-italic icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-strikethrough"))) {
        QIcon formatTextStrikethroughIcon(QStringLiteral(":/fallback_icons/png/format-text-strikethrough-3.png"));
        m_pUI->fontStrikethroughPushButton->setIcon(formatTextStrikethroughIcon);
        m_pUI->ActionFontStrikethrough->setIcon(formatTextStrikethroughIcon);
        QNTRACE(QStringLiteral("set fallback format-text-strikethrough icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("format-text-underline"))) {
        QIcon formatTextUnderlineIcon(QStringLiteral(":/fallback_icons/png/format-text-underline-4.png"));
        m_pUI->fontUnderlinePushButton->setIcon(formatTextUnderlineIcon);
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
        m_pUI->insertHorizontalLinePushButton->setIcon(insertHorizontalRuleIcon);
        m_pUI->ActionInsertHorizontalLine->setIcon(insertHorizontalRuleIcon);
        QNTRACE(QStringLiteral("set fallback insert-horizontal-rule icon"));
    }

    if (!QIcon::hasThemeIcon(QStringLiteral("insert-table"))) {
        QIcon insertTableIcon(QStringLiteral(":/fallback_icons/png/insert-table.png"));
        m_pUI->ActionInsertTable->setIcon(insertTableIcon);
        m_pUI->menuTable->setIcon(insertTableIcon);
        m_pUI->insertTableToolButton->setIcon(insertTableIcon);
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

    if (!QIcon::hasThemeIcon(QStringLiteral("tools-check-spelling"))) {
        QIcon spellcheckIcon(QStringLiteral(":/fallback_icons/png/tools-check-spelling-5.png"));
        m_pUI->spellCheckBox->setIcon(spellcheckIcon);
        QNTRACE(QStringLiteral("set fallback tools-check-spelling icon"));
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

void MainWindow::updateNoteHtmlView(QString html)
{
    m_pUI->noteSourceView->setPlainText(html);
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
