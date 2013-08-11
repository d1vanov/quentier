#include "EvernoteOAuthBrowser.h"

EvernoteOAuthBrowser::EvernoteOAuthBrowser(QWidget *parent, EvernoteServiceManager & manager) :
    QWebView(parent),
    m_manager(manager),
    m_authenticationSuccessful(false)
{
    QWidget::setMinimumHeight(200);
    QWidget::setMinimumWidth(300);

    QObject::connect(this, SIGNAL(OAUthBrowserClosedBeforeProvidingAccess(QString)),
                     &m_manager, SLOT(onOAuthFailure(QString)));
}

EvernoteOAuthBrowser::~EvernoteOAuthBrowser()
{
    QObject::disconnect(this, SIGNAL(OAUthBrowserClosedBeforeProvidingAccess(QString)),
                        &m_manager, SLOT(onOAuthFailure(QString)));
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


