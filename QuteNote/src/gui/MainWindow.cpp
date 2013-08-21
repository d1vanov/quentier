#include "MainWindow.h"
#include "NoteEditorWidget.h"
#include "ui_MainWindow.h"
#include "EvernoteOAuthBrowser.h"
#include "../client/EvernoteServiceManager.h"
#include "AskConsumerKeyAndSecret.h"
#include "AskUserNameAndPassword.h"
#include "../SimpleCrypt/src/Simplecrypt.h"
#include "../client/CredentialsModel.h"
#include <cmath>
#include <QPushButton>
#include <QLabel>
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
    m_pBrowser(new EvernoteOAuthBrowser(this, *m_pManager))
{
    Q_CHECK_PTR(m_pUI);
    Q_CHECK_PTR(m_pAskConsumerKeyAndSecretWidget);
    Q_CHECK_PTR(m_pAskUserNameAndPasswordWidget);
    Q_CHECK_PTR(m_pManager);
    Q_CHECK_PTR(m_pBrowser);

    m_pUI->setupUi(this);
    ConnectActionsToEditorSlots();

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
        CheckAndSetupConsumerKeyAndSecret();
        if (!credentials.GetConsumerKey().isEmpty() &&
            !credentials.GetConsumerSecret().isEmpty())
        {
            CheckAndSetupUserNameAndPassword();
        }
    }
    else
    {
        CheckAndSetupUserNameAndPassword();
    }

    m_pManager->connect();
}

MainWindow::~MainWindow()
{
    if (m_pUI != nullptr) {
        delete m_pUI;
        m_pUI = nullptr;
    }
}

void MainWindow::SetDefaultLayoutSettings()
{
    int mainWindowHeight = height();
    int mainWindowWidth = width();

    // Let's make widgets on the left take a reasonable amount of window width
    int helperWidgetsWidth = std::floor(0.175 * mainWindowWidth + 0.5);

    // Calculating optimal height for each helper widget. Together they are
    // going to take approx. 85% of useable height. There should be some reasonable
    // proportion between these widgets heights. For example, labels should have
    // about 30% of sum height, notebooks - about 25%, search properties - about 20%,
    // saved requests - about 20%, trash - only about 5%.

    int helperWidgetsAvailableHeight = std::floor(0.9 * mainWindowHeight + 0.5);

    int dockNotebooksHeight = std::floor(0.3 * helperWidgetsAvailableHeight + 0.5);
    int dockLabelsHeight = std::floor(0.2 * helperWidgetsAvailableHeight + 0.5);
    int dockSearchPropertiesHeight = std::floor(0.2 * helperWidgetsAvailableHeight + 0.5);
    int dockSavedRequestsHeight = std::floor(0.2 * helperWidgetsAvailableHeight + 0.5);
    int dockTrashHeight = std::floor(0.1 * helperWidgetsAvailableHeight + 0.5);

    // Now resize helper dock widgets appropriately
    if (m_pUI == nullptr) {
        qDebug() << "Warning! UI pointer is NULL!";
        return;
    }
    Ui::MainWindow & ui = *m_pUI;

    // multipliers to set min and max height and width for helper widgets
    double minHeightMultiplier = 0.6;
    double minWidthMultiplier = 0.6;
    double maxHeightMultiplier = 1.5;
    double maxWidthMultiplier = 1.25;

    // 1) Dock notebooks
    ResizeHelperDockWidget(ui.dockNotebooks, dockNotebooksHeight, helperWidgetsWidth,
                           minHeightMultiplier, minWidthMultiplier, maxHeightMultiplier,
                           maxWidthMultiplier);
    // 2) Dock labels
    ResizeHelperDockWidget(ui.dockLabels, dockLabelsHeight, helperWidgetsWidth,
                           minHeightMultiplier, minWidthMultiplier, maxHeightMultiplier,
                           maxWidthMultiplier);
    // 3) Dock search properties
    ResizeHelperDockWidget(ui.dockSearchProperties, dockSearchPropertiesHeight,
                           helperWidgetsWidth, minHeightMultiplier, minWidthMultiplier,
                           maxHeightMultiplier, maxWidthMultiplier);
    // 4) Dock saved requests
    ResizeHelperDockWidget(ui.dockSavedRequests, dockSavedRequestsHeight,
                           helperWidgetsWidth, minHeightMultiplier, minWidthMultiplier,
                           maxHeightMultiplier, maxWidthMultiplier);
    // 5) Dock trash
    ResizeHelperDockWidget(ui.dockTrash, dockTrashHeight, helperWidgetsWidth,
                           minHeightMultiplier, minWidthMultiplier, maxHeightMultiplier,
                           maxWidthMultiplier);

    // TODO: rearrange the position of each helper widget

    // TODO: change vertical sizes of notes table and note editor window
}

void MainWindow::resizeEvent(QResizeEvent * pResizeEvent)
{
    Q_CHECK_PTR(pResizeEvent);

    SetDefaultLayoutSettings();
}

EvernoteOAuthBrowser * MainWindow::OAuthBrowser()
{
    return m_pBrowser;
}

