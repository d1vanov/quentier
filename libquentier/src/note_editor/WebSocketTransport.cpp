/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WebSocketTransport.h"
#include <quentier/logging/QuentierLogger.h>
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

void WebSocketTransport::sendMessage(const QJsonObject & message)
{
    QJsonDocument doc(message);
    m_socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketTransport::textMessageReceived(const QString & messageData)
{
    QByteArray messageRawData = messageData.toUtf8();
    QJsonObject object;
    if (parseMessage(messageRawData, object)) {
        emit messageReceived(object, this);
    }
}

bool WebSocketTransport::parseMessage(QByteArray messageData, QJsonObject & object)
{
    QNTRACE("WebSocketTransport::parseMessage: " << messageData);

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(messageData, &error);
    if (!error.error)
    {
        if (!document.isObject()) {
            QNWARNING("Failed to parse JSON message that is not an object: " << messageData);
            return false;
        }

        object = document.object();
        return true;
    }

    if (error.error != QJsonParseError::GarbageAtEnd) {
        QNWARNING("Failed to parse text message as JSON object: " << messageData
                  << "; error is: " << error.errorString());
        return false;
    }

    QNTRACE("Detected \"garbage at the end\" JSON parsing error, trying to workaround; "
            "message data: " << messageData);

    // NOTE: for some reason which I can't fully comprehend yet the first part of the message
    // (i.e. the first JSON object) seems to always be the previous message which has already
    // been parsed and processed. So here we just get rid of everything but the last message.
    // FIXME: need to have a better understanding of how this thing is supposed to work
    int lastOpeningCurvyBraceIndex = messageData.lastIndexOf('{');
    if (lastOpeningCurvyBraceIndex <= 0) {
        QNWARNING("Failed to workaround \"Garbage at the end\" error, message data: " << messageData);
        return false;
    }

    messageData.remove(0, lastOpeningCurvyBraceIndex);
    return parseMessage(messageData, object);
}
