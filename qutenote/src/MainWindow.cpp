#include "MainWindow.h"
#include "color-picker-tool-button/ColorPickerToolButton.h"
#include "insert-table-tool-button/InsertTableToolButton.h"
#include "insert-table-tool-button/TableSettingsDialog.h"
#include "tests/ManualTestingHelper.h"
#include "BasicXMLSyntaxHighlighter.h"

#include <qute_note/note_editor/NoteEditor.h>
using qute_note::NoteEditor;    // workarouding Qt4 Designer's inability to work with namespaces
#include "ui_MainWindow.h"

#include <qute_note/types/Note.h>
#include <qute_note/types/Notebook.h>
#include <qute_note/types/ResourceWrapper.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <Simplecrypt.h>
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

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

using namespace qute_note;

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow),
    m_currentStatusBarChildWidget(Q_NULLPTR),
    m_pNoteEditor(Q_NULLPTR),
    m_lastNoteEditorHtml(),
    m_testNotebook(),
    m_testNote(),
    m_lastFontSizeComboBoxIndex(-1),
    m_lastFontComboBoxFontFamily(),
    m_shortcutManager(this)
{
    QNTRACE("MainWindow constructor");

    m_pUI->setupUi(this);
    setupDefaultShortcuts();
    setupUserShortcuts();

    m_pUI->noteSourceView->setHidden(true);

    m_pUI->fontSizeComboBox->clear();
    QNTRACE("fontSizeComboBox num items: " << m_pUI->fontSizeComboBox->count());
    for(int i = 0; i < m_pUI->fontSizeComboBox->count(); ++i) {
        QVariant value = m_pUI->fontSizeComboBox->itemData(i, Qt::UserRole);
        QNTRACE("Font size value for index[" << i << "] = " << value);
    }

    BasicXMLSyntaxHighlighter * highlighter = new BasicXMLSyntaxHighlighter(m_pUI->noteSourceView->document());
    Q_UNUSED(highlighter);

    m_pNoteEditor = m_pUI->noteEditorWidget;

    checkThemeIconsAndSetFallbacks();

    connectActionsToEditorSlots();
    connectEditorSignalsToSlots();

    // Stuff primarily for manual testing
    QObject::connect(m_pUI->ActionShowNoteSource, QNSIGNAL(QAction, triggered),
                     this, QNSLOT(MainWindow, onShowNoteSource));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor,noteEditorHtmlUpdated,QString),
                     this, QNSLOT(MainWindow,onNoteEditorHtmlUpdate,QString));

    QObject::connect(m_pUI->ActionSetTestNoteWithEncryptedData, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow, onSetTestNoteWithEncryptedData));
    QObject::connect(m_pUI->ActionSetTestNoteWithResources , QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow, onSetTestNoteWithResources));

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
    QObject::connect(m_pUI->fontSizeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onFontSizeComboBoxIndexChanged(int)));
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
        QNTRACE("Returned from TableSettingsDialog::exec: accepted");
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

    QNTRACE("Returned from TableSettingsDialog::exec: rejected");
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

    QNTRACE("Inserted table: rows = " << rows << ", columns = " << columns
            << ", width = " << width << ", relative width = " << (relativeWidth ? "true" : "false"));
    m_pNoteEditor->setFocus();
}

void MainWindow::onShowNoteSource()
{
    QNDEBUG("MainWindow::onShowNoteSource");
    updateNoteHtmlView(m_lastNoteEditorHtml);
    m_pUI->noteSourceView->setHidden(m_pUI->noteSourceView->isVisible());
}

void MainWindow::onSetTestNoteWithEncryptedData()
{
    QNDEBUG("MainWindow::onSetTestNoteWithEncryptedData");

    m_testNote = Note();
    m_testNote.setLocalGuid("{7ae26137-9b62-4c30-85a9-261b435f6db3}");

    QString noteContent = test::ManualTestingHelper::noteContentWithEncryption();
    m_testNote.setContent(noteContent);
    m_pNoteEditor->setNoteAndNotebook(m_testNote, m_testNotebook);
    m_pNoteEditor->setFocus();
}

