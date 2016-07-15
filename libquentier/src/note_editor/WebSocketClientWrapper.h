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

#ifndef LIB_QUENTIER_NOTE_EDITOR_WEB_SOCKET_CLIENT_WRAPPER_H
#define LIB_QUENTIER_NOTE_EDITOR_WEB_SOCKET_CLIENT_WRAPPER_H

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

#endif // LIB_QUENTIER_NOTE_EDITOR_WEB_SOCKET_CLIENT_WRAPPER_H

