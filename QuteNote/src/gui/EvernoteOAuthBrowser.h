#ifndef __QUTE_NOTE__GUI__EVERNOTE_OAUTH_BROWSER_H
#define __QUTE_NOTE__GUI__EVERNOTE_OAUTH_BROWSER_H

#include <QWebView>
#include "../client/EvernoteServiceManager.h"

/**
 * @brief The EvernoteOAuthBrowser class - thin wrapper under QWebView.
 * If its window is closed before , it sends signal to EvernoteServiceManager instance to notify it that
 * the authentication process has failed
 */
class EvernoteOAuthBrowser: public QWebView
{
    Q_OBJECT
public:
    explicit EvernoteOAuthBrowser(QWidget * parent = nullptr);
    virtual ~EvernoteOAuthBrowser() final;

public slots:
    void authSuccessful();

signals:
    void OAUthBrowserClosedBeforeProvidingAccess(QString);

protected:
    void closeEvent(QCloseEvent * pEvent);

private:
    bool m_authenticationSuccessful;
};

#endif // __QUTE_NOTE__GUI__EVERNOTE_OAUTH_BROWSER_H
