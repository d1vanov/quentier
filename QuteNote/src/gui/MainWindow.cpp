#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../note_editor/HorizontalLineExtraData.h"
#include "../note_editor/ToDoCheckboxTextObject.h"
#include "EvernoteOAuthBrowser.h"
#include "../client/EvernoteServiceManager.h"
#include "AskConsumerKeyAndSecret.h"
#include "AskUserNameAndPassword.h"
#include "../SimpleCrypt/src/Simplecrypt.h"
#include "../client/CredentialsModel.h"
#include <cmath>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QTextList>
#include <QColorDialog>
#include <QFile>
#include <QMessageBox>
#include <QtDebug>

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow),
    m_pAskConsumerKeyAndSecretWidget(new AskConsumerKeyAndSecret(this)),
    m_pAskUserNameAndPasswordWidget(new AskUserNameAndPassword(this)),
    m_currentStatusBarChildWidget(nullptr),
    m_pManager(new EvernoteServiceManager),
    m_pOAuthBrowser(new EvernoteOAuthBrowser(this, *m_pManager)),
    m_pToDoChkboxTxtObjUnchecked(new ToDoCheckboxTextObjectUnchecked),
    m_pToDoChkboxTxtObjChecked(new ToDoCheckboxTextObjectChecked)
{
    Q_CHECK_PTR(m_pUI);
    Q_CHECK_PTR(m_pAskConsumerKeyAndSecretWidget);
    Q_CHECK_PTR(m_pAskUserNameAndPasswordWidget);
    Q_CHECK_PTR(m_pManager);
    Q_CHECK_PTR(m_pOAuthBrowser);
    Q_CHECK_PTR(m_pToDoChkboxTxtObjUnchecked);
    Q_CHECK_PTR(m_pToDoChkboxTxtObjChecked);

    m_pUI->setupUi(this);

    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QAbstractTextDocumentLayout * pLayout = pNoteEdit->document()->documentLayout();
    pLayout->registerHandler(QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_UNCHECKED, m_pToDoChkboxTxtObjUnchecked);
    pLayout->registerHandler(QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_CHECKED, m_pToDoChkboxTxtObjChecked);

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
    QObject::connect(m_pUI->chooseSelectedTextColorPushButton, SIGNAL(clicked()), this, SLOT(noteChooseSelectedTextColor()));
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
        switch(credentialsFlag)
        {
        case CHECK_CONSUMER_KEY_AND_SECRET:
            Q_CHECK_PTR(m_pAskConsumerKeyAndSecretWidget);
            m_pAskConsumerKeyAndSecretWidget->show();
            break;
        case CHECK_USER_NAME_AND_PASSWORD:
            Q_CHECK_PTR(m_pAskUserNameAndPasswordWidget);
            m_pAskUserNameAndPasswordWidget->show();
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
        Q_CHECK_PTR(m_pOAuthBrowser);
    }

    m_pOAuthBrowser->load(url);
    m_pOAuthBrowser->show();
}

void MainWindow::onRequestUsernameAndPassword()
{
    checkAndSetupUserNameAndPassword();
}

