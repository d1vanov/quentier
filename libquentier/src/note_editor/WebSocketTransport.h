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

#ifndef LIB_QUENTIER_NOTE_EDITOR_WEB_SOCKET_TRANSPORT_H
#define LIB_QUENTIER_NOTE_EDITOR_WEB_SOCKET_TRANSPORT_H

#include <QtWebChannel/QWebChannelAbstractTransport>
#include <QScopedPointer>

QT_FORWARD_DECLARE_CLASS(QWebSocket)

class WebSocketTransport: public QWebChannelAbstractTransport
{
    Q_OBJECT
public:
    explicit WebSocketTransport(QWebSocket * socket);
    virtual ~WebSocketTransport();

    virtual void sendMessage(const QJsonObject & message) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void textMessageReceived(const QString & message);

private:
    bool parseMessage(QByteArray messageData, QJsonObject & object);

private:
    QWebSocket * m_socket;
};

#endif // LIB_QUENTIER_NOTE_EDITOR_WEB_SOCKET_TRANSPORT_H

