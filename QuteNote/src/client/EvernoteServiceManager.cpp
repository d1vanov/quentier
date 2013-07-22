#include "EvernoteServiceManager.h"
#include "../gui/MainWindow.h"
#include "../tools/Singleton.h"
#include <QWebView>

EvernoteServiceManager::EvernoteServiceManager()
{
    m_evernoteHostName = "https://sandbox.evernote.com";
}

EvernoteServiceManager &EvernoteServiceManager::Instance()
{
    return Singleton<EvernoteServiceManager>::Instance();
}

bool EvernoteServiceManager::setCredentials(const CredentialsModel & credentials,
                                            QString & errorMessage)
{
    if (credentials.Empty(errorMessage)) {
        return false;
    }

    m_credentials = credentials;
    return true;
}

bool EvernoteServiceManager::CheckAuthenticationState(QString & errorMessage) const
{
    switch(m_authorizationState)
    {
    case EAS_AUTHORIZED:
        return true;
    case EAS_UNAUTHORIZED_NEVER_ATTEMPTED:
        errorMessage = tr("Not authorized yet");
        return false;
    case EAS_UNAUTHORIZED_CREDENTIALS_REJECTED:
        errorMessage = tr("Last authorization attempt failed: credentials rejected");
        return false;
    case EAS_UNAUTHORIZED_QUIT:
        errorMessage = tr("Not yet authorized after last quit");
        return false;
    case EAS_UNAUTHORIZED_INTERNAL_ERROR:
        errorMessage = tr("Not authorized due to internal error, please contact developers");
        return false;
    default:
        errorMessage = tr("Not authorized: unknown error, please contact developers");
        return false;
    }
}

void EvernoteServiceManager::GetHostName(QString & hostname) const
{
    hostname = m_evernoteHostName;
}

void EvernoteServiceManager::onOAuthSuccess(std::pair<QString, QString>)
{
    // TODO: store this information encrypted in an external file
    MainWindow::Instance().setStatusBarText("Successfully authenticated to Evernote!", 2000);
}

void EvernoteServiceManager::onOAuthFailure(QString message)
{
    MainWindow::Instance().setStatusBarText("Unable to authenticate to Evernote: " + message);
}

void EvernoteServiceManager::onRequestToShowAuthorizationPage(QUrl authUrl)
{
    // TODO: create class based on QWebView so that I could catch the close window button pressing
    QWebView evernoteWebView;
    evernoteWebView.load(authUrl);
    evernoteWebView.show();
}
