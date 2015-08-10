#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__WEB_SOCKET_TRANSPORT_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__WEB_SOCKET_TRANSPORT_H

#include <QtWebChannel/QWebChannelAbstractTransport>

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
    QWebSocket * m_socket;
};

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__WEB_SOCKET_TRANSPORT_H

