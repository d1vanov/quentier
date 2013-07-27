#include "MainWindow.h"
#include "NoteEditorWidget.h"
#include "ui_MainWindow.h"
#include "EvernoteOAuthBrowser.h"
#include "../client/EvernoteServiceManager.h"
#include "AskConsumerKeyAndSecret.h"
#include "../SimpleCrypt/src/simplecrypt.h"
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
    m_currentStatusBarChildWidget(nullptr),
    m_pBrowser(new EvernoteOAuthBrowser(this))
{
    Q_CHECK_PTR(m_pUI);
    Q_CHECK_PTR(m_pAskConsumerKeyAndSecretWidget);

    m_pUI->setupUi(this);
    ConnectActionsToEditorSlots();

    EvernoteServiceManager & manager = EvernoteServiceManager::Instance();
    QObject::connect(&manager, SIGNAL(statusText(QString,int)),
                     this, SLOT(onSetStatusBarText(QString,int)));
    QObject::connect(&manager, SIGNAL(showAuthWebPage(QUrl)),
                     this, SLOT(onShowAuthWebPage(QUrl)));
    QObject::connect(m_pAskConsumerKeyAndSecretWidget,
                     SIGNAL(consumerKeyAndSecretEntered(QString,QString)),
                     &manager, SLOT(onConsumerKeyAndSecretSet(QString,QString)));

    QObject::connect(m_pAskConsumerKeyAndSecretWidget,
                     SIGNAL(cancelled(QString)),
                     this, SLOT(onSetStatusBarText(QString)));

    CheckAndSetupConsumerKeyAndSecret();
    CheckAndSetupUserLoginAndPassword();
    manager.connect();
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
    bool needToAskUserForKeyAndSecret = false;

    QFile consumerKeyAndSecretFile(":/enc_data/consks.dat");

    if (!consumerKeyAndSecretFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(0, "Error: cannot open file with consumer key and secret: ",
                                 consumerKeyAndSecretFile.errorString());
        needToAskUserForKeyAndSecret = true;
    }

    if (!needToAskUserForKeyAndSecret)
    {
        QTextStream textStream(&consumerKeyAndSecretFile);
        QString consumerKeyEncrypted = textStream.readLine();
        QString consumerSecretEncrypted = textStream.readLine();

        if (consumerKeyEncrypted.isEmpty() || consumerSecretEncrypted.isEmpty())
        {
            needToAskUserForKeyAndSecret = true;
        }
        else
        {
            SimpleCrypt crypto(CredentialsModel::RANDOM_CRYPTO_KEY);
            QString consumerKeyDecrypted = crypto.decryptToString(consumerKeyEncrypted);
            QString consumerSecretDecrypted = crypto.decryptToString(consumerSecretEncrypted);

            EvernoteServiceManager & manager = EvernoteServiceManager::Instance();
            CredentialsModel & credentials = manager.getCredentials();
            credentials.SetConsumerKey(consumerKeyDecrypted);
            credentials.SetConsumerSecret(consumerSecretDecrypted);
            // Don't try to set credentials here as user name and password are likely missing
        }
    }

    if (needToAskUserForKeyAndSecret)
    {
        Q_CHECK_PTR(m_pAskConsumerKeyAndSecretWidget);
        m_pAskConsumerKeyAndSecretWidget->show();
    }
}

void MainWindow::CheckAndSetupUserLoginAndPassword()
{
    // TODO: implement
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
    EvernoteServiceManager & manager = EvernoteServiceManager::Instance();
    QString dummyErrorString;
    if (manager.CheckAuthenticationState(dummyErrorString)) {
        if (m_pBrowser != nullptr) {
            m_pBrowser->authSuccessful();
            m_pBrowser->close();
        }
    }
}

void MainWindow::onShowAuthWebPage(QUrl url)
{
    if (m_pBrowser == nullptr) {
        m_pBrowser = new EvernoteOAuthBrowser(this);
        Q_CHECK_PTR(m_pBrowser);
    }

    m_pBrowser->load(url);
    m_pBrowser->show();
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
