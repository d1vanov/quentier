#include "MainWindow.h"
#include "insert-table-tool-button/InsertTableToolButton.h"
#include "BasicXMLSyntaxHighlighter.h"
#include "TableSettingsDialog.h"

#include <qute_note/note_editor/NoteEditor.h>
using qute_note::NoteEditor;    // workarouding Qt4 Designer's inability to work with namespaces
#include "ui_MainWindow.h"

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
#include <QWebFrame>

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow),
    m_currentStatusBarChildWidget(nullptr),
    m_pNoteEditor(nullptr)
{
    m_pUI->setupUi(this);
    m_pUI->noteSourceView->setHidden(true);

    BasicXMLSyntaxHighlighter * highlighter = new BasicXMLSyntaxHighlighter(m_pUI->noteSourceView->document());
    Q_UNUSED(highlighter);

    m_pNoteEditor = m_pUI->noteEditorWidget;

    checkThemeIconsAndSetFallbacks();

    connectActionsToEditorSlots();

    // Debug
    QObject::connect(m_pUI->ActionShowNoteSource, QNSIGNAL(QAction, triggered),
                     this, QNSLOT(MainWindow, onShowNoteSource));
    QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditor, contentChanged), this, QNSLOT(MainWindow, onNoteContentChanged));

    m_pNoteEditor->setFocus();
}

MainWindow::~MainWindow()
{
    delete m_pUI;
}

void MainWindow::connectActionsToEditorSlots()
{
    // Font actions
    QObject::connect(m_pUI->ActionFontBold, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextBold));
    QObject::connect(m_pUI->ActionFontItalic, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextItalic));
    QObject::connect(m_pUI->ActionFontUnderlined, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextUnderline));
    QObject::connect(m_pUI->ActionFontStrikeout, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextStrikeThrough));
    // Font buttons
    QObject::connect(m_pUI->fontBoldPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextBold));
    QObject::connect(m_pUI->fontItalicPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextItalic));
    QObject::connect(m_pUI->fontUnderlinePushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextUnderline));
    QObject::connect(m_pUI->fontStrikethroughPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextStrikeThrough));
    // Format actions
    QObject::connect(m_pUI->ActionAlignLeft, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextAlignLeft));
    QObject::connect(m_pUI->ActionAlignCenter, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextAlignCenter));
    QObject::connect(m_pUI->ActionAlignRight, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextAlignRight));
    QObject::connect(m_pUI->ActionInsertHorizontalLine, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextAddHorizontalLine));
    QObject::connect(m_pUI->ActionIncreaseIndentation, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextIncreaseIndentation));
    QObject::connect(m_pUI->ActionDecreaseIndentation, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextDecreaseIndentation));
    QObject::connect(m_pUI->ActionInsertBulletedList, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextInsertUnorderedList));
    QObject::connect(m_pUI->ActionInsertNumberedList, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextInsertOrderedList));
    QObject::connect(m_pUI->ActionInsertTable, QNSIGNAL(QAction, triggered), this, QNSLOT(MainWindow, noteTextInsertTableDialog));
    // Format buttons
    QObject::connect(m_pUI->formatJustifyLeftPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextAlignLeft));
    QObject::connect(m_pUI->formatJustifyCenterPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextAlignCenter));
    QObject::connect(m_pUI->formatJustifyRightPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextAlignRight));
    QObject::connect(m_pUI->insertHorizontalLinePushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextAddHorizontalLine));
    QObject::connect(m_pUI->formatIndentMorePushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextIncreaseIndentation));
    QObject::connect(m_pUI->formatIndentLessPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextDecreaseIndentation));
    QObject::connect(m_pUI->formatListUnorderedPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextInsertUnorderedList));
    QObject::connect(m_pUI->formatListOrderedPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextInsertOrderedList));
    QObject::connect(m_pUI->insertToDoCheckboxPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteTextInsertToDoCheckBox));
    QObject::connect(m_pUI->chooseTextColorPushButton, QNSIGNAL(QPushButton, clicked), this, QNSLOT(MainWindow, noteChooseTextColor));
    QObject::connect(m_pUI->insertTableToolButton, QNSIGNAL(QPushButton, createdTable,int,int,double,bool),
                     this, QNSLOT(MainWindow, noteTextInsertTable,int,int,double,bool));
}

