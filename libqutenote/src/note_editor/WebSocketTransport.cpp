#include "WebSocketTransport.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QtWebSockets/QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

WebSocketTransport::WebSocketTransport(QWebSocket * socket) :
    QWebChannelAbstractTransport(socket),
    m_socket(socket)
{
    QObject::connect(socket, &QWebSocket::textMessageReceived,
                     this, &WebSocketTransport::textMessageReceived);
}

WebSocketTransport::~WebSocketTransport()
{}

void WebSocketTransport::sendMessage(const QJsonObject &message)
{
    QJsonDocument doc(message);
    m_socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketTransport::textMessageReceived(const QString & messageData)
{
    QJsonParseError error;
    QJsonDocument message = QJsonDocument::fromJson(messageData.toUtf8(), &error);
    if (error.error) {
        QNWARNING("Failed to parse text message as JSON object:" << messageData
                   << "Error is:" << error.errorString());
        return;
    } else if (!message.isObject()) {
        QNWARNING("Received JSON message that is not an object: " << messageData);
        return;
    }
    emit messageReceived(message.object(), this);
}
