#ifndef __QUTE_NOTE__CLIENT__CREDENTIALS_MODEL_H
#define __QUTE_NOTE__CLIENT__CREDENTIALS_MODEL_H

#include <QObject>
#include <QString>

class CredentialsModel: public QObject
{
    Q_OBJECT
public:
    explicit CredentialsModel(QObject * parent = nullptr) : QObject(parent) {}
    CredentialsModel(const QString & username,
                     const QString & password,
                     const QString & consumerKey,
                     const QString & consumerSecret,
                     QObject * parent = nullptr);
    CredentialsModel(const CredentialsModel & other);
    CredentialsModel & operator=(const CredentialsModel & other);

    const QString GetUsername() const { return m_username; }
    const QString GetPassword() const { return m_password; }
    const QString GetConsumerKey() const { return m_consumerKey; }
    const QString GetConsumerSecret() const { return m_consumerSecret; }

    bool Empty(QString & errorMessage) const;

private:
    QString m_username;
    QString m_password;
    QString m_consumerKey;
    QString m_consumerSecret;
};

#endif // __QUTE_NOTE__CLIENT__CREDENTIALS_MODEL_H
