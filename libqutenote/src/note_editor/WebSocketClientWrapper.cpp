#include "WebSocketClientWrapper.h"
#include "WebSocketTransport.h"
#include <QtWebSockets/QWebSocketServer>

WebSocketClientWrapper::WebSocketClientWrapper(QWebSocketServer * server, QObject * parent) :
    QObject(parent),
    m_server(server)
{
    QObject::connect(server, &QWebSocketServer::newConnection,
                     this, &WebSocketClientWrapper::handleNewConnection);
}

void WebSocketClientWrapper::handleNewConnection()
{
    emit clientConnected(new WebSocketTransport(m_server->nextPendingConnection()));
}
