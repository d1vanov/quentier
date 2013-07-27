#ifndef __QUTE_NOTE__CLIENT__CREDENTIALS_MODEL_H
#define __QUTE_NOTE__CLIENT__CREDENTIALS_MODEL_H

#include <QObject>
#include <QString>

class CredentialsModel: public QObject
{
    Q_OBJECT
public:
    explicit CredentialsModel(QObject * parent = nullptr) : QObject(parent) {}
    CredentialsModel(const CredentialsModel & other);
    CredentialsModel & operator=(const CredentialsModel & other);

    const QString GetUsername() const { return m_username; }
    const QString GetPassword() const { return m_password; }
    const QString GetConsumerKey() const { return m_consumerKey; }
    const QString GetConsumerSecret() const { return m_consumerSecret; }
    const QString GetOAuthKey() const { return m_OAuthKey; }
    const QString GetOAuthSecret() const { return m_OAuthSecret; }

    void SetUsername(const QString & username) { m_username = username; }
    void SetPassword(const QString & password) { m_password = password; }
    void SetConsumerKey(const QString & consumerKey) { m_consumerKey = consumerKey; }
    void SetConsumerSecret(const QString & consumerSecret) { m_consumerSecret = consumerSecret; }
    void SetOAuthKey(const QString & key) { m_OAuthKey = key; }
    void SetOAuthSecret(const QString & secret) { m_OAuthSecret = secret; }

    bool Empty(QString & errorMessage) const;

    static const unsigned long long RANDOM_CRYPTO_KEY = 0x44bb133f85f412e2;

private:
    QString m_username;
    QString m_password;
    QString m_consumerKey;
    QString m_consumerSecret;
    QString m_OAuthKey;
    QString m_OAuthSecret;
};

#endif // __QUTE_NOTE__CLIENT__CREDENTIALS_MODEL_H
