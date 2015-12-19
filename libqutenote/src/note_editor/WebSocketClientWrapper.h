#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__WEB_SOCKET_CLIENT_WRAPPER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__WEB_SOCKET_CLIENT_WRAPPER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(WebSocketTransport)

class WebSocketClientWrapper: public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClientWrapper(QWebSocketServer * server, QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void clientConnected(WebSocketTransport * client);

private Q_SLOTS:
    void handleNewConnection();

private:
    QWebSocketServer * m_server;
};

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__WEB_SOCKET_CLIENT_WRAPPER_H

