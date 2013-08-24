#include "Guid.h"

namespace qute_note {

Guid::Guid() :
    m_guid()
{}

Guid::Guid(const std::string & guid) :
    m_guid(guid)
{}

bool Guid::isEmpty() const
{
    return m_guid.empty();
}

bool Guid::operator ==(const Guid & other) const
{
    return (m_guid == other.m_guid);
}

bool Guid::operator !=(const Guid & other) const
{
    return !(*this == other);
}

bool Guid::operator <(const Guid & other) const
{
    return (m_guid < other.m_guid);
}

}
