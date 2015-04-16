#include "MainWindow.h"

#include <note_editor/NoteEditor.h>
using qute_note::NoteEditor;    // workarouding Qt4 Designer's inability to work with namespaces
#include "ui_MainWindow.h"

#include "TableSettingsDialog.h"
#include <note_editor/HorizontalLineExtraData.h>
#include <note_editor/ToDoCheckboxTextObject.h>
#include "EvernoteOAuthBrowser.h"
#include "../client/EvernoteServiceManager.h"
#include "AskConsumerKeyAndSecret.h"
#include "AskUserNameAndPassword.h"
#include <Simplecrypt.h>
#include "../client/CredentialsModel.h"
#include <tools/QuteNoteCheckPtr.h>
#include <tools/BasicXMLSyntaxHighlighter.h>
#include <logging/QuteNoteLogger.h>
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
    m_pAskConsumerKeyAndSecretWidget(new AskConsumerKeyAndSecret(this)),
    m_pAskUserNameAndPasswordWidget(new AskUserNameAndPassword(this)),
    m_currentStatusBarChildWidget(nullptr),
    m_pManager(new EvernoteServiceManager),
    m_pOAuthBrowser(new EvernoteOAuthBrowser(this, *m_pManager)),
    m_pToDoChkboxTxtObjUnchecked(new ToDoCheckboxTextObjectUnchecked),
    m_pToDoChkboxTxtObjChecked(new ToDoCheckboxTextObjectChecked),
    m_pNoteEditor(nullptr)
{
    m_pUI->setupUi(this);
    m_pUI->noteSourceView->setHidden(true);
    BasicXMLSyntaxHighlighter * highlighter = new BasicXMLSyntaxHighlighter(m_pUI->noteSourceView->document());
    Q_UNUSED(highlighter);

    m_pNoteEditor = m_pUI->noteEditorWidget;

    checkThemeIconsAndSetFallbacks();

    connectActionsToEditorSlots();

    QObject::connect(m_pManager, SIGNAL(statusTextUpdate(QString,int)),
                     this, SLOT(onSetStatusBarText(QString,int)));
    QObject::connect(m_pManager, SIGNAL(showAuthWebPage(QUrl)),
                     this, SLOT(onShowAuthWebPage(QUrl)));
    QObject::connect(m_pAskConsumerKeyAndSecretWidget,
                     SIGNAL(consumerKeyAndSecretEntered(QString,QString)),
                     m_pManager, SLOT(onConsumerKeyAndSecretSet(QString,QString)));
    QObject::connect(m_pManager, SIGNAL(requestUsernameAndPassword()),
                     this, SLOT(onRequestUsernameAndPassword()));

    QObject::connect(m_pAskConsumerKeyAndSecretWidget,
                     SIGNAL(cancelled(QString)),
                     this, SLOT(onSetStatusBarText(QString)));

    // Debug
    QObject::connect(m_pUI->ActionShowNoteSource, SIGNAL(triggered()),
                     this, SLOT(onShowNoteSource()));
    QObject::connect(m_pNoteEditor, SIGNAL(contentChanged()), this, SLOT(onNoteContentChanged()));

    m_pManager->connect();
}

MainWindow::~MainWindow()
{
    if (m_pUI != nullptr) {
        delete m_pUI;
        m_pUI = nullptr;
    }

    if (!m_pToDoChkboxTxtObjUnchecked) {
        delete m_pToDoChkboxTxtObjUnchecked;
        m_pToDoChkboxTxtObjUnchecked = nullptr;
    }

    if (!m_pToDoChkboxTxtObjChecked) {
        delete m_pToDoChkboxTxtObjChecked;
        m_pToDoChkboxTxtObjChecked = nullptr;
    }
}

EvernoteOAuthBrowser * MainWindow::OAuthBrowser()
{
    return m_pOAuthBrowser;
}

