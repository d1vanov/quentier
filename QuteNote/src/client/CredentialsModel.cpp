#include "CredentialsModel.h"

CredentialsModel::CredentialsModel(const QString & username,
                                   const QString & password,
                                   const QString & consumerKey,
                                   const QString & consumerSecret,
                                   QObject * parent) :
    QObject(parent),
    m_username(username),
    m_password(password),
    m_consumerKey(consumerKey),
    m_consumerSecret(consumerSecret)
{}

CredentialsModel::CredentialsModel(const CredentialsModel & other) :
    m_username(other.m_username),
    m_password(other.m_password),
    m_consumerKey(other.m_consumerKey),
    m_consumerSecret(other.m_consumerSecret)
{}

CredentialsModel & CredentialsModel::operator=(const CredentialsModel & other)
{
    if (this != &other)
    {
        m_username = other.m_username;
        m_password = other.m_password;
        m_consumerKey = other.m_consumerKey;
        m_consumerSecret = other.m_consumerSecret;
    }

    return *this;
}

bool CredentialsModel::Empty(QString & errorMessage) const
{
    bool somethingIsEmpty = false;
    errorMessage.clear();

    if (m_username.isEmpty()) {
        errorMessage.append(tr("Username is empty. "));
        somethingIsEmpty = true;
    }

    if (m_password.isEmpty()) {
        errorMessage.append(tr("Password is empty. "));
        somethingIsEmpty = true;
    }

    if (m_consumerKey.isEmpty()) {
        errorMessage.append(tr("Consumer key is empty. "));
        somethingIsEmpty = true;
    }

    if (m_consumerSecret.isEmpty()) {
        errorMessage.append(tr("Consumer secret is empty. "));
        somethingIsEmpty = true;
    }

    if (somethingIsEmpty) {
        return true;
    }

    return false;
}