void MainWindow::ResizeHelperDockWidget(QDockWidget * pDock, const int dockHeight,
                                        const int dockWidth, const double minHeightMultiplier,
                                        const double minWidthMultiplier,
                                        const double maxHeightMultiplier,
                                        const double maxWidthMultiplier)
{
#ifndef QT_NO_DEBUG
    if (pDock == nullptr) {
        qDebug() << "MainWindow::ResizeHelperDockWidget: found NULL dock widget pointer!";
        return;
    }
#endif

    QDockWidget & rDock = *pDock;

    int dockOldHeight = pDock->height();
    int dockOldWidth = pDock->width();

    if ( (std::fabs(dockOldWidth - dockWidth) < 1) &&
         (std::fabs(dockOldHeight - dockHeight) < 1) )
    {
        rDock.resize(dockWidth, dockHeight);
    }

    rDock.setMaximumHeight(std::floor(dockHeight * maxHeightMultiplier + 0.5));
    rDock.setMaximumWidth(std::floor(dockWidth * maxWidthMultiplier + 0.5));
    rDock.setMinimumHeight(std::floor(dockHeight * minHeightMultiplier + 0.5));
    rDock.setMinimumWidth(std::floor(dockWidth * minWidthMultiplier + 0.5));
}

void MainWindow::ConnectActionsToEditorSlots()
{
    // Font
    QObject::connect(m_pUI->ActionFontBold, SIGNAL(triggered()), this, SLOT(textBold()));
    QObject::connect(m_pUI->ActionFontItalic, SIGNAL(triggered()), this, SLOT(textItalic()));
    QObject::connect(m_pUI->ActionFontUnderlined, SIGNAL(triggered()), this, SLOT(textUnderline()));
    QObject::connect(m_pUI->ActionFontStrikeout, SIGNAL(triggered()), this, SLOT(textStrikeThrough()));

    NoteEditorWidget * pNotesEditor = m_pUI->notesEditorWidget;

    QObject::connect(m_pUI->ActionAlignLeft, SIGNAL(triggered()), pNotesEditor,
                     SLOT(textAlignLeft()));
    QObject::connect(m_pUI->ActionAlignCenter, SIGNAL(triggered()), pNotesEditor,
                     SLOT(textAlignCenter()));
    QObject::connect(m_pUI->ActionAlignRight, SIGNAL(triggered()), pNotesEditor,
                     SLOT(textAlignRight()));

    QObject::connect(m_pUI->ActionInsertHorizontalLine, SIGNAL(triggered()),
                     pNotesEditor, SLOT(textAddHorizontalLine()));

    QObject::connect(m_pUI->ActionIncreaseIndentation, SIGNAL(triggered()),
                     pNotesEditor, SLOT(textIncreaseIndentation()));
    QObject::connect(m_pUI->ActionDecreaseIndentation, SIGNAL(triggered()),
                     pNotesEditor, SLOT(textDecreaseIndentation()));

    QObject::connect(m_pUI->ActionInsertBulletedList, SIGNAL(triggered()),
                     pNotesEditor, SLOT(textInsertUnorderedList()));
    QObject::connect(m_pUI->ActionInsertNumberedList, SIGNAL(triggered()),
                     pNotesEditor, SLOT(textInsertOrderedList()));
}

void MainWindow::CheckAndSetupConsumerKeyAndSecret()
{
    checkAndSetupCredentials(CHECK_CONSUMER_KEY_AND_SECRET);
}

void MainWindow::CheckAndSetupUserNameAndPassword()
{
    checkAndSetupCredentials(CHECK_USER_NAME_AND_PASSWORD);
}

void MainWindow::CheckAndSetupOAuthTokenAndSecret()
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
        if (m_pBrowser != nullptr) {
            m_pBrowser->authSuccessful();
            m_pBrowser->close();
        }
    }
}

void MainWindow::onShowAuthWebPage(QUrl url)
{
    if (m_pBrowser == nullptr) {
        m_pBrowser = new EvernoteOAuthBrowser(this, *m_pManager);
        Q_CHECK_PTR(m_pBrowser);
    }

    m_pBrowser->load(url);
    m_pBrowser->show();
}

void MainWindow::onRequestUsernameAndPassword()
{
    CheckAndSetupUserNameAndPassword();
}

void MainWindow::textBold()
{
    QPushButton * pTextBoldButton = m_pUI->notesEditorWidget->getTextBoldButton();
    if (pTextBoldButton->isCheckable()) {
        if (pTextBoldButton->isChecked()) {
            pTextBoldButton->setChecked(false);
        }
        else {
            pTextBoldButton->setChecked(true);
        }

        m_pUI->notesEditorWidget->textBold();
    }
}

void MainWindow::textItalic()
{
    QPushButton * pTextItalicButton = m_pUI->notesEditorWidget->getTextItalicButton();
    if (pTextItalicButton->isCheckable()) {
        if (pTextItalicButton->isChecked()) {
            pTextItalicButton->setChecked(false);
        }
        else {
            pTextItalicButton->setChecked(true);
        }

        m_pUI->notesEditorWidget->textItalic();
    }
}

void MainWindow::textUnderline()
{
    QPushButton * pTextUnderlineButton = m_pUI->notesEditorWidget->getTextUnderlineButton();
    if (pTextUnderlineButton->isCheckable()) {
        if (pTextUnderlineButton->isChecked()) {
            pTextUnderlineButton->setChecked(false);
        }
        else {
            pTextUnderlineButton->setChecked(true);
        }

        m_pUI->notesEditorWidget->textUnderline();
    }
}

void MainWindow::textStrikeThrough()
{
    QPushButton * pTextStrikeThroughButton = m_pUI->notesEditorWidget->getTextStrikeThroughButton();
    if (pTextStrikeThroughButton->isCheckable()) {
        if (pTextStrikeThroughButton->isChecked()) {
            pTextStrikeThroughButton->setChecked(false);
        }
        else {
            pTextStrikeThroughButton->setChecked(true);
        }

        m_pUI->notesEditorWidget->textStrikeThrough();
    }
}