void MainWindow::connectActionsToEditorSlots()
{
    // Font actions
    QObject::connect(m_pUI->ActionFontBold, SIGNAL(triggered()), this, SLOT(noteTextBold()));
    QObject::connect(m_pUI->ActionFontItalic, SIGNAL(triggered()), this, SLOT(noteTextItalic()));
    QObject::connect(m_pUI->ActionFontUnderlined, SIGNAL(triggered()), this, SLOT(noteTextUnderline()));
    QObject::connect(m_pUI->ActionFontStrikeout, SIGNAL(triggered()), this, SLOT(noteTextStrikeThrough()));
    // Font buttons
    QObject::connect(m_pUI->fontBoldPushButton, SIGNAL(clicked()), this, SLOT(noteTextBold()));
    QObject::connect(m_pUI->fontItalicPushButton, SIGNAL(clicked()), this, SLOT(noteTextItalic()));
    QObject::connect(m_pUI->fontUnderlinePushButton, SIGNAL(clicked()), this, SLOT(noteTextUnderline()));
    QObject::connect(m_pUI->fontStrikethroughPushButton, SIGNAL(clicked()), this, SLOT(noteTextStrikeThrough()));
    // Format actions
    QObject::connect(m_pUI->ActionAlignLeft, SIGNAL(triggered()), this, SLOT(noteTextAlignLeft()));
    QObject::connect(m_pUI->ActionAlignCenter, SIGNAL(triggered()), this, SLOT(noteTextAlignCenter()));
    QObject::connect(m_pUI->ActionAlignRight, SIGNAL(triggered()), this, SLOT(noteTextAlignRight()));
    QObject::connect(m_pUI->ActionInsertHorizontalLine, SIGNAL(triggered()), this, SLOT(noteTextAddHorizontalLine()));
    QObject::connect(m_pUI->ActionIncreaseIndentation, SIGNAL(triggered()), this, SLOT(noteTextIncreaseIndentation()));
    QObject::connect(m_pUI->ActionDecreaseIndentation, SIGNAL(triggered()), this, SLOT(noteTextDecreaseIndentation()));
    QObject::connect(m_pUI->ActionInsertBulletedList, SIGNAL(triggered()), this, SLOT(noteTextInsertUnorderedList()));
    QObject::connect(m_pUI->ActionInsertNumberedList, SIGNAL(triggered()), this, SLOT(noteTextInsertOrderedList()));
    QObject::connect(m_pUI->ActionInsertTable, SIGNAL(triggered()), this, SLOT(noteTextInsertTable()));
    // Format buttons
    QObject::connect(m_pUI->formatJustifyLeftPushButton, SIGNAL(clicked()), this, SLOT(noteTextAlignLeft()));
    QObject::connect(m_pUI->formatJustifyCenterPushButton, SIGNAL(clicked()), this, SLOT(noteTextAlignCenter()));
    QObject::connect(m_pUI->formatJustifyRightPushButton, SIGNAL(clicked()), this, SLOT(noteTextAlignRight()));
    QObject::connect(m_pUI->insertHorizontalLinePushButton, SIGNAL(clicked()), this, SLOT(noteTextAddHorizontalLine()));
    QObject::connect(m_pUI->formatIndentMorePushButton, SIGNAL(clicked()), this, SLOT(noteTextIncreaseIndentation()));
    QObject::connect(m_pUI->formatIndentLessPushButton, SIGNAL(clicked()), this, SLOT(noteTextDecreaseIndentation()));
    QObject::connect(m_pUI->formatListUnorderedPushButton, SIGNAL(clicked()), this, SLOT(noteTextInsertUnorderedList()));
    QObject::connect(m_pUI->formatListOrderedPushButton, SIGNAL(clicked()), this, SLOT(noteTextInsertOrderedList()));
    QObject::connect(m_pUI->insertToDoCheckboxPushButton, SIGNAL(clicked()), this, SLOT(noteTextInsertToDoCheckBox()));
    QObject::connect(m_pUI->chooseTextColorPushButton, SIGNAL(clicked()), this, SLOT(noteChooseTextColor()));
    QObject::connect(m_pUI->insertTablePushButton, SIGNAL(clicked()), this, SLOT(noteTextInsertTable()));
}

void MainWindow::checkAndSetupConsumerKeyAndSecret()
{
    checkAndSetupCredentials(CHECK_CONSUMER_KEY_AND_SECRET);
}

void MainWindow::checkAndSetupUserNameAndPassword()
{
    checkAndSetupCredentials(CHECK_USER_NAME_AND_PASSWORD);
}

void MainWindow::checkAndSetupOAuthTokenAndSecret()
{
    checkAndSetupCredentials(CHECK_OAUTH_TOKEN_AND_SECRET);
}

