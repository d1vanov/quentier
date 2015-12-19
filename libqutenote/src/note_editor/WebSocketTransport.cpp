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
    QByteArray messageRawData = messageData.toUtf8();
    QList<QJsonDocument> parsedDocs = parseMessage(messageRawData);
    const int numParsedDocs = parsedDocs.size();
    for(int i = 0; i < numParsedDocs; ++i) {
        emit messageReceived(parsedDocs[i].object(), this);
    }
}

QList<QJsonDocument> WebSocketTransport::parseMessage(QByteArray messageData)
{
    QNTRACE("WebSocketTransport::parseMessage: " << messageData);

    QList<QJsonDocument> result;

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(messageData, &error);
    if (!error.error)
    {
        if (!document.isObject()) {
            QNWARNING("Failed to parse JSON message that is not an object: " << messageData);
        }
        else {
            result << document;
        }

        return result;
    }

    if (error.error != QJsonParseError::GarbageAtEnd) {
        QNWARNING("Failed to parse text message as JSON object: " << messageData
                  << "; error is: " << error.errorString());
        return result;
    }

    QNTRACE("Detected \"garbage at the end\" JSON parsing error, trying to workaround; "
            "message data: " << messageData);

    // NOTE: for some reason which I can't fully comprehend yet the first part of the message
    // (i.e. the first JSON object) seems to always be the previous message which has already
    // been parsed and processed. So here we just get rid of everything but the last message.
    // FIXME: need to have a better understanding of how this thing is supposed to work
    int lastOpeningCurvyBraceIndex = messageData.lastIndexOf('{');
    if (lastOpeningCurvyBraceIndex > 0) {
        messageData.remove(0, lastOpeningCurvyBraceIndex);
        return parseMessage(messageData);
    }
    else {
        QNWARNING("Failed to workaround \"Garbage at the end\" error, message data: " << messageData);
        return result;
    }
}
