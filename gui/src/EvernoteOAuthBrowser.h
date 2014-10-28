#ifndef __QUTE_NOTE__GUI__EVERNOTE_OAUTH_BROWSER_H
#define __QUTE_NOTE__GUI__EVERNOTE_OAUTH_BROWSER_H

#include <QtCore>

#if QT_VERSION >= 0x050101
#include <QtWebKitWidgets/QWebView>
#else
#include <QWebView>
#endif
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
    explicit EvernoteOAuthBrowser(QWidget * parent, EvernoteServiceManager & manager);
    virtual ~EvernoteOAuthBrowser();

public slots:
    void authSuccessful();

signals:
    void OAUthBrowserClosedBeforeProvidingAccess(QString);

protected:
    void closeEvent(QCloseEvent * pEvent);

private:
    EvernoteServiceManager & m_manager;
    bool m_authenticationSuccessful;
};

#endif // __QUTE_NOTE__GUI__EVERNOTE_OAUTH_BROWSER_H
