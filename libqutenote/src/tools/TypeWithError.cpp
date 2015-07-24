#include "TypeWithError.h"
#include <cstddef>

namespace qute_note {

const QString TypeWithError::error() const
{
    return m_error;
}

void TypeWithError::setError(const QString & error)
{
    m_error = error;
}

bool TypeWithError::hasError() const
{
    return !(m_error.isEmpty());
}

void TypeWithError::clearError()
{
    m_error.resize(0);
}

TypeWithError::TypeWithError() :
    m_error()
{}

TypeWithError::TypeWithError(const TypeWithError & other) :
    m_error(other.m_error)
{}

TypeWithError::TypeWithError(TypeWithError && other) :
    m_error(std::move(other.m_error))
{}

TypeWithError & TypeWithError::operator =(const TypeWithError & other)
{
    if (this != &other) {
        m_error = other.m_error;
    }

    return *this;
}

TypeWithError & TypeWithError::operator=(TypeWithError && other)
{
    if (this != &other) {
        m_error = std::move(other.m_error);
    }

    return *this;
}

TypeWithError::~TypeWithError()
{
    clearError();
}

}
