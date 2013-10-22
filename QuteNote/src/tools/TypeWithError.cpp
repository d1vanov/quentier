#include "TypeWithError.h"
#include <cstddef>

namespace qute_note {

void TypeWithError::SetError(const QString & error)
{
    m_error = error;
}

const QString TypeWithError::GetError() const
{
    return m_error;
}

bool TypeWithError::HasError() const
{
    return !(m_error.isEmpty());
}

void TypeWithError::ClearError()
{
    m_error.clear();
}

TypeWithError::TypeWithError() :
    m_error()
{}

TypeWithError::TypeWithError(const TypeWithError & other) :
    m_error(other.m_error)
{}

TypeWithError & TypeWithError::operator =(const TypeWithError & other)
{
    if (this != &other) {
        m_error = other.m_error;
    }

    return *this;
}

TypeWithError::~TypeWithError()
{
    ClearError();
}

}