void MainWindow::checkAndSetupCredentials(MainWindow::ECredentialsToCheck credentialsFlag)
{
    bool noStoredCredentials = false;

    QFile credentialsFile;

    switch(credentialsFlag)
    {
    case CHECK_CONSUMER_KEY_AND_SECRET:
        credentialsFile.setFileName(":/enc_data/consks.dat");
        break;
    case CHECK_USER_NAME_AND_PASSWORD:
        credentialsFile.setFileName(":/enc_data/ulp.dat");
        break;
    case CHECK_OAUTH_TOKEN_AND_SECRET:
        credentialsFile.setFileName(":/enc_data/oautk.dat");
        break;
    default:
        QMessageBox::critical(0, tr("Internal error"),
                              tr("Wrong credentials flag in MainWindow::checkAndSetupCredentials."));
        QApplication::quit();
    }

    if (!credentialsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(0, "Error: cannot open file with credentials: ",
                                 credentialsFile.errorString());
        noStoredCredentials = true;
    }

    if (!noStoredCredentials)
    {
        QTextStream textStream(&credentialsFile);
        QString keyEncrypted = textStream.readLine();
        QString secretEncrypted = textStream.readLine();

        if (keyEncrypted.isEmpty() || secretEncrypted.isEmpty())
        {
            noStoredCredentials = true;
        }
        else
        {
            SimpleCrypt crypto(CredentialsModel::RANDOM_CRYPTO_KEY);
            QString keyDecrypted = crypto.decryptToString(keyEncrypted);
            QString secretDecrypted = crypto.decryptToString(secretEncrypted);

            CredentialsModel & credentials = m_pManager->getCredentials();

            switch(credentialsFlag)
            {
            case CHECK_CONSUMER_KEY_AND_SECRET:
                credentials.SetConsumerKey(keyDecrypted);
                credentials.SetConsumerSecret(secretDecrypted);
                break;
            case CHECK_USER_NAME_AND_PASSWORD:
                credentials.SetUsername(keyDecrypted);
                credentials.SetPassword(secretDecrypted);
                break;
            case CHECK_OAUTH_TOKEN_AND_SECRET:
                credentials.SetOAuthKey(keyDecrypted);
                credentials.SetOAuthSecret(secretDecrypted);
                break;
            default:
                QMessageBox::critical(0, tr("Internal error"),
                                      tr("Wrong credentials flag in MainWindow::checkAndSetupCredentials."));
                QApplication::quit();
            }
        }
    }

    if (noStoredCredentials)
    {

#define ASK_FOR_CREDENTIALS(widgetName)  \
    { \
        QUTE_NOTE_CHECK_PTR(widgetName); \
        QRect centralWidgetRect = rect(); \
        double xCenter = centralWidgetRect.left() + 0.5 * centralWidgetRect.width(); \
        double yCenter = centralWidgetRect.top() + 0.5 * centralWidgetRect.height(); \
        widgetName->show(); \
        xCenter -= widgetName->width() * 0.5; \
        yCenter -= widgetName->height() * 0.5; \
        widgetName->move(xCenter, yCenter); \
        widgetName->raise(); \
    }

        switch(credentialsFlag)
        {
        case CHECK_CONSUMER_KEY_AND_SECRET:
            ASK_FOR_CREDENTIALS(m_pAskConsumerKeyAndSecretWidget);
            break;
        case CHECK_USER_NAME_AND_PASSWORD:
            ASK_FOR_CREDENTIALS(m_pAskUserNameAndPasswordWidget);
            break;
        case CHECK_OAUTH_TOKEN_AND_SECRET:
        {
            m_pManager->authenticate();
            break;
        }
        default:
            QMessageBox::critical(0, tr("Internal error"),
                                  tr("Wrong credentials flag in MainWindow::checkAndSetupCredentials."));
            QApplication::quit();
        }

#undef ASK_FOR_CREDENTIALS
    }
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

    // Check whether authentication is done and we need to close the browser
    QString dummyErrorString;
    if (m_pManager->CheckAuthenticationState(dummyErrorString)) {
        if (m_pOAuthBrowser != nullptr) {
            m_pOAuthBrowser->authSuccessful();
            m_pOAuthBrowser->close();
        }
    }
}

void MainWindow::onShowAuthWebPage(QUrl url)
{
    if (m_pOAuthBrowser == nullptr) {
        m_pOAuthBrowser = new EvernoteOAuthBrowser(this, *m_pManager);
        QUTE_NOTE_CHECK_PTR(m_pOAuthBrowser);
    }

    m_pOAuthBrowser->load(url);
    m_pOAuthBrowser->show();
}

void MainWindow::onRequestUsernameAndPassword()
{
    checkAndSetupUserNameAndPassword();
}

void MainWindow::show()
{
    QMainWindow::show();

    const CredentialsModel & credentials = m_pManager->getCredentials();

    if (credentials.GetConsumerKey().isEmpty() ||
        credentials.GetConsumerSecret().isEmpty())
    {
        checkAndSetupConsumerKeyAndSecret();
        if (!credentials.GetConsumerKey().isEmpty() &&
            !credentials.GetConsumerSecret().isEmpty())
        {
            checkAndSetupUserNameAndPassword();
        }
    }
    else
    {
        checkAndSetupUserNameAndPassword();
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

void MainWindow::noteTextInsertTable()
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

        return;
    }

    QNTRACE("Returned from TableSettingsDialog::exec: rejected");
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

void MainWindow::setAlignButtonsCheckedState(const ESelectedAlignment alignment)
{
    switch(alignment)
    {
    case ALIGNED_LEFT:
        m_pUI->formatJustifyCenterPushButton->setChecked(false);
        m_pUI->formatJustifyRightPushButton->setChecked(false);
        break;
    case ALIGNED_CENTER:
        m_pUI->formatJustifyLeftPushButton->setChecked(false);
        m_pUI->formatJustifyRightPushButton->setChecked(false);
        break;
    case ALIGNED_RIGHT:
        m_pUI->formatJustifyLeftPushButton->setChecked(false);
        m_pUI->formatJustifyCenterPushButton->setChecked(false);
        break;
    default:
        qWarning() << "MainWindow::setAlignButtonsCheckedState: received invalid action!";
        break;
    }
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

