#include "TypeWithError.h"
#include <cstddef>
#include <cstring>

namespace qute_note {

void TypeWithError::SetError(const char * error)
{
    ClearError();

    if (error != nullptr)
    {
        size_t size = strlen(error);
        if (size != 0)
        {
            m_errorText = new char[size+1];
            std::memcpy(m_errorText, error, size);
            m_errorText[size] = '\0';
            return;
        }
    }

    // empty error
    m_errorText = new char[1];
    m_errorText[0] = '\0';
}

void TypeWithError::SetError(const std::string & error)
{
    SetError(error.c_str());
}

void TypeWithError::SetError(const QString & error)
{
    SetError(error.toStdString());
}

const char * TypeWithError::GetError() const
{
    return m_errorText;
}

bool TypeWithError::HasError() const
{
    return (m_errorText != nullptr);
}

void TypeWithError::ClearError()
{
    if (m_errorText != nullptr)
    {
        delete[] m_errorText;
        m_errorText = nullptr;
    }
}

TypeWithError::TypeWithError() :
    m_errorText(nullptr)
{}

TypeWithError::TypeWithError(const TypeWithError & other) :
    m_errorText(nullptr)
{
    if (other.m_errorText != nullptr) {
        *this = other;
    }
}

TypeWithError & TypeWithError::operator =(const TypeWithError & other)
{
    if (this != &other)
    {
        if ((m_errorText == nullptr) && (other.m_errorText == nullptr)) {
            return *this;
        }
        else if (other.HasError())
        {
            if ((m_errorText == nullptr) || (strcmp(m_errorText, other.m_errorText) != 0))
            {
                size_t size = std::strlen(other.m_errorText);
                m_errorText = new char[size+1];
                std::strcpy(m_errorText, other.m_errorText);
            }
        }
        else {
            ClearError();
        }
    }

    return *this;
}

TypeWithError::~TypeWithError()
{
    ClearError();
}

void TypeWithError::Clear()
{
    ClearError();
}

}
