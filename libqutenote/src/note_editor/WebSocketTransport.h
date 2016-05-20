#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_WEB_SOCKET_TRANSPORT_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_WEB_SOCKET_TRANSPORT_H

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

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_WEB_SOCKET_TRANSPORT_H

