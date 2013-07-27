#include "EvernoteOAuthBrowser.h"

EvernoteOAuthBrowser::EvernoteOAuthBrowser(QWidget *parent) :
    QWebView(parent)
{
    QWidget::setMinimumHeight(200);
    QWidget::setMinimumWidth(300);

    QObject::connect(this,
                     SIGNAL(OAUthBrowserClosedBeforeProvidingAccess(QString)),
                     &(EvernoteServiceManager::Instance()),
                     SLOT(onOAuthFailure(QString)));
}

EvernoteOAuthBrowser::~EvernoteOAuthBrowser()
{
    QObject::disconnect(this,
                        SIGNAL(OAUthBrowserClosedBeforeProvidingAccess(QString)),
                        &(EvernoteServiceManager::Instance()),
                        SLOT(onOAuthFailure(QString)));
}

void EvernoteOAuthBrowser::authSuccessful()
{
    m_authenticationSuccessful = true;
}

void EvernoteOAuthBrowser::closeEvent(QCloseEvent * pEvent)
{
    if (!m_authenticationSuccessful) {
        emit OAUthBrowserClosedBeforeProvidingAccess(tr("Evernote OAuth "
                  "browser was closed before authentication was provided"));
    }
    QWebView::closeEvent(pEvent);
}


