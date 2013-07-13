#include "CredentialsModel.h"
#include <sstream>

CredentialsModel::CredentialsModel(const QString & username,
                                   const QString & password,
                                   const QString & consumerKey,
                                   const QString & consumerSecret) :
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

bool CredentialsModel::Empty(const char *& errorMessage) const
{
    std::stringstream strm;
    bool somethingIsEmpty = false;

    if (m_username.isEmpty()) {
        strm << "Username is empty. ";
        somethingIsEmpty = true;
    }

    if (m_password.isEmpty()) {
        strm << "Password is empty. ";
        somethingIsEmpty = true;
    }

    if (m_consumerKey.isEmpty()) {
        strm << "Consumer key is empty. ";
        somethingIsEmpty = true;
    }

    if (m_consumerSecret.isEmpty()) {
        strm << "Consumer secret is empty. ";
        somethingIsEmpty = true;
    }

    if (somethingIsEmpty) {
        errorMessage = strm.str().c_str();
        return true;
    }

    return false;
}