void MainWindow::onSetStatusBarText(QString message, const int duration)
{
    QStatusBar * pStatusBar = m_pUI->statusBar;

    if (m_currentStatusBarChildWidget != nullptr) {
        pStatusBar->removeWidget(m_currentStatusBarChildWidget);
        m_currentStatusBarChildWidget = nullptr;
    }

    if (duration == 0) {
        m_currentStatusBarChildWidget = new QLabel(message);
        pStatusBar->addWidget(m_currentStatusBarChildWidget);
    }
    else {
        pStatusBar->showMessage(message, duration);
    }
}

void MainWindow::noteTextBold()
{
    m_pNoteEditor->textBold();
}

void MainWindow::noteTextItalic()
{
    m_pNoteEditor->textItalic();
}

void MainWindow::noteTextUnderline()
{
    m_pNoteEditor->textUnderline();
}

void MainWindow::noteTextStrikeThrough()
{
    m_pNoteEditor->textStrikethrough();
}

void MainWindow::noteTextAlignLeft()
{
    m_pNoteEditor->alignLeft();
}

void MainWindow::noteTextAlignCenter()
{
    m_pNoteEditor->alignCenter();
}

void MainWindow::noteTextAlignRight()
{
    m_pNoteEditor->alignRight();
}

void MainWindow::noteTextAddHorizontalLine()
{
    m_pNoteEditor->insertHorizontalLine();
}

void MainWindow::noteTextIncreaseIndentation()
{
    m_pNoteEditor->changeIndentation(/* increase = */ true);
}

void MainWindow::noteTextDecreaseIndentation()
{
    m_pNoteEditor->changeIndentation(/* increase = */ false);
}

void MainWindow::noteTextInsertUnorderedList()
{
    m_pNoteEditor->insertBulletedList();
}

void MainWindow::noteTextInsertOrderedList()
{
    m_pNoteEditor->insertNumberedList();
}

void MainWindow::noteChooseTextColor()
{
    // TODO: implement
}

void MainWindow::noteChooseSelectedTextColor()
{
    // TODO: implement
}

void MainWindow::noteTextInsertToDoCheckBox()
{
    m_pNoteEditor->insertToDoCheckbox();
    m_pNoteEditor->setFocus();
}

void MainWindow::noteTextInsertTableDialog()
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

void MainWindow::noteTextInsertTable(int rows, int columns, double width, bool relativeWidth)
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

    if (m_pUI->noteSourceView->isVisible()) {
        m_pUI->noteSourceView->setHidden(true);
        return;
    }

    updateNoteHtmlView();
    m_pUI->noteSourceView->setVisible(true);
}

void MainWindow::onNoteContentChanged()
{
    QNDEBUG("MainWindow::onNoteContentChanged");

    if (!m_pUI->noteSourceView->isVisible()) {
        return;
    }

    updateNoteHtmlView();
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
        m_pUI->ActionInsert->setIcon(editPasteIcon);
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
        m_pUI->chooseTextColorPushButton->setIcon(formatTextColorIcon);
        QNTRACE("set fallback format-text-color icon");
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
        m_pUI->ActionFontStrikeout->setIcon(formatTextStrikethroughIcon);
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
        m_pUI->ActionSettings->setIcon(preferencesOtherIcon);
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

void MainWindow::updateNoteHtmlView()
{
    QString noteSource = m_pNoteEditor->page()->mainFrame()->toHtml();

    int pos = noteSource.indexOf("<body>");
    if (pos >= 0) {
        noteSource.remove(0, pos + 6);
    }

    pos = noteSource.indexOf("</body>");
    if (pos >= 0) {
        noteSource.truncate(pos);
    }

    m_pUI->noteSourceView->setPlainText(noteSource);
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

