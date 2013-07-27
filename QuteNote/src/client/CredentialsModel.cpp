#include "CredentialsModel.h"

CredentialsModel::CredentialsModel(const CredentialsModel & other) :
    m_username(other.m_username),
    m_password(other.m_password),
    m_consumerKey(other.m_consumerKey),
    m_consumerSecret(other.m_consumerSecret),
    m_OAuthKey(other.m_OAuthKey),
    m_OAuthSecret(other.m_OAuthSecret)
{}

CredentialsModel & CredentialsModel::operator=(const CredentialsModel & other)
{
    if (this != &other)
    {
        m_username = other.m_username;
        m_password = other.m_password;
        m_consumerKey = other.m_consumerKey;
        m_consumerSecret = other.m_consumerSecret;
        m_OAuthKey = other.m_OAuthKey;
        m_OAuthSecret = other.m_OAuthSecret;
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

    if (m_OAuthKey.isEmpty()) {
        errorMessage.append(tr("OAuth token is empty. "));
        somethingIsEmpty = true;
    }

    if (m_OAuthSecret.isEmpty()) {
        errorMessage.append(tr("OAuth token secret is empty. "));
        somethingIsEmpty = true;
    }

    if (somethingIsEmpty) {
        return true;
    }

    return false;
}