void MainWindow::noteTextBold()
{
    QPushButton * pTextBoldButton = m_pUI->fontBoldPushButton;
    if (pTextBoldButton->isCheckable()) {
        QTextCharFormat format;
        format.setFontWeight(pTextBoldButton->isChecked() ? QFont::Bold : QFont::Normal);
        mergeFormatOnWordOrSelection(format);
    }
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteTextItalic()
{
    QPushButton * pTextItalicButton = m_pUI->fontItalicPushButton;
    if (pTextItalicButton->isCheckable()) {
        QTextCharFormat format;
        format.setFontItalic(pTextItalicButton->isChecked());
        mergeFormatOnWordOrSelection(format);
    }
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteTextUnderline()
{
    QPushButton * pTextUnderlineButton = m_pUI->fontUnderlinePushButton;
    if (pTextUnderlineButton->isCheckable()) {
        QTextCharFormat format;
        format.setFontUnderline(pTextUnderlineButton->isChecked());
        mergeFormatOnWordOrSelection(format);
    }
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteTextStrikeThrough()
{
    QPushButton * pTextStrikeThroughButton = m_pUI->fontStrikethroughPushButton;
    if (pTextStrikeThroughButton->isCheckable()) {
        QTextCharFormat format;
        format.setFontStrikeOut(pTextStrikeThroughButton->isChecked());
        mergeFormatOnWordOrSelection(format);
    }
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteTextAlignLeft()
{
    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QTextCursor cursor = pNoteEdit->textCursor();
    cursor.beginEditBlock();

    pNoteEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    setAlignButtonsCheckedState(ALIGNED_LEFT);

    cursor.endEditBlock();
    pNoteEdit->setFocus();
}

void MainWindow::noteTextAlignCenter()
{
    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QTextCursor cursor = pNoteEdit->textCursor();
    cursor.beginEditBlock();

    pNoteEdit->setAlignment(Qt::AlignHCenter);
    setAlignButtonsCheckedState(ALIGNED_CENTER);

    cursor.endEditBlock();
    pNoteEdit->setFocus();
}

void MainWindow::noteTextAlignRight()
{
    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QTextCursor cursor = pNoteEdit->textCursor();
    cursor.beginEditBlock();

    pNoteEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    setAlignButtonsCheckedState(ALIGNED_RIGHT);

    cursor.endEditBlock();
    pNoteEdit->setFocus();
}

void MainWindow::noteTextAddHorizontalLine()
{
    QuteNoteTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QTextCursor cursor = pNoteEdit->textCursor();
    cursor.beginEditBlock();

    QTextBlockFormat format = cursor.blockFormat();

    bool atStart = cursor.atStart();

    cursor.insertHtml("<hr><br>");
    cursor.setBlockFormat(format);
    cursor.movePosition(QTextCursor::Up);

    if (atStart)
    {
        cursor.movePosition(QTextCursor::Up);
        cursor.deletePreviousChar();
        cursor.movePosition(QTextCursor::Down);
    }
    else
    {
        cursor.movePosition(QTextCursor::Up);
        cursor.movePosition(QTextCursor::Up);
        QString line = cursor.block().text().trimmed();
        if (line.isEmpty()) {
            cursor.movePosition(QTextCursor::Down);
            cursor.deletePreviousChar();
            cursor.movePosition(QTextCursor::Down);
        }
        else {
            cursor.movePosition(QTextCursor::Down);
            cursor.movePosition(QTextCursor::Down);
        }
    }

    // add extra data
    cursor.movePosition(QTextCursor::Up);
    cursor.block().setUserData(new HorizontalLineExtraData);
    cursor.movePosition(QTextCursor::Down);

    cursor.endEditBlock();
    pNoteEdit->setFocus();
}

void MainWindow::noteTextIncreaseIndentation()
{
    const bool increaseIndentationFlag = true;
    changeIndentation(increaseIndentationFlag);
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteTextDecreaseIndentation()
{
    const bool increaseIndentationFlag = false;
    changeIndentation(increaseIndentationFlag);
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteTextInsertUnorderedList()
{
    QTextListFormat::Style style = QTextListFormat::ListDisc;
    insertList(style);
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteTextInsertOrderedList()
{
    QTextListFormat::Style style = QTextListFormat::ListDecimal;
    insertList(style);
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteChooseTextColor()
{
    changeTextColor(CHANGE_COLOR_ALL);
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteChooseSelectedTextColor()
{
    changeTextColor(CHANGE_COLOR_SELECTED);
    m_pUI->noteEditWidget->setFocus();
}

void MainWindow::noteTextInsertToDoCheckBox()
{
    QString checkboxUncheckedImgFileName(":/format_text_elements/checkbox_unchecked.gif");
    QFile checkboxUncheckedImgFile(checkboxUncheckedImgFileName);
    if (!checkboxUncheckedImgFile.exists()) {
        QMessageBox::warning(this, tr("Error Opening File"),
                             tr("Could not open '%1'").arg(checkboxUncheckedImgFileName));
        return;
    }

    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QTextCursor cursor = pNoteEdit->textCursor();
    cursor.beginEditBlock();

    QImage checkboxUncheckedImg(checkboxUncheckedImgFileName);
    QTextCharFormat toDoCheckboxUncheckedCharFormat;
    toDoCheckboxUncheckedCharFormat.setObjectType(QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_UNCHECKED);
    toDoCheckboxUncheckedCharFormat.setProperty(QuteNoteTextEdit::TODO_CHKBOX_TXT_DATA_UNCHECKED, checkboxUncheckedImg);

    cursor.insertText(QString(QChar::ObjectReplacementCharacter), toDoCheckboxUncheckedCharFormat);
    cursor.insertText(" ", QTextCharFormat());

    cursor.endEditBlock();

    pNoteEdit->setTextCursor(cursor);
}

void MainWindow::mergeFormatOnWordOrSelection(const QTextCharFormat & format)
{
    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QTextCursor cursor = pNoteEdit->textCursor();
    cursor.beginEditBlock();

    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    cursor.clearSelection();
    pNoteEdit->mergeCurrentCharFormat(format);

    cursor.endEditBlock();
}

void MainWindow::changeIndentation(const bool increaseIndentationFlag)
{
    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QTextCursor cursor = pNoteEdit->textCursor();
    cursor.beginEditBlock();

    if (cursor.currentList())
    {
        QTextListFormat listFormat = cursor.currentList()->format();

        if (increaseIndentationFlag) {
            listFormat.setIndent(listFormat.indent() + 1);
        }
        else {
            listFormat.setIndent(std::max(listFormat.indent() - 1, 0));
        }

        cursor.createList(listFormat);
    }
    else
    {
        int start = cursor.anchor();
        int end = cursor.position();
        if (start > end)
        {
            start = cursor.position();
            end = cursor.anchor();
        }

        QList<QTextBlock> blocksList;
        QTextBlock block = pNoteEdit->document()->begin();
        while (block.isValid())
        {
            block = block.next();
            if ( ((block.position() >= start) && (block.position() + block.length() <= end) ) ||
                 block.contains(start) || block.contains(end) )
            {
                blocksList << block;
            }
        }

        foreach(QTextBlock block, blocksList)
        {
            QTextCursor cursor(block);
            QTextBlockFormat blockFormat = cursor.blockFormat();

            if (increaseIndentationFlag) {
                blockFormat.setIndent(blockFormat.indent() + 1);
            }
            else {
                blockFormat.setIndent(std::max(blockFormat.indent() - 1, 0));
            }

            cursor.setBlockFormat(blockFormat);
        }
    }

    cursor.endEditBlock();
}

void MainWindow::insertList(const QTextListFormat::Style style)
{
    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QTextCursor cursor = pNoteEdit->textCursor();
    cursor.beginEditBlock();

    QTextBlockFormat blockFormat = cursor.blockFormat();

    QTextListFormat listFormat;
    if (cursor.currentList()) {
        listFormat = cursor.currentList()->format();
    }
    else {
        listFormat.setIndent(blockFormat.indent() + 1);
        blockFormat.setIndent(0);
        cursor.setBlockFormat(blockFormat);
    }

    listFormat.setStyle(style);

    cursor.createList(listFormat);
    cursor.endEditBlock();
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

void MainWindow::changeTextColor(const MainWindow::EChangeColor changeColorOption)
{
    QTextEdit * pNoteEdit = m_pUI->noteEditWidget;
    Q_CHECK_PTR(pNoteEdit);

    QColor color = QColorDialog::getColor(pNoteEdit->textColor(), this);
    if (!color.isValid()) {
        qWarning() << "MainWindow::changeTextColor: detected invalid color! " << color;
        return;
    }

    QTextCharFormat format;
    format.setForeground(color);

    switch(changeColorOption)
    {
    case CHANGE_COLOR_ALL:
    {
        QTextCursor cursor = pNoteEdit->textCursor();
        cursor.beginEditBlock();

        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(format);
        pNoteEdit->mergeCurrentCharFormat(format);

        cursor.endEditBlock();
        break;
    }
    case CHANGE_COLOR_SELECTED:
    {
        mergeFormatOnWordOrSelection(format);
        break;
    }
    default:
        qWarning() << "MainWindow::changeTextColor: received wrong change color option! " << changeColorOption;
        return;
    }
}