void MainWindow::onSetTestNoteWithResources()
{
    QNDEBUG("MainWindow::onSetTestNoteWithResources");

    m_testNote = Note();
    m_testNote.setLocalGuid("{ce8e5ea1-28fc-4842-a726-0d4a78dfcbe5}");

    QString noteContent = test::ManualTestingHelper::noteContentWithResources();
    m_testNote.setContent(noteContent);

    // Read the first result from qrc
    QFile resourceFile(":/test_notes/Architecture_whats_the_architecture.png");
    resourceFile.open(QIODevice::ReadOnly);
    QByteArray resourceData = resourceFile.readAll();
    resourceFile.close();

    // Assemble the first resource data
    qute_note::ResourceWrapper resource;
    resource.setLocalGuid("{e2f201df-8718-499b-ac92-4c9970170cba}");
    resource.setDataHash("84f9f1159d922e8c977d9a1539351ccf");
    resource.setDataBody(resourceData);
    resource.setDataSize(resourceData.size());
    resource.setNoteLocalGuid(m_testNote.localGuid());
    resource.setMime("image/png");

    qevercloud::ResourceAttributes resourceAttributes;
    resourceAttributes.fileName = "Architecture_whats_the_architecture.png";

    resource.setResourceAttributes(resourceAttributes);

    // Add the first resource to the note
    m_testNote.addResource(resource);

    // Read the second resource from qrc
    resourceFile.setFileName(":/test_notes/cDock_v8.2.zip");
    resourceFile.open(QIODevice::ReadOnly);
    resourceData = resourceFile.readAll();
    resourceFile.close();

    // Assemble the second resource data
    resource = qute_note::ResourceWrapper();
    resource.setLocalGuid("{c3acdcba-d6a4-407d-a85f-5fc3c15126df}");
    resource.setDataHash("377ce54f92b5f12e83d8eb3867fc1d9a");
    resource.setDataBody(resourceData);
    resource.setDataSize(resourceData.size());
    resource.setMime("application/zip");

    resourceAttributes = qevercloud::ResourceAttributes();
    resourceAttributes.fileName = "cDock_v8.2.zip";

    resource.setResourceAttributes(resourceAttributes);

    // Add the second resource to the note
    m_testNote.addResource(resource);

    // Read the third resource from qrc
    resourceFile.setFileName(":/test_notes/GrowlNotify.pkg");
    resourceFile.open(QIODevice::ReadOnly);
    resourceData = resourceFile.readAll();
    resourceFile.close();

    // Assemble the third resource data
    resource = qute_note::ResourceWrapper();
    resource.setLocalGuid("{d44d85f4-d4e2-4788-a172-4d477741b233}");
    resource.setDataHash("2e0f79af4ca47b473e5105156a18c7cb");
    resource.setDataBody(resourceData);
    resource.setDataSize(resourceData.size());
    resource.setMime("application/octet-stream");

    resourceAttributes = qevercloud::ResourceAttributes();
    resourceAttributes.fileName = "GrowlNotify.pkg";

    resource.setResourceAttributes(resourceAttributes);

    // Add the third resource to the note
    m_testNote.addResource(resource);

    m_pNoteEditor->setNoteAndNotebook(m_testNote, m_testNotebook);
    m_pNoteEditor->setFocus();
}

void MainWindow::onNoteEditorHtmlUpdate(QString html)
{
    QNDEBUG("MainWindow::onNoteEditorHtmlUpdate");
    QNTRACE("Html: " << html);

    m_lastNoteEditorHtml = html;

    if (!m_pUI->noteSourceView->isVisible()) {
        return;
    }

    updateNoteHtmlView(html);
}

void MainWindow::onNoteEditorError(QString error)
{
    QNINFO("MainWindow::onNoteEditorError: " << error);

    onSetStatusBarText(error, 20);
}

void MainWindow::onNoteEditorBoldStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorBoldStateChanged: " << (state ? "bold" : "not bold"));
    m_pUI->fontBoldPushButton->setChecked(state);
}

void MainWindow::onNoteEditorItalicStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorItalicStateChanged: " << (state ? "italic" : "not italic"));
    m_pUI->fontItalicPushButton->setChecked(state);
}

void MainWindow::onNoteEditorUnderlineStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorUnderlineStateChanged: " << (state ? "underline" : "not underline"));
    m_pUI->fontUnderlinePushButton->setChecked(state);
}

void MainWindow::onNoteEditorStrikethroughStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorStrikethroughStateChanged: " << (state ? "strikethrough" : "not strikethrough"));
    m_pUI->fontStrikethroughPushButton->setChecked(state);
}

void MainWindow::onNoteEditorAlignLeftStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorAlignLeftStateChanged: " << (state ? "true" : "false"));
    m_pUI->formatJustifyLeftPushButton->setChecked(state);

    if (state) {
        m_pUI->formatJustifyCenterPushButton->setChecked(false);
        m_pUI->formatJustifyRightPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorAlignCenterStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorAlignCenterStateChanged: " << (state ? "true" : "false"));
    m_pUI->formatJustifyCenterPushButton->setChecked(state);

    if (state) {
        m_pUI->formatJustifyLeftPushButton->setChecked(false);
        m_pUI->formatJustifyRightPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorAlignRightStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorAlignRightStateChanged: " << (state ? "true" : "false"));
    m_pUI->formatJustifyRightPushButton->setChecked(state);

    if (state) {
        m_pUI->formatJustifyLeftPushButton->setChecked(false);
        m_pUI->formatJustifyCenterPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorInsideOrderedListStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorInsideOrderedListStateChanged: " << (state ? "true" : "false"));
    m_pUI->formatListOrderedPushButton->setChecked(state);

    if (state) {
        m_pUI->formatListUnorderedPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorInsideUnorderedListStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorInsideUnorderedListStateChanged: " << (state ? "true" : "false"));
    m_pUI->formatListUnorderedPushButton->setChecked(state);

    if (state) {
        m_pUI->formatListOrderedPushButton->setChecked(false);
    }
}

void MainWindow::onNoteEditorInsideTableStateChanged(bool state)
{
    QNDEBUG("MainWindow::onNoteEditorInsideTableStateChanged: " << (state ? "true" : "false"));
    m_pUI->insertTableToolButton->setEnabled(!state);
}

void MainWindow::onNoteEditorFontFamilyChanged(QString fontFamily)
{
    QNDEBUG("MainWindow::onNoteEditorFontFamilyChanged: font family = " << fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE("Font family didn't change");
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    QFont currentFont(fontFamily);
    m_pUI->fontComboBox->setCurrentFont(currentFont);
    QNTRACE("Font family from combo box: " << m_pUI->fontComboBox->currentFont().family()
            << ", font family set by QFont's constructor from it: " << currentFont.family());

    QFontDatabase fontDatabase;
    QList<int> fontSizes = fontDatabase.pointSizes(currentFont.family(), currentFont.styleName());
    // NOTE: it is important to use currentFont.family() in the call above instead of fontFamily variable
    // because the two can be different by presence/absence of apostrophes around the font family name
    if (fontSizes.isEmpty()) {
        QNTRACE("Coulnd't find point sizes for font family " << currentFont.family() << ", will use standard sizes instead");
        fontSizes = fontDatabase.standardSizes();
    }

    m_lastFontSizeComboBoxIndex = 0;    // NOTE: clearing out font sizes combo box causes unwanted update of its index to 0, workarounding it
    m_pUI->fontSizeComboBox->clear();
    int numFontSizes = fontSizes.size();
    QNTRACE("Found " << numFontSizes << " font sizes for font family " << currentFont.family());

    for(int i = 0; i < numFontSizes; ++i) {
        m_pUI->fontSizeComboBox->addItem(QString::number(fontSizes[i]), QVariant(fontSizes[i]));
        QNTRACE("Added item " << fontSizes[i] << "pt for index " << i);
    }
    m_lastFontSizeComboBoxIndex = -1;
}

void MainWindow::onNoteEditorFontSizeChanged(int fontSize)
{
    QNDEBUG("MainWindow::onNoteEditorFontSizeChanged: font size = " << fontSize);
    int fontSizeIndex = m_pUI->fontSizeComboBox->findData(QVariant(fontSize), Qt::UserRole);
    if (fontSizeIndex >= 0)
    {
        m_lastFontSizeComboBoxIndex = fontSizeIndex;
        if (m_pUI->fontSizeComboBox->currentIndex() != fontSizeIndex) {
            m_pUI->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
            QNTRACE("fontSizeComboBox: set current index to " << fontSizeIndex << ", found font size = " << QVariant(fontSize));
        }
    }
    else
    {
        QNDEBUG("Can't find font size " << fontSize << " within those listed in font size combobox, "
                "will try to choose the closest one instead");
        const int numFontSizes = m_pUI->fontSizeComboBox->count();
        int currentSmallestDiscrepancy = 1e5;
        int currentClosestIndex = -1;
        for(int i = 0; i < numFontSizes; ++i)
        {
            QVariant value = m_pUI->fontSizeComboBox->itemData(i, Qt::UserRole);
            bool conversionResult = false;
            int valueInt = value.toInt(&conversionResult);
            if (!conversionResult) {
                QNWARNING("Can't convert value from font size combo box to int: " << value);
                continue;
            }

            int discrepancy = abs(valueInt - fontSize);
            if (currentSmallestDiscrepancy > discrepancy) {
                currentSmallestDiscrepancy = discrepancy;
                currentClosestIndex = i;
                QNTRACE("Updated current closest index to " << i << ": font size = " << valueInt);
            }
        }

        if (currentClosestIndex >= 0) {
            QNTRACE("Setting current font size index to " << currentClosestIndex);
            m_lastFontSizeComboBoxIndex = currentClosestIndex;
            if (m_pUI->fontSizeComboBox->currentIndex() != currentClosestIndex) {
                m_pUI->fontSizeComboBox->setCurrentIndex(currentClosestIndex);
            }
        }
        else {
            QNDEBUG("Couldn't find closest font size to " << fontSize);
        }
    }
}

void MainWindow::onFontComboBoxFontChanged(QFont font)
{
    QNDEBUG("MainWindow::onFontComboBoxFontChanged: font family = " << font.family());

    m_pNoteEditor->setFont(font);
    m_pNoteEditor->setFocus();
}

void MainWindow::onFontSizeComboBoxIndexChanged(int currentIndex)
{
    QNDEBUG("MainWindow::onFontSizeComboBoxIndexChanged: current index = " << currentIndex);

    if (currentIndex == m_lastFontSizeComboBoxIndex) {
        QNTRACE("Already cached that index");
        return;
    }

    if (Q_UNLIKELY(!m_pNoteEditor)) {
        QNDEBUG("Note editor is not set");
        return;
    }

    if (Q_UNLIKELY(currentIndex < 0)) {
        QNDEBUG("Invalid font size combo box index = " << currentIndex);
        return;
    }

    if (m_pUI->fontSizeComboBox->count() == 0) {
        QNDEBUG("Font size combo box is empty");
        return;
    }

    QVariant value = m_pUI->fontSizeComboBox->itemData(currentIndex, Qt::UserRole);
    bool conversionResult = false;
    int valueInt = value.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING("Can't convert font size combo box value to int: " << value);
        return;
    }

    m_lastFontSizeComboBoxIndex = currentIndex;

    QNTRACE("Parsed font size " << valueInt << " from value " << value);
    m_pNoteEditor->setFontHeight(valueInt);
}

void MainWindow::checkThemeIconsAndSetFallbacks()
{
    QNTRACE("MainWindow::checkThemeIconsAndSetFallbacks");

    if (!QIcon::hasThemeIcon("checkbox")) {
        m_pUI->insertToDoCheckboxPushButton->setIcon(QIcon(":/fallback_icons/png/checkbox-2.png"));
        QNTRACE("set fallback checkbox icon");
    }

    if (!QIcon::hasThemeIcon("dialog-information")) {
        m_pUI->ActionShowNoteAttributesButton->setIcon(QIcon(":/fallback_icons/png/dialog-information-4.png"));
        QNTRACE("set fallback dialog-information icon");
    }

    if (!QIcon::hasThemeIcon("document-new")) {
        m_pUI->ActionNoteAdd->setIcon(QIcon(":/fallback_icons/png/document-new-6.png"));
        QNTRACE("set fallback document-new icon");
    }

    if (!QIcon::hasThemeIcon("printer")) {
        m_pUI->ActionPrint->setIcon(QIcon(":/fallback_icons/png/document-print-5.png"));
        QNTRACE("set fallback printer icon");
    }

    if (!QIcon::hasThemeIcon("document-save")) {
        QIcon documentSaveIcon(":/fallback_icons/png/document-save-5.png");
        m_pUI->saveSearchPushButton->setIcon(documentSaveIcon);
        m_pUI->ActionSaveImage->setIcon(documentSaveIcon);
        QNTRACE("set fallback document-save icon");
    }

    if (!QIcon::hasThemeIcon("edit-copy")) {
        QIcon editCopyIcon(":/fallback_icons/png/edit-copy-6.png");
        m_pUI->copyPushButton->setIcon(editCopyIcon);
        m_pUI->ActionCopy->setIcon(editCopyIcon);
        QNTRACE("set fallback edit-copy icon");
    }

    if (!QIcon::hasThemeIcon("edit-cut")) {
        QIcon editCutIcon(":/fallback_icons/png/edit-cut-6.png");
        m_pUI->cutPushButton->setIcon(editCutIcon);
        m_pUI->ActionCut->setIcon(editCutIcon);
        QNTRACE("set fallback edit-cut icon");
    }

    if (!QIcon::hasThemeIcon("edit-delete")) {
        QIcon editDeleteIcon(":/fallback_icons/png/edit-delete-6.png");
        m_pUI->ActionNoteDelete->setIcon(editDeleteIcon);
        m_pUI->deleteTagButton->setIcon(editDeleteIcon);
        QNTRACE("set fallback edit-delete icon");
    }

    if (!QIcon::hasThemeIcon("edit-find")) {
        m_pUI->ActionFindInsideNote->setIcon(QIcon(":/fallback_icons/png/edit-find-7.png"));
        QNTRACE("set fallback edit-find icon");
    }

    if (!QIcon::hasThemeIcon("edit-paste")) {
        QIcon editPasteIcon(":/fallback_icons/png/edit-paste-6.png");
        m_pUI->pastePushButton->setIcon(editPasteIcon);
        m_pUI->ActionPaste->setIcon(editPasteIcon);
        QNTRACE("set fallback edit-paste icon");
    }

    if (!QIcon::hasThemeIcon("edit-redo")) {
        QIcon editRedoIcon(":/fallback_icons/png/edit-redo-7.png");
        m_pUI->redoPushButton->setIcon(editRedoIcon);
        m_pUI->ActionRedo->setIcon(editRedoIcon);
        QNTRACE("set fallback edit-redo icon");
    }

    if (!QIcon::hasThemeIcon("edit-undo")) {
        QIcon editUndoIcon(":/fallback_icons/png/edit-undo-7.png");
        m_pUI->undoPushButton->setIcon(editUndoIcon);
        m_pUI->ActionUndo->setIcon(editUndoIcon);
        QNTRACE("set fallback edit-undo icon");
    }

    if (!QIcon::hasThemeIcon("format-indent-less")) {
        QIcon formatIndentLessIcon(":/fallback_icons/png/format-indent-less-5.png");
        m_pUI->formatIndentLessPushButton->setIcon(formatIndentLessIcon);
        m_pUI->ActionDecreaseIndentation->setIcon(formatIndentLessIcon);
        QNTRACE("set fallback format-indent-less icon");
    }

    if (!QIcon::hasThemeIcon("format-indent-more")) {
        QIcon formatIndentMoreIcon(":/fallback_icons/png/format-indent-more-5.png");
        m_pUI->formatIndentMorePushButton->setIcon(formatIndentMoreIcon);
        m_pUI->ActionIncreaseIndentation->setIcon(formatIndentMoreIcon);
        QNTRACE("set fallback format-indent-more icon");
    }

    if (!QIcon::hasThemeIcon("format-justify-center")) {
        QIcon formatJustifyCenterIcon(":/fallback_icons/png/format-justify-center-5.png");
        m_pUI->formatJustifyCenterPushButton->setIcon(formatJustifyCenterIcon);
        m_pUI->ActionAlignCenter->setIcon(formatJustifyCenterIcon);
        QNTRACE("set fallback format-justify-center icon");
    }

    if (!QIcon::hasThemeIcon("format-justify-left")) {
        QIcon formatJustifyLeftIcon(":/fallback_icons/png/format-justify-left-5.png");
        m_pUI->formatJustifyLeftPushButton->setIcon(formatJustifyLeftIcon);
        m_pUI->ActionAlignLeft->setIcon(formatJustifyLeftIcon);
        QNTRACE("set fallback format-justify-left icon");
    }

    if (!QIcon::hasThemeIcon("format-justify-right")) {
        QIcon formatJustifyRightIcon(":/fallback_icons/png/format-justify-right-5.png");
        m_pUI->formatJustifyRightPushButton->setIcon(formatJustifyRightIcon);
        m_pUI->ActionAlignRight->setIcon(formatJustifyRightIcon);
        QNTRACE("set fallback format-justify-right icon");
    }

    if (!QIcon::hasThemeIcon("format-list-ordered")) {
        QIcon formatListOrderedIcon(":/fallback_icons/png/format-list-ordered.png");
        m_pUI->formatListOrderedPushButton->setIcon(formatListOrderedIcon);
        m_pUI->ActionInsertNumberedList->setIcon(formatListOrderedIcon);
        QNTRACE("set fallback format-list-ordered icon");
    }

    if (!QIcon::hasThemeIcon("format-list-unordered")) {
        QIcon formatListUnorderedIcon(":/fallback_icons/png/format-list-unordered.png");
        m_pUI->formatListUnorderedPushButton->setIcon(formatListUnorderedIcon);
        m_pUI->ActionInsertBulletedList->setIcon(formatListUnorderedIcon);
        QNTRACE("set fallback format-list-unordered icon");
    }

    if (!QIcon::hasThemeIcon("format-text-bold")) {
        QIcon formatTextBoldIcon(":/fallback_icons/png/format-text-bold-4.png");
        m_pUI->fontBoldPushButton->setIcon(formatTextBoldIcon);
        m_pUI->ActionFontBold->setIcon(formatTextBoldIcon);
        QNTRACE("set fallback format-text-bold icon");
    }

    if (!QIcon::hasThemeIcon("format-text-color")) {
        QIcon formatTextColorIcon(":/fallback_icons/png/format-text-color.png");
        m_pUI->chooseTextColorToolButton->setIcon(formatTextColorIcon);
        QNTRACE("set fallback format-text-color icon");
    }

    if (!QIcon::hasThemeIcon("color-fill")) {
        QIcon colorFillIcon(":/fallback_icons/png/color-fill.png");
        m_pUI->chooseBackgroundColorToolButton->setIcon(colorFillIcon);
        QNTRACE("set fallback color-fill icon");
    }

    if (!QIcon::hasThemeIcon("format-text-italic")) {
        QIcon formatTextItalicIcon(":/fallback_icons/png/format-text-italic-4.png");
        m_pUI->fontItalicPushButton->setIcon(formatTextItalicIcon);
        m_pUI->ActionFontItalic->setIcon(formatTextItalicIcon);
        QNTRACE("set fallback format-text-italic icon");
    }

    if (!QIcon::hasThemeIcon("format-text-strikethrough")) {
        QIcon formatTextStrikethroughIcon(":/fallback_icons/png/format-text-strikethrough-3.png");
        m_pUI->fontStrikethroughPushButton->setIcon(formatTextStrikethroughIcon);
        m_pUI->ActionFontStrikethrough->setIcon(formatTextStrikethroughIcon);
        QNTRACE("set fallback format-text-strikethrough icon");
    }

    if (!QIcon::hasThemeIcon("format-text-underline")) {
        QIcon formatTextUnderlineIcon(":/fallback_icons/png/format-text-underline-4.png");
        m_pUI->fontUnderlinePushButton->setIcon(formatTextUnderlineIcon);
        m_pUI->ActionFontUnderlined->setIcon(formatTextUnderlineIcon);
        QNTRACE("set fallback format-text-underline icon");
    }

    if (!QIcon::hasThemeIcon("go-down")) {
        QIcon goDownIcon(":/fallback_icons/png/go-down-7.png");
        m_pUI->ActionGoDown->setIcon(goDownIcon);
        QNTRACE("set fallback go-down icon");
    }

    if (!QIcon::hasThemeIcon("go-up")) {
        QIcon goUpIcon(":/fallback_icons/png/go-up-7.png");
        m_pUI->ActionGoUp->setIcon(goUpIcon);
        QNTRACE("set fallback go-up icon");
    }

    if (!QIcon::hasThemeIcon("go-previous")) {
        QIcon goPreviousIcon(":/fallback_icons/png/go-previous-7.png");
        m_pUI->ActionGoPrevious->setIcon(goPreviousIcon);
        QNTRACE("set fallback go-previous icon");
    }

    if (!QIcon::hasThemeIcon("go-next")) {
        QIcon goNextIcon(":/fallback_icons/png/go-next-7.png");
        m_pUI->ActionGoNext->setIcon(goNextIcon);
        QNTRACE("set fallback go-next icon");
    }

    if (!QIcon::hasThemeIcon("insert-horizontal-rule")) {
        QIcon insertHorizontalRuleIcon(":/fallback_icons/png/insert-horizontal-rule.png");
        m_pUI->insertHorizontalLinePushButton->setIcon(insertHorizontalRuleIcon);
        m_pUI->ActionInsertHorizontalLine->setIcon(insertHorizontalRuleIcon);
        QNTRACE("set fallback insert-horizontal-rule icon");
    }

    if (!QIcon::hasThemeIcon("insert-table")) {
        QIcon insertTableIcon(":/fallback_icons/png/insert-table.png");
        m_pUI->ActionInsertTable->setIcon(insertTableIcon);
        m_pUI->menuTable->setIcon(insertTableIcon);
        m_pUI->insertTableToolButton->setIcon(insertTableIcon);
        QNTRACE("set fallback insert-table icon");
    }

    if (!QIcon::hasThemeIcon("mail-send")) {
        QIcon mailSendIcon(":/fallback_icons/png/mail-forward-5.png");
        m_pUI->ActionSendMail->setIcon(mailSendIcon);
        QNTRACE("set fallback mail-send icon");
    }

    if (!QIcon::hasThemeIcon("preferences-other")) {
        QIcon preferencesOtherIcon(":/fallback_icons/png/preferences-other-3.png");
        m_pUI->ActionPreferences->setIcon(preferencesOtherIcon);
        QNTRACE("set fallback preferences-other icon");
    }

    if (!QIcon::hasThemeIcon("tools-check-spelling")) {
        QIcon spellcheckIcon(":/fallback_icons/png/tools-check-spelling-5.png");
        m_pUI->spellCheckBox->setIcon(spellcheckIcon);
        QNTRACE("set fallback tools-check-spelling icon");
    }

    if (!QIcon::hasThemeIcon("object-rotate-left")) {
        QIcon objectRotateLeftIcon(":/fallback_icons/png/object-rotate-left.png");
        m_pUI->ActionRotateCounterClockwise->setIcon(objectRotateLeftIcon);
        QNTRACE("set fallback object-rotate-left icon");
    }

    if (!QIcon::hasThemeIcon("object-rotate-right")) {
        QIcon objectRotateRightIcon(":/fallback_icons/png/object-rotate-right.png");
        m_pUI->ActionRotateClockwise->setIcon(objectRotateRightIcon);
        QNTRACE("set fallback object-rotate-right icon");
    }
}

void MainWindow::updateNoteHtmlView(QString html)
{
    m_pUI->noteSourceView->setPlainText(html);
}

void MainWindow::setupDefaultShortcuts()
{
    QNDEBUG("MainWindow::setupDefaultShortcuts");

    using qute_note::ShortcutManager;

#define PROCESS_ACTION_SHORTCUT(action, key, ...) \
    { \
        QKeySequence shortcut = m_pUI->Action##action->shortcut(); \
        if (shortcut.isEmpty()) { \
            QNTRACE("No shortcut was found for action " #action); \
        } \
        else { \
            m_shortcutManager.setDefaultShortcut(key, shortcut, QString(#__VA_ARGS__)); \
        } \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, ...) \
    { \
        QKeySequence shortcut = m_pUI->Action##action->shortcut(); \
        if (shortcut.isEmpty()) { \
            QNTRACE("No shortcut was found for action " #action); \
        } \
        else { \
            m_shortcutManager.setNonStandardDefaultShortcut(#action, shortcut, QString(#__VA_ARGS__)); \
        } \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::setupUserShortcuts()
{
    QNDEBUG("MainWindow::setupUserShortcuts");

#define PROCESS_ACTION_SHORTCUT(action, key, ...) \
    { \
        QKeySequence shortcut = m_shortcutManager.shortcut(key, QString(#__VA_ARGS__)); \
        if (shortcut.isEmpty()) { \
            QNTRACE("No shortcut was found for action " #action); \
        } \
        else { \
            m_pUI->Action##action->setShortcut(shortcut); \
        } \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, ...) \
    { \
        QKeySequence shortcut = m_shortcutManager.shortcut(#action, QString(#__VA_ARGS__)); \
        if (shortcut.isEmpty()) { \
            QNTRACE("No shortcut was found for action " #action); \
        } \
        else { \
            m_pUI->Action##action->setShortcut(shortcut); \
        } \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

bool MainWindow::consumerKeyAndSecret(QString & consumerKey, QString & consumerSecret, QString & error)
{
    SimpleCrypt crypto(0xB87F6B9);

    consumerKey = crypto.decryptToString(QByteArray("AwP9s05FRUQgWDvIjk7sru7uV3H5QCBk1W1"
                                                    "fiJsnZrc5qCOs75z4RrgmnN0awQ8d7X7PBu"
                                                    "HHF8JDM33DcxXHSh5Kt8fjQoz5HrPOu8i66"
                                                    "frlRFKguciY7xULwrXdqgECazLB9aveoIo7f"
                                                    "SDRK9FEM0tTOjfdYVGyC3XW86WH42AT/hVpG"
                                                    "QqGKDYNPSJktaeMQ0wVoi7u1iUCMN7L7boxl3"
                                                    "jUO1hw6EjfnO4+f6svSocjkTmrt0qagvxpb1g"
                                                    "xrjdYzOF/7XO9SB8wq0pPihnIvMQAMlVhW9lG"
                                                    "2stiXrgAL0jebYmsHWo1XFNPUN2MGi7eM22uW"
                                                    "fVFAoUW128Qgu/W4+W3dbetmV3NEDjxojsvBn"
                                                    "koe2J8ZyeZ+Ektss4HrzBGTnmH1x0HzZOrMR9"
                                                    "zm9BFP7JADvc2QTku"));
    if (consumerKey.isEmpty()) {
        error = QT_TR_NOOP("Can't decrypt the consumer key");
        return false;
    }

    consumerSecret = crypto.decryptToString(QByteArray("AwNqc1pRUVDQqMs4frw+fU1N9NJay4hlBoiEXZ"
                                                       "sBC7bffG0TKeA3+INU0F49tVjvLNvU3J4haezDi"
                                                       "wrAPVgCBsADTKz5ss3woXfHlyHVUQ7C41Q8FFS6"
                                                       "EPpgT2tM1835rb8H3+FAHF+2mu8QBIVhe0dN1js"
                                                       "S9+F+iTWKyTwMRO1urOLaF17GEHemW5YLlsl3MN"
                                                       "U5bz9Kq8Uw/cWOuo3S2849En8ZFbYmUE9DDGsO7"
                                                       "eRv9lkeMe8PQ5F1GSVMV8grB71nz4E1V4wVrHR1"
                                                       "vRm4WchFO/y2lWzq4DCrVjnqZNCWOrgwH6dnOpHg"
                                                       "glvnsCSN2YhB+i9LhTLvM8qZHN51HZtXEALwSoWX"
                                                       "UZ3fD5jspD0MRZNN+wr+0zfwVovoPIwo7SSkcqIY"
                                                       "1P2QSi+OcisJLerkthnyAPouiatyDYC2PDLhu25iu"
                                                       "09ONDC0KA=="));
    if (consumerSecret.isEmpty()) {
        error = QT_TR_NOOP("Can't decrypt the consumer secret");
        return false;
    }

    return true;
}
